#pragma once
#include <variant>
#include <memory>
#include <thread>
#include <Core/Transport/SHM/Container.h>

namespace EFG
{
namespace Core
{


template<typename... UpdateChanelEventTypes>
class Publisher
{
  public: 
    using UPDATE_CHANEL   = SHMTransport<UpdateChanelEventTypes...>;
  public:
    Publisher(const std::string& name, size_t capacity) 
    {
      mUpdateChanel.reset(new UPDATE_CHANEL(name+"_update_", Mode::CREATE_RW, capacity)); 
    }
    ~Publisher()
    {
      stop();
    }
    void stop()
    {
      mUpdateChanel->stop();
    }	    
    template<typename EVENT>
    void publish( const EVENT& event)
    {
      unsigned int delay = 1;	    
      for(uint8_t count = 0; !mUpdateChanel->allocate(event) && count < 8; ++count) // try for max ~500us otherwise drop the payload
      {	      
        usleep(delay);
        delay *=2;	
      }	
    }
  private:
    std::unique_ptr<UPDATE_CHANEL> mUpdateChanel;
};


}}
