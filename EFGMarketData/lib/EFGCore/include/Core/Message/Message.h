#pragma once 

#include <string>
#include <ctime>
#include <vector>
#include <array>
#include <atomic>
#include <mutex>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <Core/Message/Instrument.h>
#include <Core/MarketUtils/MarketUtils.h>
#include <Core/Data/String/FixedString.h>

using namespace EFG::Core::Data;


using priceType = double;
using qtyType   = double;
using TimeType  = int64_t;

namespace EFG
{
namespace Core
{
namespace Message
{

class MessageHeader
{
  public:
    // time in nanoseconds since epoch
    TimeType apiIncomingTime;
    TimeType apiOutgoingTime; // api internal latency = apiOutgoingTime - apiIncomingTime

};

class MarketEvent: public MessageHeader
{
  public:
    MarketUtils::MarketType venue;
    InstrumentId mInstrumentId;
    char symbol[32]; 
};
class BBO : public MarketEvent
{
  public:
    priceType mBidPrice;
    priceType mAskPrice;
    qtyType   mBidQty;
    qtyType   mAskQty;

};

class Ticker: public MarketEvent  
{
  public:
    priceType last_price; // last traded price
    priceType bid_price; // best buy price
    priceType ask_price; // best sell price
    priceType open_price;
    priceType high_price;
    priceType low_price;
    priceType close_price;
    qtyType   bid_size;
    qtyType   ask_size;
    qtyType   last_size;
    Ticker(priceType lp, priceType bp, priceType ap, priceType op, priceType hp, priceType lp_, qtyType bs, qtyType as, qtyType ls):
              last_price(lp), bid_price(bp), ask_price(ap), open_price(op), high_price(hp), low_price(lp_), bid_size(bs), ask_size(as), last_size(ls) {}
    Ticker() {}
};

class DepthBook : public MarketEvent
{
  public:
    const static size_t MAX_PRICE_LEVELS = 5;
    struct price_level
    {
      priceType price;
      qtyType qty;
      priceType getPrice() {return price;}
      qtyType getQuantity() {return qty;}

      void setPrice(priceType _price) { price = _price;}
      void setQuantity(qtyType _qty) {qty = _qty;}

