#include <MarketData/CME/CME.h>

namespace EFG{
  namespace MarketData{
    namespace VenueSession{
	   CME::CME(const ConfigManager::VenueSessionInfo& vsinfo): AbstractVenue(vsinfo){
	    
	      transport = std::make_unique<Transport::WssTransport>(&md_hub);
              registerCallBacks();
              MD_LOG_INFO << " Created CME Session " << '\n'; // logger - later
	   }
	   void CME::onConnection(uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req)
	   {
	   
	   }
	   void CME::onUpdate(uWS::WebSocket<uWS::CLIENT> *ws, char * data, std::size_t len_, uWS::OpCode code)
	   {
	   
	   }
	   void CME::onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, std::size_t length){
	   
	   }
            
           void CME::registerCallBacks()
           {
             using namespace std::placeholders;
             transport->registerOnConnectCallBack(std::bind(&CME::onConnection, this,_1,_2));
             transport->registerOnUpdateCallBack(std::bind(&CME::onUpdate, this,_1,_2,_3,_4));
             transport->registerOnDisconnectCallback(std::bind(&CME::onDisconnection, this,_1,_2,_3,_4));          

           }
            
           void CME::decode(char* buffer, size_t len, const TimeType& packet_hit_time)
           {

           }
          	
    }
  }

}

