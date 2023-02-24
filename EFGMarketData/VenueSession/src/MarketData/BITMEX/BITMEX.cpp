#include <MarketData/BITMEX/BITMEX.h>
#include <chrono>

#define BUFLEN 65536

namespace EFG
{
  namespace MarketData
  {
    namespace VenueSession
    {
      BITMEX::BITMEX(const ConfigManager::VenueSessionInfo &vsinfo) : AbstractVenue(vsinfo)
      {
        transport = std::make_unique<Transport::WssTransport>(&md_hub);
        registerCallBacks();
        MD_LOG_INFO << " Created BITMEX Session " << '\n'; // logger - later
      }
      void BITMEX::onConnection(uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req)
      {
        MD_LOG_INFO << " BITMEX Connected " << '\n';
        MD_LOG_INFO << " Starting Susbcription Handler for BITMEX " << '\n';
        auto timerCallBack = [ws]() {
          MD_LOG_INFO << "BITMEX Sending the ping after 4 secs " << '\n';
          ws->send("ping");
        };
        timer = std::make_unique<Timer::ConditionalTimer>(timerCallBack, 4000); // if no update from xch for 4 sec, send ping
        timer->start();

        /* don't need auth for MD */
        // const std::string &api_key = venueSessionInfo.getApiKey();
        // const std::string &secret_key = venueSessionInfo.getSecretKey();
        // int expires = time(NULL) + 5;
        // std::string method = "GET";
        // std::string endpoint = "/realtime";
        // json postdict;
        // std::string sign = getSign(secret_key, expires, method, endpoint, postdict);

        // json apiKeyObject;
        // apiKeyObject["op"] = "authKeyExpires";
        // apiKeyObject["args"] = json::array({api_key, expires, sign});
        // ws->send(apiKeyObject.dump().c_str());

        subscriptionHandler->init(ws);
        subscriptionHandler->run();
      }
      
