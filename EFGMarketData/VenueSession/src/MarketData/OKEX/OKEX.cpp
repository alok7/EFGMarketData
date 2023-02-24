#include <MarketData/OKEX/OKEX.h>
#include <iterator>
#include <unordered_set>

#define BUFLEN 65536

namespace EFG
{
  namespace MarketData
  {
    namespace VenueSession
    {
      OKEX::OKEX(const ConfigManager::VenueSessionInfo &vsinfo) : AbstractVenue(vsinfo)
      {
        registerCallBacks();
        MD_LOG_INFO << "Created OKEX Session " << '\n'; // logger - later
      }
      void OKEX::onConnection(uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req)
      {
        MD_LOG_INFO << "OKEX Connected" << '\n';
        //{"op":"login","args":["<api_key>","<passphrase>","<timestamp>","<sign>"]}
        auto timerCallBack = [ws]() {
          MD_LOG_INFO << "OKEX Sending the ping after 4 secs " << '\n';
          ws->send("ping");
        };
        timer = std::make_unique<Timer::ConditionalTimer>(timerCallBack, 4000); // if no update from xch for 4 sec, send ping
        timer->start();

        /* don't need auth for MD */
        // these keys from config - later
        // const std::string &api_key = venueSessionInfo.getApiKey();
        // const std::string &secret_key = venueSessionInfo.getSecretKey();
        // const std::unordered_map<std::string,std::string>& extra_param = venueSessionInfo.getExtraParams();
        // std::string passphrase = (extra_param.find("passphrase"))->second;

        // auto getTime = [](char *timestamp) {
        //   time_t t;
        //   time(&t);
        //   sprintf(timestamp, "%ld", t);
        // };
        // char timestamp[32];
        // getTime(timestamp);
        // std::string sign = Utils::GetSign(secret_key, timestamp, "GET", "/users/self/verify", "");

        // json loginObj;
        // loginObj["op"] = "login";
        // loginObj["args"] = json::array({api_key, passphrase, timestamp, sign});
        // ws->send(loginObj.dump().c_str());
        subscriptionHandler->init(ws);
        subscriptionHandler->run(); // run async mode
      }
      void OKEX::onUpdate(uWS::WebSocket<uWS::CLIENT> *ws, char *message, std::size_t len_, uWS::OpCode code)
      {
        try
        {
          timer->resetTimer();
          TimeType packet_hit_time = Utils::getHRTime();
          char data[BUFLEN] = {0};
          int datalen = sizeof(data);
          Utils::gzDecompress(message, len_, data, &datalen, 0);
          // MD_LOG_INFO << "Decoding incoming data " << std::string(data, datalen) << ", apiIncomingTime "<< packet_hit_time <<'\n';
          decode(data, datalen, packet_hit_time);
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
      void OKEX::onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, std::size_t length)
      {
        try
        {
          MD_LOG_INFO << "OKEX Disconnected " << std::to_string(Utils::getHRTime()) << '\n'; // logger - later!
          timer->stop();
          subscriptionHandler->stop();
          subscriptionHandler = std::make_unique<VenueSession::SubscriptionHandler>(); //move assignment, will deallocate the prev object
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
      void OKEX::registerCallBacks()
      {
        using namespace std::placeholders;
        transport->registerOnConnectCallBack(std::bind(&OKEX::onConnection, this, _1, _2));
        transport->registerOnUpdateCallBack(std::bind(&OKEX::onUpdate, this, _1, _2, _3, _4));
        transport->registerOnDisconnectCallback(std::bind(&OKEX::onDisconnection, this, _1, _2, _3, _4));
      }
      void OKEX::decode(char *buffer, size_t len, const TimeType &packet_hit_time)
      {
        Document decoder;
        if (decoder.Parse(buffer, len).HasParseError())
        {
          if (std::string(buffer, len) == "pong") return;
          MD_LOG_INFO << "Not able to parse update from okex " << std::string(buffer, len) << '\n'; // logger
          return;
        }
        auto timestampToEpoch = [](const std::string& timestamp) -> TimeType {
          std::string t_sec = timestamp.substr(0, 19);
          std::string t_ms = "0";
          
          // sometimes the timestamp does not have ms
          if (timestamp.length() > 19) 
            t_ms = timestamp.substr(20, 3);
          std::tm t = {};
          std::istringstream ss(t_sec);
          if (ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S")) {
            int64_t t_epoch_ns = int64_t(std::mktime(&t)) * 1000000000 + int64_t(stoi(t_ms)) * 1000000;
            return t_epoch_ns;
          }
          MD_LOG_INFO << "OKEX::timestampToEpoch Timestamp parse failed! " << "\n";
          return 0;
        };
        //for market dump
        std::string timestamp = "none";
        // get table/event type "
        unsigned char i = 0, pos = 0;
        unsigned char indexDoubleQuote[] = {0, 0, 0, 0}; // index of 1st quote, 2nd quote and so on;
        while (pos < 4 && i < len)
        {
          if (buffer[i] == '"')
            indexDoubleQuote[pos++] = i;
          ++i;
        }
        std::string response_type(buffer + indexDoubleQuote[2] + 1, indexDoubleQuote[3] - indexDoubleQuote[2] - 1);

        switch (*(buffer + 2))
        {
        case 't': // table
        {
          switch (Utils::hash_32_fnv1a(response_type.c_str())) // run time
          {
          case Utils::hash_32_fnv1a("spot/ticker"): // compile time
          {
            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            using normalized_msg_type = Message::Ticker;
            normalized_msg_type normalized_ticker(Utils::convertToPrice(data[0]["last"].GetString()),
                                                  Utils::convertToPrice(data[0]["best_bid"].GetString()),
                                                  Utils::convertToPrice(data[0]["best_ask"].GetString()),
                                                  Utils::convertToPrice(data[0]["open_24h"].GetString()),
                                                  Utils::convertToPrice(data[0]["high_24h"].GetString()),
                                                  Utils::convertToPrice(data[0]["low_24h"].GetString()),
                                                  std::stod(data[0]["best_bid_size"].GetString()),
                                                  std::stod(data[0]["best_ask_size"].GetString()),
                                                  std::stod(data[0]["last_qty"].GetString()));
            normalized_ticker.apiIncomingTime = packet_hit_time;
            //normalized_ticker.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            normalized_ticker.venue = MarketUtils::MarketType::OKEX_SPOT;
            //normalized_ticker.symbol = data[0]["instrument_id"].GetString();
            //normalized_ticker.incomingSequence = seq++;
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_ticker));
            break;
          }

          case Utils::hash_32_fnv1a("spot/order"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("spot/order_algo"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("spot/trade"): // compile time
          {

            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            std::unordered_set<std::string> symbols;

            using normalized_msg_type = Message::Trade;
            for (SizeType i = 0; i < data.Size(); i++)
            {
              auto side = data[i]["side"].GetString()[0];
              normalized_msg_type normalized_trade(Utils::convertToPrice(data[i]["price"].GetString()),
                                                   std::stod(data[i]["size"].GetString()),
                                                   side);

              //normalized_trade.symbol = data[i]["instrument_id"].GetString();
              normalized_trade.venue = MarketUtils::MarketType::OKEX_SPOT;
              normalized_trade.apiIncomingTime = packet_hit_time;
              //normalized_trade.incomingSequence = seq++;
              //normalized_trade.exchangeOutgoingTime = timestampToEpoch(data[i]["timestamp"].GetString());
              
              // symbols.insert(normalized_trade.symbol);
              // side == 'b' ? updateLatestBidTrade(normalized_trade) : updateLatestAskTrade(normalized_trade);
              MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_trade));
            }

            // for (const auto& symbol : symbols)
            // {
            //   sendTradeUpdate(MarketUtils::MarketType::OKEX_SPOT, symbol);
            // }
            break;
          }

          case Utils::hash_32_fnv1a("spot/depth5"): // compile time
          {
            //snapshot --> clear book first
            using normalized_msg_type = Message::DepthBook;
            const Value &data = decoder["data"];
            std::string symb = data[0]["instrument_id"].GetString();
            TimeType timestamp = timestampToEpoch(data[0]["timestamp"].GetString());
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();

            //static uint64_t seq = 1;
            normalized_msg_type normalized_depth;
            using order_side = EFG::MarketData::order_book_side;
            const Value &bids_data = data[0]["bids"];
            const Value &asks_data = data[0]["asks"];
            for (SizeType i = 0; i < bids_data.Size(); i++)
            {
              int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, std::stod(bids_data[i][1].GetString()), std::stoll(bids_data[i][2].GetString())));
            }

            for (SizeType i = 0; i < asks_data.Size(); i++)
            {
              int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, std::stod(asks_data[i][1].GetString()), std::stoll(asks_data[i][2].GetString())));
            }
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            normalized_depth.apiIncomingTime = packet_hit_time;
            //normalized_depth.symbol = data[0]["instrument_id"].GetString();
            normalized_depth.venue = MarketUtils::MarketType::OKEX_SPOT;
            //normalized_depth.incomingSequence = seq++;
            //normalized_depth.exchangeOutgoingTime = timestamp;
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
            break;
          }
          case Utils::hash_32_fnv1a("spot/depth"):
          {
            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            using normalized_msg_type = Message::DepthBook;
            using order_side = EFG::MarketData::order_book_side;
            normalized_msg_type normalized_depth;
            const Value &bids_data = data[0]["bids"];
            const Value &asks_data = data[0]["asks"];
            std::string symb = data[0]["instrument_id"].GetString();
            TimeType timestamp = timestampToEpoch(data[0]["timestamp"].GetString());
            std::string action_type = decoder["action"].GetString();
            switch (Utils::hash_32_fnv1a(action_type.c_str()))
            {
            case Utils::hash_32_fnv1a("partial"):
            {
              // Clear the book
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
              // Insert all
              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, std::stod(bids_data[i][1].GetString()), std::stoll(bids_data[i][2].GetString())));
              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, std::stod(asks_data[i][1].GetString()), std::stoll(asks_data[i][2].GetString())));
              }

