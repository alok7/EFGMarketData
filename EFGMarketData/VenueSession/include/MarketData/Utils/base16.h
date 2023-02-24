#pragma once 

#include <string>
#include <stdexcept>

namespace EFG
{
namespace MarketData
{
namespace BASE16 
{

    static std::string const base16_chars =
            "0123456789ABCDEF";

/**
 * @return true if c is a valid base64 character
 */
    static inline bool is_base16(unsigned char c) {
        return ((c >= 47 && c <= 57) || // /-9
                (c >= 65 && c <= 70) || // A-F
                (c >= 97 && c <= 102)); // a-f
    }

   inline std::string base16_encode(unsigned char const * input, size_t len) {
        std::string ret;
        ret.reserve(len * 2);
        
        for(int i=0; i<len; i++){
            ret.push_back(base16_chars[input[i] >> 4]);
            ret.push_back(base16_chars[input[i] & 15]);
        }

        return ret;
    }

    inline std::string base16_encode(std::string const & input) {
        return base16_encode(
                reinterpret_cast<const unsigned char *>(input.data()),
                input.size()
        );
    }

    inline int hex_value(char hex_digit)
    {
        switch (hex_digit) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return hex_digit - '0';

        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            return hex_digit - 'A' + 10;

        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            return hex_digit - 'a' + 10;
        }
        throw std::invalid_argument("invalid hex digit");
    }

    inline std::string base16_decode(std::string const & input) {
        size_t in_len = input.size();

        std::string ret;
        ret.reserve(in_len / 2);
        if (in_len & 1) throw std::invalid_argument("odd length");
        
        for (auto it = input.begin(); it != input.end(); )
        {
            int hi = hex_value(*it++);
            int lo = hex_value(*it++);
            ret.push_back(hi << 4 | lo);
        }
        return ret;
    }

} 
}
}
