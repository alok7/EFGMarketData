#include <MarketData/BINANCE_FUTURES/BINANCE_FUTURES.h>

#define BUFLEN 65536

namespace EFG
{
  namespace MarketData
  {
    namespace VenueSession
    {
      BINANCE_FUTURES::BINANCE_FUTURES(const ConfigManager::VenueSessionInfo &vsinfo) : AbstractVenue(vsinfo)
      {
        transport = std::make_unique<Transport::WssTransport>(&md_hub);
        registerCallBacks();
        MD_LOG_INFO << "Created BINANCE FUTURES Session " << '\n'; // logger - later
      }
      void BINANCE_FUTURES::onConnection(uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req)
      {
        MD_LOG_INFO << "BINANCE FUTURES Connected " << '\n';
        subscriptionHandler->init(ws);
        subscriptionHandler->run();
        MD_LOG_INFO << "Started Susbcription Handler for BINANCE FUTURES" << '\n';
      }
      void BINANCE_FUTURES::onUpdate(uWS::WebSocket<uWS::CLIENT> *ws, char *message, std::size_t len_, uWS::OpCode code)
      {
        try
        {
          TimeType packet_hit_time = Utils::getHRTime();
          //MD_LOG_INFO << " Decoding incoming data " << std::string(message, len_) << '\n';
          decode(message, len_, packet_hit_time);
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
      void BINANCE_FUTURES::onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, std::size_t length)
      {
        try
        {
          MD_LOG_INFO << "BINANCE FUTURES Disconnected" << std::to_string(Utils::getHRTime()) << '\n';
          subscriptionHandler->stop();
          subscriptionHandler = std::make_unique<VenueSession::SubscriptionHandler>();
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

      void BINANCE_FUTURES::registerCallBacks()
      {
        using namespace std::placeholders;
        transport->registerOnConnectCallBack(std::bind(&BINANCE_FUTURES::onConnection, this, _1, _2));
        transport->registerOnUpdateCallBack(std::bind(&BINANCE_FUTURES::onUpdate, this, _1, _2, _3, _4));
        transport->registerOnDisconnectCallback(std::bind(&BINANCE_FUTURES::onDisconnection, this, _1, _2, _3, _4));
      }
      void BINANCE_FUTURES::decode(char *buffer, size_t len, const TimeType &packet_hit_time)
      {
        Document decoder;

        if (decoder.Parse(buffer, len).HasParseError())
        {
          MD_LOG_INFO << "Not able to parse response " << '\n'; // logger
          return;
        }
        //get the update type
        unsigned char i = 0, pos = 0;
        unsigned char indexDoubleQuote[] = {0, 0, 0, 0, 0, 0, 0, 0}; // index of 1st quote, 2nd quote and so on;
        while (pos < 8 && i < len)
        {
          if (buffer[i] == '"')
            indexDoubleQuote[pos++] = i;
          ++i;
        }
        // response type between first and second quote
        std::string response_type(buffer + indexDoubleQuote[0] + 1, indexDoubleQuote[1] - indexDoubleQuote[0] - 1);
        switch (Utils::hash_32_fnv1a(response_type.c_str()))
        {
        case Utils::hash_32_fnv1a("stream"):
        {

          // get the event type
          std::string stream_type(buffer + indexDoubleQuote[2] + 1, indexDoubleQuote[3] - indexDoubleQuote[2] - 1);
          std::string event = stream_type.substr(stream_type.find("@") + 1);
          switch (Utils::hash_32_fnv1a(event.c_str()))
          {

          case Utils::hash_32_fnv1a("trade"):
          {
            // MD_LOG_INFO << "BINANCE Trade: " << buffer << '\n';

            using normalized_msg_type = Message::Trade;
            const Value &data = decoder["data"];
            normalized_msg_type normalized_trade(Utils::convertToPrice(data["p"].GetString()),
                                                 std::stod(data["q"].GetString()),
                                                 data["m"].GetBool() ? 'S' : 'B');
	    std::string instrument_id = data["s"].GetString();
            strcpy(normalized_trade.symbol, instrument_id.c_str());
            normalized_trade.venue = MarketUtils::MarketType::BINANCE_FUTURES;
            normalized_trade.apiIncomingTime = packet_hit_time;
	    lastTrade = normalized_trade;
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_trade));

            break;
          }

          case Utils::hash_32_fnv1a("depth@100ms"):
          case Utils::hash_32_fnv1a("depth@500ms"):
          case Utils::hash_32_fnv1a("depth@250ms"):
          case Utils::hash_32_fnv1a("depth@0ms"):
          case Utils::hash_32_fnv1a("depth"):
          {
            /*		  
            if (bookSnapshotLastUpdateId >= decoder["data"]["u"].GetInt64())
            {
              return;
            }
            using normalized_msg_type = Message::DepthBook;
            using order_side = EFG::MarketData::order_book_side;
            normalized_msg_type normalized_depth;
            const Value &bid_updates = decoder["data"]["b"];
            const Value &ask_updates = decoder["data"]["a"];
            std::string symb = decoder["data"]["s"].GetString();
            for (SizeType i = 0; i < bid_updates.Size(); i++)
            {
              int64_t price = Utils::convertToPrice(bid_updates[i][0].GetString());
              double qty = std::stod(bid_updates[i][1].GetString());
              uint64_t orders_count = 1;
              auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::bid, price);
              if (it->first == price)
              {
                // price already exists
                if (0 == qty) // Delete
                {
                  this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).remove(order_side::bid, it);
                }
                else // Modify
                {
                  it->second = normalized_msg_type::price_level(price, qty, orders_count);
                }
              }
              else // Price level doesn't exist, Insert
              {
                if (qty == 0)
                  continue; // inserted order cannot be of zero size
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price,
                                                                                                 normalized_msg_type::price_level(price, qty, orders_count));
              }
            }

            for (SizeType i = 0; i < ask_updates.Size(); i++)
            {
              int64_t price = Utils::convertToPrice(ask_updates[i][0].GetString());
              double qty = std::stod(ask_updates[i][1].GetString());
              uint64_t orders_count = 1;
              auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::ask, price);
              if (it->first == price)
              {
                // price already exists
                if (0 == qty) // Delete
                {
                  this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).remove(order_side::ask, it);
                }
                else // Modify
                {
                  it->second = normalized_msg_type::price_level(price, qty, orders_count);
                }
              }
              else // Price level doesn't exist, Insert
              {
                if (qty == 0)
                  continue; // inserted order cannot be of zero size
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price,
                                                                                                 normalized_msg_type::price_level(price, qty, orders_count));
              }
            }
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
	    std::string instrument_id = decoder["data"]["s"].GetString();
            //strcpy(normalized_depth.symbol, instrument_id.c_str());

            normalized_depth.venue = MarketUtils::MarketType::BINANCE_FUTURES;
            normalized_depth.apiIncomingTime = packet_hit_time;
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
	    */
            break;
      }

      case Utils::hash_32_fnv1a("bookTicker"): //bookTicker
      {

        using normalized_msg_type = Message::Ticker;
        const Value &data = decoder["data"];
        normalized_msg_type normalized_ticker(0,
                                              Utils::convertToPrice(data["b"].GetString()),
                                              Utils::convertToPrice(data["a"].GetString()),
                                              0,
                                              0,
                                              0,
                                              std::stod(data["B"].GetString()),
                                              std::stod(data["A"].GetString()),
                                              0);
        normalized_ticker.apiIncomingTime = packet_hit_time;
	normalized_ticker.last_price = lastTrade.price;
        normalized_ticker.venue = MarketUtils::MarketType::BINANCE_FUTURES;
	std::string instrument_id = data["s"].GetString();
        strcpy(normalized_ticker.symbol, instrument_id.c_str());
        MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_ticker));
        break;
      }
      case Utils::hash_32_fnv1a("depth5@0ms"):
      case Utils::hash_32_fnv1a("depth10@0ms"):
      case Utils::hash_32_fnv1a("depth20@0ms"):
      case Utils::hash_32_fnv1a("depth5@100ms"):
      case Utils::hash_32_fnv1a("depth10@100ms"):
      case Utils::hash_32_fnv1a("depth20@100ms"):
      case Utils::hash_32_fnv1a("depth5@250ms"):
      case Utils::hash_32_fnv1a("depth10@250ms"):
      case Utils::hash_32_fnv1a("depth20@250ms"):
      case Utils::hash_32_fnv1a("depth5@500ms"):
      case Utils::hash_32_fnv1a("depth10@500ms"):
      case Utils::hash_32_fnv1a("depth20@500ms"):
      {
        //if (bookSnapshotLastUpdateId > 0)
        //return;
        using normalized_msg_type = Message::DepthBook;
        std::string symb = decoder["data"]["s"].GetString();
        transform(symb.begin(), symb.end(), symb.begin(), ::toupper);
        this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
        const Value &bids_data = decoder["data"]["b"];
        const Value &asks_data = decoder["data"]["a"];

        using order_side = EFG::MarketData::order_book_side;
        normalized_msg_type normalized_depth;
        for (SizeType i = 0; i < bids_data.Size(); i++)
        {
          int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, std::stod(bids_data[i][1].GetString()), 1));
        }
        for (SizeType i = 0; i < asks_data.Size(); i++)
        {
          int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, std::stod(asks_data[i][1].GetString()), 1));
        }
        this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
        this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
        normalized_depth.apiIncomingTime = packet_hit_time;
        strcpy(normalized_depth.symbol, symb.c_str());
        normalized_depth.venue = MarketUtils::MarketType::BINANCE_FUTURES;
        MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
        break;
      }
      }
      break;
    }
    }
  }
  void BINANCE_FUTURES::decodeBinary(char *buffer, size_t len)
  {
  }

}
}
}
