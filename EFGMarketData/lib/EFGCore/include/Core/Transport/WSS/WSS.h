#pragma once 
#include <uWS.h>

namespace EFG
{
namespace Core
{
namespace Transport
{
    
    class WssTransport {
      public:
          WssTransport(uWS::Hub* hub): h(hub)
          {
	    group = h->createGroup<uWS::CLIENT>();
	  }
          ~WssTransport()
          {
            delete group;
          }
	  template<typename CallBackConnect>
	  void registerOnConnectCallBack(CallBackConnect onConnectionHandler){
	    group->onConnection(onConnectionHandler);
	  }
	  template<typename CallBackHttpRequest>
          void registerOnHttpRequestCallBack(CallBackHttpRequest onHttpResquestHandler){
      	    group->onHttpRequest(onHttpResquestHandler);
	  }
	  template<typename CallBackUpdate>
	  void registerOnUpdateCallBack(CallBackUpdate onUpdateHandler){
            group->onMessage(onUpdateHandler);
	  }
	  template<typename CallBackDisconnect>
	  void registerOnDisconnectCallback(CallBackDisconnect onDisconnectHandler){
	    group->onDisconnection(onDisconnectHandler);
	  }
	  template<typename ...EXTRA>
	  void connect(std::string url, EXTRA... args){
	    h->connect(url, nullptr, {}, 17000, group);
	  }
	  void run() {
	    h->run();
	  }
          /*
	  static uWS::Hub& getTransport() {
		return h;
	  }
	  static uWS::Hub h;
          */
     private:
         uWS::Group<uWS::CLIENT> *group;
	 uWS::Hub* h; 
            
      };

}
}
}