      void BITMEX::onUpdate(uWS::WebSocket<uWS::CLIENT> *ws, char *message, std::size_t len_, uWS::OpCode code)
      {
        try
        {
          timer->resetTimer();
          TimeType packet_hit_time = Utils::getHRTime();
          // message is not necessarily null terminated, null terminate the message
          char new_msg[len_ + 1] = {0};
          memcpy(new_msg, message, len_);
          //MD_LOG_INFO << " Decoding incoming data " << std::string(new_msg, len_) << '\n';

          decode(new_msg, len_, packet_hit_time);
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
      void BITMEX::onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, std::size_t length)
      {

        try
        {
          MD_LOG_INFO << " BITMEX Disconnected " << std::to_string(Utils::getHRTime()) << '\n';
          subscriptionHandler->stop();
          timer->stop();
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

      void BITMEX::registerCallBacks()
      {
        using namespace std::placeholders;
        transport->registerOnConnectCallBack(std::bind(&BITMEX::onConnection, this, _1, _2));
        transport->registerOnUpdateCallBack(std::bind(&BITMEX::onUpdate, this, _1, _2, _3, _4));
        transport->registerOnDisconnectCallback(std::bind(&BITMEX::onDisconnection, this, _1, _2, _3, _4));
      }

      void BITMEX::decode(char *buffer, size_t len, const TimeType &packet_hit_time)
      {

        Document decoder;
        if (decoder.Parse(buffer, len).HasParseError())
        {
          if ("pong" == std::string(buffer, len))
          {
            MD_LOG_INFO << "pong received" << '\n';
          }
          else
          {
            MD_LOG_INFO << "Not able to parse response " << '\n'; // logger
          }
          return;
        }
        auto timestampToEpoch = [](const std::string& timestamp) -> TimeType {
          std::string t_sec = timestamp.substr(0, 19), t_ms = timestamp.substr(20, 3);
          std::tm t = {};
          std::istringstream ss(t_sec);
          if (ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S")) {
            TimeType t_epoch_ns = TimeType(std::mktime(&t)) * 1000000000 + TimeType(stoi(t_ms)) * 1000000;
            return t_epoch_ns;
          }
          std::cerr << "Timestamp parse failed!" << "\n";

          return 0;
        };
        //get the update type
        unsigned char i = 0, pos = 0;
        unsigned char indexDoubleQuote[] = {0, 0, 0, 0}; // index of 1st quote, 2nd quote and so on;
        while (pos < 4 && i < len)
        {
          if (buffer[i] == '"')
            indexDoubleQuote[pos++] = i;
          ++i;
        }
        // response type between third and fourth quote
        std::string response_type(buffer + indexDoubleQuote[2] + 1, indexDoubleQuote[3] - indexDoubleQuote[2] - 1);

        switch (Utils::hash_32_fnv1a(response_type.c_str()))
        {
        case Utils::hash_32_fnv1a("orderBookL2_25"):
        case Utils::hash_32_fnv1a("orderBookL2"):
        case Utils::hash_32_fnv1a("orderBook10"):
        {
          const Value &data = decoder["data"];

          using normalized_msg_type = Message::DepthBook;
          using order_side = EFG::MarketData::order_book_side;
          static auto get_type = [this](char side) {
            switch (side)
            {
            case 'S':
              return order_side::ask;
            case 'B':
              return order_side::bid;
            }
            return order_side::undef;
          };

          normalized_msg_type normalized_depth;
          
          const char *verifyAction = decoder["action"].GetString();

          if (verifyAction[0] != 'p' && this->isPartialReceived == false)
          {
            MD_LOG_INFO << "Partial not yet received, cannot process message " << '\n';
            break;
          }
          // MD_LOG_INFO << "Valid message received" << "\n";
          
          if(data.Size()==0) break;

          switch (verifyAction[0])
          {
          case 'p': // partial
          {
            this->isPartialReceived = true;
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(data[0]["symbol"].GetString()).clear();
            this->priceIdMap.clear();
            for (SizeType i = 0; i < data.Size(); i++)
            {
              int64_t price = data[i]["price"].GetDouble() * 1000000000;
              double size = data[i]["size"].GetDouble();
              int64_t id = data[i]["id"].GetInt64();
              uint64_t orders_count = 1;
              char side = data[i]["side"].GetString()[0];
              std::string symb = data[i]["symbol"].GetString();
              this->priceIdMap[id] = price;
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(get_type(side), price, normalized_msg_type::price_level(price, size, orders_count));
            }
            break;
          }

          case 'u': // update
          {
            for (SizeType i = 0; i < data.Size(); i++)
            {
              double qty = data[i]["size"].GetDouble();
              int64_t id = data[i]["id"].GetInt64();
              char side = data[i]["side"].GetString()[0];
              int64_t price = this->priceIdMap[id];
              std::string symb = data[i]["symbol"].GetString();
              auto& lob = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb);
              auto it = lob.find(get_type(side), price);
              if (it!=lob.end(get_type(side)) && it->first == price)
              {
                if (qty == 0)
                {
                  this->priceIdMap.erase(id);
                  this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).remove(get_type(side), it);
                }
                else
                {
                  it->second.qty = qty;
                }
              }
            }
            break;
          }

          case 'd': // delete
          {
            for (SizeType i = 0; i < data.Size(); i++)
            {
              int64_t id = data[i]["id"].GetInt64();
              char side = data[i]["side"].GetString()[0];
              int64_t price = this->priceIdMap[id];
              std::string symb = data[i]["symbol"].GetString();
              auto& lob = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb);
              auto it = lob.find(get_type(side), price);
              if (it!= lob.end(get_type(side)) && it->first == price) 
              {
                this->priceIdMap.erase(id);
                lob.remove(get_type(side), it);
              }

            }
            break;
          }

          case 'i': // insert
          {
            for (SizeType i = 0; i < data.Size(); i++)
            {
              int64_t price = data[i]["price"].GetDouble() * 1000000000;
              double qty = data[i]["size"].GetDouble();
              int64_t id = data[i]["id"].GetInt64();
              uint64_t orders_count = 1;
              char side = data[i]["side"].GetString()[0];
              this->priceIdMap[id] = price;
              std::string symb = data[i]["symbol"].GetString();
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(get_type(side), price, normalized_msg_type::price_level(price, qty, orders_count));
            }
            break;
          }
          default:
          {
            MD_LOG_INFO << "Invalid message received, breaking...\n";
            break;
          }
          }
          std::string symb = data[0]["symbol"].GetString();
          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
          normalized_depth.apiIncomingTime = packet_hit_time;
          //normalized_depth.symbol = data[0]["symbol"].GetString();
          normalized_depth.venue = MarketUtils::MarketType::BITMEX;
          MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
          break;
        }

          // Example quote output:
          /* [{'timestamp': '2020-06-16T05:18:23.953Z', 'symbol': 'XBTUSD', 'bidSize': 203, 'bidPrice': 9537.5, 'askPrice': 9538, 'askSize': 686},
        * {'timestamp': '2020-06-16T05:18:42.157Z', 'symbol': 'XBTUSD', 'bidSize': 18963, 'bidPrice': 9537.5, 'askPrice': 9538, 'askSize': 686},
        * {'timestamp': '2020-06-16T05:18:44.692Z', 'symbol': 'XBTUSD', 'bidSize': 18963, 'bidPrice': 9537.5, 'askPrice': 9538, 'askSize': 288}]
        */

        case Utils::hash_32_fnv1a("quote"):
        case Utils::hash_32_fnv1a("quoteBin1m"):
        case Utils::hash_32_fnv1a("quoteBin5m"):
        case Utils::hash_32_fnv1a("quoteBin1h"):
        case Utils::hash_32_fnv1a("quoteBin1d"):
        {

          /*if(this->isPartialReceived == false)
          {
            return;
          }
          */
          using normalized_msg_type = Message::Ticker;

          const Value &data = decoder["data"];
          // std::vector<msg_type::level> levels;
          // msg_type::update_type action;

          for (SizeType i = 0; i < data.Size(); i++)
          {
            normalized_msg_type normalized_ticker;

            normalized_ticker.bid_size = data[i]["bidSize"].GetDouble();
            normalized_ticker.bid_price = data[i]["bidPrice"].GetDouble() * 1000000000;
            normalized_ticker.ask_price = data[i]["askPrice"].GetDouble() * 1000000000;
            normalized_ticker.ask_size = data[i]["askSize"].GetDouble();

            //normalized_ticker.symbol = data[i]["symbol"].GetString();
            normalized_ticker.apiIncomingTime = packet_hit_time;
            //normalized_ticker.exchangeOutgoingTime = timestampToEpoch(data[i]["timestamp"].GetString());
            normalized_ticker.venue = MarketUtils::MarketType::BITMEX;
            // levels.emplace_back(symbol, bidSize, bidPrice, askPrice, askSize);
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_ticker));
          }
          break;
        }
        case Utils::hash_32_fnv1a("tradeBin1m"):
        case Utils::hash_32_fnv1a("tradeBin5m"):
        case Utils::hash_32_fnv1a("tradeBin1h"):
        case Utils::hash_32_fnv1a("tradeBin1d"):
        case Utils::hash_32_fnv1a("trade"):
        {
          /*if(this->isPartialReceived == false)
          {
            return;
          }
          */
          // MD_LOG_INFO << "BITMEX Trade: " << buffer << '\n';
          const Value &data = decoder["data"];
          using normalized_msg_type = Message::Trade;
          for (int i = data.Size()-1; i >= 0; i--)
          {
            normalized_msg_type normalized_trade(
                data[i]["price"].GetDouble() * 1000000000,
                data[i]["size"].GetInt64(),
                tolower(data[i]["side"].GetString()[0]));
            //normalized_trade.symbol = data[i]["symbol"].GetString();
            normalized_trade.venue = MarketUtils::MarketType::BITMEX;
            normalized_trade.apiIncomingTime = packet_hit_time;
            //normalized_trade.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_trade));
          }
          break;
        }