      price_level(priceType price_, qtyType qty_, int64_t orders_count_=1): price(price_), qty(qty_) {}
      price_level() {}
    };
    double midPrice()
    {
      return (bids[0].price + asks[0].price)/2.0;
    }
    DepthBook() {}
    std::array<price_level, MAX_PRICE_LEVELS> bids;
    std::array<price_level, MAX_PRICE_LEVELS> asks;
};
class Trade : public MarketEvent
{
  public:
    priceType price;
    qtyType size;
    char side; // 'b', 's', 'n'
    Trade(priceType pr, qtyType sz, char s): price(pr), size(sz), side(s) {}
    Trade() {}
};
class AggregateTrade : public MarketEvent
{
  public:
    priceType last_price;
    priceType bid_price;
    priceType ask_price;
    priceType high_price;
    priceType low_price;
    priceType open_price;
    priceType close_price;
    priceType duration_weigted_avg_price;
    priceType avg_price;
    qtyType last_size;
    qtyType bid_size;
    qtyType ask_size;
};
class Liquidation : public MarketEvent
{
  public:
    priceType price;
    qtyType leaves_size;
    char side = 'N'; // 'B', 'S', 'N'
    std::string order_id;
    Liquidation(priceType pr, qtyType sz, char s, std::string ord_id): price(pr), leaves_size(sz), side(s), order_id(ord_id) {}
    Liquidation() {}
};

class Position
{
  public:
    MarketUtils::MarketType destination;
    std::string account;
    std::string strategy;
    std::string symbol;
    qtyType net_position; // position.short_quantity + position.long_quantity
    qtyType short_quantity;
    priceType short_avg_price;
    qtyType long_quantity;
    priceType long_avg_price;
    priceType liquid_price;
    int64_t timestamp;
};

enum OrderSide
{
  BUY  = 0,
  SELL = 1,
  SIDE_NONE = 2
}; 
enum OrderTimeInForce
{
    FOK  = 0, // Fill or Kill
    FAK  = 1, // Fill And Kill
    IOC  = 2, // Immediate or Cancel
    AON  = 3, // All or None
    GFD  = 4, // Good for Day
    GTC  = 5,  // Good till Cancel
    POST = 6,
    TIF_NONE = 7
};

enum OrderType
{
    MARKET      = 0,
    LIMIT       = 1,
    STOP        = 2,
    STOP_LIMIT  = 3,
    PEG         = 4,
    MKT_TOUCHED = 5,
    LMT_TOUCHED = 6,
    POST_ONLY   = 7,
    TAKE_PROFIT = 8,
    TAKE_PROFIT_LIMIT = 9,
    LIMIT_MAKER = 10,
    TYPE_NONE   = 11
 };

enum OrderStatus
{
    NEW                  = 0, // OPEN
    REPLACE              = 1,
    CANCEL               = 2,
    CANCELLED            = 3,
    PARTIALLY_FILLED     = 4,
    FULLY_FILLED         = 5,
    PENDING_NEW          = 6,
    PENDING_REPLACE      = 7,
    PENDING_CANCEL       = 8,
    INTERNAL_RATE_LIMIT_REJECT_NEW       = 9,
    INTERNAL_RATE_LIMIT_REJECT_REPLACE   = 10,
    INTERNAL_RATE_LIMIT_REJECT_CANCEL    = 11,
    EXCHANGE_RATE_LIMIT_REJECT_NEW       = 12,
    EXCHANGE_RATE_LIMIT_REJECT_REPLACE   = 13,
    EXCHANGE_RATE_LIMIT_REJECT_CANCEL    = 14,
    // Rejected
    FAILED_NEW                  = 15,
    FAILED_REPLACE              = 16,       // Invalid amend: orderQty, leavesQty, price, stopPx unchanged
    FAILED_CANCELLED            = 17,
    REJECT_ERROR                = 18, // general reject error 
    EXPIRED                     = 19,
    EXCHANGE_RATE_LIMIT_REJECT  = 20,
    STATUS_NONE                 = 21
};

class Order
{
  public:
    Order& operator=(const Order& _order)
    {
      if(&_order==this)
       return *this;
      
      destination = _order.destination;
      instrument_id = _order.instrument_id;
      client_oid = _order.client_oid;
      order_id = _order.order_id;
      orig_client_oid = _order.orig_client_oid;
      client_id = _order.client_id;
      order_status = _order.order_status;
      reject_reason = _order.reject_reason;
      side = _order.side;
      order_qty = _order.order_qty;
      display_qty = _order.display_qty;
      price = _order.price;
      order_type = _order.order_type;
      tif = _order.tif;
      execInst = _order.execInst;
      leaves_qty = _order.leaves_qty;
      request_error = _order.request_error;

      return *this;
    }
    MarketUtils::MarketType destination;
    std::string instrument_id;
    std::string client_oid;
    std::string order_id;
    std::string orig_client_oid;
    std::string client_id;
    OrderStatus order_status = OrderStatus::STATUS_NONE;
    std::string reject_reason;
    OrderSide side = OrderSide::SIDE_NONE;
    qtyType  order_qty;
    qtyType  display_qty;
    priceType price;
    OrderType order_type = OrderType::TYPE_NONE;
    OrderTimeInForce tif = OrderTimeInForce::TIF_NONE;
    std::string execInst;
    qtyType leaves_qty; // amount of shares open for further execution 
    // Rejection of Request
    std::string request_error;
};


namespace Request
{
    class BaseRequest
    {
      public:
        size_t getOrdinal()
        {
          return ordinal;
        }
        void setOrdinal(size_t _ordinal)
        {
          ordinal = _ordinal;
        }
      private:
        size_t ordinal; 
    }; 

}
namespace Response 
{

