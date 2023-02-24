#pragma once 
#include <Core/Message/Message.h>

using namespace EFG::Core;

namespace EFG
{
namespace MarketGateway
{
namespace Message
{
namespace KITE_ZERODHA 
{
  namespace Request
  {
    class NewOrder : public EFG::Core::Message::Request::BaseRequest
    {
      public:
        Order(){}
        FixedString<32> mVariety;
        FixedString<4>  mExchange;
        FixedString<32> mSymbol;
        FixedString<32> mTxnType;
        int             mQty;
        FixedString<32> mProduct;
        FixedString<32> mOrderType;
        double          mPrice;
        FixedString<32> mValidity;
        double          mTrigPrice;
        double          mSQOff;
        double          mSL;
        double          mTrailSL;
        int             mDiscQty;
        FixedString<32> mTag;
    };
    class CancelOrder
    {

    };
    class ReplaceOrder
    {

    }; 
  }
}
  
namespace BINANCE_SPOT
{
  using namespace EFG::Core;
 
  template <typename ...PARAMS> struct parameter_pack
  {
    template <template <typename...> typename S> using apply = S<PARAMS...>;
  };
  namespace Request
  {
    class Order : public EFG::Core::Message::Request::BaseRequest
    {
      public:
        Order(){}
        Order(const Order& order)
        {
          symbol = order.symbol;
          side = order.side;
          type = order.type;
          timeInForce = order.timeInForce;
          quantity = order.quantity;
          quoteOrderQty = order.quoteOrderQty;
          price = order.price;
          orderStatus = order.orderStatus;;
          newClientOrderId = order.newClientOrderId;
          stopPrice = order.stopPrice;
          icebergQty = order.icebergQty;
          timestamp = order.timestamp;
        }
        Order& operator=(const Order& order)
        {
          if(&order==this)
            return *this;
          symbol = order.symbol;
          side = order.side;
          type = order.type;
          timeInForce = order.timeInForce;
          quantity = order.quantity;
          quoteOrderQty = order.quoteOrderQty;
          price = order.price;
          orderStatus = order.orderStatus;
          newClientOrderId = order.newClientOrderId;
          stopPrice = order.stopPrice;
          icebergQty = order.icebergQty;
          timestamp = order.timestamp;
          return *this;
        } 
        FixedString<32> symbol;
        FixedString<4>  side;
        FixedString<32> type;
        FixedString<32> timeInForce;
        FixedString<32> quantity;
        FixedString<32> quoteOrderQty;
        FixedString<32> price;
        FixedString<32> orderStatus;
        FixedString<32> newClientOrderId;
        FixedString<32> stopPrice;
        FixedString<32> icebergQty;
        int64_t timestamp;
    };

    class CancelOrder
    {
      public:
        CancelOrder()
        {
        }
        CancelOrder(const CancelOrder& order)
        {
          symbol = order.symbol;
          orderId = order.orderId;
          origClientOrderId = order.origClientOrderId;
          newClientOrderId = order.newClientOrderId;
        }
        CancelOrder& operator=(const CancelOrder& order)
        {
          if(&order==this)
            return *this;

          symbol = order.symbol;
          orderId = order.orderId;
          origClientOrderId = order.origClientOrderId;
          newClientOrderId = order.newClientOrderId;
          return *this;
        }
        FixedString<32> symbol;
        FixedString<32> orderId;
        FixedString<32> origClientOrderId;
        FixedString<32> newClientOrderId;
        int64_t timestamp;
    };

    class CancelAllOrder
    {
      public:
        CancelAllOrder()
        {
        }
        CancelAllOrder(const CancelAllOrder& order)
        {
          symbol = order.symbol;
        }
        CancelAllOrder& operator=(const CancelAllOrder& order)
        {
          if(&order==this)
            return *this;

          symbol = order.symbol;
          return *this;
        }
        FixedString<32> symbol;
        int64_t timestamp;
    };
    class AmendOrder
    {
      public:
        AmendOrder(){}
        AmendOrder(const AmendOrder& order)
        {
          side = order.side;
          type = order.type;
          timeInForce = order.timeInForce;
          symbol = order.symbol;
          orderId = order.orderId;
          origClOrdID = order.origClOrdID;
          clOrdID = order.clOrdID;
          orderQty = order.orderQty;
          price = order.price;
        }
        AmendOrder& operator=(const AmendOrder& order)
        {
          if(&order==this)
            return *this;
          side = order.side;
          type = order.type;
          timeInForce = order.timeInForce;
          symbol = order.symbol;
          orderId = order.orderId;
          origClOrdID = order.origClOrdID;
          clOrdID = order.clOrdID;
          orderQty = order.orderQty;
          price = order.price;
          return *this;
        }
        FixedString<4>  side;
        FixedString<32> type;
        FixedString<32> timeInForce;
        FixedString<32> symbol;
        FixedString<32> orderId;
        FixedString<32> origClOrdID;
        FixedString<32> clOrdID;
        FixedString<32> orderQty;
        FixedString<32> price;

    };
  }
  namespace Response 
  {
    class ExecutionReport : public EFG::Core::Message::Response::ExecutionReport 
    {
      public:

    }; 
    using Traits = parameter_pack<ExecutionReport>;
  }

}
}
}
}
