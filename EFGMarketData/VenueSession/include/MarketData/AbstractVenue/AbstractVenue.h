#pragma once
#include <MarketData/Logger.h>
#include <memory>
#include <atomic>
#include <MarketData/VenueSessionInfo.h>
#include <Core/Transport/HTTPRequest/HTTPRequest.h>
#include <Core/Transport/WSS/WSS.h>
#include <uWS.h>
#include <MarketData/AbstractVenue/SubscriptionHandler.h>
#include <MarketData/json.h>
#include <MarketData/CommonUtils.h>
#include <Core/MarketUtils/MarketUtils.h>
#include <MarketData/orderbook.h>
#include <MarketData/EventProcessingQueue.h>
#include <Core/Message/Message.h>
#include <MarketData/Timer.h>
#include <chrono>
#include <sstream>
#include <locale>
#include <iomanip>
#include <mutex>
#include <MarketData/compile_config.h>
#include <exception>
#include <stdexcept>
#include <type_traits>

#define IsTicker 0
#define IsTrade 1
#define IsDepthBook 2
#define IsLiquidation 3


#ifndef likely
  #define likely(x) __builtin_expect((x), 1)
#endif

#ifndef unlikely
  #define unlikely(x) __builtin_expect((x), 0)
#endif

using namespace EFG::MarketData;
using namespace EFG::Core;
using namespace EFG::MarketData::ConfigManager;
using json = nlohmann::json;

namespace EFG
{
namespace MarketData
{

extern uWS::Hub md_hub;

namespace VenueSession
{
      class AbstractVenue
      {
      public:
        typedef OrderBook<const priceType, Message::DepthBook> SpotOrderBookType;
        typedef std::unordered_map<std::string, OrderBook<const priceType, Message::DepthBook>> MapOrderBookType;

        AbstractVenue(const ConfigManager::VenueSessionInfo &);
        virtual ~AbstractVenue(){};
        void submitRequest(std::string &request);
        void submitRequests(const std::vector<std::string> &requests);
        bool isConnected();
        void start();

        template<typename RET_TYPE>
        typename std::enable_if<std::is_same<RET_TYPE, OrderBook<const priceType, Message::DepthBook>>::value, RET_TYPE>::type&
        getOrderBook(std::string symb) 
        {
          return map_order_book[symb];
        }

      protected:
        bool first_update_received;
        bool snapshot_received;
        std::unique_ptr<EFG::Core::Transport::WssTransport> transport;
        std::unique_ptr<EFG::Core::Transport::HTTPRequest> httpRequest;
        std::unique_ptr<Timer::ConditionalTimer> timer;
        std::unique_ptr<Timer::LoopThreadTimer> new_timer;
        std::unique_ptr<VenueSession::SubscriptionHandler> subscriptionHandler;

        MapOrderBookType map_order_book;
        const ConfigManager::VenueSessionInfo &venueSessionInfo;
        Clock::time_point lastMDUpdateTime;
        std::atomic<bool> _isConnected = false;
	Message::Trade lastTrade;

      };

} 
}   
} 
