#include <MarketData/RingbufferConfig.h>

namespace EFG
{
namespace MarketData
{
namespace ConfigManager
{
  RingbufferConfig::RingbufferConfig()
  {

  }
  const std::string& RingbufferConfig::getMode() const 
  {
    return mode; 
  }
   
  const std::string& RingbufferConfig::getDataName() const
  {
    return data_name; 
  }
  const std::string& RingbufferConfig::getSemaphoreName() const
  {
    return semaphore_name; 
  }
  int RingbufferConfig::getDataCapacity() const
  {
    return data_capacity; 
  }
  const std::string& RingbufferConfig::getMetaDataName() const
  {
    return metadata_name; 
  }
  int RingbufferConfig::getMetaDataCapacity() const
  {
    return metadata_capacity; 
  }

  void RingbufferConfig::setMode(std::string _mode)
  {
    mode = _mode;
  }
  void RingbufferConfig::setDataName(std::string _name)
  {
    data_name = _name;
  }
  void RingbufferConfig::setSemaphoreName(std::string _name)
  {
    semaphore_name = _name;
  }
  void RingbufferConfig::setDataCapacity(int capacity)
  {
    data_capacity = capacity;
  }
  void RingbufferConfig::setMetaDataName(std::string _name)
  {
    metadata_name = _name;
  }
  void RingbufferConfig::setMetaDataCapacity(int capacity)
  {
    metadata_capacity = capacity;
  }

}
}
}
