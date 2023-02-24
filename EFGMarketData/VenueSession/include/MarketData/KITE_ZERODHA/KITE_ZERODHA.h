#pragma once
#include "kitepp.hpp"
#include <MarketData/AbstractVenue/AbstractVenue.h>
#include <MarketData/CommonUtils.h>
#include <openssl/sha.h>
#include <sstream>

using namespace EFG::MarketData;
namespace kc = kiteconnect;

namespace EFG { namespace MarketData { namespace VenueSession{
     
class KITE_ZERODHA : public AbstractVenue{
public:
  KITE_ZERODHA(const ConfigManager::VenueSessionInfo&);
  ~KITE_ZERODHA(){};
  void subscribe(kc::kiteWS* ws, const std::string& mode, const std::vector<int>& instruments)
  {
    MD_LOG_INFO << "Submit subscriptions to KITE_ZERODHA" << '\n';
    ws->setMode(mode, instruments);
  }
  void onConnection(kc::kiteWS *ws);
  void onUpdate(kc::kiteWS *ws, const std::vector<kc::tick>& ticks);
  void onError(kc::kiteWS *ws, int code, const std::string& message);
  void onConnectionError(kc::kiteWS *ws);
  void onDisconnection(kc::kiteWS *ws, int code, const std::string& message);
  void registerCallBacks();
  template<typename ...EXTRA>
  void connect(std::string url, EXTRA... args)
  {
    mKite->connect(); 
  }
  void decode(char* buffer, size_t len, const TimeType&);
  void decodeBinary(char* buffer, size_t len);
private:
  std::unique_ptr<kc::kiteWS> mKite;
  std::string sha256(std::string str)
  {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
      ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
  }
  void setHeader()
  {
    httpRequest->clearHeader();
    httpRequest->appendHeader("X-Kite-Version", "3");
    httpRequest->buildHeader();
    
  } 
  template<typename PAYLOAD, typename KEY, typename VALUE>
  void add(PAYLOAD& payload, const KEY& key, const VALUE& value)
  {   
    if(!value.empty()) payload.insert({key, value});
  } 
}; 

}}}

