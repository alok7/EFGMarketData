#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <MarketData/ConfigReader.h>
#include <Core/MarketUtils/MarketUtils.h>
#include <MarketData/VenueSessionInfo.h>
#include <MarketData/RingbufferConfig.h>

using namespace EFG::MarketData;
using namespace EFG::Core;

namespace EFG{
  namespace MarketData{
    namespace ConfigManager{
      class MarketDataInfoManager{
        public:
          MarketDataInfoManager(std::string config_file);
          std::unordered_map<std::string, std::unique_ptr<ConfigManager::VenueSessionInfo>>& getVenueSessionInfoMap();
          ConfigManager::RingbufferConfig& getIPCReadRingbufferConfig();
          ConfigManager::RingbufferConfig& getIPCWriteRingbufferConfig();
        private:
          void buildVenueSessionsInfo(CONFIGReader& sessionInfoTree, std::string section);
          void loadIPCSHMQueueConfig(ConfigManager::RingbufferConfig& shm_cfg, const std::string shm_queue_cfg);
          std::unordered_map<std::string, std::unique_ptr<ConfigManager::VenueSessionInfo>> VenueSessionsInfoMap;
          ConfigManager::RingbufferConfig ipc_readqueue_config;
          ConfigManager::RingbufferConfig ipc_writequeue_config; 
      };

    }
  }
}

