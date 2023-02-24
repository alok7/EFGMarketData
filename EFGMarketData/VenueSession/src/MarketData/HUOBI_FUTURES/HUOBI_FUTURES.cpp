#include <MarketData/HUOBI_FUTURES/HUOBI_FUTURES.h>
#include <unordered_set>

#define BUFLEN 65536

namespace EFG
{
namespace MarketData
{
  namespace VenueSession
  {
    HUOBI_FUTURES::HUOBI_FUTURES(const ConfigManager::VenueSessionInfo &vsinfo) : AbstractVenue(vsinfo)
    {
      transport = std::make_unique<Transport::WssTransport>(&md_hub);
      registerCallBacks();
      MD_LOG_INFO << "Created  HUOBI_FUTURES Session " << '\n';
    }
    void HUOBI_FUTURES::onConnection(uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req)
    {
      MD_LOG_INFO << " HUOBI_FUTURES Connected " << '\n';
      subscriptionHandler->init(ws);
      subscriptionHandler->run();
    }
    void HUOBI_FUTURES::onUpdate(uWS::WebSocket<uWS::CLIENT> *ws, char *message, std::size_t len_, uWS::OpCode code)
    {
      try
      {
        TimeType packet_hit_time = Utils::getHRTime();
        char data[BUFLEN] = {0};
        int datalen = sizeof(data);
        Utils::gzDecompress(message, len_, data, &datalen, 2);
        //MD_LOG_INFO << " Decoding incoming data " << std::string(data, datalen) << '\n';
        decode(data, datalen, ws, packet_hit_time);
      }
      catch (const std::exception &e)
      {
        std::cerr << "Exception on Decode " << e.what() << '\n';
      }
      catch (...)
      {
        std::cerr << "Unknown Exception on Decode " << '\n';
      }
    }
    void HUOBI_FUTURES::onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, std::size_t length)
    {

      try
      {
        MD_LOG_INFO << " HUOBI_FUTURES Disconnected " << std::to_string(Utils::getHRTime()) << '\n';
        subscriptionHandler->stop();
        subscriptionHandler = std::make_unique<VenueSession::SubscriptionHandler>();
        registerCallBacks();
        this->submitRequests(this->venueSessionInfo.getSubscriptions());
        this->connect(this->venueSessionInfo.getWssEndpoint());
      }
      catch (const std::exception &e)
      {
        std::cerr << "Exception on Disconnect Event " << e.what() << '\n';
      }
      catch (...)
      {
        std::cerr << "Unknown Exception on Disconnet " << '\n';
      }
    }
    void HUOBI_FUTURES::registerCallBacks()
    {
      using namespace std::placeholders;
      transport->registerOnConnectCallBack(std::bind(&HUOBI_FUTURES::onConnection, this, _1, _2));
      transport->registerOnUpdateCallBack(std::bind(&HUOBI_FUTURES::onUpdate, this, _1, _2, _3, _4));
      transport->registerOnDisconnectCallback(std::bind(&HUOBI_FUTURES::onDisconnection, this, _1, _2, _3, _4));
    }
    void HUOBI_FUTURES::decode(char *buffer, size_t len, uWS::WebSocket<uWS::CLIENT> *ws, const TimeType &packet_hit_time)
    {

      Document decoder;
      //get the update type
      unsigned char i = 0, pos = 0;
      unsigned char indexDoubleQuote[] = {0, 0, 0, 0}; // index of 1st quote, 2nd quote and so on;
      while (pos < 4 && i < len)
      {
        if (buffer[i] == '"')
          indexDoubleQuote[pos++] = i;
        ++i;
      }
      // response type between first and second quote
      std::string response_type(buffer + indexDoubleQuote[0] + 1, indexDoubleQuote[1] - indexDoubleQuote[0] - 1);
      switch (Utils::hash_32_fnv1a(response_type.c_str()))
      {
      case Utils::hash_32_fnv1a("ping"):
      {
        if (decoder.Parse(buffer, len).HasParseError())
        {
          MD_LOG_INFO << "Not able to parse hearbeat " << '\n'; // logger
          return;
        }
        char beat[1024] = {0};
        sprintf(beat, "{\"pong\":%ld}", decoder["ping"].GetInt64());
        std::string pong_msg = beat;
        if (ws != nullptr)
          ws->send(pong_msg.c_str());
        break;
      }
      case Utils::hash_32_fnv1a("ch"):
      {

        if (decoder.Parse(buffer, len).HasParseError())
        {
          MD_LOG_INFO << "Not able to parse hearbeat " << '\n'; // logger
          return;
        }
        std::string sub_type = decoder["ch"].GetString();
        std::string update_type = sub_type.substr(sub_type.find(".") + 1).substr(sub_type.substr(sub_type.find(".") + 1).find(".") + 1);
        std::string instrument_id = sub_type.substr(sub_type.find(".") + 1).substr(0, sub_type.substr(sub_type.find(".") + 1).find(".") + 1);
        //MD_LOG_INFO << "update_type " << update_type << '\n' ;
        switch (Utils::hash_32_fnv1a(update_type.c_str()))
        {
        case Utils::hash_32_fnv1a("depth.step0"):
        case Utils::hash_32_fnv1a("depth.step1"):
        case Utils::hash_32_fnv1a("depth.step2"):
        case Utils::hash_32_fnv1a("depth.step3"):
        case Utils::hash_32_fnv1a("depth.step4"):
        case Utils::hash_32_fnv1a("depth.step5"):
        case Utils::hash_32_fnv1a("depth.step6"):
        case Utils::hash_32_fnv1a("depth.step7"):
        case Utils::hash_32_fnv1a("depth.step8"):
        case Utils::hash_32_fnv1a("depth.step9"):
        case Utils::hash_32_fnv1a("depth.step10"):
        case Utils::hash_32_fnv1a("depth.step11"):
        {

          if (decoder.Parse(buffer, len).HasParseError())
          {
            MD_LOG_INFO << "Not able to parse depth " << '\n'; // logger
            return;
          }
          using normalized_msg_type = Message::DepthBook;
          std::string symb = instrument_id;

          const Value &data = decoder["tick"];
          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
          static uint64_t seq = 1;

          using order_side = EFG::MarketData::order_book_side;
          normalized_msg_type normalized_depth;

          const Value &bids_data = data["bids"];
          const Value &asks_data = data["asks"];
          for (SizeType i = 0; i < bids_data.Size(); i++)
          {
            //MD_LOG_INFO << "Bid price " << bids_data[i][0].GetDouble() << ", volume " << bids_data[i][1].GetDouble() << '\n';
            int64_t price = bids_data[i][0].GetDouble() * 1000000000;
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, bids_data[i][1].GetDouble(), 1));
          }

          for (SizeType i = 0; i < asks_data.Size(); i++)
          {
            //MD_LOG_INFO << "Bid price " << asks_data[i][0].GetDouble() << ", volume " << asks_data[i][1].GetDouble() << '\n';
            int64_t price = asks_data[i][0].GetDouble() * 1000000000;
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, asks_data[i][1].GetDouble(), 1));
          }
          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
          normalized_depth.apiIncomingTime = packet_hit_time;
          //strcpy(normalized_depth.symbol, instrument_id.c_str());
          normalized_depth.venue = MarketUtils::MarketType::HUOBI_FUTURES;
          //normalized_depth.incomingSequence = seq++;
          //normalized_depth.exchangeOutgoingTime = data["ts"].GetInt64();
          MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
          break;
        }

