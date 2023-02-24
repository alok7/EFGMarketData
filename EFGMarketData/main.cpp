#include <MarketData/MarketData.h>
#include <Core/Message/Message.h>
#include <MarketData/ConfigReader.h>
#include <iomanip>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <MarketData/Logger.h>
#include <Core/Transport/SHM/Publisher.h>
#include <Core/Transport/SHM/Subscriber.h>

#define PRICE_FACTOR 1000000000.0

using PUBLISHER =  EFG::Core::Publisher<EFG::Core::Message::DepthBook, EFG::Core::Message::Trade, 
        EFG::Core::Message::Ticker, EFG::Core::Message::AggregateTrade>;

int main(int argc, char *argv[], char *envp[]){
 
  if(argc < 2)
  {

  }
  else
  {
    sigset_t blockedSignal;
    sigemptyset(&blockedSignal);
    sigaddset(&blockedSignal, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &blockedSignal, NULL);

    MDLog::LoggerType<false> ring_logger_md(2);
    std::string logfile_suffix = "_" + std::to_string(EFG::MarketData::Utils::getHRTime() / int(1e9));
    MDLog::initialize(ring_logger_md, MD_LOG_DIR, std::string(MD_LOG_FILE + logfile_suffix), 1024);

    std::string md_config(argv[1]);
    MarketData::Init(md_config);
    auto& MD = MarketData::Instance();

    FeedHandlers::ClientFeedHandler feed_handlers;
    
    PUBLISHER publisher("md_feed", 4194304);

    auto tickerHandler = [&publisher](auto event) 
    { 
      publisher.publish(event);
    };
    
    auto depthHandler = [&publisher](auto event)
    {
      /*	    
      EFG::Core::Message::BBO bbo;
      bbo.mBidPrice = event.bids[0].price;
      bbo.mAskPrice = event.asks[0].price;
      bbo.mBidQty = event.bids[0].qty;
      bbo.mAskQty = event.asks[0].qty;
      strcpy(bbo.symbol, event.symbol);
      std::cout << "bbo update: " << " bidPrice " << bbo.mBidPrice << " askPrice " << bbo.mAskPrice 
	        << " bidQty " << bbo.mBidQty << " askQty " << bbo.mAskQty << " symbol " << std::string(bbo.symbol) << std::endl; 
      //publisher.publish(bbo);
      */
      publisher.publish(event);
    };

    auto tradeHandler = [&publisher](auto event) 
    {
      //std::cout << "trade update: " << " price " << event.price << " qty " << event.size << " symbol " << std::string(event.symbol) << std::endl; 
      //MD_LOG_INFO << "MD TradePrice after queue: " << event.price << '\n'; 
      publisher.publish(event);
    };

    auto aggTradeHandler = [&publisher](auto event) 
    {
      publisher.publish(event);
    };

    feed_handlers.registerTickerHandler(tickerHandler);
    feed_handlers.registerDepthHandler(depthHandler);
    feed_handlers.registerTradeHandler(tradeHandler);
    feed_handlers.registerAggTradeHandler(aggTradeHandler);

    feed_handlers.run();
    MD.run();
 
  }

  return 0;

}
