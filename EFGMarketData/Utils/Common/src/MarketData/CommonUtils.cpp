#include <MarketData/CommonUtils.h>

namespace EFG{
namespace MarketData{
namespace Utils{

  using namespace std;
  using namespace BASE64;
  using namespace EFG::MarketData;

 char * GetTimestamp(char *timestamp, int len) {
     time_t t;
     time(&t);
     struct tm* ptm = gmtime(&t);
     strftime(timestamp,len,"%FT%T.123Z", ptm);
     return timestamp;
  }


 string GetSign(string key, string timestamp, string method, string requestPath, string body) {
    string sign;
    unsigned char * mac = NULL;
    unsigned int mac_length = 0;
    string data = timestamp + method + requestPath + body;
    int ret = Utils::HMAC::HmacEncode("sha256", key.c_str(), key.length(), data.c_str(), data.length(), mac, mac_length);
    sign = base64_encode(mac, mac_length);
    return sign;
 }

 string BuildParams(string requestPath, map<string,string> m) {
    string str = requestPath;
    bool first = true;
    for(auto i=m.begin();i!=m.end();i++) {
        if (first) {
            str += "?";
            first = false;
        } else {
            str += "&";
        }
        str += i->first;
        str += "=";
        str += i->second;
    }
    return str;
 }

 string getLevelStr(int level) {
    string levelStr;
    for(int levelI = 0;levelI<level ; levelI++){
        levelStr.append("\t");
    }
    return levelStr;
 }


 string JsonFormat(string jsonStr) {
    int level = 0;
    string jsonForMatStr;
    for(int i=0;i<jsonStr.length();i++){
        char c = jsonStr.at(i);
        if(level>0&&'\n'==jsonForMatStr.at(jsonForMatStr.length()-1)){
            jsonForMatStr += getLevelStr(level);
        }
        switch (c) {
            case '{':
            case '[':
                jsonForMatStr += c;
                jsonForMatStr += '\n';
                level++;
                break;
            case ',':
                jsonForMatStr += c;
                jsonForMatStr += '\n';
                break;
            case '}':
            case ']':
                jsonForMatStr += '\n';
                level--;
                jsonForMatStr += getLevelStr(level);
                jsonForMatStr += c;
                break;
            default:
                jsonForMatStr += c;
                break;
        }
    }

    return jsonForMatStr;

  }
  unsigned int str_hex(unsigned char *str,unsigned char *hex)
  {
     unsigned char ctmp, ctmp1,half;
     unsigned int num=0;
     do{
        do{
            half = 0;
            ctmp = *str;
            if(!ctmp) break;
            str++;
        }while((ctmp == 0x20)||(ctmp == 0x2c)||(ctmp == '\t'));
        if(!ctmp) break;
        if(ctmp>='a') ctmp = ctmp -'a' + 10;
        else if(ctmp>='A') ctmp = ctmp -'A'+ 10;
        else ctmp=ctmp-'0';
        ctmp=ctmp<<4;
        half = 1;
        ctmp1 = *str;
        if(!ctmp1) break;
        str++;
        if((ctmp1 == 0x20)||(ctmp1 == 0x2c)||(ctmp1 == '\t'))
        {
            ctmp = ctmp>>4;
            ctmp1 = 0;
        }
        else if(ctmp1>='a') ctmp1 = ctmp1 - 'a' + 10;
        else if(ctmp1>='A') ctmp1 = ctmp1 - 'A' + 10;
        else ctmp1 = ctmp1 - '0';
        ctmp += ctmp1;
        *hex = ctmp;
        hex++;
        num++;
     }while(1);
     if(half)
     {
        ctmp = ctmp>>4;
        *hex = ctmp;
        num++;
     }
     return(num);
  }

 void hex_str(unsigned char *inchar, unsigned int len, unsigned char *outtxt)
 {
    unsigned char hbit,lbit;
    unsigned int i;
    for(i=0;i<len;i++)
    {
        hbit = (*(inchar+i)&0xf0)>>4;
        lbit = *(inchar+i)&0x0f;
        if (hbit>9) outtxt[2*i]='A'+hbit-10;
        else outtxt[2*i]='0'+hbit;
        if (lbit>9) outtxt[2*i+1]='A'+lbit-10;
        else    outtxt[2*i+1]='0'+lbit;
    }
    outtxt[2*i] = 0;
 }


}
}
}
