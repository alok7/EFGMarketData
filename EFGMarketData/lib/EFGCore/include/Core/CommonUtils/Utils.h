#pragma once
#include <limits>
#include <time.h>
#include <stdint.h>

namespace EFG
{
namespace Core
{
namespace Utils
{

static const constexpr int64_t multipliers[]=
{
  1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000,
  10000000000, 100000000000,1000000000000
};

inline double toDouble(const char* _str)
{
  int64_t value = 0;
  while(_str && ' '==_str[0]) ++_str;
  if(*_str=='\0')
  {
    perror("empty string!");
  }
  int8_t sign = 1;
  if('-'==_str[0])
  {  
    sign = -1;
  }
  unsigned short mantisa = 0;
  bool decimalFound(false);

  while(*_str!='\0')
  {
    switch(*_str)
    {
      case '.':
      {
        decimalFound = true;
        break;
      }
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      {
        value = value*10;
        value +=(*_str-'0');
        mantisa +=decimalFound;
        break;
      }
      default:
      {
        break;
      }
     
    }
    ++_str;
  }
  return static_cast<double>(sign*value)/multipliers[mantisa];

}
inline int64_t toInteger(const char* _str)
{
  return static_cast<int64_t>(toDouble(_str));
}

template<size_t precisionDigits>
struct DoublePrecision
{
  const static size_t value = 10*DoublePrecision<precisionDigits-1>::value;  
};
template<>
struct DoublePrecision<0>
{
  const static size_t value = 1;  
};

template<size_t BUFFER_SIZE, size_t precisionDigits>
class fmt
{
  private:
    char mBuffer[BUFFER_SIZE];
    uint8_t idx = BUFFER_SIZE -1;
  public:
    fmt()
    {
      mBuffer[idx--] = '\0';
    }
    inline char* toString()
    {
      return &mBuffer[idx+1];
    }
    void convert(double num)
    {
      short sign = num < 0 ? -1 : 1;
      double num_abs = num*sign;
      size_t precision = DoublePrecision<precisionDigits>::value;
      size_t base = static_cast<size_t>(num_abs);
      size_t fraction = static_cast<size_t>((num_abs-static_cast<double>(base))*precision); 
      // group by two digit optimization
      while(fraction>=100)
      {
        const char* src = digit2(static_cast<size_t>(fraction%100));
        mBuffer[idx--] = src[1];
        mBuffer[idx--] = src[0];
        fraction /=100;
      }
      if(fraction < 10)
      {
        mBuffer[idx--] = static_cast<char>(fraction +'0');
        mBuffer[idx--] = '.';
      }
      else
      {
        const char* src = digit2(static_cast<size_t>(fraction));
        mBuffer[idx--] = src[1];
        mBuffer[idx--] = src[0];
        mBuffer[idx--] = '.';
      }

      while(base>=100)
      {
        const char* src = digit2(static_cast<size_t>(base%100));
        mBuffer[idx--] = src[1];
        mBuffer[idx--] = src[0];
        base /=100;
      }
      if(base < 10)
      {
        mBuffer[idx--] = static_cast<char>(base +'0');
      }
      else
      {
        const char* src = digit2(static_cast<size_t>(base));
        mBuffer[idx--] = src[1];
        mBuffer[idx--] = src[0];
      }
      if(sign < 0) mBuffer[idx--] = '-';
      
    }
    void convert(int num)
    {
      short sign = num < 0 ? -1 : 1;
      num = num*sign;
      // group by two digit optimization
      while(num>=100)
      {
        const char* src = digit2(static_cast<size_t>(num%100));
        mBuffer[idx--] = src[1];
        mBuffer[idx--] = src[0];
        num /=100;
      }
      if(num < 10)
      {
        mBuffer[idx--] = static_cast<char>(num +'0');
      }
      else
      {
        const char* src = digit2(static_cast<size_t>(num));
        mBuffer[idx--] = src[1];
        mBuffer[idx--] = src[0];
      }
      if(sign < 0) mBuffer[idx--] = '-';

    }
  private:
    inline static constexpr const char* digit2(size_t value)
    {
      return &"0001020304050607080910111213141516171819"
              "2021222324252627282930313233343536373839"
              "4041424344454647484950515253545556575859"
              "6061626364656667686970717273747576777879"
              "8081828384858687888990919293949596979899"[value*2];
    }
};


// default 9 digit precisions after decimal for decimal number conversion to string
template<typename T, size_t precision=9>
class Format
{

};

template<size_t precision>
class Format<int, precision>: public fmt<std::numeric_limits<unsigned long long>::digits10+2, precision>
{

};

template<size_t precision>
class Format<double, precision>: public fmt<2*(std::numeric_limits<unsigned long long>::digits10+2), precision>
{

};


namespace Time
{

inline int64_t epoc()
{
  struct timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);
  return (tp.tv_sec*1000000000 + tp.tv_nsec);
}

// for elapsed time measurement = now_1 - now_0;
inline int64_t now()
{
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);
  return (start.tv_sec*1000000000 + start.tv_nsec);
}

}

}
}
}
