#pragma once

#include <atomic>
#include <iostream>
#include <chrono>
#include <functional>
#include <thread>
#include <uWS.h>
#include <cstdint>
#include <optional>
#include <vector>
#include <future>
#include <condition_variable>

using Clock = std::chrono::high_resolution_clock;

namespace EFG
{
namespace MarketData
{

namespace Timer
{

  class ConditionalTimer {
  public:
    ConditionalTimer() {}
    ConditionalTimer(std::function<void(void)> func, const long &interval) {
      m_func = func;
      m_interval = interval;
      _isInit = true;
    }
    ConditionalTimer(const long &interval) {
      m_interval = interval;
    }
    void join()
    {
      if(m_thread.joinable()) m_thread.join();
    }
    void init(uWS::WebSocket<uWS::CLIENT> *ws)
    {   
      this->_ws = ws; 
      _isInit.store(true);
    } 
    void start() {
      start_time=Clock::now();
      m_running.store(true);
      m_thread = std::thread([this]() {
        auto shifted_interval_in_chrono = std::chrono::milliseconds(m_interval);
        while (m_running.load() && _isInit.load()) {
          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start_time).count();
	        if(duration>=m_interval)
          {
            m_func();
            start_time = Clock::now();
          }
          std::this_thread::sleep_until(start_time + shifted_interval_in_chrono);
        }
      });
    }
    template<typename TIMER_CALLBACK>
    void run(TIMER_CALLBACK&& cb)
    {
      m_running.store(true);
      _isInit.store(true);
      m_thread = std::thread([this, moved_cb = std::move(cb)]() {
        while (m_running.load() && _isInit.load()) {
          std::this_thread::sleep_for (std::chrono::milliseconds(m_interval));
          moved_cb();
        }
      });
    }
    void stop() {
      _isInit.store(false);
      m_running.store(false);
    }
    void restart() {
      stop();
      start();
    }
    void resetTimer()
    {
      start_time = Clock::now();
    }	
    bool isRunning() { 
      return m_running.load(); 
    }
    void setFunc(std::function<void(void)>&& func) {
      _isInit = true;
      m_func = std::move(func);
    }
    long getInterval() { 
      return m_interval; 
    }
    void setInterval(const long &interval) {
      m_interval = interval;
    }

    ~ConditionalTimer() {
      stop();
      if(m_thread.joinable())
      {
        m_thread.join();
      }

    }

    private:
      std::function<void(void)> m_func;
      long m_interval; // in millis
      Clock::time_point start_time; 
      std::thread m_thread;
      std::atomic<bool> m_running = false;
      std::atomic<bool> _isInit = false;
      uWS::WebSocket<uWS::CLIENT> *_ws = nullptr;

  };



class LoopThreadTimer
{
  public:
    LoopThreadTimer() = default;
    LoopThreadTimer(const LoopThreadTimer&) = delete;
    LoopThreadTimer& operator=(const LoopThreadTimer&) = delete;

    LoopThreadTimer(LoopThreadTimer&&) = delete;
    LoopThreadTimer& operator=(LoopThreadTimer&&) = delete;

    template<typename Cb>
    LoopThreadTimer(const long duration, Cb&& cb)
    {
      start(duration, std::move(cb));
    }

    virtual ~LoopThreadTimer()
    {
     stop();
     if (_thread.joinable())
       _thread.join();
    }

    template<typename Cb>
    void start(const long duration, Cb&& cb)
    {
       auto thread_cb = 
          [this, duration, moved_cb = std::move(cb)]()
	  {
	    while (!_stop)
	    {
              std::this_thread::sleep_for (std::chrono::milliseconds(duration));
	      if (!_stop)
	        moved_cb();
	    }
	  };
	auto task = std::packaged_task<decltype(thread_cb())()>(std::forward<decltype(thread_cb)>(thread_cb));
		
        _thread = std::thread(std::move(task));
     }

     void  stop()
     {
       _stop = true;
     }
  private:
    std::thread	_thread;
    std::atomic<bool>	_stop { false };
	
};


}

}
}
