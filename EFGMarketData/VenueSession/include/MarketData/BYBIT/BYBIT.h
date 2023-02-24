#pragma once
#include <MarketData/AbstractVenue/AbstractVenue.h>
#include <MarketData/CommonUtils.h>
#include <MarketData/Utils/hmac.h>

using namespace EFG::MarketData;

namespace EFG{
  namespace MarketData{
    namespace VenueSession{
      class BYBIT : public AbstractVenue{
        public:
	   BYBIT(const ConfigManager::VenueSessionInfo&);
	   ~BYBIT(){};
	   void onConnection(uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req) ;
	   void onUpdate(uWS::WebSocket<uWS::CLIENT> *ws, char * data, std::size_t len_, uWS::OpCode code);
	   void onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, std::size_t length);
           void registerCallBacks();
           template<typename ...EXTRA>
           void connect(std::string url, EXTRA... args)
           {
             transport->connect(url, args...); 
           }
           void decode(char* buffer, size_t len, const TimeType&);
           void decodeBinary(char* buffer, size_t len);
      };

    }
  }

}

