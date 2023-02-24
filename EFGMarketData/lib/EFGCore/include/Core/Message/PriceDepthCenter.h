#pragma once

#include <Core/Message/Message.h>

namespace EFG
{
namespace Core
{
namespace Message
{

inline constexpr size_t NextPowerOftwo(size_t n)
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
class PriceDepthCenter
{
public:
  using SamplingTime = SamplingTimePeriod;
  static const constexpr size_t mSamples = NextPowerOftwo(TimeHorizon::Units/SamplingTime::Units);
  void onEvent(const DepthBook& depth)
  {
    uint64_t ts = depth.apiIncomingTime;
    auto it = mBuffer.find(depth.mInstrumentId);
    if(it!=mBuffer.end())
    {
      auto& depths = *(it->second);
      auto& last_depth = depths.peek();
      if(((ts - depths.mLastUpdateTime) >= SamplingTime::Units) || !Sampled)
      {
        depths.insert(depth);
        depths.mLastUpdateTime  = ts;
      }
    }
    else
    {
      auto new_depths = std::make_unique<RingBuffer<DepthBook, mSamples>>();
      new_depths->mLastUpdateTime  = ts;
      new_depths->insert(depth);
      mBuffer.insert({depth.mInstrumentId, std::move(new_depths)});
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
  template<typename F, typename PRED>
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
  DepthBook& peek(const InstrumentId& id) const
  {
    auto it = mBuffer.find(id);
    return (it->second)->peek();
  }	  
  DepthBook& peek(const InstrumentId& id, size_t window) const
  {
    auto it = mBuffer.find(id);
    return (it->second)->peek(window);
  }
  	  
private:
  std::unordered_map<InstrumentId, std::unique_ptr<RingBuffer<DepthBook, mSamples>>, InstrumentIdHash> mBuffer;
}; 


}
}
}
