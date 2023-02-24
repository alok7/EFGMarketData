#include <MarketData/HUOBI_SPOT/HUOBI_SPOT.h>

#define BUFLEN 65536

namespace EFG
{
namespace MarketData
{
  namespace VenueSession
  {
    HUOBI_SPOT::HUOBI_SPOT(const ConfigManager::VenueSessionInfo &vsinfo) : AbstractVenue(vsinfo)
    {
      transport = std::make_unique<Transport::WssTransport>(&md_hub);
      registerCallBacks();
      MD_LOG_INFO << "Created  HUOBI_SPOT Session " << '\n';
    }
    void HUOBI_SPOT::onConnection(uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req)
    {
      MD_LOG_INFO << " HUOBI_SPOT Connected " << '\n';
      subscriptionHandler->init(ws);
      subscriptionHandler->run();
    }
    void HUOBI_SPOT::onUpdate(uWS::WebSocket<uWS::CLIENT> *ws, char *message, std::size_t len_, uWS::OpCode code)
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
    void HUOBI_SPOT::onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, std::size_t length)
    {

      try
      {
        MD_LOG_INFO << " HUOBI_SPOT Disconnected " << std::to_string(Utils::getHRTime()) << '\n';
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
    void HUOBI_SPOT::registerCallBacks()
    {
      using namespace std::placeholders;
      transport->registerOnConnectCallBack(std::bind(&HUOBI_SPOT::onConnection, this, _1, _2));
      transport->registerOnUpdateCallBack(std::bind(&HUOBI_SPOT::onUpdate, this, _1, _2, _3, _4));
      transport->registerOnDisconnectCallback(std::bind(&HUOBI_SPOT::onDisconnection, this, _1, _2, _3, _4));
    }
    // get instrument from channel name - later
    void HUOBI_SPOT::decode(char *buffer, size_t len, uWS::WebSocket<uWS::CLIENT> *ws, const TimeType &packet_hit_time)
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
        /*
                static bool callOnce = [this](){
                  subscriptionHandler->run();
                  MD_LOG_INFO << " Starting Susbcription Handler for HUOBI_SPOT " << '\n';
                  return true;
                } ();
                */
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
        //std::string firstPart = sub_type.substr(sub_type.find(".")+1);
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
        {
          if (first_update_received)
            break;

          if (decoder.Parse(buffer, len).HasParseError())
          {
            MD_LOG_INFO << "Not able to parse depth " << '\n';
            return;
          }
          using normalized_msg_type = Message::DepthBook;
          std::string symb = instrument_id;
          const Value &data = decoder["tick"];

          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
          //static uint64_t seq = 1;

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

          snapshot_received = true;

          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
          normalized_depth.apiIncomingTime = packet_hit_time;
          //normalized_depth.symbol = instrument_id;
          normalized_depth.venue = MarketUtils::MarketType::HUOBI_SPOT;
          //normalized_depth.incomingSequence = seq++;
          //normalized_depth.exchangeOutgoingTime = data["ts"].GetInt64();
          MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
          break;
        }
        case Utils::hash_32_fnv1a("mbp.5"):
        case Utils::hash_32_fnv1a("mbp.20"):
        case Utils::hash_32_fnv1a("mbp.150"):
        {
          if (!snapshot_received)
            break;
          //always check the prevSeqNum of current msg to the seNum of prevoios msg - if doesn't match, request snapshot

          //static uint64_t seq = 1;
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

          first_update_received = true;

          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
          normalized_depth.apiIncomingTime = packet_hit_time;
          //normalized_depth.symbol = instrument_id;
          normalized_depth.venue = MarketUtils::MarketType::HUOBI_SPOT;
          //normalized_depth.incomingSequence = seq++;
          //normalized_depth.exchangeOutgoingTime = decoder["ts"].GetInt64();
          MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
          break;
        }
        case Utils::hash_32_fnv1a("mbp.refresh.5"):
        case Utils::hash_32_fnv1a("mbp.refresh.10"):
        case Utils::hash_32_fnv1a("mbp.refresh.20"):
        {

          if (decoder.Parse(buffer, len).HasParseError())
          {
            MD_LOG_INFO << "Not able to parse mbp " << '\n'; // logger
            return;
          }

          //snapshot --> clear book first
          using normalized_msg_type = Message::DepthBook;
          std::string symb = instrument_id;
          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
          //static uint64_t seq = 1;
          const Value &data = decoder["tick"];

          using order_side = EFG::MarketData::order_book_side;
          normalized_msg_type normalized_depth;
          const Value &bids_data = data["bids"];
          const Value &asks_data = data["asks"];

          for (SizeType i = 0; i < bids_data.Size(); i++)
          {
            int64_t price = bids_data[i][0].GetDouble() * 1000000000;
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, bids_data[i][1].GetDouble(), 1));
          }

          for (SizeType i = 0; i < asks_data.Size(); i++)
          {
            int64_t price = asks_data[i][0].GetDouble() * 1000000000;
            this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, asks_data[i][1].GetDouble(), 1));
          }

          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
          this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
          normalized_depth.apiIncomingTime = packet_hit_time;
          //normalized_depth.symbol = instrument_id;
          normalized_depth.venue = MarketUtils::MarketType::HUOBI_SPOT;
          //normalized_depth.incomingSequence = seq++;
          //normalized_depth.exchangeOutgoingTime = decoder["ts"].GetInt64();
          MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
          break;
        }
        case Utils::hash_32_fnv1a("bbo"):
        {

          //static uint64_t seq = 1;
          if (decoder.Parse(buffer, len).HasParseError())
          {
            MD_LOG_INFO << "Not able to parse bbo " << '\n'; // logger
            return;
          }
          const Value &data = decoder["tick"];
          using normalized_msg_type = Message::Ticker;
          normalized_msg_type normalized_ticker;
          normalized_ticker.bid_price = data["bid"].GetDouble() * 1000000000;
          normalized_ticker.ask_price = data["ask"].GetDouble() * 1000000000;
          normalized_ticker.bid_size = data["bidSize"].GetDouble();
          normalized_ticker.ask_size = data["askSize"].GetDouble();

          normalized_ticker.apiIncomingTime = packet_hit_time;
          //normalized_ticker.symbol = instrument_id;
          normalized_ticker.venue = MarketUtils::MarketType::HUOBI_SPOT;
          //normalized_ticker.incomingSequence = seq++;
          MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_ticker));
          break;
        }
        case Utils::hash_32_fnv1a("trade.detail"):
        {

          //static uint64_t seq = 1;
          if (decoder.Parse(buffer, len).HasParseError())
          {
            MD_LOG_INFO << "Not able to parse trade detail " << '\n'; // logger
            return;
          }
          //MD_LOG_INFO << "trade detail " << '\n';
          const Value &data = decoder["tick"]["data"];
          using normalized_msg_type = Message::Trade;
          for (int i = data.Size()-1; i >= 0; i--)
          {
            normalized_msg_type normalized_trade(data[i]["price"].GetDouble() * 1000000000,
                                                  data[i]["amount"].GetDouble(),
                                                  data[i]["direction"].GetString()[0]);
            normalized_trade.venue = MarketUtils::MarketType::HUOBI_SPOT;
            normalized_trade.apiIncomingTime = packet_hit_time;
            //normalized_trade.symbol = instrument_id;
            //normalized_trade.incomingSequence = seq++;
            //normalized_trade.exchangeOutgoingTime = data[i]["ts"].GetInt64();
            MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_trade));
          }
          break;
        }
        case Utils::hash_32_fnv1a("detail"):
        {

          if (decoder.Parse(buffer, len).HasParseError())
          {
            MD_LOG_INFO << "Not able to parse detail " << '\n'; // logger
            return;
          }
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
        /*const Value& status = decoder["status"];
                if(status.GetString()=="ok")
                {
                  if(decoder.HasMember("subbed"))
                  {
                    MD_LOG_INFO << "Subscribed sucessfully " << decoder["subbed"].GetString() << '\n'; // logg
                  }
                  else
                  {
                    MD_LOG_INFO << "Unsubscribed sucessfully " << decoder["unsubbed"].GetString() << '\n'; // logg
                  }
                }
                */
        break;
      }
      default:
      {
      }
      }
    }
    void HUOBI_SPOT::decodeBinary(char *buffer, size_t len)
    {
    }
  }
}

}
