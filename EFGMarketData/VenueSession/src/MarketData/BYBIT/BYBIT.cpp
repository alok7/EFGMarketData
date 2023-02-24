#include <MarketData/BYBIT/BYBIT.h>
#include <chrono>

#define BUFLEN 65536

namespace EFG{
  namespace MarketData{
    namespace VenueSession{
	   BYBIT::BYBIT(const ConfigManager::VenueSessionInfo& vsinfo): AbstractVenue(vsinfo){
	      transport = std::make_unique<Transport::WssTransport>(&md_hub);
              registerCallBacks();
              MD_LOG_INFO << " Created BYBIT Session " << '\n'; // logger - later
	   }
	   void BYBIT::onConnection(uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req)
	   {
	     MD_LOG_INFO << " BYBIT Connected " << '\n';
             //authenticate here 
             std::string api_key("qQEljeIgOf5YGgdQAg"); // from config
             std::string secret_key("ogyteNdu15uqCJKq6O3uUe6vvfZOXT5yuKJW"); // from config
             using namespace std::chrono;
	     uint64_t ts_ms = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
	     std::string expires = std::to_string(ts_ms + 1000); 
             unsigned char * mac = NULL;
             unsigned int mac_length = 0;
             std::string data = "GET/realtime" + expires;
             int ret = Utils::HMAC::HmacEncode("sha256", secret_key.c_str(), secret_key.length(), data.c_str(), data.length(), mac, mac_length);
             char sign[64]={0};
             for(int i = 0; i < 32; i++) {
		sprintf(&sign[i*2], "%02x", (unsigned int)mac[i]);
	     }
             std::string autentication_str = std::string("{\"op\": \"auth\", \"args\": [\"" + api_key + "\",\"") + expires + "\",\"" + sign + "\"]}";
             ws->send(autentication_str.c_str()); 
             //
             std::string hbeat =  "{\"op\":\"ping\"}";
             ws->send(hbeat.c_str()); // Need to send every 30 sec-1 min -- later
             subscriptionHandler->init(ws); 
             subscriptionHandler->run();
             MD_LOG_INFO << " Starting Susbcription Handler for BYBIT " << '\n';
	   }
	   void BYBIT::onUpdate(uWS::WebSocket<uWS::CLIENT> *ws, char * message, std::size_t len_, uWS::OpCode code)
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
             catch(...)
             {
               std::cerr << "Unknown Exception on Decode " << '\n';
             }
             
	   }
	   void BYBIT::onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, std::size_t len_){
	     
             try
             {
               MD_LOG_INFO << "BYBIT Disconnected " << std::to_string(Utils::getHRTime()) << '\n';
               subscriptionHandler->stop();
               subscriptionHandler = std::make_unique<VenueSession::SubscriptionHandler>();
               registerCallBacks();
	       this->submitRequests(this->venueSessionInfo.getSubscriptions());
	       this->connect(this->venueSessionInfo.getWssEndpoint());
             }
             catch(const std::exception& e)
             {
               std::cerr << "Exception on Disconnect Event " << e.what() << '\n';
             }
             catch(...)
             {
               std::cerr << "Unknown Exception on Disconnet " << '\n';
             }
 	   
	   } 
          	
