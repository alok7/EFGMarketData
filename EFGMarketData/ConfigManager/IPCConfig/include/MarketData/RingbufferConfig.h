#pragma once
#include <string>

namespace EFG
{
namespace MarketData
{
namespace ConfigManager
{
  class RingbufferConfig
  {
    public:
      RingbufferConfig();
      const std::string& getMode() const;
      const std::string& getDataName() const;
      const std::string& getSemaphoreName() const;
      int getDataCapacity() const;
      const std::string& getMetaDataName() const;
      int getMetaDataCapacity() const;

      void setMode(std::string _mode);
      void setDataName(std::string _name) ;
      void setSemaphoreName(std::string _name);
      void setDataCapacity(int);
      void setMetaDataName(std::string _name);
      void setMetaDataCapacity(int);
    private:
      std::string mode;
      std::string data_name;
      std::string semaphore_name;
      size_t data_capacity;
      std::string metadata_name;
      size_t metadata_capacity;        

  }; 

} 
}
}
