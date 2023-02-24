#pragma once

#include <uWS.h>
#include <iostream>
#include <memory>
#include <MarketData/MarketDataInfoManager.h>
#include <type_traits>
#include <MarketData/HUOBI_SPOT/HUOBI_SPOT.h>
#include <MarketData/HUOBI_FUTURES/HUOBI_FUTURES.h>
#include <MarketData/HUOBI_SWAP/HUOBI_SWAP.h>
#include <MarketData/OKEX/OKEX.h>
#include <MarketData/BINANCE_SPOT/BINANCE_SPOT.h>
#include <MarketData/BINANCE_FUTURES/BINANCE_FUTURES.h>
#include <MarketData/BITMEX/BITMEX.h>
#include <MarketData/BYBIT/BYBIT.h>
#include <MarketData/BITFINEX/BITFINEX.h>
#include <MarketData/COINBASE/COINBASE.h>
#include <MarketData/CME/CME.h>
#include <MarketData/HUOBI_SWAP_USDT/HUOBI_SWAP_USDT.h>
#include <MarketData/BINANCE_OPTIONS/BINANCE_OPTIONS.h>
#include <MarketData/BINANCE_COIN_FUTURES/BINANCE_COIN_FUTURES.h>
#include <MarketData/KITE_ZERODHA/KITE_ZERODHA.h>
#include <MarketData/ClientFeedHandlers.h>
#include <MarketData/ringbuffer.h>
#include <MarketData/compile_config.h>


using namespace EFG::MarketData;

class MarketData{
  private:
    MarketData(std::string _config_path);
    ~MarketData()
    {

    }
  public:
    static auto& Instance()
    {
      static MarketData md_instance(md_config_path);
      return md_instance;
    }
    static void Init(std::string _config_path)
    {
      md_config_path = _config_path;
    }
    void run(); 
    std::vector<std::shared_ptr<VenueSession::AbstractVenue>>& getActiveSessions();
    std::unique_ptr<ConfigManager::MarketDataInfoManager> mdInfoManager;
  private:
    std::vector<std::shared_ptr<VenueSession::AbstractVenue>> activeSessions;
    static std::string md_config_path; 
};

