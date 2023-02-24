#pragma once
#include <MarketData/AbstractVenue/AbstractVenue.h>
#include <MarketData/CommonUtils.h>

using namespace EFG::MarketData;

namespace EFG{
  namespace MarketData{
    namespace VenueSession{
      class BINANCE_OPTIONS : public AbstractVenue{
        public:
	   BINANCE_OPTIONS(const ConfigManager::VenueSessionInfo&);
	   ~BINANCE_OPTIONS(){};
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
           int64_t bookSnapshotLastUpdateId = -1;
           bool orderbook_snapshot_received = false;
      }; 

    }
  }

}

