#pragma once

namespace EFG{
namespace MarketData{
namespace Utils{
namespace HMAC{
  
int HmacEncode(const char * algo,
        const char * key, unsigned int key_length,
        const char * input, unsigned int input_length,
        unsigned char * &output, unsigned int &output_length);

}
}
}
}