        case Utils::hash_32_fnv1a("depth.size_20.high_freq"):  // incremental or snapshot
        case Utils::hash_32_fnv1a("depth.size_150.high_freq"): // incremental or snapshot
        {

          static uint64_t seq = 1;
          using normalized_msg_type = Message::DepthBook;
          using order_side = EFG::MarketData::order_book_side;
          normalized_msg_type normalized_depth;

          const Value &data = decoder["tick"];
          const Value &bids_data = data["bids"];
          const Value &asks_data = data["asks"];
          std::string symb = instrument_id;

          for (SizeType i = 0; i < bids_data.Size(); i++)
          {
            int64_t price = bids_data[i][0].GetDouble() * 1000000000;
            double qty = bids_data[i][1].GetDouble();
            uint64_t orders_count = 1;
            auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::bid, price);
            if (it->first == price) // price exist
            {
              if (0 == qty) // Delete
              {
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).remove(order_side::bid, it);
              }
              else // Modify
              {
                it->second = normalized_msg_type::price_level(price, qty, orders_count);
              }
            }
            else
            { // price doses't exist -- just insert
              if (qty == 0)
                continue; // inserted order cannot be of zero size
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, qty, orders_count));
            }
          }

          for (SizeType i = 0; i < asks_data.Size(); i++)
          {
            int64_t price = asks_data[i][0].GetDouble() * 1000000000;
            double qty = asks_data[i][1].GetDouble();
            uint64_t orders_count = 1;
            auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::ask, price);
            if (it->first == price) // price exist
            {
              if (0 == qty) // Delete
              {
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).remove(order_side::ask, it);
              }
              else // Modify
              {
                it->second = normalized_msg_type::price_level(price, qty, orders_count);
              }
            }
            else
            { // price doses't exist -- just insert
              if (qty == 0)
                continue; // inserted order cannot be of zero size
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, qty, orders_count));
            }
          }

          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
          normalized_depth.apiIncomingTime = packet_hit_time;
          //strcpy(normalized_depth.symbol, instrument_id.c_str());
          normalized_depth.venue = MarketUtils::MarketType::HUOBI_FUTURES;
          //normalized_depth.incomingSequence = seq++;
          //normalized_depth.exchangeOutgoingTime = data["ts"].GetInt64();
          MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
          break;
        }
        case Utils::hash_32_fnv1a("index.1min"):
        case Utils::hash_32_fnv1a("index.5min"):
        case Utils::hash_32_fnv1a("index.15min"):
        case Utils::hash_32_fnv1a("index.30min"):
        case Utils::hash_32_fnv1a("index.60min"):
        case Utils::hash_32_fnv1a("index.4hour"):
        case Utils::hash_32_fnv1a("index.1day"):
        case Utils::hash_32_fnv1a("index.1mon"):
        {

          if (decoder.Parse(buffer, len).HasParseError())
          {
            MD_LOG_INFO << "Not able to parse index " << '\n'; // logger
            return;
          }
          //MD_LOG_INFO << "index " << '\n';
          const Value &data = decoder["tick"];

          break;
        }
        case Utils::hash_32_fnv1a("trade.detail"):
        {

          static uint64_t seq = 1;
          if (decoder.Parse(buffer, len).HasParseError())
          {
            MD_LOG_INFO << "Not able to parse trade detail " << '\n'; // logger
            return;
          }
          const Value &data = decoder["tick"]["data"];
          // std::unordered_set<std::string> symbols;
          
          using normalized_msg_type = Message::Trade;

          for (int i = data.Size()-1; i >= 0; i--)
          {
            auto side = data[i]["direction"].GetString()[0];
            normalized_msg_type normalized_trade(data[i]["price"].GetDouble() * 1000000000,
                                                  data[i]["quantity"].GetDouble(),
                                                  side);
            normalized_trade.venue = MarketUtils::MarketType::HUOBI_FUTURES;
            normalized_trade.apiIncomingTime = packet_hit_time;
            //strcpy(normalized_trade.symbol, instrument_id.c_str());
            //normalized_trade.incomingSequence = seq++;
            //normalized_trade.exchangeOutgoingTime = data[i]["ts"].GetInt64();
            // symbols.insert(normalized_trade.symbol);
            // side == 'b' ? updateLatestBidTrade(normalized_trade) : updateLatestAskTrade(normalized_trade);
            
            // MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_trade));
          }

          // for (const auto& symbol : symbols)
          // {
          //   sendTradeUpdate(MarketUtils::MarketType::HUOBI_FUTURES, symbol);
          // }
          break;
        }
        case Utils::hash_32_fnv1a("detail"):
        {

          if (decoder.Parse(buffer, len).HasParseError())
          {
            MD_LOG_INFO << "Not able to parse detail " << '\n'; // logger
            return;
          }
          break;
        }
        case Utils::hash_32_fnv1a("basis.1min.open"):
        case Utils::hash_32_fnv1a("basis.5min.open"):
        case Utils::hash_32_fnv1a("basis.15min.open"):
        case Utils::hash_32_fnv1a("basis.30min.open"):
        case Utils::hash_32_fnv1a("basis.60min.open"):
        case Utils::hash_32_fnv1a("basis.4hour.open"):
        case Utils::hash_32_fnv1a("basis.1day.open"):
        case Utils::hash_32_fnv1a("basis.1mon.open"):
        case Utils::hash_32_fnv1a("basis.1min.close"):
        case Utils::hash_32_fnv1a("basis.5min.close"):
        case Utils::hash_32_fnv1a("basis.10min.close"):
        case Utils::hash_32_fnv1a("basis.15min.close"):
        case Utils::hash_32_fnv1a("basis.30min.close"):
        case Utils::hash_32_fnv1a("basis.60min.close"):
        case Utils::hash_32_fnv1a("basis.4hour.close"):
        case Utils::hash_32_fnv1a("basis.1day.close"):
        case Utils::hash_32_fnv1a("basis.1mon.close"):
        case Utils::hash_32_fnv1a("basis.1min.high"):
        case Utils::hash_32_fnv1a("basis.5min.high"):
        case Utils::hash_32_fnv1a("basis.10min.high"):
        case Utils::hash_32_fnv1a("basis.15min.high"):
        case Utils::hash_32_fnv1a("basis.30min.high"):
        case Utils::hash_32_fnv1a("basis.60min.high"):
        case Utils::hash_32_fnv1a("basis.4hour.high"):
        case Utils::hash_32_fnv1a("basis.1day.high"):
        case Utils::hash_32_fnv1a("basis.1mon.high"):
        case Utils::hash_32_fnv1a("basis.1min.low"):
        case Utils::hash_32_fnv1a("basis.5min.low"):
        case Utils::hash_32_fnv1a("basis.10min.low"):
        case Utils::hash_32_fnv1a("basis.15min.low"):
        case Utils::hash_32_fnv1a("basis.30min.low"):
        case Utils::hash_32_fnv1a("basis.60min.low"):
        case Utils::hash_32_fnv1a("basis.4hour.low"):
        case Utils::hash_32_fnv1a("basis.1day.low"):
        case Utils::hash_32_fnv1a("basis.1mon.low"):
        {

          if (decoder.Parse(buffer, len).HasParseError())
          {
            MD_LOG_INFO << "Not able to parse basis " << '\n'; // logger
            return;
          }
          //MD_LOG_INFO << "basis " << '\n';
          const Value &data = decoder["tick"];
          break;
        }
        case Utils::hash_32_fnv1a("kline.1min"):
        case Utils::hash_32_fnv1a("kline.5min"):
        case Utils::hash_32_fnv1a("kline.15min"):
        case Utils::hash_32_fnv1a("kline.30min"):
        case Utils::hash_32_fnv1a("kline.60min"):
        case Utils::hash_32_fnv1a("kline.4hour"):
        case Utils::hash_32_fnv1a("kline.1day"):
        case Utils::hash_32_fnv1a("kline.1mon"):
        case Utils::hash_32_fnv1a("kline.1week"):
        case Utils::hash_32_fnv1a("kline.1year"):
        {
          break;
        }
        default:
        {
        }
        }
        break;
      }
      case Utils::hash_32_fnv1a("id"):
      {

        if (decoder.Parse(buffer, len).HasParseError())
        {
          MD_LOG_INFO << "Not able to parse sub/unsub event " << '\n'; // logger
          return;
        }
        const Value &status = decoder["status"];
        if (status.GetString() == "ok")
        {
          if (decoder.HasMember("subbed"))
          {
            MD_LOG_INFO << "Subscribed sucessfully " << decoder["subbed"].GetString() << '\n'; // logg
          }
          else
          {
            MD_LOG_INFO << "Unsubscribed sucessfully " << decoder["unsubbed"].GetString() << '\n'; // logg
          }
        }
        break;
      }
      default:
      {
      }
      }
    }
    void HUOBI_FUTURES::decodeBinary(char *buffer, size_t len)
    {
    }
  }
}

}