        case Utils::hash_32_fnv1a("liquidation") :
        {          
          /**
           * msg_type here doesn't matter because
           * 1. BITMEX liquidation messages are the same for FUTURES/SWAPS/...
           * 2. The message will be received by MarketStrategy regardless of the message type here
           */
          using normalized_msg_type = Message::Liquidation;
          if (decoder.HasMember("data") && decoder["data"].IsArray()) {
            auto& data = decoder["data"];
            for (SizeType i = 0; i < data.Size(); i++) {
              normalized_msg_type message;
              if (data[i].HasMember("price")) {
                message.price = data[i]["price"].GetDouble() * 1000000000;
              }
              else continue;

              message.side = 'N';
              if (data[i].HasMember("side") && !data[i]["side"].IsNull()) {
                message.side = data[i]["side"].GetString()[0];
              }
              if (data[i].HasMember("leavesQty")) {
                message.leaves_size = data[i]["leavesQty"].GetDouble();
              }
              if (data[i].HasMember("symbol")) {
                //message.symbol = data[i]["symbol"].GetString();
              }
              if (data[i].HasMember("orderID") && !(data[i]["orderID"].IsNull()))
              {
                message.order_id = data[i]["orderID"].GetString();
              }
              message.venue = MarketUtils::MarketType::BITMEX;
              message.apiIncomingTime = packet_hit_time;
              MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(message));
            }
          }
          else 
          {
            MD_LOG_INFO << "BITMEX::decode Liquidation response does not have data " << buffer << '\n';
          }
          break;
        }

       }
     }
     void BITMEX::decodeBinary(char *buffer, size_t len)
     {
     }

    } // namespace VenueSession
  }   // namespace MarketData
} // namespace EFG
