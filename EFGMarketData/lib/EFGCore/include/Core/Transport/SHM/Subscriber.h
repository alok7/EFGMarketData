#pragma once
#include <variant>
#include <memory>
#include <thread>
#include <chrono>
#include <Core/Transport/SHM/Container.h>

namespace EFG
{
namespace Core
{


template<typename... UpdateChanelEventTypes>
class Subscriber
{
  public: 
    using UPDATE_CHANEL   = SHMTransport<UpdateChanelEventTypes...>;
  public:
    Subscriber(const std::string& name, size_t capacity) 
    {
      mUpdateChanel.reset(new UPDATE_CHANEL(name+"_update_", Mode::RW_ONLY, capacity)); 
    }
    template<typename... Fs>
    void start(Fs&&... fs)
    {
      mWorkerUpdate = std::thread([this, ... fs = std::forward<Fs>(fs)]()
      {
        using namespace std::chrono_literals;
        while(!mUpdateChanel->isInitialized()){std::this_thread::sleep_for(200ms);} // wait
        mUpdateChanel->setReaderPosition();	
        mUpdateChanel->readAsync(fs...);
      });
    }
    ~Subscriber()
    {
      //stop();
      if(mWorkerUpdate.joinable())
        mWorkerUpdate.join();	      
    }
    void stop()
    {
      mUpdateChanel->stop();
    }	    
  private:
    std::unique_ptr<UPDATE_CHANEL> mUpdateChanel;
    std::thread mWorkerUpdate;
};


}}
