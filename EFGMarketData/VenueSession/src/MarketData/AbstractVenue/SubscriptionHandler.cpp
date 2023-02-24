#include <MarketData/AbstractVenue/SubscriptionHandler.h>

namespace EFG{
namespace MarketData{
namespace VenueSession{

    SubscriptionHandler::SubscriptionHandler(){}

    void SubscriptionHandler::submit(std::string request)
    {
      //std::lock_guard<SpinLock> lock(_syncRequest);
      requestQueue.push(request);
    }
    void SubscriptionHandler::init(uWS::WebSocket<uWS::CLIENT> *ws)
    {
      this->_ws = ws;
      isInit.store(true);
    }
    void SubscriptionHandler::load()
    {

    }

    void SubscriptionHandler::join()
    {
      if(m_thread.joinable()) m_thread.join();
    }

    void SubscriptionHandler::run()
    {
      m_running.store(true);	
      m_thread = std::thread([this]() {
          while(m_running.load())
          {
            while(!requestQueue.empty() && isInit.load())
            {
              //std::lock_guard<SpinLock> lock(_syncRequest);
              std::string& request = requestQueue.front();
              if(_ws!=nullptr)
              {
                _ws->send(request.c_str());
                MD_LOG_INFO << "Subscription Submitted to Exchange: " << request << '\n'; 
                requestQueue.pop();
              }
              std::this_thread::sleep_for (std::chrono::seconds(5)); // succesive delay btw subscriptions, from config for each venue
            }
            std::this_thread::sleep_for (std::chrono::seconds(5)); // succesive delay when queue is empty
          }
       });

    }
    void SubscriptionHandler::stop()
    {
      m_running.store(false);
      isInit.store(false);
      while(!requestQueue.empty()) requestQueue.pop();
    }
    

}
}
}
