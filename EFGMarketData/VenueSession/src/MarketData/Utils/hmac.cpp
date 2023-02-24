#include <MarketData/Utils/hmac.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <string.h>
#include <iostream>

using namespace std;

namespace EFG{
namespace MarketData{
namespace Utils{
namespace HMAC{
  
    int HmacEncode(const char * algo,
               const char * key, unsigned int key_length,
               const char * input, unsigned int input_length,
               unsigned char * &output, unsigned int &output_length) {
      
      const EVP_MD * engine = nullptr;
      if(strcasecmp("sha512", algo) == 0) {
        engine = EVP_sha512();
      }
      else if(strcasecmp("sha256", algo) == 0) {
        engine = EVP_sha256();
      }
      else if(strcasecmp("sha1", algo) == 0) {
        engine = EVP_sha1();
      }
      else if(strcasecmp("md5", algo) == 0) {
        engine = EVP_md5();
      }
      else if(strcasecmp("sha224", algo) == 0) {
        engine = EVP_sha224();
      }
      else if(strcasecmp("sha384", algo) == 0) {
        engine = EVP_sha384();
      }
      else if(strcasecmp("sha", algo) == 0) {
        //engine = EVP_sha();
      }
      else {
        cout << "Algorithm " << algo << " is not supported by this program!" << endl; // Logger --later 
        return -1;
      }

      output = (unsigned char*)malloc(EVP_MAX_MD_SIZE);

      //HMAC_CTX ctx;
      HMAC_CTX *ctx = HMAC_CTX_new();
      //HMAC_CTX_init(&ctx);
      HMAC_Init_ex(ctx, key, strlen(key), engine, NULL);
      HMAC_Update(ctx, (unsigned char*)input, strlen(input));

      HMAC_Final(ctx, output, &output_length);
      HMAC_CTX_free(ctx);

      return 0;
   }
}
}
}
}
