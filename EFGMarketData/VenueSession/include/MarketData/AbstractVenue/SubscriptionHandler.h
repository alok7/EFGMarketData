#pragma once
#include <iostream>
#include <uWS.h>
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <future>
#include <MarketData/Logger.h>

namespace EFG{
namespace MarketData{
namespace VenueSession{
  class SubscriptionHandler{
    public:
      SubscriptionHandler();
      SubscriptionHandler(SubscriptionHandler &&) = default;
      ~SubscriptionHandler()
      {
        stop();
        if(m_thread.joinable()) // make sure, thread is not stuck here
        {
          m_thread.join();
        }
 
      };
      void submit(std::string request);
      void init(uWS::WebSocket<uWS::CLIENT> *ws);
      void join();
      void run();
      void stop();
    private:
      void load();
      std::queue<std::string> requestQueue;
      /*
      struct SpinLock {
    
        void lock()
        {
          while (locked.test_and_set(std::memory_order_acquire)){;}
        }
        void unlock()
        {
          locked.clear(std::memory_order_release);  
        }
      private:
        std::atomic_flag locked = ATOMIC_FLAG_INIT;    
    };
    SpinLock _syncRequest;
    */
    std::atomic<bool> isInit = false;
    uWS::WebSocket<uWS::CLIENT> *_ws = nullptr;
    std::atomic<bool> m_running = false;
    std::thread m_thread;
 };

}
}
}
