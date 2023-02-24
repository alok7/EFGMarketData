#pragma once
#include <mutex>
#include <atomic>
#include <chrono>
#include <utility>
#include <array>
#include <MarketData/compile_config.h>
#include <Core/Message/Message.h>
#include <Core/Queue/SPSC/readerwriterqueue.h>

using namespace EFG::Core;
using namespace moodycamel;

namespace EFG
{
namespace MarketData
{

  template<typename T>
  struct RBSize
  {
  };

  template<>
  struct RBSize<Message::Ticker>
  {
    static const int64_t size = TICKER_QUEUE_SIZE;
  };

  template<>
  struct RBSize<Message::Trade>
  {
    static const int64_t size = TRADE_QUEUE_SIZE;
  };

  template<>
  struct RBSize<Message::AggregateTrade>
  {
    static const int64_t size = TRADE_QUEUE_SIZE;
  };

  template<>
  struct RBSize<Message::DepthBook>
  {
    static const int64_t size = DEPTHBOOK_QUEUE_SIZE;
  };

  template<>
  struct RBSize<Message::Liquidation>
  {
    static const int64_t size = LIQUIDATION_QUEUE_SIZE;
  };

  template<typename EventType, int64_t RB_CAPACITY= RBSize<EventType>::size>
  class MarketDataEventQueue
  {
    private:
      MarketDataEventQueue()
      {
	queueImpl = std::make_unique<ReaderWriterQueue<EventType>>(RB_CAPACITY);
      }
    public:
      static auto& Instance()
      {
         static MarketDataEventQueue<EventType> instance;
         return instance;
      };
      void push(const EventType& event)
      {
	unsigned int delay = 1;
        for(uint8_t count = 0; !queueImpl->try_enqueue(event) && count < 8; ++count)	
        {
	  usleep(delay);
	  delay *=2;
        }
      }
      std::pair<EventType, bool> pop()
      {
	EventType element;
	bool succeeded = queueImpl->try_dequeue(element);
        return std::make_pair(element, succeeded);
      }
    private:
      std::unique_ptr<ReaderWriterQueue<EventType>> queueImpl;
  };

}
}
