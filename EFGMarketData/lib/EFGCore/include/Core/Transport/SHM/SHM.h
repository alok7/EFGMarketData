#pragma once

#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <atomic>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <cassert>
#include <time.h>
#include <type_traits>
#include <cstring>
#include <sched.h>
#include <time.h>
#include <unistd.h>
#include <limits>

#define LIKELY(x) __builtin_expect((x), 1)
#define UNLIKELY(X) __builtin_expect((x), 0)
#define ACCESS_ONCE(x) (*static_cast<typename std::remove_reference<decltype(x)>::type volatile *>(&(x)))

namespace EFG
{
namespace Core
{


enum Mode{CREATE_RW, RW_ONLY};


struct MasterRecord
{
  MasterRecord(const std::string& name_)
  {
    std::memset(this, 0, sizeof(MasterRecord)); 
    std::memcpy(mName, name_.c_str(), std::min(sizeof(mName), name_.length()));
    mLatestHeadPos.store(0);
    mLatestFinalizedPos.store(0);
    mConsumersCount.store(0);
    clearLockBit();
    for(size_t i = 0; i < 32; ++i)
    {
      mConsumersPos[i] = std::numeric_limits<size_t>::max();  
    }	    
  }
  void clearLockBit()
  {
    isLocked.store(false);
    isSnapShotPublished.store(false);
  } 
  // make sure below calls are protected
  size_t getSlowestConsumerPos()
  {
    size_t pos = std::numeric_limits<size_t>::max();
    for(size_t i = 0; i < mConsumersCount.load(); ++i)
    {
      pos = std::min(mConsumersPos[i], pos);
    }
    return pos;    
  }
  void updateConsumerPos(size_t consumer, size_t pos)
  {
    mConsumersPos[consumer] = pos; 
  }	  
  alignas(64) char mName[128];
  alignas(64) size_t mCapacity;
  alignas(64) size_t mConsumersPos[32]; // Max 32 concurrent consumers are supported;
  alignas(64) std::atomic<size_t> mConsumersCount;
  alignas(64) std::atomic<size_t> mLatestHeadPos;
  alignas(64) std::atomic<size_t> mLatestFinalizedPos;
  alignas(64) std::atomic<bool> isLocked = {false};
  alignas(64) std::atomic<bool> isSnapShotPublished = {false};
};


class SpinLock
{
  public:	
    constexpr SpinLock(MasterRecord& master): mLock(master.isLocked)
    {
      lock();
    }
    ~SpinLock()
    {
      unlock();
    }
    SpinLock(const SpinLock& s) = delete;
    SpinLock& operator=(const SpinLock& s) = delete;
  private:
    inline void lock() noexcept 
    {
      for(uint8_t count = 0; !try_lock(); ++count)
      {
        count < 16 ? __builtin_ia32_pause() : std::this_thread::yield();
	count = (count&(count&15));
      }	      
    }
    inline bool try_lock() noexcept 
    {
      return !mLock.load(std::memory_order_relaxed) && !mLock.exchange(true, std::memory_order_acquire);
    }
    void unlock() noexcept
    {
      mLock.store(false, std::memory_order_release);
    }
  private:
    std::atomic<bool>& mLock;
};

template<typename RECORD_TYPE> 
class SHMStore final
{
  public:
    explicit SHMStore(const std::string name_, size_t recordsCount_, Mode mode_) : mName(name_), mRecordsCount(recordsCount_), mMode(mode_) 
    {
       
    }
    inline RECORD_TYPE& at(size_t pos)
    {
      return *reinterpret_cast<RECORD_TYPE*>(static_cast<char*>(mAddr)+sizeof(RECORD_TYPE)*pos);
    }    
    ~SHMStore()
    {
      munmap(mAddr, mRecordsCount*sizeof(RECORD_TYPE));
      close(fd);
      if(mMode==Mode::CREATE_RW)
      {
        if(mName.find("_snapshot_")!=std::string::npos)
        {
	  shm_unlink(mName.c_str());
	}		
      }
      if(mLoadWorker.joinable())
        mLoadWorker.join();
    }
    void create(bool& exists)
    {
      size_t STORAGE_SIZE = mRecordsCount*sizeof(RECORD_TYPE); 
      fd = shm_open(mName.c_str(), O_RDWR, 0666);
      if(fd < 0)
      {
        fd = shm_open(mName.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd  < 0)
        {
          perror("create, shm_open()");
          mInit.store(false);
        }
        int res = ftruncate(fd, STORAGE_SIZE); 
        // map shared memory to process address space
        mAddr = static_cast<char*>(mmap(NULL, STORAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
          
        volatile size_t sum =0;
        for(size_t i=0; i < mRecordsCount; ++i)
        {
          volatile char* addToWarm = reinterpret_cast<char*>(mAddr) + sizeof(RECORD_TYPE)*i;
          //sum += *addToWarm;
        }
        mInit.store(true);
        exists = false;
	if(mName.find("_master")==std::string::npos)
          std::cout << "[INFO]: SHM created - " << mName << std::endl;
      }
      else
      {
	if(mName.find("_master")==std::string::npos)
          std::cout << "[INFO]: SHM already exists, loading - " << mName << std::endl;
        load();
        exists = true;
      }
   }
   void asyncLoad()
   {
     mLoadWorker = std::thread([this](){load();});
   }
   void load()
   {
     size_t STORAGE_SIZE = mRecordsCount*sizeof(RECORD_TYPE);
     fd = shm_open(mName.c_str(), O_RDWR, 0666);

     while (fd < 0)
     {
        if(fd < 0)
        {
          perror("Load error: shm_open()");
          size_t tryCount = 7;
          while(tryCount-- > 0)
          {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
          }
          mInit.store(false);
          fd = shm_open(mName.c_str(), O_RDWR, 0666);
        }
     }

     if(mName.find("_master")==std::string::npos)
       std::cout << "[INFO]: SHM Loaded " << mName << std::endl;
     mAddr = static_cast<char*>(mmap(NULL, STORAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
     mInit.store(true);
   }
 private:   
   int fd;
   void *mAddr; // address to start mapping
   size_t mRecordsCount;
   std::string mName;
   std::thread mLoadWorker;
   Mode mMode;        
 public:
   std::atomic<bool> mInit = {false};   
};

template<typename RECORD_TYPE> 
class Store
{
  private:
    using DataNodes  = SHMStore<RECORD_TYPE>;
    using MasterNode = SHMStore<MasterRecord>;
  public:
    Store(const std::string& name_, Mode mode_, size_t recordCount_)
    {
      mRecordCount = recordCount_;
      assert((!(mRecordCount==0) & !(mRecordCount & (mRecordCount - 1))) && "SHM size requires a power of two");
      std::string masterNodeName = name_ + "_master";
      std::cout << '\n';
      mMasterNode = std::make_unique<MasterNode>(masterNodeName, 1, mode_); 
      mDataNodes  = std::make_unique<DataNodes>(name_, mRecordCount, mode_); 
      if(mode_==CREATE_RW)
      {
        bool masterNodeExist = false;
	bool dataNodeExist = false;
        mMasterNode->create(masterNodeExist);
	mDataNodes->create(dataNodeExist);
        if(!masterNodeExist)
        {
          auto master= new(&mMasterNode->at(0)) MasterRecord(masterNodeName);
          // reset master
          master->clearLockBit();
          master->mLatestHeadPos.store(0);
          master->mLatestFinalizedPos.store(0);
	  master->mCapacity = mRecordCount;
        }
        else
        {
           auto& master = mMasterNode->at(0);
           master.mLatestHeadPos.store(master.mLatestFinalizedPos.load());
	   master.clearLockBit();
	   std::cout << '\n';
	   std::cout << "[INFO]: lastFinalizedPos " << master.mLatestFinalizedPos.load()
		     << ", lastWriterPos " << master.mLatestHeadPos.load() << std::endl;
        }
      }
      else // RW_ONLY
      {
        mMasterNode->asyncLoad();
        mDataNodes->asyncLoad();
      }

    }
    inline RECORD_TYPE& read(int index) noexcept 
    {
      return mDataNodes->at(index); 
    }
    RECORD_TYPE& allocate() noexcept 
    {
      auto& master = mMasterNode->at(0);
      size_t currentIndex = master.mLatestHeadPos.load(std::memory_order_relaxed);  
      size_t nextIndex = increment(currentIndex);  
      auto& record = mDataNodes->at(currentIndex);
      master.mLatestHeadPos.store(nextIndex, std::memory_order_relaxed);
      return record; // uninitialized memory
    }
    inline MasterRecord& masterNode()
    {
      return mMasterNode->at(0);
    }
    bool isInitialized() noexcept 
    {
      return mDataNodes->mInit.load() && mMasterNode->mInit.load();
    } 
    inline void finalize() noexcept // finalize the slot after each write 
    {
      auto& master = mMasterNode->at(0);
      master.mLatestFinalizedPos.store(increment(master.mLatestFinalizedPos.load(std::memory_order_relaxed)), std::memory_order_acquire);
    }
    inline size_t increment(size_t index) noexcept
    {
      return (index+1)&(mRecordCount-1);
    }
    size_t decrement(size_t index) noexcept
    {
      return index == 0 ? (mRecordCount-1) : (index -1);
    }
    void resetMasterNode() noexcept
    {
      auto& master = mMasterNode->at(0);
      master.mLatestHeadPos.store(0);
      master.mLatestFinalizedPos.store(0);
      master.mCapacity = mRecordCount;
      master.clearLockBit();
    }
  private:
    std::string getDateStr()
    {
      time_t t = time(0);   // get time now
      struct tm * now = localtime( & t );
      char buffer [80];
      strftime (buffer,sizeof(buffer),"%Y-%m-%d.",now);
      return std::string(buffer, sizeof(buffer)); 
    }
    std::unique_ptr<DataNodes> mDataNodes;
    std::unique_ptr<MasterNode> mMasterNode;
    size_t mRecordCount;
      
  };

}
}
