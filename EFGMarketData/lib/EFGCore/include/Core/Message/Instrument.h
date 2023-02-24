#pragma once
#include <cstdint>
#include <string>
#include <stdio.h>
#include <string.h>

namespace EFG
{
namespace Core
{
namespace Message
{

struct InstrumentId 
{
  using type = int64_t;
  type mId;
  InstrumentId() : mId(-1)
  {
  }
  InstrumentId(type id) : mId(id)
  {

  }
  InstrumentId& operator=(const InstrumentId& id)
  {
    mId = id.get();
    return *this;  
  }
  InstrumentId& operator=(const type& id)
  {
    mId = id;
    return *this;  
  }
  InstrumentId& operator=(InstrumentId&& id)
  {
    mId = std::move(id.get());
    return *this;
  }
  InstrumentId(const InstrumentId& id)
  {
    mId = id.get();
  }
  InstrumentId(InstrumentId&& id)
  {
    mId = std::move(id.get());
  }
  bool operator==(const InstrumentId id)
  {
    return mId==id.get();
  }
  type& get()
  {
    return mId;
  }
  const type& get() const
  {
    return mId;
  }

};

struct InstrumentName
{
  char mName[16];
  InstrumentName()
  {
    memset(mName, '\0', sizeof(mName));
  }
  InstrumentName(const std::string& name)
  {
    auto const endPos = std::min(name.length(), sizeof(mName) - 1);
    memcpy(mName, name.data(), endPos);
    memset(mName + endPos, '\0', sizeof(mName) - endPos);
  }
  InstrumentName(const char* name)
  {
    auto const endPos = std::min(strlen(name), sizeof(mName) - 1);
    memcpy(mName, name, endPos);
    memset(mName + endPos, '\0', sizeof(mName) - endPos);
  }
  InstrumentName(const InstrumentName& name)
  {
    memcpy(mName, name.data(), sizeof(mName));
  }
  InstrumentName& operator=(const InstrumentName& name)
  {
    if(this!=&name)
    {
      memcpy(mName, name.data(), sizeof(mName));
    }
    return *this;
  }
  bool operator==(const InstrumentName& name) const
  {
    return memcmp(const_cast<char*>(mName), name.data(), sizeof(mName)) == 0; 
  }
  const char* data()
  {
    return mName;
  }
  const char* data() const
  {
    return mName;
  } 
  std::string toString() const
  {
    return std::string(mName);
  }

};

inline bool operator==(const std::string& left, const InstrumentName& right)
{
  return memcmp(const_cast<char*>(left.data()), right.data(), sizeof(right.mName))==0;
}
inline bool operator==(const InstrumentName& left, const std::string& right)
{
  return memcmp(const_cast<char*>(left.data()), right.data(), sizeof(left.mName))==0;
}


inline bool operator==(const int64_t& left, const InstrumentId& right)
{
  return left==right.get();
}
inline bool operator==(const InstrumentId& left, const int64_t& right)
{
  return left.get()==right;
}
inline bool operator==(const InstrumentId& left, const InstrumentId&  right)
{
  return left.get()==right.get();
}

struct InstrumentIdHash
{
  std::size_t operator()(const InstrumentId& instrument) const noexcept 
  {
    auto hashfn = std::hash<int> {};
    return hashfn(instrument.get());
  }

};

struct InstrumentNameHash
{
  std::size_t operator()(const InstrumentName& instrument) const noexcept 
  {
    auto hashfn = std::hash<std::string> {};
    return hashfn(std::string(instrument.mName, sizeof(instrument.mName)));
  }

};

}
}
}
