#include<MarketData/VenueSessionInfo.h>

namespace EFG{
  namespace MarketData{
    namespace ConfigManager{
       
       VenueSessionInfo::VenueSessionInfo()
       {
       } 
       const std::string& VenueSessionInfo::getWssEndpoint() const{
         return _wssBaseEndPoint;
       }
       const std::string& VenueSessionInfo::getHttpEndPoint() const{
         return _httpBaseEndPoint;
       }
       const std::string& VenueSessionInfo::getSubscriptionFile() const{
         return _subscription_file; 
       }
       const std::string& VenueSessionInfo::getName() const
       {
         return _name;
       }
       const std::string& VenueSessionInfo::getApiKey() const {
         return _api_key;
       }
       const std::string& VenueSessionInfo::getSecretKey() const {
         return _secret_key;
       }
       int VenueSessionInfo::getRateLimit() const {
         return _rate_limit;
       }
       const std::unordered_map<std::string,std::string>& VenueSessionInfo::getExtraParams() const {
         return _extra_params;
       }
       void VenueSessionInfo::setWssEndpoint(const std::string& wssendpoint)
       {
         _wssBaseEndPoint = wssendpoint;
       }
       void VenueSessionInfo::setHttpEndpoint(const std::string& httpendpoint)
       {
         _httpBaseEndPoint = httpendpoint;
       }
       const std::vector<std::string>& VenueSessionInfo::getSubscriptions() const
       {
         return _subscriptions;
       }
       void VenueSessionInfo::setName(const std::string& name)
       {
         _name = name;  
       }
       void VenueSessionInfo::setApiKey(const std::string& api_key) {
	 _api_key = api_key;
       }
       void VenueSessionInfo::setSecretKey(const std::string& secret_key) {
         _secret_key = secret_key;
       }
       void VenueSessionInfo::setRateLimit(int rate_limit) {
         _rate_limit = rate_limit;
       }
       void VenueSessionInfo::setSubscriptionFile(const std::string& file_path)
       {
         _subscription_file = file_path;
       }
       void VenueSessionInfo::setExtraParams(std::unordered_map<std::string,std::string>& extra_params){
         _extra_params = extra_params;
       }
       void VenueSessionInfo::loadSubscriptions(){
         CONFIGReader subscriptionReader(_subscription_file);
         if (subscriptionReader.ParseError() < 0) {
           MD_LOG_INFO << "Can't load subscription file " << _subscription_file << '\n'; // use logger. Later!
           return; 
         }
         std::unordered_map<std::string, std::string> subscriptionLookup = subscriptionReader.GetKeyValues("subscriptions");
         for(auto& p: subscriptionLookup)
         {
           _subscriptions.emplace_back(p.second);
         }
         
       }

    }
  }
}

