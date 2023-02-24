#pragma once

#include <curl/curl.h>
#include <string>
#include <unordered_map>
#include <Core/Transport/HTTPRequest/HTTPConfig.h>

namespace EFG
{
namespace Core
{
namespace Transport
{  

  class HTTPRequest
  {
    public:
      HTTPRequest(HTTPConfig info, std::unordered_map<std::string, std::string> _header);
      HTTPRequest(HTTPConfig info);
      ~HTTPRequest();
      void setHeader(std::unordered_map<std::string, std::string>);
      void appendHeader(const std::string& key, const std::string& value);
      void clearHeader();
      void buildHeader();
      std::string get(const std::string& path, const std::unordered_map<std::string, std::string>& params);
      std::string get(const std::string& path);
      std::string post(const std::string& path, const std::string& data);
      std::string put(const std::string& path, const std::string& data);
      std::string del(const std::string& path, const std::string& data);
      static inline std::string BuildQuery(const std::unordered_map<std::string, std::string>& params)
      {
        std::string query = "";
        for (auto it = params.begin(); it != params.end(); it++) {
          query += it->first + "=" + it->second + "&";
        }
        return query;
      }
    private:
      /*
        userp pointer is set up by --> curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response)
        userp pointer points to response.
      */
      static size_t ConnectionWriteCallback( void *data, size_t size, size_t nitems, void *userp) noexcept
      {
        (static_cast <std::string*>(userp))
          ->append(static_cast <char*>(data), size * nitems); // appends the requested content to userp
        return size * nitems;
      };

      void Init();
      void LoadGETRequestOptions();
      void LoadPOSTRequestOptions();
      void LoadPUTRequestOptions();
      void LoadDELRequestOptions();
      void ConfigureAdditionalGETRequestOptions(const std::string& uri);
      void ConfigureAdditionalPOSTRequestOptions(const std::string& uri, const std::string& data);
      void ConfigureAdditionalPUTRequestOptions(const std::string& uri, const std::string& data); 
      void ConfigureAdditionalDELRequestOptions(const std::string& uri, const std::string& data); 
    private:
      CURL* GETHandler;
      CURL* POSTHandler;
      CURL* PUTHandler;
      CURL* DELHandler;

      std::unordered_map<std::string, std::string> clientHeader;
      struct curl_slist* curlHeader = nullptr;
     
      static constexpr long CONNECTION_TIMEOUT = 30L;
      HTTPConfig info; // store ref 

  };


}
}
}
