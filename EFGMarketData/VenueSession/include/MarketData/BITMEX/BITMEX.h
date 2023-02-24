#pragma once
#include <MarketData/AbstractVenue/AbstractVenue.h>
#include <MarketData/CommonUtils.h>
#include <map>
using namespace EFG::MarketData;

namespace EFG
{
  namespace MarketData
  {
    namespace VenueSession
    {
      class BITMEX : public AbstractVenue
      {
      public:
        BITMEX(const ConfigManager::VenueSessionInfo &);
        ~BITMEX(){};
        void onConnection(uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req);
        void onUpdate(uWS::WebSocket<uWS::CLIENT> *ws, char *data, std::size_t len_, uWS::OpCode code);
        void onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, std::size_t length);
        void registerCallBacks();
        template <typename... EXTRA>
        void connect(std::string url, EXTRA... args)
        {
          transport->connect(url, args...);
        }
        void decode(char *buffer, size_t len, const TimeType &);
        void decodeBinary(char *buffer, size_t len);
        std::string getSign(std::string secret_key, int expires, std::string method, std::string endpoint, json postdict)
        {
          std::string data;
          unsigned char *mac = NULL;
          unsigned int mac_length = 0;
          if (!postdict.empty())
          {
            data = postdict.dump();
          }
          std::string message = method + endpoint + std::to_string(expires) + data;
          int ret = Utils::HMAC::HmacEncode("sha256", secret_key.c_str(), secret_key.length(), message.c_str(), message.length(), mac, mac_length);
          std::string sign = BASE16::base16_encode(mac, mac_length);
          return sign;
        }
      private:
        std::map<int64_t, int64_t> priceIdMap;
        bool isPartialReceived = false;
      };

    } // namespace VenueSession
  }   // namespace MarketData

} // namespace EFG
