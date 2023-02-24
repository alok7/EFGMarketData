#include <MarketData/BITFINEX/BITFINEX.h>

#define BUFLEN 65536

namespace EFG
{
  namespace MarketData
  {
    namespace VenueSession
    {
      BITFINEX::BITFINEX(const ConfigManager::VenueSessionInfo &vsinfo) : AbstractVenue(vsinfo)
      {
	transport = std::make_unique<Transport::WssTransport>(&md_hub);
        registerCallBacks();
        MD_LOG_INFO << " Created BITFINEX Session " << '\n'; // logger - later
      }
      void BITFINEX::onConnection(uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req)
      {
        MD_LOG_INFO << " BITFINEX Connected " << '\n';
        subscriptionHandler->init(ws);
        subscriptionHandler->run();
      }
      void BITFINEX::onUpdate(uWS::WebSocket<uWS::CLIENT> *ws, char *message, std::size_t len_, uWS::OpCode code)
      {
        try
        {
          //MD_LOG_INFO << " Decoding incoming data " << std::string(message, len_) << '\n';
          TimeType packet_hit_time = Utils::getHRTime();
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
      void BITFINEX::onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, std::size_t len_)
      {

        try
        {
          MD_LOG_INFO << "BITFINEX Disconnected " << std::to_string(Utils::getHRTime()) << '\n';
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

      void BITFINEX::registerCallBacks()
      {
        using namespace std::placeholders;
        transport->registerOnConnectCallBack(std::bind(&BITFINEX::onConnection, this, _1, _2));
        transport->registerOnUpdateCallBack(std::bind(&BITFINEX::onUpdate, this, _1, _2, _3, _4));
        transport->registerOnDisconnectCallback(std::bind(&BITFINEX::onDisconnection, this, _1, _2, _3, _4));
      }
      void BITFINEX::decode(char *buffer, size_t len, const TimeType &packet_hit_time)
        {

        
        Document decoder;
        if (decoder.Parse(buffer, len).HasParseError())
        {          
          MD_LOG_INFO << "Not able to parse resonse " << '\n'; // logger
          return;
        }
        switch (buffer[0])
        {
        case '[': // snapshot & update
        {
          if (5 == decoder[1].GetType()) // string
          {
            std::string hb_string = decoder[1].GetString();
            if (trim(hb_string) == "hb")
            {
              return; // heartbeat
            }
          }
          switch (Utils::hash_32_fnv1a(channelNameMap[decoder[0].GetInt()].c_str()))
          {
          case Utils::hash_32_fnv1a("ticker"):
          {
            using normalized_msg_type = Message::Ticker;
            normalized_msg_type normalized_ticker;
            normalized_ticker.last_price = decoder[1][6].GetDouble() * 1000000000,
            normalized_ticker.bid_price = decoder[1][0].GetDouble() * 1000000000,
            normalized_ticker.ask_price = decoder[1][2].GetDouble() * 1000000000,
            normalized_ticker.open_price = -1,
            normalized_ticker.high_price = decoder[1][8].GetDouble() * 1000000000,
            normalized_ticker.low_price = decoder[1][9].GetDouble() * 1000000000;
            normalized_ticker.bid_size = decoder[1][1].GetDouble();
            normalized_ticker.ask_size = decoder[1][3].GetDouble();
            normalized_ticker.last_size = -1;
            normalized_ticker.apiIncomingTime = packet_hit_time;
            normalized_ticker.venue = MarketUtils::MarketType::BITFINEX;
	    std::string instrument_id = channelSymbolMap[decoder[0].GetInt()];
            //strcpy(normalized_ticker.symbol, instrument_id.c_str());
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_ticker));
            break;
          }
          case Utils::hash_32_fnv1a("trades"):
          {
            using normalized_msg_type = Message::Trade;
            if (decoder.Size() == 2)
            {
              
              const Value &data = decoder[1];
              for (SizeType i = 0; i < data.Size(); i++)
              {
                uint64_t price = data[i][3].GetDouble() * 1000000000;
                double qty = data[i][2].GetDouble();
                char side = qty < 0 ? 's' : 'b';
                uint64_t trade_id = data[i][0].GetInt64();
                // uint64_t trade_id = 0;
                normalized_msg_type normalized_trade(price, fabs(qty), side);
	        std::string instrument_id = channelSymbolMap[decoder[0].GetInt()];
                //strcpy(normalized_trade.symbol, instrument_id.c_str());
                normalized_trade.venue = MarketUtils::MarketType::BITFINEX;
                normalized_trade.apiIncomingTime = packet_hit_time;
                MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_trade));
              }
            }
            else
            {
              const Value &data = decoder[2];
              uint64_t price = data[3].GetDouble() * 1000000000;
              double qty = data[2].GetDouble();
              char side = qty < 0 ? 's' : 'b';
              uint64_t trade_id = data[0].GetInt64();
              normalized_msg_type normalized_trade(price, fabs(qty), side);
	      std::string instrument_id = channelSymbolMap[decoder[0].GetInt()];
              //strcpy(normalized_trade.symbol, instrument_id.c_str());
              normalized_trade.venue = MarketUtils::MarketType::BITFINEX;
              normalized_trade.apiIncomingTime = packet_hit_time;
              MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_trade));
            }
            break;
          }
          case Utils::hash_32_fnv1a("book"):
          {

            using normalized_msg_type = Message::DepthBook;
            using order_type = EFG::MarketData::order_book_side;
            normalized_msg_type normalized_depth;
            std::string symb = channelSymbolMap[decoder[0].GetInt()];
            if (decoder[1].IsArray() && decoder[1][0].IsArray())
            {
              // snapshot
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
              const Value &data = decoder[1];
              for (SizeType i = 0; i < data.Size(); i++)
              {
                uint64_t price = data[i][0].GetDouble()*1000000000;
                double qty = data[i][2].GetDouble();
                uint64_t count = data[i][1].GetInt64();
                if (count == 0)
                  continue;
                order_type type = qty > 0 ? order_type::bid : order_type::ask;
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(type, price, 
                  normalized_msg_type::price_level(price, fabs(qty), count));
              }
            }
            else
            {
              // update
              const Value &data = decoder[1];

              int64_t price = data[0].GetDouble()*1000000000;
              double qty = data[2].GetDouble();
              uint64_t count = data[1].GetInt64();
              order_type type = qty > 0 ? order_type::bid : order_type::ask;
              qty = fabs(qty);
              if (count == 0) // Remove the order
              {
                auto it_del = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(type, price);
                
                if (it_del->first == price)
                  this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).remove(type, it_del);
              }
              else // update the order
              {
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(type, price);
                if (it->first == price) { // modify
                  it->second = normalized_msg_type::price_level(price, fabs(qty), count);
                }
                else if (qty != 0) { // insert
                  this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(type, price, 
                    normalized_msg_type::price_level(price, fabs(qty), count));
                }
              }
            }
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
            normalized_depth.apiIncomingTime = packet_hit_time;
	    std::string instrument_id = channelSymbolMap[decoder[0].GetInt()];
            //strcpy(normalized_depth.symbol, instrument_id.c_str());
            normalized_depth.venue = MarketUtils::MarketType::BITFINEX;
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));

