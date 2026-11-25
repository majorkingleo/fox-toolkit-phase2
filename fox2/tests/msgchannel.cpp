/********************************************************************************
*                                                                               *
*                      Inter-Thread Message Channel Test                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
********************************************************************************/
#include "fx.h"
#include "FXMessageChannel.h"
#include "testargs.h"

#include <cstdio>
#include <cstring>


/*******************************************************************************/

static const FX::FXint MESSAGE_COUNT=1000;


// Receive messages in the application thread
class MessageTarget : public FX::FXObject {
  FXDECLARE(MessageTarget)
private:
  FX::FXApp* app;
  FX::FXint count;
  FXlong sum;
  FXbool invalid;
public:
  enum {
    ID_MESSAGE=1,
    ID_TIMEOUT,
    ID_LAST
    };
public:
  MessageTarget():app(nullptr),count(0),sum(0),invalid(false){}
  explicit MessageTarget(FX::FXApp* application):app(application),count(0),sum(0),invalid(false){}
  long onMessage(FX::FXObject*,FX::FXSelector,void* ptr){
    FX::FXint value=0;
    if(!ptr){
      invalid=true;
      }
    else{
      std::memcpy(&value,ptr,sizeof(value));
      }
    count++;
    sum+=value;
    if(count==MESSAGE_COUNT) app->stop();
    return 1;
    }
  long onTimeout(FX::FXObject*,FX::FXSelector,void*){
    app->stop(1);
    return 1;
    }
  FX::FXint getCount() const { return count; }
  FXlong getSum() const { return sum; }
  FXbool hasInvalidPayload() const { return invalid; }
  };


// Message handler map
FXDEFMAP(MessageTarget) MessageTargetMap[]={
  FXMAPFUNC(FX::SEL_COMMAND,MessageTarget::ID_MESSAGE,MessageTarget::onMessage),
  FXMAPFUNC(FX::SEL_TIMEOUT,MessageTarget::ID_TIMEOUT,MessageTarget::onTimeout)
  };


// Object implementation
FXIMPLEMENT(MessageTarget,FX::FXObject,MessageTargetMap,ARRAYNUMBER(MessageTargetMap))


// Send a fixed batch of messages from a worker thread
class MessageSender : public FX::FXThread {
private:
  FX::FXMessageChannel* channel;
  MessageTarget* target;
  FX::FXbool failed;
public:
  MessageSender(FX::FXMessageChannel* messageChannel,MessageTarget* messageTarget):channel(messageChannel),target(messageTarget),failed(false){}
  virtual FX::FXint run(){
    for(FX::FXint i=1; i<=MESSAGE_COUNT; i++){
      if(!channel->message(target,FXSEL(FX::SEL_COMMAND,MessageTarget::ID_MESSAGE),&i,sizeof(i))){
        failed=true;
        break;
        }
      }
    return failed ? 1 : 0;
    }
  FX::FXbool hasFailed() const { return failed; }
  };


// Start
int main(int argc,char* argv[]){
  fxtestargs(argc,argv);
  FX::FXApp app("MessageChannelTest","FOX");
  app.init(argc,argv);
  FX::FXMessageChannel channel(&app);
  MessageTarget target(&app);
  app.create();
  app.addTimeout(&target,FXSEL(FX::SEL_TIMEOUT,MessageTarget::ID_TIMEOUT),FX::FXTime(10000000000LL),nullptr);
  if(!app.hasTimeout(&target,FXSEL(FX::SEL_TIMEOUT,MessageTarget::ID_TIMEOUT))){
    std::printf("FAILED: could not add watchdog timeout\n");
    return 1;
    }
  MessageSender sender(&channel,&target);
  if(!sender.start()){
    std::printf("FAILED: could not start sender thread\n");
    return 1;
    }
  FX::FXint result=app.run();
  app.removeTimeout(&target,FXSEL(FX::SEL_TIMEOUT,MessageTarget::ID_TIMEOUT));
  FXbool joined=sender.join();
  FXlong expected=FXlong(MESSAGE_COUNT)*(MESSAGE_COUNT+1)/2;
  if(result!=0 || !joined || sender.hasFailed() || target.getCount()!=MESSAGE_COUNT || target.getSum()!=expected || target.hasInvalidPayload()){
    std::printf("FAILED: result=%d joined=%d send-failed=%d count=%d sum=%lld expected=%lld invalid=%d\n",
      result,(int)joined,(int)sender.hasFailed(),target.getCount(),(long long)target.getSum(),(long long)expected,(int)target.hasInvalidPayload());
    return 1;
    }
  std::printf("all %d message callbacks and payloads passed\n",MESSAGE_COUNT);
  return 0;
  }