    class BaseResponse
    {
      public:
        size_t getOrdinal()
        {
          return ordinal;
        }
        void setOrdinal(size_t _ordinal)
        {
          ordinal = _ordinal;
        }
      private:
        size_t ordinal; 
    }; 
    class ExecutionReport : public BaseResponse
    {
      public:
        MarketUtils::MarketType destination;
        FixedString<32> instrument_id;
        FixedString<32> client_oid;
        FixedString<32> order_id;
        FixedString<32> orig_client_oid;
        FixedString<32> client_id;
        OrderStatus order_status = OrderStatus::STATUS_NONE;
        FixedString<32> reject_reason;
        OrderSide side = OrderSide::SIDE_NONE;
        double order_qty;
        double display_qty;
        double price;
        OrderType order_type = OrderType::TYPE_NONE;
        OrderTimeInForce tif = OrderTimeInForce::TIF_NONE;
        FixedString<8> execInst;
        double leaves_qty; // amount of shares open for further execution 
        // Rejection of Request
        FixedString<32> request_error;
    };

    class Position : public BaseResponse
    {
      public:
        MarketUtils::MarketType destination;
        FixedString<16> symbol;
        double net_position;
        double  short_quantity;
        double short_avg_price;
        double long_quantity;
        double long_avg_price;
        double liquid_price;
        int64_t timestamp;
    };

    class Asset : public BaseResponse
    {
      public:
        MarketUtils::MarketType destination;
        FixedString<8> currency;
        double balance;
        double hold;
        double available;
        int64_t timestamp;
    };

}


namespace TimeUnits
{
  template<int N>
  struct Nanos
  {
    static constexpr size_t Units = 1*N;
  };

  template<int N>
  struct Micros
  {
    static constexpr size_t Units = 1000*Nanos<1>::Units*N;
  };

  template<int N>
  struct Millis
  {
    static constexpr size_t Units = 1000*Micros<1>::Units*N;
  };

  template<int N>
  struct Secs
  {
    static constexpr size_t Units = 1000*Millis<1>::Units*N;
  };

  template<size_t N>
  struct Minutes
  {
    static constexpr size_t Units = 60*Secs<N>::Units;
  };
  template<size_t N>
  using Mins = Minutes<N>;

