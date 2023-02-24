#pragma once

#include <Core/Message/Message.h>

namespace EFG
{
namespace Core
{
namespace Message
{

template<size_t N>
class TradeCenter
{
public:

  void onEvent(const Trade& trade)
  {
    uint64_t ts = trade.apiIncomingTime;
    auto it = mBuffer.find(trade.mInstrumentId);
    if(it!=mBuffer.end())
    {
      auto& trades = *(it->second);
      trades.insert(trade);
      trades.mLastUpdateTime  = ts;
    }
    else
    {
      auto new_trades = std::make_unique<RingBuffer<Trade, N>>();
      new_trades->mLastUpdateTime  = ts;
      new_trades->insert(trade);
      mBuffer.insert({trade.mInstrumentId, std::move(new_trades)});
    } 
  } 
  template<typename F>
  void visit(const InstrumentId& id, size_t lastN, F&& f) const
  {
    auto it = mBuffer.find(id);
    if(it!=mBuffer.end())
    {
      (it->second)->visit(lastN, f);        
    } 
  }
  template<typename F>
  void rvisit(const InstrumentId& id, size_t lastN, F&& f) const
  {
    auto it = mBuffer.find(id);
    if(it!=mBuffer.end())
    {
      (it->second)->rvisit(lastN, f);        
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
  size_t visitWindow(const InstrumentId& id, PRED& pred) const
  {
    auto it = mBuffer.find(id);
    if(it!=mBuffer.end())
    {
      return (it->second)->visitWindow(pred);        
    }
    return 0; 
  }
  const size_t size(const InstrumentId& id) const
  {
    auto it = mBuffer.find(id);
    return it!=mBuffer.end() ? (it->second)->size() : 0;
  }
private:
  std::unordered_map<InstrumentId, std::unique_ptr<RingBuffer<Trade, N>>, InstrumentIdHash> mBuffer;
}; 


}
}
}
