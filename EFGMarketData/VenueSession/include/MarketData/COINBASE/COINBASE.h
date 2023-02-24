#pragma once
#include <MarketData/AbstractVenue/AbstractVenue.h>
#include <MarketData/CommonUtils.h>

using namespace EFG::MarketData;

namespace EFG{
  namespace MarketData{
    namespace VenueSession{
      class COINBASE : public AbstractVenue{
        public:
	   COINBASE(const ConfigManager::VenueSessionInfo&);
	   ~COINBASE(){};
	   void onConnection(uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req);
	   void onUpdate(uWS::WebSocket<uWS::CLIENT> *ws, char * data, std::size_t len_, uWS::OpCode code);
	   void onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, std::size_t length);
	
           void registerCallBacks();
           std::unique_ptr<Transport::WssTransport> transport;
           template<typename ...EXTRA>
           void connect(std::string url, EXTRA... args)
           {
             transport->connect(url, args...); 
           }
           void decode(char* buffer, size_t len, const TimeType&);
      };

    }
  }

}

