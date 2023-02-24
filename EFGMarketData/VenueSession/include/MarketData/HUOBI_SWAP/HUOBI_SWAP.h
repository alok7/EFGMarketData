#pragma once
#include <MarketData/AbstractVenue/AbstractVenue.h>
#include <MarketData/CommonUtils.h>

using namespace EFG::MarketData;

namespace EFG{
  namespace MarketData{
    namespace VenueSession{
      class HUOBI_SWAP : public AbstractVenue{
        public:
	   HUOBI_SWAP(const ConfigManager::VenueSessionInfo&);
	   ~HUOBI_SWAP(){};
	   void onConnection(uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req) ;
	   void onUpdate(uWS::WebSocket<uWS::CLIENT> *ws, char * data, std::size_t len_, uWS::OpCode code);
	   void onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, std::size_t length);
           
           void registerCallBacks();
           template<typename ...EXTRA>
           void connect(std::string url,EXTRA... args)
           {
             transport->connect(url, args...); 
           }
           void decode(char* buffer, size_t len, uWS::WebSocket<uWS::CLIENT> *ws, const TimeType&);
           void decodeBinary(char* buffer, size_t len);
 	   void reConnect();
           std::unique_ptr<Transport::WssTransport> transport;
 	
      };
      
    }
  }

}

