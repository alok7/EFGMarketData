#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <iomanip>
#include <sstream>
#include <cctype>

#include <Core/rapidjson/document.h>
#include <Core/rapidjson/writer.h>
#include <Core/rapidjson/stringbuffer.h>

struct MarketUtils
{
  enum MarketType 
  {
    HUOBI_SPOT=0,
    HUOBI_FUTURES=1,
    HUOBI_SWAP=2,
    HUOBI_SWAP_USDT=3,
    OKEX_SPOT=4,
    OKEX_FUTURES=5,
    OKEX_MARGIN=6,
    OKEX_SWAP=7,
    OKEX_OPTIONS=8,
    BINANCE_SPOT=9,
    BINANCE_FUTURES=10,
    BINANCE_OPTIONS=11,
    BINANCE_COIN_FUTURES=12,
    BITMEX=13,
    FTX=14,
    BYBIT=15,
    CME=16,
    BITFINEX=17,
    COINBASE=18,
    KITE_ZERODHA=19,
    SIZE=20,
    UNKNOWN=21
  };
};
namespace EFG
{
namespace Core
{
  static inline std::unordered_map<std::string,MarketUtils::MarketType> lookup = {
      {"HUOBI_SPOT",MarketUtils::MarketType::HUOBI_SPOT},
      {"HUOBI_FUTURES", MarketUtils::MarketType::HUOBI_FUTURES},
      {"HUOBI_SWAP", MarketUtils::MarketType::HUOBI_SWAP},
      {"HUOBI_SWAP_USDT", MarketUtils::MarketType::HUOBI_SWAP_USDT},
      {"OKEX_SPOT",MarketUtils::MarketType::OKEX_SPOT}, 
      {"OKEX_FUTURES",MarketUtils::MarketType::OKEX_FUTURES}, 
      {"OKEX_MARGIN",MarketUtils::MarketType::OKEX_MARGIN}, 
      {"OKEX_SWAP",MarketUtils::MarketType::OKEX_SWAP}, 
      {"OKEX_OPTIONS",MarketUtils::MarketType::OKEX_OPTIONS}, 
      {"BINANCE_SPOT",MarketUtils::MarketType::BINANCE_SPOT},
      {"BINANCE_FUTURES", MarketUtils::MarketType::BINANCE_FUTURES},
      {"BINANCE_OPTIONS", MarketUtils::MarketType::BINANCE_OPTIONS},
      {"BINANCE_COIN_FUTURES", MarketUtils::MarketType::BINANCE_COIN_FUTURES},
      {"BITMEX",MarketUtils::MarketType::BITMEX},
      {"FTX",MarketUtils::MarketType::FTX},
      {"BYBIT",MarketUtils::MarketType::BYBIT},
      {"CME",MarketUtils::MarketType::CME},
      {"BITFINEX",MarketUtils::MarketType::BITFINEX},
      {"KITE_ZERODHA",MarketUtils::MarketType::KITE_ZERODHA},
      {"COINBASE",MarketUtils::MarketType::COINBASE}
  };  
  const static inline std::string SPACE = "\t\n\v\f\r ";
 
  inline std::string& leftTrim(std::string& str) {
    str.erase(0, str.find_first_not_of(SPACE));
    return str;
  }
 
  inline std::string& rightTrim(std::string& str) {
    str.erase(str.find_last_not_of(SPACE) + 1);
    return str;
  }
 inline std::string& trim(std::string& str)
 {
    return leftTrim(rightTrim(str));
 }
namespace Market
{

 inline std::string toString(MarketUtils::MarketType _type){
  
  switch(_type){
    case MarketUtils::MarketType::HUOBI_SPOT:
      return "HUOBI_SPOT";
    case MarketUtils::MarketType::HUOBI_FUTURES:
      return "HUOBI_FUTURES";
    case MarketUtils::MarketType::HUOBI_SWAP:
      return "HUOBI_SWAP";
    case MarketUtils::MarketType::HUOBI_SWAP_USDT:
      return "HUOBI_SWAP_USDT";
    case MarketUtils::MarketType::OKEX_SPOT:
      return "OKEX_SPOT";
    case MarketUtils::MarketType::OKEX_FUTURES:
      return "OKEX_FUTURES";
    case MarketUtils::MarketType::OKEX_SWAP:
      return "OKEX_SWAP";
    case MarketUtils::MarketType::OKEX_MARGIN:
      return "OKEX_MARGIN";
    case MarketUtils::MarketType::OKEX_OPTIONS:
      return "OKEX_OPTIONS";
    case MarketUtils::MarketType::BINANCE_SPOT:
      return "BINANCE_SPOT";
    case MarketUtils::MarketType::BINANCE_FUTURES:
      return "BINANCE_FUTURES";
    case MarketUtils::MarketType::BINANCE_OPTIONS:
      return "BINANCE_OPTIONS";
    case MarketUtils::MarketType::BINANCE_COIN_FUTURES:
      return "BINANCE_COIN_FUTURES";
    case MarketUtils::MarketType::BITMEX:
      return "BITMEX";
    case MarketUtils::MarketType::FTX:
      return "FTX";
    case MarketUtils::MarketType::BYBIT:
      return "BYBIT";
    case MarketUtils::MarketType::CME:
      return "CME";
    case MarketUtils::MarketType::COINBASE:
      return "COINBASE";
    case MarketUtils::MarketType::KITE_ZERODHA:
      return "KITE_ZERODHA";
    case MarketUtils::MarketType::BITFINEX:
      return "BITFINEX";
    default:
      return "UNKNOWN";
  }  
}

 inline MarketUtils::MarketType toEnum( std::string market_name)
 {
   if(lookup.find(market_name)!=lookup.end()) return lookup[market_name];
   return MarketUtils::MarketType::UNKNOWN;
 }
 inline std::pair<std::string, std::string> splitPairString(const std::string& str)
 {
    auto first = str.find_last_of('{');
    if(first == std::string::npos) return {};

    auto mid = str.find_first_of(',', first);
    if(mid == std::string::npos) return {};

    auto last = str.find_first_of('}', mid);
    if(last == std::string::npos) return {};

    return { str.substr(first+1, mid-first-1), str.substr(mid+1, last-mid-1) };
 } 

 char *get_timestamp();

  //! escape non-url characters to %NN
  std::string url_encode(const std::string &value);


}
}
}
