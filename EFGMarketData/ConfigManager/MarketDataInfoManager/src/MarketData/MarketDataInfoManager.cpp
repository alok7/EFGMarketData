#include <MarketData/MarketDataInfoManager.h>

namespace EFG{
  namespace MarketData{
    namespace ConfigManager{
      MarketDataInfoManager::MarketDataInfoManager(std::string config_file )
      {
        CONFIGReader mdInfoTree(config_file);
        if (mdInfoTree.ParseError() < 0) {
           MD_LOG_INFO << "Can't load md config \n"; 
        }
        else{
           std::string sessions_cfg =  mdInfoTree.Get("SESSIONS", "sessions_cfg", "UNKNOWN");
           CONFIGReader sessionInfoTree(sessions_cfg);
           if(sessionInfoTree.ParseError() < 0)
           {
             MD_LOG_INFO << "Can't load md session config \n"; 
           }
           std::set<std::string> sections = sessionInfoTree.Sections();
           for(std::string section: sections)
           {
             buildVenueSessionsInfo(sessionInfoTree, section);
           }
           std::string read_shm_queue_cfg =  mdInfoTree.Get("SHM_CLIENT", "read_shm_queue_cfg", "UNKNOWN");
           std::string write_shm_queue_cfg =  mdInfoTree.Get("SHM_CLIENT", "write_shm_queue_cfg", "UNKNOWN");
           loadIPCSHMQueueConfig(ipc_readqueue_config, read_shm_queue_cfg);
           loadIPCSHMQueueConfig(ipc_writequeue_config, write_shm_queue_cfg); 
                   
        } 
        
      }
      std::unordered_map<std::string, std::unique_ptr<ConfigManager::VenueSessionInfo>>& MarketDataInfoManager::getVenueSessionInfoMap()
      {
        return VenueSessionsInfoMap; 
      }
      ConfigManager::RingbufferConfig& MarketDataInfoManager::getIPCReadRingbufferConfig() 
      {
        return ipc_readqueue_config;
      }
      ConfigManager::RingbufferConfig& MarketDataInfoManager::getIPCWriteRingbufferConfig() 
      {
        return ipc_writequeue_config;
      }
      void MarketDataInfoManager::buildVenueSessionsInfo(CONFIGReader& sessionInfoTree, std::string section)
      {
         std::unique_ptr<ConfigManager::VenueSessionInfo> sessionInfo = std::make_unique<ConfigManager::VenueSessionInfo>();
         
         std::string wssEndpoint = sessionInfoTree.Get(section,"wss_endpoint", "UNKNOWN"); 
         
         std::string httpEndpoint = sessionInfoTree.Get(section,"http_endpoint", "UNKNOWN"); 

         std::string sub_file = sessionInfoTree.Get(section,"subscription_file", "UNKNOWN"); 

         std::string name = sessionInfoTree.Get(section,"name", "UNKNOWN"); 

	 std::string api_key = sessionInfoTree.Get(section,"api_key", "UNKNOWN");

         std::string secret_key = sessionInfoTree.Get(section,"secret_key", "UNKNOWN");

         int rate_limit = sessionInfoTree.GetInteger(section,"rate_limit", 0);

         std::unordered_map<std::string,std::string> params = sessionInfoTree.GetMap(section,"extra_params");

         sessionInfo->setWssEndpoint(trim(wssEndpoint));
         sessionInfo->setHttpEndpoint(trim(httpEndpoint));
         sessionInfo->setSubscriptionFile(trim(sub_file)); 
         sessionInfo->setName(trim(name));
	 sessionInfo->setApiKey(trim(api_key));
	 sessionInfo->setSecretKey(trim(secret_key));
	 sessionInfo->setRateLimit(rate_limit);
         sessionInfo->loadSubscriptions();
         sessionInfo->setExtraParams(params);

         VenueSessionsInfoMap.insert({sessionInfo->getName(), std::move(sessionInfo)});
                
      }

      void MarketDataInfoManager::loadIPCSHMQueueConfig(ConfigManager::RingbufferConfig& shm_cfg, const std::string shm_queue_cfg)
      {

        CONFIGReader infoTree(shm_queue_cfg);
        if (infoTree.ParseError() < 0) {
           MD_LOG_INFO << "Can't load ipc md shm ringbuffer file config \n"; // use logger
        }
        std::string mode = infoTree.Get("store","mode", "UNKNOWN");
        std::string data_name = infoTree.Get("store","data_name", "UNKNOWN");
        std::string semaphore_name = infoTree.Get("store","semaphore_name", "UNKNOWN");
        int data_capacity = infoTree.GetInteger("store","data_capacity", 0);
        std::string metedata_name = infoTree.Get("store","bookkeeper_name", "UNKNOWN");
        int metadata_capacity = infoTree.GetInteger("store","bookkeeper_capacity", 0);

        shm_cfg.setMode(mode);
        shm_cfg.setDataName(data_name);
        shm_cfg.setSemaphoreName(semaphore_name);
        shm_cfg.setDataCapacity(data_capacity);
        shm_cfg.setMetaDataName(metedata_name);
        shm_cfg.setMetaDataCapacity(metadata_capacity);
      }

    }
  }
}