              break;
            }
            case Utils::hash_32_fnv1a("update"):
            {

              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                double qty = std::stod(bids_data[i][1].GetString());
                long long orders_count = std::stoll(bids_data[i][2].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::bid, price);
                if (it->first == price)
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
                else // Price level doesn't exist, Insert
                {
                  if(qty!=0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, qty, orders_count));
                }
              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                double qty = std::stod(asks_data[i][1].GetString());
                long long orders_count = std::stoll(asks_data[i][2].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::ask, price);
                if (this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).exists(order_side::ask, price)) // price level exits
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
                else // Price level doesn't exist, Insert
                {
                  if(qty!=0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, qty, orders_count));
                }
              }
              break;
            }
            }

            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            normalized_depth.apiIncomingTime = packet_hit_time;
            //normalized_depth.symbol = data[0]["instrument_id"].GetString();
            normalized_depth.venue = MarketUtils::MarketType::OKEX_SPOT;
            //normalized_depth.incomingSequence = seq++;
            //normalized_depth.exchangeOutgoingTime = timestamp;
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
            break;
          }
          case Utils::hash_32_fnv1a("spot/depth_l2_tbt"): // tick by tick
          {
            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            using normalized_msg_type = Message::DepthBook;            
            using order_side = EFG::MarketData::order_book_side;
            
            normalized_msg_type normalized_depth; // this messge needs to be from memory pool

            const Value &bids_data = data[0]["bids"];
            const Value &asks_data = data[0]["asks"];
            std::string symb = data[0]["instrument_id"].GetString();
            TimeType timestamp = timestampToEpoch(data[0]["timestamp"].GetString());
            std::string action_type = decoder["action"].GetString();

            switch (Utils::hash_32_fnv1a(action_type.c_str()))
            {
            case Utils::hash_32_fnv1a("partial"):
            {
              // Clear the book
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
              // Insert all
              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(
                  price, std::stod(bids_data[i][1].GetString()), std::stoll(bids_data[i][3].GetString())));
              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(
                  price, std::stod(asks_data[i][1].GetString()), std::stoll(asks_data[i][3].GetString())));
              }

              break;
            }
            case Utils::hash_32_fnv1a("update"):
            {


              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                double qty = std::stod(bids_data[i][1].GetString());
                uint64_t orders_count = std::stoll(bids_data[i][3].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::bid, price);

                if (it->first == price) { // Price level exists
                  if (0 == qty) { // Delete
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).remove(order_side::bid, it);
                  }
                  else // Modify
                  {
                    it->second = normalized_msg_type::price_level(price, qty, orders_count);
                  }
                }
                else // Price level doesn't exist, Insert
                {
                  if (qty != 0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, 
                       normalized_msg_type::price_level(price, qty, orders_count));
                }

              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                double qty = std::stod(asks_data[i][1].GetString());
                uint64_t orders_count = std::stoll(asks_data[i][3].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::ask, price);
                if (it->first == price) // price level exits
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
                else // Price level doesn't exist, Insert
                {
                  if (qty != 0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, 
                        normalized_msg_type::price_level(price, qty, orders_count));
                }
              }
              break;
            }
            }
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
            normalized_depth.apiIncomingTime = packet_hit_time;
            //normalized_depth.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            //normalized_depth.symbol = data[0]["instrument_id"].GetString();
            normalized_depth.venue = MarketUtils::MarketType::OKEX_SPOT;
            //normalized_depth.incomingSequence = seq++;
            //normalized_depth.exchangeOutgoingTime = timestamp;
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
            break;
          }
          case Utils::hash_32_fnv1a("futures/order"): // compile time
          {

            break;
          }
          case Utils::hash_32_fnv1a("spot/account"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("spot/margin_account"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("futures/order_algo"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("futures/trade"): // compile time
          {
            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            std::unordered_set<std::string> symbols;

            using normalized_msg_type = Message::Trade;
            for (SizeType i = 0; i < data.Size(); i++)
            {
              auto side = data[i]["side"].GetString()[0];
              normalized_msg_type normalized_trade(Utils::convertToPrice(data[i]["price"].GetString()),
                                                   std::stod(data[i]["qty"].GetString()),
                                                   side);

              //normalized_trade.symbol = data[i]["instrument_id"].GetString();
              normalized_trade.venue = MarketUtils::MarketType::OKEX_FUTURES;
              normalized_trade.apiIncomingTime = packet_hit_time;
              //normalized_trade.incomingSequence = seq++;
              //normalized_trade.exchangeOutgoingTime = timestampToEpoch(data[i]["timestamp"].GetString());
              
            
              MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_trade));
            }
            break;
          }

          case Utils::hash_32_fnv1a("futures/ticker"): // compile time
          {

            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            using normalized_msg_type = Message::Ticker;
            normalized_msg_type normalized_ticker(Utils::convertToPrice(data[0]["last"].GetString()),
                                                  Utils::convertToPrice(data[0]["best_bid"].GetString()),
                                                  Utils::convertToPrice(data[0]["best_ask"].GetString()),
                                                  Utils::convertToPrice(data[0]["open_24h"].GetString()),
                                                  Utils::convertToPrice(data[0]["high_24h"].GetString()),
                                                  Utils::convertToPrice(data[0]["low_24h"].GetString()),
                                                  std::stod(data[0]["best_bid_size"].GetString()),
                                                  std::stod(data[0]["best_ask_size"].GetString()),
                                                  std::stod(data[0]["last_qty"].GetString()));
            normalized_ticker.apiIncomingTime = packet_hit_time;
            //normalized_ticker.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            normalized_ticker.venue = MarketUtils::MarketType::OKEX_FUTURES;
            //normalized_ticker.symbol = data[0]["instrument_id"].GetString();
            //normalized_ticker.incomingSequence = seq++;
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_ticker));
            break;
          }
          case Utils::hash_32_fnv1a("futures/price_range"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("futures/estimated_price"): // compile time
          {
            break;
          }
          case Utils::hash_32_fnv1a("futures/depth5"): // compile time
          {
            //snapshot --> clear book first
            using normalized_msg_type = Message::DepthBook;
            const Value &data = decoder["data"];
            std::string symb = data[0]["instrument_id"].GetString();
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
            //static uint64_t seq = 1;
            
            using order_side = EFG::MarketData::order_book_side;
            normalized_msg_type normalized_depth; // this messge needs to be from memory pool
            const Value &bids_data = data[0]["bids"];
            const Value &asks_data = data[0]["asks"];
            for (SizeType i = 0; i < bids_data.Size(); i++)
            {
              int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, std::stod(bids_data[i][1].GetString()), std::stoll(bids_data[i][3].GetString())));
            }

            for (SizeType i = 0; i < asks_data.Size(); i++)
            {
              int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, std::stod(asks_data[i][1].GetString()), std::stoll(asks_data[i][3].GetString())));
            }
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            normalized_depth.apiIncomingTime = packet_hit_time;
            //normalized_depth.symbol = data[0]["instrument_id"].GetString();
            normalized_depth.venue = MarketUtils::MarketType::OKEX_FUTURES;
            //normalized_depth.incomingSequence = seq++;
            //normalized_depth.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
            break;
          }
          case Utils::hash_32_fnv1a("futures/depth"):
          {

            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            using normalized_msg_type = Message::DepthBook;
            using order_side = EFG::MarketData::order_book_side;
            normalized_msg_type normalized_depth; // this messge needs to be from memory pool
            const Value &bids_data = data[0]["bids"];
            const Value &asks_data = data[0]["asks"];
            std::string symb = data[0]["instrument_id"].GetString();

            std::string action_type = decoder["action"].GetString();
            switch (Utils::hash_32_fnv1a(action_type.c_str()))
            {
            case Utils::hash_32_fnv1a("partial"):
            {
              // Clear the book
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
              // Insert all
              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, std::stod(bids_data[i][1].GetString()), std::stoll(bids_data[i][3].GetString())));
              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, std::stod(asks_data[i][1].GetString()), std::stoll(asks_data[i][3].GetString())));
              }

              break;
            }
            case Utils::hash_32_fnv1a("update"):
            {

              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                double qty = std::stod(bids_data[i][1].GetString());
                long long orders_count = std::stoll(bids_data[i][3].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::bid, price);
                if (it->first == price)
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
                else // Price level doesn't exist, Insert
                {
                  if(qty!=0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, qty, orders_count));
                }
              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                double qty = std::stod(asks_data[i][1].GetString());
                long long orders_count = std::stoll(asks_data[i][3].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::ask, price);
                if (this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).exists(order_side::ask, price)) // price level exits
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
                else // Price level doesn't exist, Insert
                {
                  if(qty!=0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, qty, orders_count));
                }
              }
              break;
            }
            }

            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            normalized_depth.apiIncomingTime = packet_hit_time;
            //normalized_depth.symbol = data[0]["instrument_id"].GetString();
            normalized_depth.venue = MarketUtils::MarketType::OKEX_FUTURES;
            //normalized_depth.incomingSequence = seq++;
            //normalized_depth.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
            break;
          }
          case Utils::hash_32_fnv1a("futures/depth_l2_tbt"): // tick by tick
          {
            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            using normalized_msg_type = Message::DepthBook;
            using order_side = EFG::MarketData::order_book_side;
            
            normalized_msg_type normalized_depth; // this messge needs to be from memory pool

            const Value &bids_data = data[0]["bids"];
            const Value &asks_data = data[0]["asks"];
            std::string symb = data[0]["instrument_id"].GetString();

            std::string action_type = decoder["action"].GetString();

            switch (Utils::hash_32_fnv1a(action_type.c_str()))
            {
            case Utils::hash_32_fnv1a("partial"):
            {
              // Clear the book
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
              // Insert all
              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(
                  price, std::stod(bids_data[i][1].GetString()), std::stoll(bids_data[i][3].GetString())));
              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(
                  price, std::stod(asks_data[i][1].GetString()), std::stoll(asks_data[i][3].GetString())));
              }

              break;
            }
            case Utils::hash_32_fnv1a("update"):
            {

              // A single update received from OKEX is an aggregation of multiple updates
              // Need to disintegrate it into multiple updates each with a single action
              // which can be cancel, trade, modify, insert, or none (when we don't want to apply trades)

              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                double qty = std::stod(bids_data[i][1].GetString());
                uint64_t orders_count = std::stoll(bids_data[i][3].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::bid, price);

                if (it->first == price) { // Price level exists
                  if (0 == qty) { // Delete
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).remove(order_side::bid, it);
                  }
                  else // Modify
                  {
                    it->second = normalized_msg_type::price_level(price, qty, orders_count);
                  }
                }
                else // Price level doesn't exist, Insert
                {
                  if (qty != 0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, 
                      normalized_msg_type::price_level(price, qty, orders_count));
                }

              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                double qty = std::stod(asks_data[i][1].GetString());
                uint64_t orders_count = std::stoll(asks_data[i][3].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::ask, price);
                if (it->first == price) // price level exits
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
                else // Price level doesn't exist, Insert
                {
                  if (qty != 0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, 
                      normalized_msg_type::price_level(price, qty, orders_count));
                }
              }
              break;
            }
            }
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
            normalized_depth.apiIncomingTime = packet_hit_time;
            //normalized_depth.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            //normalized_depth.symbol = data[0]["instrument_id"].GetString();
            normalized_depth.venue = MarketUtils::MarketType::OKEX_FUTURES;
            //normalized_depth.incomingSequence = seq++;
            //normalized_depth.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
            break;
          }

          case Utils::hash_32_fnv1a("futures/mark_price"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("futures/position"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("futures/instruments"): // compile time
          {
            break;
          }
          case Utils::hash_32_fnv1a("swap/position"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("swap/order"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("swap/order_algo"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("swap/ticker"): // compile time
          {

            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            using normalized_msg_type = Message::Ticker;
            normalized_msg_type normalized_ticker(Utils::convertToPrice(data[0]["last"].GetString()),
                                                  Utils::convertToPrice(data[0]["best_bid"].GetString()),
                                                  Utils::convertToPrice(data[0]["best_ask"].GetString()),
                                                  Utils::convertToPrice(data[0]["open_24h"].GetString()),
                                                  Utils::convertToPrice(data[0]["high_24h"].GetString()),
                                                  Utils::convertToPrice(data[0]["low_24h"].GetString()),
                                                  std::stod(data[0]["best_bid_size"].GetString()),
                                                  std::stod(data[0]["best_ask_size"].GetString()),
                                                  std::stod(data[0]["last_qty"].GetString()));
            normalized_ticker.apiIncomingTime = packet_hit_time;
            normalized_ticker.venue = MarketUtils::MarketType::OKEX_SWAP;
            //normalized_ticker.symbol = data[0]["instrument_id"].GetString();
            //normalized_ticker.incomingSequence = seq++;
            //normalized_ticker.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_ticker));
            break;
          }

          case Utils::hash_32_fnv1a("swap/trade"): // compile time
          {
            const Value &data = decoder["data"];
            //static uint64_t seq = 1;


            using normalized_msg_type = Message::Trade;

            for (SizeType i = 0; i < data.Size(); i++)
            {
              auto side = data[i]["side"].GetString()[0];
              normalized_msg_type normalized_trade(Utils::convertToPrice(data[i]["price"].GetString()),
                                                   std::stod(data[i]["size"].GetString()),
                                                   side);

              //normalized_trade.symbol = data[i]["instrument_id"].GetString();
              normalized_trade.venue = MarketUtils::MarketType::OKEX_SWAP;
              normalized_trade.apiIncomingTime = packet_hit_time;
              //normalized_trade.incomingSequence = seq++;
              //normalized_trade.exchangeOutgoingTime = timestampToEpoch(data[i]["timestamp"].GetString());
              
            
              MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_trade));
            }

            break;
          }

          case Utils::hash_32_fnv1a("swap/funding_rate"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("swap/depth5"): // compile time
          {
            //snapshot --> clear book first
            using normalized_msg_type = Message::DepthBook;
            
            const Value &data = decoder["data"];
            std::string symb = data[0]["instrument_id"].GetString();
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
            //static uint64_t seq = 1;
            
            using order_side = EFG::MarketData::order_book_side;
            normalized_msg_type normalized_depth; // this messge needs to be from memory pool
            const Value &bids_data = data[0]["bids"];
            const Value &asks_data = data[0]["asks"];

            for (SizeType i = 0; i < bids_data.Size(); i++)
            {
              int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, std::stod(bids_data[i][1].GetString()), std::stoll(bids_data[i][3].GetString())));
            }

            for (SizeType i = 0; i < asks_data.Size(); i++)
            {
              int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, std::stod(asks_data[i][1].GetString()), std::stoll(asks_data[i][3].GetString())));
            }
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            normalized_depth.apiIncomingTime = packet_hit_time;
            //normalized_depth.symbol = data[0]["instrument_id"].GetString();
            normalized_depth.venue = MarketUtils::MarketType::OKEX_SWAP;
            //normalized_depth.incomingSequence = seq++;
            //normalized_depth.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
            break;
          }
          case Utils::hash_32_fnv1a("swap/depth"):
          {
            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            using normalized_msg_type = Message::DepthBook;
            using order_side = EFG::MarketData::order_book_side;
            normalized_msg_type normalized_depth; // this messge needs to be from memory pool
            const Value &bids_data = data[0]["bids"];
            const Value &asks_data = data[0]["asks"];
            std::string symb = data[0]["instrument_id"].GetString();

            
            std::string action_type = decoder["action"].GetString();
            switch (Utils::hash_32_fnv1a(action_type.c_str()))
            {
            case Utils::hash_32_fnv1a("partial"):
            {
              // Clear the book
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
              // Insert all
              //MD_LOG_INFO << "pushing snapshot " << '\n';
              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, std::stod(bids_data[i][1].GetString()), std::stoll(bids_data[i][3].GetString())));
              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, std::stod(asks_data[i][1].GetString()), std::stoll(asks_data[i][3].GetString())));
              }

              break;
            }
            case Utils::hash_32_fnv1a("update"):
            {
              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                double qty = std::stod(bids_data[i][1].GetString());
                long long orders_count = std::stoll(bids_data[i][3].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::bid, price);
                if (it->first == price)
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
                else // Price level doesn't exist, Insert
                {
                  if(qty!=0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, qty, orders_count));
                }
              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                double qty = std::stod(asks_data[i][1].GetString());
                long long orders_count = std::stoll(asks_data[i][3].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::ask, price);
                if (this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).exists(order_side::ask, price)) // price level exits
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
                else // Price level doesn't exist, Insert
                {
                  if(qty!=0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, qty, orders_count));
                }
              }
              break;
            }
            }

            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            normalized_depth.apiIncomingTime = packet_hit_time;
            //normalized_depth.symbol = data[0]["instrument_id"].GetString();
            normalized_depth.venue = MarketUtils::MarketType::OKEX_SWAP;
            //normalized_depth.incomingSequence = seq++;
            //normalized_depth.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
            break;
          }
          case Utils::hash_32_fnv1a("swap/depth_l2_tbt"): // tick by tick
          {

            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            using normalized_msg_type = Message::DepthBook;
            using order_side = EFG::MarketData::order_book_side;
            normalized_msg_type normalized_depth; // this messge needs to be from memory pool
            const Value &bids_data = data[0]["bids"];
            const Value &asks_data = data[0]["asks"];
            std::string symb = data[0]["instrument_id"].GetString();


            std::string action_type = decoder["action"].GetString();
            switch (Utils::hash_32_fnv1a(action_type.c_str()))
            {
            case Utils::hash_32_fnv1a("partial"):
            {
              // Clear the book
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
              // Insert all
              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, std::stod(bids_data[i][1].GetString()), std::stoll(bids_data[i][3].GetString())));
              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, std::stod(asks_data[i][1].GetString()), std::stoll(asks_data[i][3].GetString())));
              }

              break;
            }
            case Utils::hash_32_fnv1a("update"):
            {

              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                double qty = std::stod(bids_data[i][1].GetString());
                long long orders_count = std::stoll(bids_data[i][3].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::bid, price);
                if (it->first == price)
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
                else // Price level doesn't exist, Insert
                {
                  if(qty!=0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, qty, orders_count));
                }
              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                double qty = std::stod(asks_data[i][1].GetString());
                long long orders_count = std::stoll(asks_data[i][3].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::ask, price);
                if (it->first == price) // price level exits
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
                else // Price level doesn't exist, Insert
                {
                  if(qty!=0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, qty, orders_count));
                }
              }
              break;
            }
            }

            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            normalized_depth.apiIncomingTime = packet_hit_time;
            //normalized_depth.symbol = data[0]["instrument_id"].GetString();
            normalized_depth.venue = MarketUtils::MarketType::OKEX_SWAP;
            //normalized_depth.incomingSequence = seq++;
            //normalized_depth.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
            break;
          }

          case Utils::hash_32_fnv1a("swap/mark_price"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("option/position"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("option/order"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("option/summary"): // compile time
          {
            break;
          }

          case Utils::hash_32_fnv1a("option/trade"): // compile time
          {
            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            using normalized_msg_type = Message::Trade;
            for (SizeType i = 0; i < data.Size(); i++)
            {
              normalized_msg_type normalized_trade(Utils::convertToPrice(data[i]["price"].GetString()),
                                                   std::stod(data[i]["qty"].GetString()),
                                                   data[i]["side"].GetString()[0]);

              //normalized_trade.symbol = data[i]["instrument_id"].GetString();
              normalized_trade.venue = MarketUtils::MarketType::OKEX_OPTIONS;
              normalized_trade.apiIncomingTime = packet_hit_time;
              //normalized_trade.incomingSequence = seq++;
              //normalized_trade.exchangeOutgoingTime = timestampToEpoch(data[i]["timestamp"].GetString());
              MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_trade));
            }
            break;
          }

          case Utils::hash_32_fnv1a("option/ticker"): // compile time
          {

            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            using normalized_msg_type = Message::Ticker;
            normalized_msg_type normalized_ticker(Utils::convertToPrice(data[0]["last"].GetString()),
                                                  Utils::convertToPrice(data[0]["best_bid"].GetString()),
                                                  Utils::convertToPrice(data[0]["best_ask"].GetString()),
                                                  Utils::convertToPrice(data[0]["open_24h"].GetString()),
                                                  Utils::convertToPrice(data[0]["high_24h"].GetString()),
                                                  Utils::convertToPrice(data[0]["low_24h"].GetString()),
                                                  std::stod(data[0]["best_bid_size"].GetString()),
                                                  std::stod(data[0]["best_ask_size"].GetString()),
                                                  std::stod(data[0]["last_qty"].GetString()));
            normalized_ticker.apiIncomingTime = packet_hit_time;
            normalized_ticker.venue = MarketUtils::MarketType::OKEX_OPTIONS;
            //normalized_ticker.symbol = data[0]["instrument_id"].GetString();
            //normalized_ticker.incomingSequence = seq++;
            //normalized_ticker.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_ticker));
            break;
          }

          case Utils::hash_32_fnv1a("option/depth5"): // compile time
          {
            //snapshot --> clear book first
            using normalized_msg_type = Message::DepthBook;
            
            const Value &data = decoder["data"];
            std::string symb = data[0]["instrument_id"].GetString();
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();

            //static uint64_t seq = 1;
            
            using order_side = EFG::MarketData::order_book_side;
            normalized_msg_type normalized_depth; // this messge needs to be from memory pool
            const Value &bids_data = data[0]["bids"];
            const Value &asks_data = data[0]["asks"];
            for (SizeType i = 0; i < bids_data.Size(); i++)
            {
              int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, std::stod(bids_data[i][1].GetString()), std::stoll(bids_data[i][3].GetString())));
            }

            for (SizeType i = 0; i < asks_data.Size(); i++)
            {
              int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, std::stod(asks_data[i][1].GetString()), std::stoll(asks_data[i][3].GetString())));
            }
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            normalized_depth.apiIncomingTime = packet_hit_time;
            //normalized_depth.symbol = data[0]["instrument_id"].GetString();
            normalized_depth.venue = MarketUtils::MarketType::OKEX_OPTIONS;
            //normalized_depth.incomingSequence = seq++;
            //normalized_depth.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
            break;
          }
          case Utils::hash_32_fnv1a("option/depth"):
          {

            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            using normalized_msg_type = Message::DepthBook;
            using order_side = EFG::MarketData::order_book_side;
            normalized_msg_type normalized_depth; // this messge needs to be from memory pool
            const Value &bids_data = data[0]["bids"];
            const Value &asks_data = data[0]["asks"];
            std::string symb = data[0]["instrument_id"].GetString();

            std::string action_type = decoder["action"].GetString();
            switch (Utils::hash_32_fnv1a(action_type.c_str()))
            {
            case Utils::hash_32_fnv1a("partial"):
            {
              // Clear the book
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
              // Insert all
              //MD_LOG_INFO << "pushing snapshot " << '\n';
              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, std::stod(bids_data[i][1].GetString()), std::stoll(bids_data[i][3].GetString())));
              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, std::stod(asks_data[i][1].GetString()), std::stoll(asks_data[i][3].GetString())));
              }

              break;
            }
            case Utils::hash_32_fnv1a("update"):
            {

              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                double qty = std::stod(bids_data[i][1].GetString());
                long long orders_count = std::stoll(bids_data[i][2].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::bid, price);
                if (it->first == price)
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
                else // Price level doesn't exist, Insert
                {
                  if(qty!=0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, qty, orders_count));
                }
              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                double qty = std::stod(asks_data[i][1].GetString());
                long long orders_count = std::stoll(asks_data[i][3].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::ask, price);
                if (this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).exists(order_side::ask, price)) // price level exits
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
                else // Price level doesn't exist, Insert
                {
                  if(qty!=0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, qty, orders_count));
                }
              }
              break;
            }
            }

            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            normalized_depth.apiIncomingTime = packet_hit_time;
            //normalized_depth.symbol = data[0]["instrument_id"].GetString();
            normalized_depth.venue = MarketUtils::MarketType::OKEX_OPTIONS;
            //normalized_depth.incomingSequence = seq++;
            //normalized_depth.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
            break;
          }
          case Utils::hash_32_fnv1a("option/depth_l2_tbt"): // tick by tick
          {

            const Value &data = decoder["data"];
            //static uint64_t seq = 1;

            using normalized_msg_type = Message::DepthBook;
            using order_side = EFG::MarketData::order_book_side;
            normalized_msg_type normalized_depth;
            const Value &bids_data = data[0]["bids"];
            const Value &asks_data = data[0]["asks"];
            std::string symb = data[0]["instrument_id"].GetString();


            std::string action_type = decoder["action"].GetString();
            switch (Utils::hash_32_fnv1a(action_type.c_str()))
            {
            case Utils::hash_32_fnv1a("partial"):
            {
              // Clear the book
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
              // Insert all
              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, std::stod(bids_data[i][1].GetString()), std::stoll(bids_data[i][3].GetString())));
              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, std::stod(asks_data[i][1].GetString()), std::stoll(asks_data[i][3].GetString())));
              }

              break;
            }
            case Utils::hash_32_fnv1a("update"):
            {

              for (SizeType i = 0; i < bids_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(bids_data[i][0].GetString());
                double qty = std::stod(bids_data[i][1].GetString());
                long long orders_count = std::stoll(bids_data[i][3].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::bid, price);
                if (it->first == price)
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
                else // Price level doesn't exist, Insert
                {
                  if(qty!=0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, qty, orders_count));
                }
              }

              for (SizeType i = 0; i < asks_data.Size(); i++)
              {
                int64_t price = Utils::convertToPrice(asks_data[i][0].GetString());
                double qty = std::stod(asks_data[i][1].GetString());
                long long orders_count = std::stoll(asks_data[i][3].GetString());
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::ask, price);
                if (it->first == price) // price level exits
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
                else // Price level doesn't exist, Insert
                {
                  if(qty!=0)
                    this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, qty, orders_count));
                }
              }
              break;
            }
            }

            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            normalized_depth.apiIncomingTime = packet_hit_time;
            //normalized_depth.symbol = data[0]["instrument_id"].GetString();
            normalized_depth.venue = MarketUtils::MarketType::OKEX_OPTIONS;
            //normalized_depth.incomingSequence = seq++;
            //normalized_depth.exchangeOutgoingTime = timestampToEpoch(data[0]["timestamp"].GetString());
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
            break;
          }

          case Utils::hash_32_fnv1a("option/instruments"): // compile time
          {
            break;
          }
          case Utils::hash_32_fnv1a("index/ticker"): // compile time
          {
            break;
          }
          default:
          {
            MD_LOG_WARN << "Unknown Data Table OKEX " << std::string(buffer, len) << '\n';
          }
          }

          break;
        }
        case 'e': // event
        {
          // handle error/success notification
          switch (Utils::hash_32_fnv1a(response_type.c_str()))
          {

          case Utils::hash_32_fnv1a("error"): // compile time
          {
            //MD_LOG_INFO << "Error:: " << decoder["message"].GetString() << ", " << decoder["errorCode"].GetInt() << '\n';
            break;
          }

          case Utils::hash_32_fnv1a("login"): // compile time
          {
            MD_LOG_INFO << "Login message - should never reach here " << '\n';
            break;
          }
          case Utils::hash_32_fnv1a("subscribe"): // compile time
          {
            //MD_LOG_INFO << "Sucessful subscription to: " << decoder["channel"].GetString() << '\n';
            break;
          }

          case Utils::hash_32_fnv1a("unsubscribe"): // compile time
          {
            //MD_LOG_INFO << "Sucessful unsubscription to: " << decoder["channel"].GetString() << '\n';
            break;
          }
          default:
          {
            MD_LOG_WARN << "Unkown Event OKEX " << std::string(buffer, len) << '\n';
          }
          }
          break;
        }
        default:
        {
          MD_LOG_WARN << "Unknown buffer OKEX " << std::string(buffer, len) << '\n';
        }
        }
      }
      void OKEX::decodeBinary(char *buffer, size_t len)
      {
      }

    } // namespace VenueSession
  }   // namespace MarketData

} // namespace EFG
