
#include "SescConf.h"
#include "PortManager.h"
#ifdef ENABLE_NBSD
#include "NBSDPortManagerArbitrer.h"
#endif


PortManager *PortManager::create(const char *section, MemObj *mobj)
{
  if (SescConf->checkCharPtr(section, "port")) {
    const char *sub  = SescConf->getCharPtr(section, "port");
    const char *type = SescConf->getCharPtr(sub,"type");
    if (strcasecmp(type,"banked") == 0) {
      return new PortManagerBanked(sub, mobj);
#ifdef ENABLE_NBSD
    }else if (strcasecmp(type,"arbitrer") == 0) {
      return new PortManagerArbitrer(sub, mobj);
#endif
    }else{
      MSG("ERROR: %s PortManager %s type %s is unknown",section, sub, type);
      SescConf->notCorrect();
    }

  }

  return new PortManagerBanked(section,mobj);
}

PortManagerBanked::PortManagerBanked(const char *section, MemObj *_mobj)
  : PortManager(_mobj)
{
  int numPorts = SescConf->getInt(section, "bkNumPorts");
  int portOccp = SescConf->getInt(section, "bkPortOccp");

  char tmpName[512];
  const char *name = mobj->getName();

	hitDelay  = SescConf->getInt(section, "hitDelay");
	missDelay = SescConf->getInt(section, "missDelay");
	SescConf->isBetween(section,"missDelay",0,hitDelay);

	tagDelay  = hitDelay-missDelay;
	dataDelay = hitDelay-tagDelay;

	numBanks   = SescConf->getInt(section, "numBanks");
	SescConf->isBetween(section, "numBanks", 1, 1024);  // More than 1024???? (more likely a bug in the conf)
  SescConf->isPower2(section,"numBanks");
  int32_t log2numBanks = log2i(numBanks);
  if (numBanks>1)
    numBanksMask = (1<<log2numBanks)-1;
  else
    numBanksMask = 0;

  bkPort = new PortGeneric* [numBanks]; 
  for (uint32_t i = 0; i < numBanks; i++){
    sprintf(tmpName, "%s_bk(%d)", name,i);
    bkPort[i] = PortGeneric::create(tmpName, numPorts, portOccp);
    I(bkPort[i]);
  }
  I(bkPort[0]);
  int fillPorts = 1;
  int fillOccp  = 1;
  if (SescConf->checkInt(section,"sendFillPortOccp")) {
    fillPorts = SescConf->getInt(section, "sendFillNumPorts");
    fillOccp  = SescConf->getInt(section, "sendFillPortOccp");
  }
  sprintf(tmpName, "%s_sendFill", name);
  sendFillPort = PortGeneric::create(tmpName,fillPorts,fillOccp);

  maxRequests = SescConf->getInt(section, "maxRequests");
  if(maxRequests == 0)
    maxRequests = 32768; // It should be enough

  curRequests = 0;

  lineSize = SescConf->getInt(section,"bsize");
  if (SescConf->checkInt(section,"bankShift")) {
    bankShift = SescConf->getInt(section,"bankShift");
    bankSize  = 1<<bankShift;
  }else{
    bankShift = log2i(lineSize);
    bankSize  = lineSize;
  }
  if (SescConf->checkInt(section,"recvFillWidth")) {
    recvFillWidth = SescConf->getInt(section,"recvFillWidth");
    SescConf->isPower2(section,"recvFillWidth");
    SescConf->isBetween(section,"recvFillWidth",1,lineSize);
  }else{
    recvFillWidth = lineSize;
  }

	blockTime = 0;
}

Time_t PortManagerBanked::nextBankSlot(AddrType addr, bool en) 
{ 
  int32_t bank = (addr>>bankShift) & numBanksMask;

  return bkPort[bank]->nextSlot(en); 
}

Time_t PortManagerBanked::calcNextBankSlot(AddrType addr) 
{ 
  int32_t bank = (addr>>bankShift) & numBanksMask;

  return bkPort[bank]->calcNextSlot(); 
}

void PortManagerBanked::nextBankSlotUntil(AddrType addr, Time_t until, bool en) 
{ 
  uint32_t bank = (addr>>bankShift) & numBanksMask;

  bkPort[bank]->occupyUntil(until); 
}

Time_t PortManagerBanked::reqDone(MemRequest *mreq)
{
  if (mreq->isWarmup())
    return globalClock+1;

  Time_t when=sendFillPort->nextSlot(mreq->getStatsFlag())+dataDelay;

  // TRACE 
  //MSG("%5lld @%lld %-8s done %12llx curReq=%d",mreq->getID(),when,mobj->getName(),mreq->getAddr(),curRequests);

  return when;
}

void PortManagerBanked::reqRetire(MemRequest *mreq)
{
  curRequests--;
  I(curRequests>=0);
}

bool PortManagerBanked::isBusy(AddrType addr) const
{
  if(curRequests >= maxRequests)
    return true;

  return false;
}

void PortManagerBanked::req(MemRequest *mreq)
/* main processor read entry point {{{1 */
{
	//I(curRequests<=maxRequests && !mreq->isWarmup());

  if (!mreq->isRetrying())
    curRequests++;

  // TRACE 
  //MSG("%5lld @%lld %-8s req  %12llx curReq=%d",mreq->getID(),globalClock,mobj->getName(),mreq->getAddr(),curRequests);

  if (mreq->isWarmup())
    mreq->redoReqAbs(globalClock+1);
  else
    mreq->redoReqAbs(nextBankSlot(mreq->getAddr(), mreq->getStatsFlag())+tagDelay);
}
// }}}

Time_t PortManagerBanked::snoopFillBankUse(MemRequest *mreq) {
  Time_t max = 0;
  Time_t max_fc = 0;
  for(int fc = 0; fc<lineSize;   fc += recvFillWidth) {
    max_fc++;
    for(int i = 0;i<recvFillWidth;i += bankSize) {
      Time_t t = nextBankSlot(mreq->getAddr()+fc+i,mreq->getStatsFlag());
      if ((t+max_fc)>max)
        max = t+max_fc;
    }
  }

  // Make sure that all the banks are busy until the max time
  Time_t cur_fc = 0;
  for(int fc = 0; fc<lineSize ;  fc += recvFillWidth) {
    cur_fc++;
    for(int i = 0;i<recvFillWidth;i += bankSize) {
      nextBankSlotUntil(mreq->getAddr()+fc+i,max-max_fc+cur_fc, mreq->getStatsFlag());
    }
  }

  return max;
}

void PortManagerBanked::blockFill(MemRequest *mreq) 
	// Block the cache ports for fill requests {{{1
{
	blockTime = globalClock;

  snoopFillBankUse(mreq);
}
// }}}

void PortManagerBanked::reqAck(MemRequest *mreq)
/* request Ack {{{1 */
{
  Time_t until;

  if (mreq->isWarmup())
    until = globalClock+1;
  else
    until = snoopFillBankUse(mreq);

	blockTime =0;

	mreq->redoReqAckAbs(until);
}
// }}}

void PortManagerBanked::setState(MemRequest *mreq)
/* set state {{{1 */
{
	mreq->redoSetStateAbs(globalClock+1);
}
// }}}

void PortManagerBanked::setStateAck(MemRequest *mreq)
/* set state ack {{{1 */
{
	mreq->redoSetStateAckAbs(globalClock+1);
}
// }}}

void PortManagerBanked::disp(MemRequest *mreq)
/* displace a CCache line {{{1 */
{
  Time_t t = snoopFillBankUse(mreq);
	mreq->redoDispAbs(t);
}
// }}}