  template<size_t N>
  struct Hours
  {
    static constexpr size_t Units = 60*Minutes<N>::Units;
  };
}
/*
template<typename T, typename FirstCallBack, typename... RestCallBacks>
inline void visit(const T& t, FirstCallBack&& f,  RestCallBacks&&... restCallBacks)
{
  f(t); 
  visit(restCallBacks...); 
}

template<typename T, typename FirstCallBack>
inline void visit(const T& t, FirstCallBack&& f)
{  
  f(t);  
}
*/
template<typename T, size_t capacity>
class RingBuffer
{
  public:
    RingBuffer()
    {
      queueImpl.resize(capacity);	    
      static_assert((capacity >= 1) & !(capacity & (capacity-1)), "Buffer size must be power of two");
    }
    void insert(const T& event)
    {
      int64_t current_write_pos = writer_pos;
      int64_t next_write_pos = increaseIndex(current_write_pos);
      if(current_write_pos + 1 == capacity)
        overwriting = true;

      queueImpl[current_write_pos] = event;
      writer_pos = next_write_pos;
    }
    template <typename F>
    void visit(int lastN, F&& f) const // visit from oldest to latest 
    {
      int firstSeq = firstSequence(lastN);
      int lastSeq = lastSequence();
      if(lastSeq >= firstSeq)
      {
        for(int i = firstSeq; i <= lastSeq; ++i )
        {
          f(queueImpl[i]);
        }
      }
      else 
      {
        for(int i = firstSeq; i < capacity; ++i)
        {
          f(queueImpl[i]);
        }
        for(int i =0; i <=lastSeq; ++i)
        {
          f(queueImpl[i]);
        }
      }
    }
    template<typename F>
    void rvisit(int lastN, F&& f) const // visit from latest to oldest 
    {
      int firstSeq = firstSequence(lastN);
      int lastSeq = lastSequence();
      if(lastSeq >= firstSeq)
      {
        for(int i = lastSeq; i >= firstSeq; --i )
        {
          f(queueImpl[i]);
        }
      }
      else 
      {
        for(int i = lastSeq; i >=0; --i)
        {
          f(queueImpl[i]);
        }
        for(int i = capacity-1; i >=firstSeq; --i)
        {
          f(queueImpl[i]);
        }
      }
    }
    template<typename F, typename PRED>
    void rvisit(PRED&& pred, F&& f) const // visit from latest to oldest 
    {
      int lastSeq = lastSequence();
      for(int i = lastSeq; i >=0 && pred(queueImpl[i]); --i)
      {
        f(queueImpl[i]);
      }
      if(overwriting)
      {	      
        for(int i = capacity-1; i > lastSeq && pred(queueImpl[i]); --i)
        {
          f(queueImpl[i]);
        }
      }	
    }
    template<typename PRED>
    size_t visitWindowSize(PRED&& pred) const
    {
      size_t N = 0;	    
      int lastSeq = lastSequence();
      for(int i = lastSeq; i >=0; --i)
      {
        if(!pred(queueImpl[i]))
	{
	  return N;
	}
	++N;
      }
      if(overwriting)
      {	      
        for(int i = capacity-1; i > lastSeq; --i)
        {
          if(!pred(queueImpl[i]))
          {
	    return N;
	  }
          ++N;	  
        }
      }	
      return N;
    }	    
    inline int size() const
    {
      return overwriting ? capacity : writer_pos; // no pop in this ringbuffer 
    }
    inline bool empty() const
    {
      return writer_pos == 0 && !overwriting;
    }
    T& peek()
    {
      return at(lastSequence());
    }
    T& peek(size_t window)
    {
      return at(firstSequence(window));
    }
    int64_t mLastUpdateTime;
  protected:
    inline T& at(int i) // check array out of bound 
    {
      if(i >= size())
      {
        throw std::runtime_error("Error: Ringbuffer out of bound access");
      }
      return queueImpl[i];
    }
    inline int lastSequence() const
    {
      if(empty())
      {
        throw std::runtime_error("Error: LastSeq Query, Ringbuffer Empty ");
      }
      return (writer_pos -1) < 0 ? (capacity -1) : (writer_pos -1);  
    }
    inline int firstSequence(int lastN) const
    {
      if(empty())
      {
        throw std::runtime_error("Error: FirstSeq Query, Ringbuffer Empty ");
      }
      int n = writer_pos - lastN;
      return n >= 0 ? n : (overwriting ? (capacity- abs(n)) : 0);
    }
    inline int64_t increaseIndex(int64_t pos)
    {
      return (pos+1)&(capacity-1); //(pos+1)%queue_capacity
    } 
    std::vector<T> queueImpl;
    int64_t writer_pos = 0;
    bool overwriting = false;
};
template<int N>
class Int32Buffer : public RingBuffer<int32_t, N>
{

};
template<int N>
class UInt32Buffer : public RingBuffer<uint32_t, N>
{

};
template<int N>
class Int64Buffer : public RingBuffer<int64_t, N>
{

};
template<int N>
class UInt64Buffer : public RingBuffer<uint64_t, N>
{

};
template<int N>
class Int16Buffer : public RingBuffer<int16_t, N>
{

};
template<int N>
class UInt16Buffer : public RingBuffer<uint16_t, N>
{

};
template<int N>
class Int8Buffer : public RingBuffer<int8_t, N>
{

};
template<int N>
class UInt8Buffer : public RingBuffer<uint8_t, N>
{

};
template<int N>
class DoubleBuffer : public RingBuffer<double, N>
{
public:
  double sum(size_t window)
  {
    double s = 00.0;	  
    auto f = [&s](double v)
    {
      s += v;
    };
    rvisit(window, f);
    return s;
  }

};
class TradeBuffer : public RingBuffer<Trade, 2048>
{
  // define the stats method all here
  public:
    Trade& getLastTrade()
    {
      return peek();
    }
    Trade& getFirstTradeInWindow(size_t window)
    {
      return peek(window);
    } 
}; 
template<typename SamplingTimeUnit>
class PriceDepthBuffer : public RingBuffer<DepthBook, 32768>
{
  // define the stats method all here
  public:
    int64_t lastUpdateTime;
};
template<typename SamplingTimeUnit>
class TickerBuffer : public RingBuffer<Ticker, 32768>
{
  public:
    Ticker& getLastTicker()
    {
      return peek();
    }
    Ticker& getFirstTradeInWindow(size_t window)
    {
      return peek(window);
    } 
    int64_t lastUpdateTime;
}; 


}
}
}
