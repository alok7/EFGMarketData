#include <MarketData/AbstractVenue/AbstractVenue.h>

namespace EFG
{
namespace MarketData
{

uWS::Hub md_hub;

namespace VenueSession
{


  AbstractVenue::AbstractVenue(const ConfigManager::VenueSessionInfo& vsinfo): 
    venueSessionInfo(vsinfo), snapshot_received(false), first_update_received(false) {
    transport = std::make_unique<EFG::Core::Transport::WssTransport>(&md_hub);
    subscriptionHandler = std::make_unique<VenueSession::SubscriptionHandler>();
  }
  
  void AbstractVenue::submitRequest(std::string& request) {
    subscriptionHandler->submit(request);
  }
  
  void AbstractVenue::submitRequests(const std::vector<std::string>& requests) {
    for(std::string request : requests) 
      subscriptionHandler->submit(request);
  }
  
  bool AbstractVenue::isConnected() {
    return _isConnected.load();
  }

  void AbstractVenue::start() {
    timer->join();
    subscriptionHandler->join(); 
  }

}
}
}
