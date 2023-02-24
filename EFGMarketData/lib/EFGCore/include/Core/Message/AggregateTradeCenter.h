#pragma once

#include <Core/Message/Message.h>

namespace EFG
{
namespace Core
{
namespace Message
{

inline constexpr size_t _getNextPowerOftwo(size_t n)
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
class AggregateTradeCenter
{
public:
  using SamplingTime = SamplingTimePeriod;
  static const constexpr size_t mSamples = _getNextPowerOftwo(TimeHorizon::Units/SamplingTime::Units);
  using AggregateTradeType = RingBuffer<Core::Message::AggregateTrade, mSamples>; 
  void onEvent(const Trade& trade, double bidPrice, double askPrice)
  {
    Core::Message::Ticker ticker;
    ticker.bid_price = ticker.bid_price;
    ticker.ask_price = ticker.ask_price;
    onEvent(trade, ticker);
  }
  void onEvent(const AggregateTrade& aggTrade)
  {
    uint64_t ts = aggTrade.apiIncomingTime;
    auto it = mBuffer.find(aggTrade.mInstrumentId);
    if(it!=mBuffer.end())
    {
      auto& aggTrades = *(it->second);
      auto& last_aggTrade = aggTrades.peek();
      if(((ts - aggTrades.mLastUpdateTime) >= SamplingTime::Units) || !Sampled)
      {
        const_cast<AggregateTrade&>(aggTrade).duration_weigted_avg_price = aggTrade.last_price; // start of new window duration
        aggTrades.mLastUpdateTime  = ts;
        const_cast<AggregateTrade&>(aggTrade).high_price   = aggTrade.last_price;
        const_cast<AggregateTrade&>(aggTrade).low_price    = aggTrade.last_price;
        const_cast<AggregateTrade&>(aggTrade).open_price   = aggTrade.last_price;
        const_cast<AggregateTrade&>(aggTrade).close_price  = aggTrade.last_price;
        const_cast<AggregateTrade&>(aggTrade).avg_price    = aggTrade.last_price;
        mDuration = 0;
        aggTrades.insert(aggTrade);
      }
      else
      {
        last_aggTrade.high_price  = std::max(aggTrade.last_price, last_aggTrade.high_price);
        last_aggTrade.low_price   = std::min(aggTrade.last_price, last_aggTrade.low_price);
        last_aggTrade.close_price = aggTrade.last_price;
        last_aggTrade.bid_price   = aggTrade.bid_price;
        last_aggTrade.ask_price   = aggTrade.ask_price;
        last_aggTrade.avg_price   = (aggTrade.last_price + last_aggTrade.avg_price)/2.0;
        last_aggTrade.duration_weigted_avg_price = (last_aggTrade.duration_weigted_avg_price*mDuration + aggTrade.last_price*(ts-mLastTradeUpdateTime))/(mDuration+(ts-mLastTradeUpdateTime));
	mDuration +=(ts-mLastTradeUpdateTime);
        last_aggTrade.bid_size += aggTrade.bid_size;
        last_aggTrade.ask_size += aggTrade.ask_size;
        last_aggTrade.last_size = aggTrade.last_size;
      }
    }
    else
    {
      auto new_aggTrades = std::make_unique<RingBuffer<AggregateTrade, mSamples>>();
      const_cast<AggregateTrade&>(aggTrade).high_price   = aggTrade.last_price;
      const_cast<AggregateTrade&>(aggTrade).low_price    = aggTrade.last_price;
      const_cast<AggregateTrade&>(aggTrade).open_price   = aggTrade.last_price;
      const_cast<AggregateTrade&>(aggTrade).close_price  = aggTrade.last_price;
      const_cast<AggregateTrade&>(aggTrade).avg_price    = aggTrade.last_price;
      const_cast<AggregateTrade&>(aggTrade).duration_weigted_avg_price = aggTrade.last_price; // start of first window duration 
      new_aggTrades->mLastUpdateTime  = ts;
      mDuration = 0;
      new_aggTrades->insert(aggTrade);
      mBuffer.insert({aggTrade.mInstrumentId, std::move(new_aggTrades)});
    } 
    mLastTradeUpdateTime = ts;
  }
  void onEvent(const Trade& trade, const Ticker& ticker)
  {
    uint64_t ts = trade.apiIncomingTime;
    auto it = mBuffer.find(trade.mInstrumentId);
    if(it!=mBuffer.end())
    {
      auto& aggTrades = *(it->second);
      auto& last_aggTrade = aggTrades.peek();
      if(((ts - aggTrades.mLastUpdateTime) >= SamplingTime::Units) || !Sampled)
      {
        Core::Message::AggregateTrade aggTrade;
        aggTrade.mInstrumentId = trade.mInstrumentId;
        aggTrade.high_price = trade.price;
        aggTrade.low_price  = trade.price;
        aggTrade.bid_price = ticker.bid_price;
        aggTrade.ask_price = ticker.ask_price;
        last_aggTrade.avg_price   = trade.price;
        aggTrade.duration_weigted_avg_price = trade.price; // start of new window duration
        aggTrade.apiIncomingTime = ts;
        aggTrades.mLastUpdateTime  = ts;
        mDuration = 0;
        switch(trade.side)
        {
          case 'B':
          case 'b':
          { 
            aggTrade.bid_size = trade.size;
            aggTrade.ask_size = 0;
            break;
          }
          case 'S':
          case 's':
          {
            aggTrade.ask_size = trade.size;
            aggTrade.bid_size = 0;
            break;
          }
        }
        aggTrades.insert(aggTrade);
      }
      else
      {
        last_aggTrade.high_price = std::max(trade.price, last_aggTrade.high_price);
        last_aggTrade.low_price  = std::min(trade.price, last_aggTrade.low_price);
        last_aggTrade.bid_price = ticker.bid_price;
        last_aggTrade.ask_price = ticker.ask_price;
        last_aggTrade.avg_price   = (trade.price + last_aggTrade.avg_price)/2.0;
        last_aggTrade.duration_weigted_avg_price = (last_aggTrade.duration_weigted_avg_price*mDuration + trade.price*(ts-mLastTradeUpdateTime))/(mDuration+(ts-mLastTradeUpdateTime));
	mDuration +=(ts-mLastTradeUpdateTime);
        switch(trade.side)
        {
          case 'B':
          case 'b':
          {
            last_aggTrade.bid_size += trade.size;
            break;
          }
          case 'S':
          case 's':
          {
            last_aggTrade.ask_size += trade.size;
            break;
          }
        }
      }
    }
    else
    {
      auto new_aggTrades = std::make_unique<RingBuffer<AggregateTrade, mSamples>>();
      Core::Message::AggregateTrade aggTrade;
      aggTrade.mInstrumentId = trade.mInstrumentId;
      aggTrade.high_price = trade.price;
      aggTrade.low_price  = trade.price;
      aggTrade.bid_price = ticker.bid_price;
      aggTrade.ask_price = ticker.ask_price;
      aggTrade.avg_price   = trade.price;
      aggTrade.duration_weigted_avg_price = trade.price; // start of first window duration 
      aggTrade.apiIncomingTime = ts;
      new_aggTrades->mLastUpdateTime  = ts;
      mDuration = 0;
      switch(trade.side)
      {
        case 'B':
        case 'b':
        {
          aggTrade.bid_size = trade.size;
          aggTrade.ask_size = 0;
          break;
        }
        case 'S':
        case 's':
        {
          aggTrade.ask_size = trade.size;
          aggTrade.bid_size = 0;
          break;
        }
      }
      new_aggTrades->insert(aggTrade);
      mBuffer.insert({trade.mInstrumentId, std::move(new_aggTrades)});
    } 
    mLastTradeUpdateTime = ts;
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
  AggregateTrade& peek(const InstrumentId& id) const
  {
    auto it = mBuffer.find(id);
    return (it->second)->peek();
  }	  
  AggregateTrade& peek(const InstrumentId& id, size_t window) const
  {
    auto it = mBuffer.find(id);
    return (it->second)->peek(window);
  }	  
private:
  std::unordered_map<InstrumentId, std::unique_ptr<RingBuffer<AggregateTrade, mSamples>>, InstrumentIdHash> mBuffer;
  uint64_t mLastTradeUpdateTime = 0;
  uint64_t mDuration = 0;
}; 


}
}
}
