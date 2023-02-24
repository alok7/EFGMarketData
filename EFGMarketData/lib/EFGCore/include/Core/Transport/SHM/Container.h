#pragma once
#include <variant>
#include <Core/Transport/SHM/SHM.h>

namespace EFG
{
namespace Core
{

template<class... Ts> struct visitors : Ts... { using Ts::operator()...; };
template<class... Ts> visitors(Ts...) -> visitors<Ts...>;


template<typename ...CONTENTS>
class SHMTransport : public Store<std::variant<CONTENTS...>>
{
  private: 
    using RECORD_TYPE = std::variant<CONTENTS...> ;
    using Base = Store<RECORD_TYPE> ;
  public:
    SHMTransport(std::string _name, Mode _mode, size_t recordCount_=1048576) : Store<std::variant<CONTENTS...>>(_name, _mode, recordCount_), mName(_name)
    {
      mReaderPos = 0;
    }
    ~SHMTransport()
    {
      mStop = true;
    }
    template<typename CONTENT_TYPE>
    bool allocate(const CONTENT_TYPE& content) noexcept 
    {
      auto& master = masterNode();
      SpinLock lock(master);
      size_t slowestReadPos = master.getSlowestConsumerPos();
      size_t writePos = master.mLatestFinalizedPos.load(std::memory_order_acquire);
      size_t nextWritePos = this->increment(writePos);
      if(nextWritePos==slowestReadPos) return false; // Full state is head + 1 == tail
      auto& slot = Base::allocate();
      auto record = new(&slot) RECORD_TYPE(content);
      this->finalize();
      return true;
    }
    template<typename CONTENT_TYPE>
    CONTENT_TYPE& read(size_t pos)
    {
      auto& record = Base::read(pos);
      return std::get<CONTENT_TYPE>(record);
    }
    inline MasterRecord& masterNode()
    {
      return Base::masterNode();
    }
    template<typename... Fs>
    void readAsync(Fs... fs)
    {
      auto& master = masterNode();
      
      {
        SpinLock lock(master);
        mConsumerIndex = master.mConsumersCount.load();
        master.mConsumersCount.fetch_add(1, std::memory_order_relaxed); // later - make sure doesn't increases max consumer capacity  
      }	
      auto visitor = visitors<Fs...>{fs...};
      while(!mStop)
      {      
        if(mReaderPos != master.mLatestFinalizedPos.load(std::memory_order_acquire)) // Empty state is head == tail
        {
          SpinLock lock(master);
	  size_t current = mReaderPos;
          mReaderPos  = this->increment(current);
	  master.updateConsumerPos(mConsumerIndex, mReaderPos);
          auto& record = Base::read(current);
          try
          {
            std::visit(visitor, record);
          }
          catch(...) // shouldn't ever happen
          {
            std::cerr << "bad variant read error " << std::endl;
            mReaderPos = this->decrement(mReaderPos);
          }
        }
      }
    }
    void setReaderPosition()
    {
      auto& master = masterNode();
      mReaderPos = master.mLatestFinalizedPos.load();
      std::cout << "[INFO]: SHMReader " << mName << " startReadPos " << mReaderPos << std::endl;
    }
    void setReaderPosition(size_t pos)
    {
      mReaderPos = pos;
      std::cout << "[INFO]: SHMReader " << mName << " startReadPos " << mReaderPos << std::endl;
    }
    void stop()
    {
      mStop = true;
    }
    void readSnapShot( bool enable)
    {
      auto& master = masterNode();
      master.isSnapShotPublished.store(enable);
    }
    private:
      size_t mReaderPos;   
      size_t mConsumerIndex;   
      bool mStop = false; 
      std::string mName;
  };

}
}
