#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <MarketData/ConfigReader.h>
#include <MarketData/Logger.h>

namespace EFG{
  namespace MarketData{
    namespace ConfigManager{
      class VenueSessionInfo{
        public:
          VenueSessionInfo();
          const std::string& getWssEndpoint() const;
          const std::string& getHttpEndPoint() const;
          const std::string& getSubscriptionFile() const;
	  const std::string& getApiKey() const;
	  const std::string& getSecretKey() const;
	  int getRateLimit() const;
          const std::vector<std::string>& getSubscriptions() const;
          const std::string& getName() const;
          const std::unordered_map<std::string,std::string>& getExtraParams() const;
	  void setWssEndpoint(const std::string&);
          void setHttpEndpoint(const std::string&);
          void setName(const std::string&);
	  void setApiKey(const std::string&);
	  void setSecretKey(const std::string&);
	  void setRateLimit(int);
          void setExtraParams(std::unordered_map<std::string,std::string>&);
          void setSubscriptionFile(const std::string&);
          void loadSubscriptions();
        private:
          std::vector<std::string>_subscriptions;
          std::string _wssBaseEndPoint;
          std::string _httpBaseEndPoint;
          std::string _name;
          std::string _subscription_file;
	  std::string _api_key;
	  std::string _secret_key;
	  int _rate_limit;
          std::unordered_map<std::string,std::string> _extra_params;
      };

    }
  }
}

