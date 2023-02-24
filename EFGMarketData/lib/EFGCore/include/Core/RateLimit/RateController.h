#pragma once
#include <queue>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
/*
Sliding window log based ratelimitter!  
**/
namespace EFG
{
namespace Core
{

  class RateController
  {
    public:
      RateController(int64_t _rate_limit_count, int64_t _rate_limit_interval_ms);
      bool allow(); 
    private:
      std::queue <int64_t> timestamps_log;
      int64_t rate_limit_count;
      int64_t rate_limit_interval_ms; 

  };




}
}
