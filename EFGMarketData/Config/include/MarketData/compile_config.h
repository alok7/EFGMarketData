#pragma once

#include <Core/MarketUtils/MarketUtils.h>

#define ENABLE_DEBUGGING            true
#define ENABLE_ORDER_BOOK_PRINTING  true
#define ORDERBOOK_DEPTH             20

#define TICKER_QUEUE_SIZE 1048576
#define TRADE_QUEUE_SIZE 1048576
#define DEPTHBOOK_QUEUE_SIZE 4194304
#define LIQUIDATION_QUEUE_SIZE 16

#define MD_LOG_DIR        "/tmp/"
#define MD_LOG_FILE       "marketdata"

#define SPOT_MARKET 0
#define FUTURES_MARKET 1
#define SWAP_MARKET 2
#define OPTIONS_MARKET 3

#define MARKET_TYPE  SPOT_MARKET

#if MARKET_TYPE == SPOT_MARKET
  #define normalized_depth_msg_type SPOT::Message::DepthBook 
#elif MARKET_TYPE == FUTURES_MARKET 
  #define normalized_depth_msg_type FUTURES::Message::DepthBook
#elif MARKET_TYPE == SWAP_MARKET
  #define normalized_depth_msg_type SWAP::Message::DepthBook
#elif MARKET_TYPE == OPTIONS_MARKET
  #define normalized_depth_msg_type OPTIONS::Message::DepthBook
#else
  #define normalized_depth_msg_type SPOT::Message::DepthBook
#endif