           void BYBIT::registerCallBacks()
           {
             using namespace std::placeholders;
             transport->registerOnConnectCallBack(std::bind(&BYBIT::onConnection, this,_1,_2));
             transport->registerOnUpdateCallBack(std::bind(&BYBIT::onUpdate, this,_1,_2,_3,_4));
             transport->registerOnDisconnectCallback(std::bind(&BYBIT::onDisconnection, this,_1,_2,_3,_4));          

           }
           void BYBIT::decode(char* buffer, size_t len, const TimeType& packet_hit_time)
           {
             Document decoder;
             //buffer[len] = '\0'; // explicitly terminate string 
             
             if (decoder.Parse(buffer, len).HasParseError()) {
               MD_LOG_INFO << "Not able to parse resonse " << '\n';// logger
               return;
             }
             //get the update type 
             unsigned char i = 0, pos = 0;
             unsigned char indexDoubleQuote[] = {0, 0, 0, 0}; // index of 1st quote, 2nd quote and so on;
             while(pos < 4){ 
               if(buffer[i]=='"')indexDoubleQuote[pos++]=i;
               ++i;
             }
             // response type between first and second quote 
             std::string response_type(buffer+indexDoubleQuote[0]+1, indexDoubleQuote[1]-indexDoubleQuote[0]-1);
             
             switch(Utils::hash_32_fnv1a(response_type.c_str()))
             {
               case Utils::hash_32_fnv1a("topic") :
               {
                 //update type
                 std::string _type = decoder["topic"].GetString();
                 std::string sub_type = _type.substr(0, _type.find("."));
                 switch(Utils::hash_32_fnv1a(sub_type.c_str()))
                 {
                   case Utils::hash_32_fnv1a("orderBookL2_25") :
                   case Utils::hash_32_fnv1a("orderBook_200") :
                   {
                     

                     using normalized_msg_type = Message::DepthBook;
                     using order_side = EFG::MarketData::order_book_side;
		     normalized_msg_type normalized_depth;
 
                     const Value& data = decoder["data"];
                     std::string symb = data[0]["symbol"].GetString();

                     switch(decoder["type"].GetString()[0])
                     {
                       case 's' : // snapshot
                       {
                         this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).clear();
                         for (SizeType i = 0; i < data.Size(); i++){
                           int64_t price = Utils::convertToPrice(data[i]["price"].GetString());
                           switch (data[i]["side"].GetString()[0])
                           {
                             case 'B' :
                             {
		                this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(
                                   price,  data[i]["size"].GetDouble(), 1 ));
                               break;
                             }
                             case 'S' :
                             {
		               this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(
                                   price,  data[i]["size"].GetDouble(), 1 ));
                               break;
                             }
                           }
                         }

                         this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
                         this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
                         normalized_depth.apiIncomingTime = packet_hit_time;
                         //normalized_depth.symbol = data[0]["symbol"].GetString();
                         normalized_depth.venue = MarketUtils::MarketType::BYBIT;

                         MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
                         break;
                       }
                       case 'd' : //delta - update
                       {
                         if(data["delete"].Size() > 0)
                         {
                           for (SizeType i = 0; i < data["delete"].Size(); i++){

                             int64_t price = Utils::convertToPrice(data["delete"][i]["price"].GetString());
                             switch(data["delete"][i]["side"].GetString()[0])
                             {
                               case 'B':
                               {
                                 auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::bid, price);
                                 //normalized_depth.bids.push_back(normalized_msg_type::price_level(
                                 // price, 0, 1));
                                 this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).remove(order_side::bid, it);
                                 break;
                               }
                               case 'S':
                               {
                                 auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::ask, price);
                                 //normalized_depth.asks.push_back(normalized_msg_type::price_level(
                                 // price, 0, 1));
                                 this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).remove(order_side::ask, it);
                                 break;
                               }
                             }
                           }
                           //normalized_depth.symbol = data["delete"][0]["symbol"].GetString();
                         }
                          
