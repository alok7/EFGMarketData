#pragma once

#include <Core/Message/Message.h>

namespace EFG
{
namespace Core
{
namespace Message
{

inline constexpr size_t getNextPowerOftwo(size_t n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

template<typename SamplingTimePeriod, typename TimeHorizon = TimeUnits::Hours<12>, bool Sampled=false>
class TickerCenter
{
public:
  using SamplingTime = SamplingTimePeriod;
  static const constexpr size_t mSamples = getNextPowerOftwo(TimeHorizon::Units/SamplingTime::Units);
  using TickersType = RingBuffer<Core::Message::Ticker, mSamples>; 
  void onEvent(const Trade& trade, const DepthBook& lastDepth)
  {
    uint64_t ts = trade.apiIncomingTime;
    auto it = mBuffer.find(trade.mInstrumentId);
    if(it!=mBuffer.end())
    {
      auto& tickers = *(it->second);
      auto& last_ticker = tickers.peek();
      if(((ts - tickers.mLastUpdateTime) >= SamplingTime::Units) || !Sampled)
      {
        Core::Message::Ticker ticker;
        ticker.mInstrumentId = trade.mInstrumentId;
        ticker.last_price = trade.price;
        ticker.open_price = trade.price;
        ticker.close_price = trade.price;
        ticker.high_price = trade.price;
        ticker.low_price  = trade.price;
        ticker.last_size  = trade.size;
        ticker.bid_price = lastDepth.bids[0].price;
        ticker.ask_price = lastDepth.asks[0].price;
        ticker.apiIncomingTime = ts;
        tickers.mLastUpdateTime  = ts;
        switch(trade.side)
        {
          case 'B':
          case 'b':
          { 
            ticker.bid_size = trade.size;
            ticker.ask_size = 0;
            break;
          }
          case 'S':
          case 's':
          {
            ticker.ask_size = trade.size;
            ticker.bid_size = 0;
            break;
          }
        }
        tickers.insert(ticker);
      }
      else
      {
        last_ticker.last_price = trade.price;
        last_ticker.high_price = std::max(trade.price, last_ticker.high_price);
        last_ticker.low_price  = std::min(trade.price, last_ticker.low_price);
        last_ticker.last_size  = trade.size;
        last_ticker.close_price = trade.price;
        switch(trade.side)
        {
          case 'B':
          case 'b':
          {
            last_ticker.bid_size += trade.size;
            break;
          }
          case 'S':
          case 's':
          {
            last_ticker.ask_size += trade.size;
            break;
          }
        }
      }
    }
    else
    {
      auto new_tickers = std::make_unique<RingBuffer<Ticker, mSamples>>();
      Core::Message::Ticker ticker;
      ticker.mInstrumentId = trade.mInstrumentId;
      ticker.last_price = trade.price;
      ticker.open_price = trade.price;
      ticker.close_price = trade.price;
      ticker.high_price = trade.price;
      ticker.bid_price = lastDepth.bids[0].price;
      ticker.ask_price = lastDepth.asks[0].price;
      ticker.low_price  = trade.price;
      ticker.last_size  = trade.size;
      ticker.apiIncomingTime = ts;
      new_tickers->mLastUpdateTime  = ts;
      switch(trade.side)
      {
        case 'B':
        case 'b':
        {
          ticker.bid_size = trade.size;
          ticker.ask_size = 0;
          break;
        }
        case 'S':
        case 's':
        {
          ticker.ask_size = trade.size;
          ticker.bid_size = 0;
          break;
        }
      }
      new_tickers->insert(ticker);
      mBuffer.insert({trade.mInstrumentId, std::move(new_tickers)});
    } 
  } 
  void onEvent(const AggregateTrade& trade)
  {
    uint64_t ts = trade.apiIncomingTime;
    auto it = mBuffer.find(trade.mInstrumentId);
    if(it!=mBuffer.end())
    {
      auto& tickers = *(it->second);
      auto& last_ticker = tickers.peek();
      if(((ts - tickers.mLastUpdateTime) >= SamplingTime::Units) || !Sampled)
      {
        Core::Message::Ticker ticker;
        ticker.mInstrumentId = trade.mInstrumentId;
        ticker.last_price = trade.last_price;
        ticker.open_price  = trade.last_price;
        ticker.close_price = trade.last_price;
        ticker.high_price = trade.last_price;
        ticker.low_price  = trade.last_price;
        ticker.last_size  = trade.last_size;
        ticker.bid_price  = trade.bid_price;
        ticker.ask_price  = trade.ask_price;
        ticker.bid_size   = trade.bid_size;
        ticker.ask_size   = trade.ask_size;
        ticker.apiIncomingTime = ts;
        tickers.mLastUpdateTime  = ts;
        tickers.insert(ticker);
      }
      else
      {
        last_ticker.last_price  = trade.last_price;
        last_ticker.close_price = trade.last_price;
        last_ticker.high_price = std::max(trade.last_price, last_ticker.high_price);
        last_ticker.low_price  = std::min(trade.last_price, last_ticker.low_price);
        last_ticker.bid_price  = trade.bid_price;
        last_ticker.ask_price  = trade.ask_price;
        last_ticker.last_size  = trade.last_size;
        last_ticker.bid_size   += trade.bid_size;
        last_ticker.ask_size   += trade.ask_size;
      }
    }
    else
    {
      auto new_tickers = std::make_unique<RingBuffer<Ticker, mSamples>>();
      Core::Message::Ticker ticker;
      ticker.mInstrumentId = trade.mInstrumentId;
      ticker.last_price = trade.last_price;
      ticker.open_price = trade.last_price;
      ticker.close_price = trade.last_price;
      ticker.high_price = trade.last_price;
      ticker.bid_price  = trade.bid_price;
      ticker.ask_price  = trade.ask_price;
      ticker.low_price  = trade.last_price;
      ticker.last_size  = trade.last_size;
      ticker.bid_size   = trade.bid_size;
      ticker.ask_size   = trade.ask_size;
      ticker.apiIncomingTime = ts;
      new_tickers->mLastUpdateTime  = ts;
      new_tickers->insert(ticker);
      mBuffer.insert({trade.mInstrumentId, std::move(new_tickers)});
    } 
  } 
  void onEvent(const Ticker& ticker)
  {
    uint64_t ts = ticker.apiIncomingTime;
    auto it = mBuffer.find(ticker.mInstrumentId);
    if(it!=mBuffer.end())
    {
      auto& tickers = *(it->second);
      tickers.insert(ticker);
    }
    else
    {
      auto new_tickers = std::make_unique<RingBuffer<Ticker, mSamples>>();
      new_tickers->insert(ticker);
      mBuffer.insert({ticker.mInstrumentId, std::move(new_tickers)});
    } 
  } 
  template<typename F>
  void visit(const InstrumentId& id, size_t N, F&& f) const
  {
    auto it = mBuffer.find(id);
    if(it!=mBuffer.end())
    {
      (it->second)->visit(N, f);        
    } 
  }
  template<typename F>
  void rvisit(const InstrumentId& id, size_t N, F&& f) const
  {
    auto it = mBuffer.find(id);
    if(it!=mBuffer.end())
    {
      (it->second)->rvisit(N, f);        
    } 
  }
  template<typename F, typename PRED>
  void rvisit(const InstrumentId& id, PRED& pred, F&& f) const
  {
    auto it = mBuffer.find(id);
    if(it!=mBuffer.end())
    {
      (it->second)->rvisit(pred, f);        
    } 
  }
  template<typename PRED>
  size_t visitWindowSize(const InstrumentId& id, PRED& pred) const
  {
    auto it = mBuffer.find(id);
    if(it!=mBuffer.end())
    {
      return (it->second)->visitWindowSize(pred);        
    }
    return 0; 
  }
  const size_t size(const InstrumentId& id) const
  {
    auto it = mBuffer.find(id);
    return it!=mBuffer.end() ? (it->second)->size() : 0;
  }
  Ticker& peek(const InstrumentId& id, size_t window) const
  {
    auto it = mBuffer.find(id);
    return (it->second)->peek(window);
  }	  
  Ticker& peek(const InstrumentId& id) const
  {
    auto it = mBuffer.find(id);
    return (it->second)->peek();
  }	  
private:
  std::unordered_map<InstrumentId, std::unique_ptr<RingBuffer<Ticker, mSamples>>, InstrumentIdHash> mBuffer;
}; 


}
}
}
