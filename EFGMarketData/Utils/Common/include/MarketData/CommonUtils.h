#pragma once

#include <ctime>
#include <chrono>
#include <string>
#include <map>
#include <zlib.h>
#include <MarketData/Utils/hmac.h>
#include <MarketData/Utils/base64.h>
#include <MarketData/Utils/base16.h>
#include <iostream>
#include <Core/rapidjson/document.h>
#include <Core/rapidjson/writer.h>
#include <Core/rapidjson/stringbuffer.h>

using namespace rapidjson;

namespace EFG{
namespace MarketData{
namespace Utils{

typedef int64_t TimeType;

static const int64_t multipliers[] =
{
  1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000
};
#define TOTAL_PRECISION 20
#define MANTISSA_PRECISION (sizeof(multipliers)/sizeof(multipliers[0]) -1)

  char * GetTimestamp(char *timestamp, int len);
  inline TimeType getHRTime() {
	  struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int64_t epoch_nanos = (uint64_t)ts.tv_sec * 1000000000LL + (uint64_t)ts.tv_nsec;
    return epoch_nanos;
  }
  std::string BuildParams(std::string requestPath, std::map<std::string,std::string> m);
  std::string JsonFormat(std::string jsonStr);
  inline int gzDecompress(const char *src, int srcLen, char *dst, int* dstLen, short decompress_format) /* deflate=0, zlib=1, gzip=2*/
  {
	static int  windowBits[3] = {-MAX_WBITS, MAX_WBITS, MAX_WBITS + 16}; 
	z_stream _stream;
	_stream.zalloc = Z_NULL;
	_stream.zfree = Z_NULL;
	_stream.opaque = Z_NULL;

	_stream.avail_in = srcLen; 
	_stream.next_in = (Bytef*)src; 
	_stream.avail_out = *dstLen; 
	_stream.next_out = (Bytef*)dst; 
	inflateInit2(&_stream, windowBits[decompress_format]);
	inflate(&_stream, Z_NO_FLUSH);
	inflateEnd(&_stream);
	
	*dstLen = _stream.total_out;
	return 0;
  }
  inline int gzCompress(const char *src, int srcLen, char *dst, int* dstLen, short decompress_format) {
  	static int  windowBits[3] = {-MAX_WBITS, MAX_WBITS, MAX_WBITS + 16};
	z_stream _stream;
	_stream.zalloc = Z_NULL;
	_stream.zfree = Z_NULL;
	_stream.opaque = Z_NULL;
	_stream.avail_in = srcLen;
	_stream.next_in = (Bytef*)src;
	_stream.avail_out = *dstLen;
	_stream.next_out = (Bytef*)dst;
	int ret = deflateInit2(&_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowBits[decompress_format], 9, Z_DEFAULT_STRATEGY);
	deflate(&_stream, Z_FINISH);
	deflateEnd(&_stream);
	*dstLen = _stream.total_out;
	return 0;
  }
  std::string GetSign(std::string key, std::string timestamp, std::string method, std::string requestPath, std::string body);
  unsigned int str_hex(unsigned char *str,unsigned char *hex);
  void hex_str(unsigned char *inchar, unsigned int len, unsigned char *outtxt);
  inline uint64_t constexpr hash_32_fnv1a(const char* key)
  {
    
    uint32_t hash = 0x811c9dc5;
    uint32_t prime = 0x1000193;
    size_t len = strlen(key)+1;
    for(int i = 0; i < len; ++i) {
        uint8_t ch = key[i];
        hash = hash ^ ch;
        hash *= prime;
    }
    return hash;
  }
  inline __attribute__((always_inline)) int64_t convertToPrice(std::string price_str)
  {

   int64_t value = 0;
   if(price_str.empty()) return value;
   short sign = 1;
   unsigned short pos = -1;
   if('-'==price_str[0]){
    sign = -1;
   }
   
   unsigned short mantissa = 0;
   bool decimalFound(false);
   while(++pos < price_str.length()){
     switch(price_str[pos])
     {
       case '.':
       {
         if(true==decimalFound)
         {
           // throw error 
         }
         else
         { 
           decimalFound = true;
         }
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
         value =value*10;
         value += (price_str[pos]-'0'); 
         mantissa +=decimalFound;
         break;
       }
       default:
       // throw not a valid price 
       {
       }
       
     }
   }
   value = (sign*value*multipliers[MANTISSA_PRECISION - mantissa]);
   return value;
   
  }
 
}
}
}
