#include <MarketData/COINBASE/COINBASE.h>

namespace EFG{
  namespace MarketData{
    namespace VenueSession{
	   COINBASE::COINBASE(const ConfigManager::VenueSessionInfo& vsinfo): AbstractVenue(vsinfo){
	   
	      transport = std::make_unique<Transport::WssTransport>(&md_hub);
              registerCallBacks();
              MD_LOG_INFO << " Created COINBASE Session " << '\n'; // logger - later
	   }
	   void COINBASE::onConnection(uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req)
	   {
	   
	   }
	   void COINBASE::onUpdate(uWS::WebSocket<uWS::CLIENT> *ws, char * data, std::size_t len_, uWS::OpCode code)
	   {
	   
	   }
	   void COINBASE::onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, std::size_t length){
	   
	   } 
          	
           void COINBASE::registerCallBacks()
           {
             using namespace std::placeholders;
             transport->registerOnConnectCallBack(std::bind(&COINBASE::onConnection, this,_1,_2));
             transport->registerOnUpdateCallBack(std::bind(&COINBASE::onUpdate, this,_1,_2,_3,_4));
             transport->registerOnDisconnectCallback(std::bind(&COINBASE::onDisconnection, this,_1,_2,_3,_4));          

           }
            
           void COINBASE::decode(char* buffer, size_t len, const TimeType& packet_hit_time)
           {

           }
    }
  }

}

