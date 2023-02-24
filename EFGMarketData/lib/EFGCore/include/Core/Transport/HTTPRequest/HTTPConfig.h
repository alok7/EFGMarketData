#pragma once

#include <string>

namespace EFG {

namespace Core {

namespace Transport {

  class HTTPConfig {
  public:
    HTTPConfig() = default;
    const std::string& getHttpEndPoint() const;
    void setHttpEndpoint(const std::string &);
  private:
    std::string _httpBaseEndPoint;
  };
}
}
}
