#include <MarketData/KITE_ZERODHA/KITE_ZERODHA.h>

namespace EFG { namespace MarketData { namespace VenueSession{
  
  KITE_ZERODHA::KITE_ZERODHA(const ConfigManager::VenueSessionInfo &vsinfo) : AbstractVenue(vsinfo)
  {
    auto& apiKey = vsinfo.getApiKey();
    auto& secretKey = vsinfo.getSecretKey();
    auto& params = const_cast<std::unordered_map<std::string,std::string>&>(vsinfo.getExtraParams());
    std::string requestToken = params["requestToken"];
    std::cout << "request token " << requestToken << '\n';
    std::cout << "secret key " << secretKey << '\n';
    std::cout << "api key " << apiKey << '\n';
    std::string checksum = sha256(apiKey+requestToken+secretKey);
    mKite = std::make_unique<kc::kiteWS>(&md_hub, apiKey, 5, true, 5);
    registerCallBacks();
    EFG::Core::Transport::HTTPConfig httpConfig;
    httpConfig.setHttpEndpoint(vsinfo.getHttpEndPoint());
    httpRequest = std::make_unique<Transport::HTTPRequest>(httpConfig);

    std::unordered_map<std::string, std::string> payload;
    add(payload, "api_key", apiKey);
    add(payload, "request_token", requestToken);
    add(payload, "checksum", checksum);
    std::string queryStr = Transport::HTTPRequest::BuildQuery(payload);
    setHeader();
    const std::string& response = httpRequest->post("/token", queryStr); 
    Document decoder;
    decoder.Parse(response.c_str()); 
    std::string accessToken = decoder["data"]["access_token"].GetString();
    mKite->setAccessToken(accessToken); 
    MD_LOG_INFO << "Created KITE_ZERODHA Session, response " << response << '\n';
    std::cout << "Created KITE_ZERODHA Session, accessToken " << accessToken << '\n';
     
  }
  void KITE_ZERODHA::onConnection(kc::kiteWS *ws)
  {
    MD_LOG_INFO << "KITE_ZERODHA Connected " << '\n'; 
    std::cout << "KITE_ZERODHA Connected " << '\n'; 
    MD_LOG_INFO << "Submit subscriptions to KITE_ZERODHA" << '\n';
    //ws->setMode("full", {8972290,12481794}); // subscribing NIFTY23JANFUT, NIFTY23FEBFUT
    ws->setMode("full", {8972290}); // subscribing NIFTY23JANFUT
  }
  void KITE_ZERODHA::onUpdate(kc::kiteWS *ws, const std::vector<kc::tick>& ticks)
  {
    TimeType packet_hit_time = Utils::getHRTime();
    for(const auto& i : ticks)
    {
      using normalized_msg_type = Message::DepthBook;
      normalized_msg_type normalized_depth;
      normalized_depth.apiIncomingTime = packet_hit_time;
      normalized_depth.venue           = MarketUtils::MarketType::KITE_ZERODHA;
      
      // trade side classification algo -- Lee and Ready, Pan and Poteshman  
      bool isBuy = (i.lastPrice >= (i.marketDepth.buy[0].price + i.marketDepth.sell[0].price)/2.0);
      std::string side = isBuy ? "Buy" : "Sell";
      MD_LOG_INFO << "side " << side << " last price " << i.lastPrice
      << " last qty " << i.lastTradedQuantity << " pending buy " << i.totalBuyQuantity << " pending sell " << i.totalSellQuantity
      << " open " << i.OHLC.open << " high " << i.OHLC.high << " low " << i.OHLC.low << " close " << i.OHLC.close << '\n';
      
      int it = 0;
      for(auto& b : i.marketDepth.buy)
      {
        normalized_depth.bids[it].price    = b.price;
        normalized_depth.bids[it].qty      = b.quantity;
        //MD_LOG_INFO << " bid: price " << b.price << " qty " << b.quantity << " orders " << b.orders << '\n';
        //std::cout << " bid: price " << b.price << " qty " << b.quantity << " orders " << b.orders << '\n';
        ++it;
      }
      it = 0;
      for(auto& a : i.marketDepth.sell)
      {
        normalized_depth.asks[it].price    = a.price;
        normalized_depth.asks[it].qty      = a.quantity;
        //MD_LOG_INFO << " ask: price " << a.price << " qty " << a.quantity << " orders " << a.orders << '\n';
        //std::cout << " ask: price " << a.price << " qty " << a.quantity << " orders " << a.orders << '\n';
        ++it;
      }
      MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
      MD_LOG_INFO << "best bid: price " << i.marketDepth.buy[0].price << " qty " << i.marketDepth.buy[0].quantity << '\n';
      MD_LOG_INFO << "best ask: price " << i.marketDepth.sell[0].price << " qty " << i.marketDepth.sell[0].quantity << '\n';

      Message::AggregateTrade normalized_trade;
      normalized_trade.last_price      = i.lastPrice;
      normalized_trade.bid_price       = i.marketDepth.buy[0].price;
      normalized_trade.ask_price       = i.marketDepth.sell[0].price;
      normalized_trade.high_price      = i.OHLC.high; // day high
      normalized_trade.low_price       = i.OHLC.low; // day low
      normalized_trade.open_price      = i.OHLC.open; // market open price
      normalized_trade.close_price     = i.OHLC.close; // last trading day close
      //normalized_trade.avg_price     = i.averageTradePrice; // vwap
      normalized_trade.last_size       = i.lastTradedQuantity;
      if(isBuy)
      {
        normalized_trade.bid_size      = i.lastTradedQuantity;
        normalized_trade.ask_size     = 0;
      }
      else
      {
        normalized_trade.ask_size      = i.lastTradedQuantity;
        normalized_trade.bid_size     = 0;
      }
      normalized_trade.avg_price       = i.lastPrice;
      normalized_trade.apiIncomingTime = packet_hit_time;
      normalized_trade.venue           = MarketUtils::MarketType::KITE_ZERODHA;
      
      //normalized_trade.mInstrumentId = i.instrumentToken;
      MarketDataEventQueue<Message::AggregateTrade>::Instance().push(std::move(normalized_trade));
    }
  }
  void KITE_ZERODHA::onError(kc::kiteWS *ws, int code, const std::string& message)
  {
    MD_LOG_INFO << "Error code" << code << " message " << message << '\n';
    std::cout << "Error code" << code << " message " << message << '\n';
  }
  void KITE_ZERODHA::onConnectionError(kc::kiteWS *ws)
  {
    MD_LOG_INFO << "Couldn't connect "<< '\n';
    std::cout << "Couldn't connect "<< '\n';
  }
  void KITE_ZERODHA::onDisconnection(kc::kiteWS *ws, int code, const std::string& message)
  {
    MD_LOG_INFO << "Closed the connection, code " << code << " message " << message << '\n';
    std::cout << "Closed the connection, code " << code << " message " << message << '\n';
  }
  void KITE_ZERODHA::registerCallBacks()
  {
    using namespace std::placeholders;
    mKite->onConnect = std::bind(&KITE_ZERODHA::onConnection, this, _1);
    mKite->onTicks = std::bind(&KITE_ZERODHA::onUpdate, this, _1, _2);
    mKite->onError = std::bind(&KITE_ZERODHA::onError, this, _1, _2, _3);
    mKite->onConnectError = std::bind(&KITE_ZERODHA::onConnectionError, this, _1);
    mKite->onClose = std::bind(&KITE_ZERODHA::onDisconnection, this, _1, _2, _3);
  }

}}}
