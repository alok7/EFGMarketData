#pragma once

#include <utility>
#include <Core/Message/Instrument.h>
#include <Core/csvUtils/fastReader.h>

namespace EFG
{
namespace Core
{
namespace Message
{

class InstrumentCenter
{
public:
  InstrumentCenter()
  {
  }
  bool init(const std::string& instrumentFile)
  {
    if(loadCSV(instrumentFile))
    {
      int id; std::string name, market;
      while(mReader.readRow(id, name, market))
      {
	InstrumentName instrumentName(name);
	InstrumentId   instrumentId(id);
        mIds.insert({instrumentName, instrumentId});
        mNames.insert({instrumentId, instrumentName});
      }
      return true;
    }
    return false;     
  }
  bool getName(const InstrumentId& id, InstrumentName& name)
  {
    auto it = mNames.find(id);
    if(it!=mNames.end())
    {
      name = it->second;
      return true;
    }
    return false;
  }
  bool getId(const InstrumentName& name, InstrumentId& id)
  {
    auto it = mIds.find(name);
    if(it!=mIds.end())
    {
      id = it->second;
      return true;
    }
    return false;
  }
private:
  bool loadCSV(const std::string& instrumentFile)
  {
    return mReader.init(instrumentFile, "id",
                                        "name",
				        "market");  
  }
  std::unordered_map<InstrumentName, InstrumentId, InstrumentNameHash> mIds;
  std::unordered_map<InstrumentId, InstrumentName, InstrumentIdHash> mNames;
  EFG::Core::CSV::Reader<3> mReader;
};


}
}
}
