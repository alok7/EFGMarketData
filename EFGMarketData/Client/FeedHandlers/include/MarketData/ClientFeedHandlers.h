#pragma once
#include <atomic>
#include <thread>
#include <functional>
#include <Core/Message/Message.h>
#include <MarketData/EventProcessingQueue.h>
#include <MarketData/CommonUtils.h>

using namespace EFG::MarketData;
using namespace EFG::Core;


namespace EFG
{
namespace MarketData
{
namespace FeedHandlers
{
  class ClientFeedHandler
  {
    public:
      ClientFeedHandler()
      {

      }
      ~ClientFeedHandler()
      {
        init.store(false);
        if(m_thread.joinable())
        {
          m_thread.join();
        }
      }
      template<typename HandlerTicker>
      void registerTickerHandler(HandlerTicker handler)
      {
        ticker_callback = handler;
      }
      template<typename HandlerTrade>
      void registerTradeHandler(HandlerTrade handler)
      {
        trade_callback = handler;
      }
      template<typename HandlerAggTrade>
      void registerAggTradeHandler(HandlerAggTrade handler)
      {
        aggTrade_callback = handler;
      }
      template<typename HandlerDepth>
      void registerDepthHandler(HandlerDepth handler)
      {
        depth_callback = handler;
      }
      template<typename HandlerLiquidation>
      void registerLiquidationHandler(HandlerLiquidation handler)
      {
          liquidation_callback = handler;
      }
      void run()
      {
       
        init.store(true);
        m_thread = std::thread([this](){

            MarketDataEventQueue<Core::Message::Ticker>& ticker_event_queue = MarketDataEventQueue<Core::Message::Ticker>::Instance();
            MarketDataEventQueue<Core::Message::Trade>& trade_event_queue = MarketDataEventQueue<Core::Message::Trade>::Instance();
            MarketDataEventQueue<Core::Message::AggregateTrade>& aggTrade_event_queue = MarketDataEventQueue<Core::Message::AggregateTrade>::Instance();
            MarketDataEventQueue<Core::Message::DepthBook>& depth_event_queue = MarketDataEventQueue<Core::Message::DepthBook>::Instance();
            //MarketDataEventQueue<Core::Message::Liquidation>& liquidation_event_queue = MarketDataEventQueue<Core::Message::Liquidation>::Instance();

            while(init.load())
            {
            
              auto ticker_event = ticker_event_queue.pop();
              if(ticker_event.second && ticker_callback)
              {
                (ticker_event.first).apiOutgoingTime = Utils::getHRTime(); 
                ticker_callback(std::move(ticker_event.first));
              }
            
              auto trade_event = trade_event_queue.pop();
              if(trade_event.second && trade_callback)
              {
                (trade_event.first).apiOutgoingTime = Utils::getHRTime();
	      	      trade_callback(std::move(trade_event.first));
              }
            
              auto aggTrade_event = aggTrade_event_queue.pop();
              if(aggTrade_event.second && aggTrade_callback)
              {
                (aggTrade_event.first).apiOutgoingTime = Utils::getHRTime();
	      	      aggTrade_callback(std::move(aggTrade_event.first));
              }
              auto depth_event = depth_event_queue.pop();
              if(depth_event.second && depth_callback)
              {
	        (depth_event.first).apiOutgoingTime = Utils::getHRTime();
                depth_callback(std::move(depth_event.first));
              }
               /*
              auto liquidation_event = liquidation_event_queue.pop();
              if(liquidation_event.second && liquidation_callback)
              {
                (liquidation_event.first).apiOutgoingTime = Utils::getHRTime();
                liquidation_callback(std::move(liquidation_event.first));
              }
              */

           }
        });

      }
      // getOrderBook(){} // pull based query

      //! Sets internal thread to run only on `cpuset`. Returns 0 if successful
      int set_cpu_affinity(cpu_set_t cpuset)
      {
        int rc = pthread_setaffinity_np(m_thread.native_handle(), sizeof(cpu_set_t), &cpuset);
        return rc;
      }
    private:
      std::function<void(Core::Message::Ticker&&)> ticker_callback;
      std::function<void(Core::Message::DepthBook&&)> depth_callback; 
      std::function<void(Core::Message::Trade&&)> trade_callback;
      std::function<void(Core::Message::AggregateTrade&&)> aggTrade_callback;
      std::function<void(Core::Message::Liquidation&&)> liquidation_callback;
     
      std::thread m_thread; 
      std::atomic<bool> init = {false};
  }; 

}
}
}
