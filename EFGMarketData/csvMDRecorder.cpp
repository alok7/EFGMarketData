/*
CSV md Recorder
**/


#include <MarketData/MarketData.h>
#include <Core/Message/Message.h>
#include <MarketData/ConfigReader.h>
#include <iomanip>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <MarketData/Logger.h>

#define PRICE_FACTOR 1000000000.0

class Application
{
  private:
    FILE* mDepthStream = {nullptr};
    FILE* mTradeStream = {nullptr};
    uint64_t seq = 0;
    bool init(std::string& fileName, FILE*& stream, const char* header=nullptr)
    {
      bool newFile = false;
      int fd = open(fileName.c_str(), O_APPEND | O_NONBLOCK | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP, S_IROTH);
      if(fd==-1)
      {
        newFile = true;
        fd = open(fileName.c_str(), O_CREAT | O_NONBLOCK | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP, S_IROTH);
        if(fd==-1)
        {
          std::cerr << "ERROR: CSVWriter failed to open/create file " << fileName << std::endl;
          return false;
        }
      }
      stream = fdopen(fd, "a");
      if(!stream)
      {
        std::cerr << "ERROR:failed to open stream " << fileName << std::endl;
        return false;
      }
      if(newFile)
      {
        if(header)
          fprintf(stream, "%s\n", header);
      }
      return true;
    }
  public:
    bool init(std::string& depthFile, std::string& tradeFile, const char* headerDepth=nullptr, const char* headerTrade=nullptr)
    {
      return init(depthFile, mDepthStream, headerDepth) && init(tradeFile, mTradeStream, headerTrade);
    }
    void onUpdate(const EFG::Core::Message::Ticker& ticker)
    {
   
    }
    void onUpdate(const EFG::Core::Message::DepthBook& event)
    {
      ++seq;
      fprintf(mDepthStream, "%ld,%ld,%ld,%d,%s,",event.apiIncomingTime,event.exchangeOutgoingTime,seq,static_cast<int>(event.venue),event.symbol.c_str());
      for(auto& pl : event.bids)
      {
        fprintf(mDepthStream, "%lf,%lf,",static_cast<double>(pl.price)/PRICE_FACTOR,pl.qty);
      }
      for(auto& pl : event.asks)
      {
        fprintf(mDepthStream, "%lf,%lf,",static_cast<double>(pl.price)/PRICE_FACTOR,pl.qty);
      }
      fprintf(mDepthStream, "\n");
    }
    void onUpdate(const EFG::Core::Message::Trade& event)
    {
      ++seq;
      fprintf(mTradeStream, "%ld,%ld,%ld,%d,%s,",event.apiIncomingTime,event.exchangeOutgoingTime,seq,static_cast<int>(event.venue),event.symbol.c_str());
      char side =  toupper(event.side);
      fprintf(mTradeStream, "%lf,%lf,%c",static_cast<double>(event.price)/PRICE_FACTOR,event.size,side);
      fprintf(mTradeStream, "\n");
    }
};


int main(int argc, char *argv[], char *envp[]){
 
  if(argc < 2)
  {

  }
  else{
    sigset_t blockedSignal;
    sigemptyset(&blockedSignal);
    sigaddset(&blockedSignal, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &blockedSignal, NULL);
    /*
    EFGLog::LoggerType<false> ring_logger_md(2);
    std::string logfile_suffix = "_" + std::to_string(EFG::MarketData::Utils::getHRTime() / int(1e9));
    EFGLog::initialize(ring_logger_md, MD_LOG_DIR, std::string(MD_LOG_FILE + logfile_suffix), 1024);
    */
    std::string md_config(argv[1]);
    MarketData::Init(md_config);
    auto& MD = MarketData::Instance();

    std::string mdDepthRecordFile(argv[2]);
    std::string mdTradeRecordFile(argv[3]);
    Application app;
    std::string headerDepth = "xchOutTime,apiInTime,seq,venue,symbol,pb1,qb1,pb2,qb2,pb3,qb3,pb4,qb4,pb5,qb5,pb6,qb6,pb7,qb7,pb8,qb8,pb9,qb9,pb10,qb10,pa1,qa1,pa2,qa2,pa3,qa3,pa4,qa4,pa5,qa5,pa6,qa6,pa7,qa7,pa8,qa8,pa9,qa9,pa10,qa10";
    std::string headerTrade = "xchOutTime,apiInTime,seq,venue,symbol,p,q,s";
    app.init(mdDepthRecordFile, mdTradeRecordFile, headerDepth.c_str(), headerTrade.c_str());
   

    FeedHandlers::ClientFeedHandler feed_handlers;

    auto tickerHandler = [&app](auto event) 
    { 
      app.onUpdate(event);
    };
    
    auto depthHandler = [&app](auto event)
    {
      app.onUpdate(event);
    };

    auto tradeHandler = [&app](auto event) 
    {
      app.onUpdate(event);
    };

    feed_handlers.registerTickerHandler(tickerHandler);
    feed_handlers.registerDepthHandler(depthHandler);
    feed_handlers.registerTradeHandler(tradeHandler);

    //! MD -> Strat
    feed_handlers.run();

    //! MD
    std::thread md_worker([&MD](){
      MD.run();
    });

    md_worker.join();
 
  }

  return 0;

}
