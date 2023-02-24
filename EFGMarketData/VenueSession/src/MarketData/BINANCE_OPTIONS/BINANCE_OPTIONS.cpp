#include <MarketData/BINANCE_OPTIONS/BINANCE_OPTIONS.h>

namespace EFG
{
namespace MarketData
{
namespace VenueSession
{
  BINANCE_OPTIONS::BINANCE_OPTIONS(const ConfigManager::VenueSessionInfo &vsinfo) : AbstractVenue(vsinfo)
  {
    transport = std::make_unique<Transport::WssTransport>(&md_hub);
    registerCallBacks();
    MD_LOG_INFO << "Created BINANCE OPTIONS Session " << '\n'; 
  }
  void BINANCE_OPTIONS::onConnection(uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req)
  {
    MD_LOG_INFO << "BINANCE OPTIONS Connected " << '\n';
    subscriptionHandler->init(ws);
    subscriptionHandler->run();
    MD_LOG_INFO << "Started Susbcription Handler for BINANCE OPTIONS" << '\n';
  }
  void BINANCE_OPTIONS::onUpdate(uWS::WebSocket<uWS::CLIENT> *ws, char *message, std::size_t len_, uWS::OpCode code)
  {
    try
    {
      TimeType packet_hit_time = Utils::getHRTime();
      //MD_LOG_INFO << " Decoding incoming data " << std::string(message, len_) << '\n';
      decode(message, len_, packet_hit_time);
    }
    catch (const std::exception &e)
    {
      std::cerr << "Exception on Decode " << e.what() << '\n';
    }
    catch (...)
    {
      std::cerr << "Unknown Exception on Decode " << '\n';
    }
  }
  void BINANCE_OPTIONS::onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, std::size_t length)
  {
    try
    {
      MD_LOG_INFO << "BINANCE OPTIONS Disconnected" << std::to_string(Utils::getHRTime()) << '\n';
      subscriptionHandler->stop();
      subscriptionHandler = std::make_unique<VenueSession::SubscriptionHandler>();
      this->submitRequests(this->venueSessionInfo.getSubscriptions());
      this->connect(this->venueSessionInfo.getWssEndpoint());
    }
    catch (const std::exception &e)
    {
      std::cerr << "Exception on Disconnect Event " << e.what() << '\n';
    }
    catch (...)
    {
      std::cerr << "Unknown Exception on Disconnet " << '\n';
    }
  }

  void BINANCE_OPTIONS::registerCallBacks()
  {
    using namespace std::placeholders;
    transport->registerOnConnectCallBack(std::bind(&BINANCE_OPTIONS::onConnection, this, _1, _2));
    transport->registerOnUpdateCallBack(std::bind(&BINANCE_OPTIONS::onUpdate, this, _1, _2, _3, _4));
    transport->registerOnDisconnectCallback(std::bind(&BINANCE_OPTIONS::onDisconnection, this, _1, _2, _3, _4));
  }
  void BINANCE_OPTIONS::decode(char *buffer, size_t len, const TimeType &packet_hit_time)
  {

  }
  void BINANCE_OPTIONS::decodeBinary(char *buffer, size_t len)
  {

  }


} 
}
} 