                         if(data["update"].Size() > 0)
                         {
                           for (SizeType i = 0; i < data["update"].Size(); i++){

                             int64_t price = Utils::convertToPrice(data["update"][i]["price"].GetString());
                             double qty = data["update"][i]["size"].GetDouble();
			     uint64_t orders_count = 1;

                             switch(data["update"][i]["side"].GetString()[0])
                             {
                               case 'B':
                               {
			         auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::bid, price);
                                 it->second = normalized_msg_type::price_level(price, qty, orders_count);
                                 break;
                               }
                               case 'S':
                               {

			         auto it = this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).lowerBound(order_side::ask, price);
                                 it->second = normalized_msg_type::price_level(price, qty, orders_count);
                                 break;
                               }
                             }
                           }
                           //normalized_depth.symbol = data["update"][0]["symbol"].GetString();
                         }

                         if(data["insert"].Size() > 0)
                         {
                           for (SizeType i = 0; i < data["insert"].Size(); i++){

                             int64_t price = Utils::convertToPrice(data["insert"][i]["price"].GetString());
                             double qty = data["insert"][i]["size"].GetDouble();

                             switch(data["insert"][i]["side"].GetString()[0])
                             {
                               case 'B':
                               {
                                 if (qty == 0) continue; // inserted order cannot be of zero size
		                 this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::bid, price, normalized_msg_type::price_level(price, qty, 1) );
                                 break;
                               }
                               case 'S':
                               {
                                 if (qty == 0) continue; // inserted order cannot be of zero size
		                 this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).insert(order_side::ask, price, normalized_msg_type::price_level(price, qty, 1) );
                                 break;
                               }
                             }
                           }
                           //normalized_depth.symbol = data["insert"][0]["symbol"].GetString();
                         }
                         this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getBids(normalized_depth.bids);
                         this->getOrderBook<OrderBook<const priceType, normalized_msg_type>>(symb).getAsks(normalized_depth.asks);
                         normalized_depth.apiIncomingTime = packet_hit_time;
                         normalized_depth.venue = MarketUtils::MarketType::BYBIT;
                         MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_depth));
                         break;
                       }
                     }
                     break;
                   }
                  
                   case Utils::hash_32_fnv1a("trade") :
                   {
                     /*
		     using msg_type = BYBITSchema::Messages::trade;
                     msg_type all_trade_msg;
                     std::vector<msg_type::level> trades;
		     */
		     using normalized_msg_type = Message::Trade;
		     std::vector<normalized_msg_type> normalized_trades;
                     const Value& data = decoder["data"];
                     for (SizeType i = 0; i < data.Size(); i++){
                       // trades.emplace_back(data[i]["trade_id"].GetString(), data[i]["symbol"].GetString(), data[i]["price"].GetDouble(), data[i]["side"].GetString()[0], data[i]["size"].GetDouble());
                       normalized_msg_type normalized_trade(data[i]["price"].GetDouble()*1000000000,
				                            data[i]["size"].GetDouble(),
							    tolower(data[i]["side"].GetString()[0]));
		       //normalized_trade.symbol = data[i]["symbol"].GetString();
		       normalized_trade.venue = MarketUtils::MarketType::BYBIT;
		       normalized_trade.apiIncomingTime = packet_hit_time;
		       // normalized_trades.push_back(std::move(normalized_trade));
           MarketDataEventQueue<normalized_msg_type>::Instance().push(std::move(normalized_trade));
		     }
                     // all_trade_msg.trades = std::move(trades);                    
                       
                     break;
                   }
                   
                   case Utils::hash_32_fnv1a("insurance") :
                   {
                     break;
                   }
                   
                   case Utils::hash_32_fnv1a("instrument_info") :
                   {
                     break;
                   }
                   
                   case Utils::hash_32_fnv1a("klineV2") :
                   {
                     break;
                   }
 
                 } 
                 break;
               }
                
               case Utils::hash_32_fnv1a("success") :
               {
                 if(decoder["success"].GetBool())
                 {
                   const Value& response_arg = decoder["request"]["args"];
                   if(!response_arg.IsNull())
                   {
                     std::string subs("");
                     for (SizeType i = 0; i < response_arg.Size(); i++){
                       std::string sub = response_arg[i].GetString();
                       subs += (sub + ", ");
                     } 
                     MD_LOG_INFO << "Successful Subscription to " << subs << '\n';
                   }
                   else{
                     MD_LOG_INFO << "pong recived"<< '\n';
                   }
                 }
                 break;
               }
 
             }
           }
           void BYBIT::decodeBinary(char* buffer, size_t len)
           {

           } 
    }
  }

}