            break;
          }
          case Utils::hash_32_fnv1a("rawbook"): // more granular than book
          {


            using normalized_msg_type = Message::DepthBook;
            using order_type = EFG::MarketData::order_book_side;
            normalized_msg_type normalized_depth;
            std::string symb = channelSymbolMap[decoder[0].GetInt()];
            if (decoder[1].IsArray() && decoder[1][0].IsArray())
            {
              // snapshot
              this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
              const Value &data = decoder[1];
              for (SizeType i = 0; i < data.Size(); i++)
              {
                int64_t price = data[i][1].GetDouble()*1000000000;
                double qty = data[i][2].GetDouble();
                uint64_t id = data[i][0].GetInt64();
                int64_t orders_count = 1;
                order_type type = qty > 0 ? order_type::bid : order_type::ask;
                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(type, id, normalized_msg_type::price_level(price, fabs(qty), 1));
              }
            }
            else
            {
              // update
              const Value &data = decoder[1];
              int64_t price = data[1].GetDouble()*1000000000;
              double qty = data[2].GetDouble();
              uint64_t id = data[0].GetInt64();
              uint64_t orders_count = 1;
              order_type type = qty > 0 ? order_type::bid : order_type::ask;
              qty = fabs(qty);
              if (price == 0)
              {
                // remove the order, search for the order in both asks and bids using id
                auto it1 = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_type::bid, id);
                if (it1->first == id) {
                  this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).remove(order_type::bid, it1);
                }
                auto it2 = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_type::ask, id);
                if (it2->first == id) {
                  this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).remove(order_type::ask, it2);
                }
              }
              else
              {
                // update the order
                auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(type, id);
                if (it->first == id) { // modify
                  it->second = normalized_msg_type::price_level(price, fabs(qty), orders_count);
                }
                else if (qty != 0) {  // insert
                  this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(type, id, 
                    normalized_msg_type::price_level(price, fabs(qty), orders_count));
                }
              }
            }
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
            normalized_depth.apiIncomingTime = packet_hit_time;
	    std::string instrument_id = channelSymbolMap[decoder[0].GetInt()];
            //strcpy(normalized_depth.symbol, instrument_id.c_str());
            normalized_depth.venue = MarketUtils::MarketType::BITFINEX;
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));

            break;
          }

          case Utils::hash_32_fnv1a("fticker"):
          {
            //MD_LOG_INFO << "fticker " << '\n';
            break;
          }
          case Utils::hash_32_fnv1a("ftrades"):
          {
            break;
          }
          case Utils::hash_32_fnv1a("fbook"):
          {
            break;
          }
          case Utils::hash_32_fnv1a("frawbook"):
          {
            break;
          }
          case Utils::hash_32_fnv1a("liquidationfeed"):
          {
            // https://docs.bitfinex.com/reference#ws-public-status
            using normalized_msg_type = Message::Liquidation;
            const Value& data = decoder[1][0];
            uint64_t price = data[11].GetDouble()*1000000000;
            double qty = data[5].GetDouble();
            char side = 'N';
            std::string order_id = std::to_string(data[1].GetInt64());
            normalized_msg_type normalized_liquidation(price, fabs(qty), side, order_id);
	    std::string instrument_id = channelSymbolMap[decoder[0].GetInt()];
            //strcpy(normalized_liquidation.symbol, instrument_id.c_str());
            normalized_liquidation.venue = MarketUtils::MarketType::BITFINEX;
            normalized_liquidation.apiIncomingTime = packet_hit_time;
            break;
          }
        }

        break;
      }
        case '{': // subs response
        {

          //get the update type
          unsigned char i = 0, pos = 0;
          unsigned char indexDoubleQuote[] = {0, 0, 0, 0}; // index of 1st quote, 2nd quote and so on;
          while (pos < 4 && i < strlen(buffer))
          {
            if (buffer[i] == '"')
              indexDoubleQuote[pos++] = i;
            ++i;
          }
          // response type between third and fourth quote
          std::string event_type(buffer + indexDoubleQuote[2] + 1, indexDoubleQuote[3] - indexDoubleQuote[2] - 1);
          switch (Utils::hash_32_fnv1a(event_type.c_str()))
          {
          case Utils::hash_32_fnv1a("subscribed"): // sucessful subscription
          {
            std::string channelName = decoder["channel"].GetString();
                     std::string symbol("");
                     if(channelName!="status")  symbol = decoder["symbol"].GetString();

            if (channelName == "book")
            {
              std::string precision("");
              if (decoder.HasMember("prec"))
                precision = decoder["prec"].GetString();

              if (precision == "R0")
                channelName.insert(0, "raw");
            }
                     if(channelName=="status"){
                       if(decoder["key"].GetString()[0]=='l'){
                         channelName =  "liquidationfeed";
                       }
                     }
                     else if('f'==symbol[0] && 'f'!=channelName[0]) channelName.insert(0, 1, 'f');

            int channelId = decoder["chanId"].GetInt();
            channelNameMap.insert({channelId, channelName});
            channelSymbolMap.insert({channelId,symbol});
            break;
          }
          case Utils::hash_32_fnv1a("unsubscribed"): // sucessful unsubscription
          {
            break;
          }
          case Utils::hash_32_fnv1a("error"):
          {
            break;
          }

          case Utils::hash_32_fnv1a("info"):
          {
            if (decoder.HasMember("platform"))
            {
              if (1 == decoder["platform"]["status"].GetInt())
              {
                MD_LOG_INFO << "Platform is operative " << '\n';
              }
              else
              {
                MD_LOG_INFO << "Platform is in maintainence " << '\n';
              }
            }
            break;
          }
          }

          break;
        }
        }
      }      
      void BITFINEX::decodeBinary(char* buffer, size_t len)
      {

      }

    } // namespace VenueSession
  }   // namespace MarketData

} // namespace EFG
