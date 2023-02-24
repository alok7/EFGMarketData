#include <MarketData/MarketData.h>

MarketData::MarketData(std::string config_path)
{
  mdInfoManager.reset(new ConfigManager::MarketDataInfoManager(config_path));
  activeSessions.reserve(MarketUtils::MarketType::SIZE);
  for(int i =0; i < MarketUtils::MarketType::SIZE; ++i) activeSessions.push_back(nullptr);

}

std::vector<std::shared_ptr<VenueSession::AbstractVenue>>& MarketData::getActiveSessions()
{
  return activeSessions;
}

void MarketData::run() 
{
  std::unordered_map<std::string, std::unique_ptr<ConfigManager::VenueSessionInfo>>& info_map = mdInfoManager->getVenueSessionInfoMap();
  for(auto& p: info_map){
    
    std::string venue_name = p.first;
    const std::vector<std::string>& subscriptions = p.second->getSubscriptions();
    switch(Market::toEnum(venue_name)){
      case MarketUtils::MarketType::HUOBI_SPOT :
      {
        auto huobi_spot = std::make_shared<VenueSession::HUOBI_SPOT>(*(p.second));
        MD_LOG_INFO << "Connecting HUOBI_SPOT...." << '\n';
        huobi_spot->submitRequests(subscriptions);
	huobi_spot->connect(p.second->getWssEndpoint());
        activeSessions[Market::toEnum(venue_name)]= std::move(huobi_spot);
        break;
      }
      
      case MarketUtils::MarketType::KITE_ZERODHA :
      {
        auto kite = std::make_shared<VenueSession::KITE_ZERODHA>(*(p.second));
        MD_LOG_INFO << "Connecting KITE_ZERODHA...." << '\n';
	kite->connect(p.second->getWssEndpoint());
        activeSessions[Market::toEnum(venue_name)]= std::move(kite);
        break;
      }
      case MarketUtils::MarketType::HUOBI_FUTURES :
      {
        auto huobi_futures = std::make_shared<VenueSession::HUOBI_FUTURES>(*(p.second));
        MD_LOG_INFO << "Connecting HUOBI_FUTURES...." << '\n';
        huobi_futures->submitRequests(subscriptions);
        huobi_futures->connect(p.second->getWssEndpoint());
        activeSessions[Market::toEnum(venue_name)]= std::move(huobi_futures);
        break;
      }
          
      case MarketUtils::MarketType::HUOBI_SWAP :
      {
        auto huobi_swap = std::make_shared<VenueSession::HUOBI_SWAP>(*(p.second));
        MD_LOG_INFO << "Connecting HUOBI_SWAP...." << '\n';
        huobi_swap->submitRequests(subscriptions);
        huobi_swap->connect(p.second->getWssEndpoint());
        activeSessions[Market::toEnum(venue_name)] = std::move(huobi_swap);
        break;
      }
      
      case MarketUtils::MarketType::BINANCE_SPOT :
      {
        auto binance_spot = std::make_shared<VenueSession::BINANCE_SPOT>(*(p.second));
        MD_LOG_INFO << "Connecting BINANCE_SPOT..." << '\n';
        binance_spot->submitRequests(subscriptions);
        binance_spot->connect(p.second->getWssEndpoint());   
        activeSessions[Market::toEnum(venue_name)] = std::move(binance_spot);
        break;
      }
      
      case MarketUtils::MarketType::BITFINEX :
      {
        auto bitfinex = std::make_shared<VenueSession::BITFINEX>(*(p.second));
        MD_LOG_INFO << "Connecting BITFINEX..." << '\n';
        bitfinex->submitRequests(subscriptions);
        bitfinex->connect(p.second->getWssEndpoint());
        activeSessions[Market::toEnum(venue_name)] = std::move(bitfinex);
        break;   
      }

      case MarketUtils::MarketType::OKEX_SPOT :
      case MarketUtils::MarketType::OKEX_FUTURES :
      case MarketUtils::MarketType::OKEX_SWAP :
      case MarketUtils::MarketType::OKEX_OPTIONS :
      {
        auto okex = std::make_shared<VenueSession::OKEX>(*(p.second));
        MD_LOG_INFO << "Connecting " << venue_name << "..." << '\n';
        okex->submitRequests(subscriptions); 
        okex->connect(p.second->getWssEndpoint());   
        activeSessions[Market::toEnum(venue_name)] = std::move(okex);
        break;
      }
      
      case MarketUtils::MarketType::BINANCE_FUTURES :
      {
        auto binance_futures = std::make_shared<VenueSession::BINANCE_FUTURES>(*(p.second));
        MD_LOG_INFO << "Connecting BINANCE_FUTURES..." << '\n';
        binance_futures->submitRequests(subscriptions);
        binance_futures->connect(p.second->getWssEndpoint());   
        activeSessions[Market::toEnum(venue_name)] = std::move(binance_futures);
        break;
      }
          
      case MarketUtils::MarketType::BITMEX :
      {
        auto bitmex = std::make_shared<VenueSession::BITMEX>(*(p.second));
        MD_LOG_INFO << "Connecting BITMEX..." << '\n';
        bitmex->submitRequests(subscriptions);
        bitmex->connect(p.second->getWssEndpoint());   
        activeSessions[Market::toEnum(venue_name)] = std::move(bitmex);
        break;
      }

      case MarketUtils::MarketType::BYBIT :
      {
        auto bybit = std::make_shared<VenueSession::BYBIT>(*(p.second));
        MD_LOG_INFO << "Connecting BYBIT..." << '\n';
        bybit->submitRequests(subscriptions);
        bybit->connect(p.second->getWssEndpoint()); 
        activeSessions[Market::toEnum(venue_name)] = std::move(bybit);  
        break;
      }

         
      case MarketUtils::MarketType::COINBASE :
      {
        auto coinbase = std::make_shared<VenueSession::COINBASE>(*(p.second));
        MD_LOG_INFO << "Connecting COINBASE..." << '\n';
        coinbase->connect(p.second->getWssEndpoint());  
        activeSessions[Market::toEnum(venue_name)] = std::move(coinbase);
        break; 
      }
          
      case MarketUtils::MarketType::CME :
      {
        auto cme = std::make_shared<VenueSession::CME>(*(p.second));
        MD_LOG_INFO << "Connecting CME..." << '\n';
        cme->connect(p.second->getWssEndpoint());
        activeSessions[Market::toEnum(venue_name)] = std::move(cme);
        break;   
      }
          
      case MarketUtils::MarketType::HUOBI_SWAP_USDT :
      {
        auto huobi_swap_usdt = std::make_shared<VenueSession::HUOBI_SWAP_USDT>(*(p.second));
        MD_LOG_INFO << "Connecting HUOBI_SWAP_USDT..." << '\n';
        huobi_swap_usdt->connect(p.second->getWssEndpoint());  
        activeSessions[Market::toEnum(venue_name)] = std::move(huobi_swap_usdt);
        break; 
      }
      case MarketUtils::MarketType::BINANCE_OPTIONS :
      {
        auto binance_options = std::make_shared<VenueSession::BINANCE_OPTIONS>(*(p.second));
        MD_LOG_INFO << "Connecting BINANCE_OPTIONS..." << '\n';
        binance_options->connect(p.second->getWssEndpoint());  
        activeSessions[Market::toEnum(venue_name)] = std::move(binance_options);
        break; 
      }
      case MarketUtils::MarketType::BINANCE_COIN_FUTURES :
      {
        auto  binance_coin_futures = std::make_shared<VenueSession::BINANCE_COIN_FUTURES>(*(p.second));
        MD_LOG_INFO << "Connecting BINANCE_COIN_FUTURES..." << '\n';
        binance_coin_futures->connect(p.second->getWssEndpoint());  
        activeSessions[Market::toEnum(venue_name)] = std::move(binance_coin_futures);
        break; 
      }
    }
  }
  MD_LOG_INFO << "Running MarketData Server" << "\n";
  md_hub.run();
  MD_LOG_INFO << "MarketData server Stopped!" << "\n";

}

std::string MarketData::md_config_path="";

