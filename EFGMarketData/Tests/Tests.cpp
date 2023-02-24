#include <iostream>
#include <uWS.h>
#include <MarketData/ConfigReader.h>
#include <MarketData/Logger.h>

class Tests{
  public:
    void runTests(){  
      CONFIGReader reader("../Config/VenueInfo.cfg");
      if (reader.ParseError() < 0) {
        MD_LOG_INFO << "Can't load 'VenueInfo.cfg'\n";
        return;
      }
      MD_LOG_INFO << reader.Get("OKEX_SESSION", "wss_endpoint", "UNKNOWN") << "\n"
              << reader.Get("HUOBI_SESSION", "wss_endpoint", "UNKNOWN") << "\n";
      /****************************************************************************/
      uWS::Hub hub;
      hub.connect("wss://real.okex.com:8443/ws/v3");
      hub.onConnection([](uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req) {
       MD_LOG_INFO << "Connected to " << '\n';
      });
      hub.run();
 }
};

