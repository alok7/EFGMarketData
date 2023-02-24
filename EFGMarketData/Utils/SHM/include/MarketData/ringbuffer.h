#pragma once

#include <stdio.h> 
#include <errno.h> 
#include <unistd.h> 
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <string_view>
#include <tuple>
#include <limits>
#include <condition_variable>
#include <mutex>  

namespace EFG
{
namespace MarketData
{

template < typename T, typename Tuple >
struct TupleIndex;

template < typename T,typename ... Types >
struct TupleIndex < T, std::tuple < T, Types... > > 
{
  static constexpr const std::size_t value = 0;
};

template < typename T, typename U, typename ... Types >
struct TupleIndex < T, std::tuple < U, Types... > > 
{
  static constexpr const std::size_t value = 1 + TupleIndex < T, std::tuple < Types... > >::value;
};

typedef std::tuple < char, int8_t, int16_t, int32_t, int64_t, double, char * > SupportedTypes;

class SHMRingBuffer
{
    public:

    SHMRingBuffer(std::string data_store_name, std::string metadata_store_name, std::string semaphore_name, std::string mode, size_t data_size, size_t metadata_size)
    {
       this->m_data_store_name = data_store_name;  
       this->m_metadata_store_name = metadata_store_name;
       this->m_semaphore_name = semaphore_name;
       this->data_size = data_size; 
       this->metadata_size = metadata_size;
       this->m_mode = mode; 
       if(mode=="Writer")
       {
         initNewSharedMemory();
         *reinterpret_cast<int32_t*>((char*)(addr_metadata)+ 2*sizeof(int32_t)) = std::numeric_limits<int>::max(); // init write high water mark
         *reinterpret_cast<int32_t*>((char*)(addr_metadata)) = m_writer_pos; // init the write pos = 0 
       }
       else // Reader
       {
         std::cout << "Initializing existing shared memory \n";
         m_thread = std::thread([this](){initExistingSharedMemory();});
       }

    }

    ~SHMRingBuffer()
    {
       if(m_mode=="Writer")
       {
         munmap(addr_data, data_size);
         munmap(addr_metadata, metadata_size);
         close(fd_data);
         close(fd_metadata);
         shm_unlink(m_data_store_name.c_str()); // Delete the shared memory object 
         shm_unlink(m_metadata_store_name.c_str());
         //semaphore cleanup
         sem_close(shm_semaphore);
         sem_unlink(m_semaphore_name.c_str()); 
       }
       else // Reader
       {
         munmap(addr_data, data_size);
         munmap(addr_metadata, metadata_size);
         close(fd_data);
         close(fd_metadata);
       }
       if(m_thread.joinable()) m_thread.join();
    }

    void initNewSharedMemory()
    {
        fd_data = shm_open(m_data_store_name.c_str(), O_CREAT | O_RDWR, 0666);
        fd_metadata = shm_open(m_metadata_store_name.c_str(), O_CREAT | O_RDWR, 0666);

        if (fd_data < 0){
            std::cout << "Creating the data shared memory failed" << '\n';
            perror("shm_open()");
            return; 
        }
        if (fd_metadata < 0){
            std::cout << "Creating the metadata shared memory failed" << '\n';
            perror("shm_open()"); 
            return;
        }
        rc = ftruncate(fd_data, data_size);
        if (rc)
        {
          std::cout << "Resizing the data shared memory failed" << '\n';
          return;
        }
        rc = ftruncate(fd_metadata, metadata_size);

        if (rc)
        {
          std::cout << "Resizing the metadata shared memory failed" << '\n';
          return;
        }
        addr_data = static_cast<char*>(mmap(NULL, data_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_data, 0));
        addr_metadata = static_cast<char*>(mmap(NULL, metadata_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_metadata, 0));
        if (addr_data == MAP_FAILED)
        {
          std::cout << "Mapping the data shared memory failed" << '\n';
          return;
        }
        if (addr_metadata == MAP_FAILED)
        {
          std::cout << "Mapping the metadata shared memory failed" << '\n';
          return;
        }
        shm_semaphore = sem_open(m_semaphore_name.c_str(), O_CREAT, 0666, 0);
        if (shm_semaphore == SEM_FAILED) 
        {
          std::cout << "Creating the semaphore failed" << '\n';
          return;
        }
        release_semaphore();
       
    }
    char* shm_data_buffer()
    {
      return (char*)(addr_data);
    }
    char* shm_metadata_buffer()
    {
      return (char*)(addr_metadata);
    }
    void initExistingSharedMemory()
    {
      size_t tryCount = 77;
      while(--tryCount > 0)
      {
        std::lock_guard<std::mutex> L{read_wait_notify_mutex};
        bool error = false;
        shm_semaphore = sem_open(m_semaphore_name.c_str(), 0);
        if (shm_semaphore == SEM_FAILED) 
        {
          error = true;
          std::cout << "Semaphore failed to open " << '\n';
        }
        else 
        {
          fd_data = shm_open(m_data_store_name.c_str(), O_RDWR, 0666);
          fd_metadata = shm_open(m_metadata_store_name.c_str(), O_RDWR, 0666);  
          if(fd_data < 0)
          {
            error = true;
            std::cout << "Loading the data shared memory failed" << '\n';
            perror("shm_open()");
          }
          if(fd_metadata < 0)
          {
            error = true;
            std::cout << "Loading the metadata shared memory failed" << '\n';
            perror("shm_open()");
          }
        }
        if(!error)
        {
           std::cout << "Loaded the shared memory data " << '\n';
           read_wait_notify_cv.notify_all();
           break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(5)); 
      }
      if(tryCount<=0) return; // the shared memeory couldn't be loaded 
      addr_data = static_cast<char*>(mmap(NULL, data_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_data, 0));
      addr_metadata = static_cast<char*>(mmap(NULL, metadata_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_metadata, 0));
      *reinterpret_cast<int32_t*>((char*)(addr_metadata) + sizeof(int32_t)) = m_reader_pos; // init the read pos = 0
      release_semaphore();
    }
    
    template < typename Arg >
    Arg decode() // for literal int, double etc..
    {
      if(m_reader_pos>=high_watermark_pos() || m_reader_pos >= data_size)
      {
        m_reader_pos = 0;
      }
      char* data_buf = shm_data_buffer();
      data_buf +=m_reader_pos;
      Arg arg = *reinterpret_cast < Arg * >(data_buf);
      m_reader_pos += sizeof(arg);
      char* metadata_buf = shm_metadata_buffer();
      *reinterpret_cast<int32_t*>(metadata_buf+sizeof(int32_t)) = m_reader_pos;
      return arg;
    }
    std::tuple<std::string_view, bool >decode_buffer( )
    {
      if(m_reader_pos>=high_watermark_pos() || m_reader_pos >= data_size)
      {
        m_reader_pos = 0;
      }
      char* data_buf = shm_data_buffer();
      data_buf +=m_reader_pos;
      int32_t length = *(reinterpret_cast<int32_t*>(data_buf));
      data_buf += sizeof(length);
      m_reader_pos += (sizeof(length) + length);
      char* metadata_buf = shm_metadata_buffer();
      *reinterpret_cast<int32_t*>(metadata_buf+sizeof(int32_t)) = m_reader_pos;
      return {std::string_view(data_buf, length), (*data_buf == '\r')};
    }
    std::string_view decode_buffer( int32_t length )
    {
      //acquire_semaphore();
      char* data_buf = shm_data_buffer();
      data_buf +=m_reader_pos;
      m_reader_pos += length;
      char* metadata_buf = shm_metadata_buffer();
      *reinterpret_cast<int32_t*>(metadata_buf+sizeof(int32_t)) = m_reader_pos;
      //release_semaphore();
      return std::string_view(data_buf, length);
    }
    bool packet_ends( )
    {
      //acquire_semaphore();
      char* data_buf = shm_data_buffer();
      data_buf +=m_reader_pos;
      int32_t length = *(reinterpret_cast<int32_t*>(data_buf));
      data_buf += sizeof(length);
      //release_semaphore();
      return *data_buf == '\r';
    }
    template<typename Arg>
    bool encode(Arg arg, bool packet_ends)
    {
      if(m_writer_pos < reader_pos() && m_writer_pos + sizeof(arg) >= reader_pos())// wrap-around happened and reader is slow and leading 
      {
        return false;
      }
      if(m_writer_pos + sizeof(arg)> data_size)
      {
        char* metadata_buf = shm_metadata_buffer();
        *reinterpret_cast<int32_t*>(metadata_buf+ 2*sizeof(int32_t)) = m_writer_pos; // update the write high water mark
        m_writer_pos = 0;
      }
      char* data_buf = shm_data_buffer();
      data_buf +=m_writer_pos;
      *reinterpret_cast<Arg*>(data_buf) = arg;
      m_writer_pos += sizeof(arg);
      if(packet_ends)
      {
        char* metadata_buf = shm_metadata_buffer();
        *reinterpret_cast<int32_t*>(metadata_buf) = m_writer_pos;
      }
      return true;
    }
    /*template < typename Arg >
    void encode(Arg arg, bool packet_ends)
    {
	encode < Arg >(arg, packet_ends);
    }
    */
    bool encode_c_string(const char* arg, int32_t len, bool packet_ends ) // make sure arg is nullterminated 
    {
      if(arg==nullptr) return true; // nothing to write

      if(m_writer_pos < reader_pos() && m_writer_pos + sizeof(len) + len >= reader_pos())// wrap-around happened and reader is slow and leading 
      {
        return false;
      }
      if(m_writer_pos + sizeof(len) + len > data_size) 
      {
        char* metadata_buf = shm_metadata_buffer();
        *reinterpret_cast<int32_t*>(metadata_buf+ 2*sizeof(int32_t)) = m_writer_pos; // update the write high water mark
        m_writer_pos = 0;
      }
      char* data_buf = shm_data_buffer();
      data_buf +=m_writer_pos;
      *reinterpret_cast<int32_t*>(data_buf) = (len);
      data_buf += sizeof(len);
      memcpy(data_buf, arg, len); 
      m_writer_pos += (len + sizeof(len));
      if(packet_ends)
      {
        char* metadata_buf = shm_metadata_buffer();
        *reinterpret_cast<int32_t*>(metadata_buf) = m_writer_pos; // update the write position 
      }
      return true;

    }
    bool write(std::string& arg, bool packet_ends= false)
    {
      return encode_c_string(arg.c_str(), arg.length(), packet_ends);
    }

    bool writeFixedBuffer(int8_t arg, bool packet_ends = false)
    {
      return encode<int8_t>(arg, packet_ends); 
    }
    bool writeFixedBuffer(int16_t arg, bool packet_ends = false)
    {
      return encode<int16_t>(arg, packet_ends); 
    }
    bool writeFixedBuffer(int32_t arg, bool packet_ends= false)
    {
      return encode<int32_t>(arg, packet_ends); 
    }
    bool writeFixedBuffer(int64_t arg, bool packet_ends= false)
    {
      return encode<int64_t>(arg, packet_ends);
    }
    bool writeFixedBuffer(double arg, bool packet_ends= false)
    {
      return encode<double>(arg, packet_ends);
    }
    bool writeFixedBuffer(char arg, bool packet_ends= false)
    {
      return encode<char>(arg, packet_ends);
    }
    bool write(char* arg, bool packet_ends= false)
    {
      return encode_c_string(arg, strlen(arg), packet_ends);
    }
    int acquire_semaphore()
    {
      int res = sem_wait(shm_semaphore);
      if(res)
      {
        std::cout << "semaphore lock failed" << '\n';
      }
      return res;
    }
    int release_semaphore()
    {
      int res = sem_post(shm_semaphore);
      if(res)
      {
        std::cout << "semaphore unlock failed" << '\n';
      }
      return res; 
    }
    int32_t reader_pos()
    {
      int32_t pos = *reinterpret_cast<int32_t*>(shm_metadata_buffer()+sizeof(int32_t));// from [sizeof(int32_t), 2*sizeof(int32_t)]
      return pos;
    }
    int32_t writer_pos()
    {
      int32_t pos = *reinterpret_cast<int32_t*>(shm_metadata_buffer());// from [0, sizeof(int32_t)] 
      return pos;
    }
    void reset_reader_pos(int32_t pos)
    {
      *reinterpret_cast<int32_t*>(shm_metadata_buffer()+sizeof(int32_t)) = pos; // update the read position
      m_reader_pos = pos;
    }
    void  reset_writer_pos(int32_t pos)
    {
      *reinterpret_cast<int32_t*>(shm_metadata_buffer()) = pos; // update the write position 
      m_writer_pos = pos;
    }
    int32_t high_watermark_pos()
    {
      int32_t pos = *reinterpret_cast<int32_t*>(shm_metadata_buffer() + 2*sizeof(int32_t));// from [2*sizeof(int32_t), 3*sizeof(int32_t)] 
      return pos;
    }
    void wait_loading_read_shm()
    {
      std::unique_lock<std::mutex> L{read_wait_notify_mutex};
      read_wait_notify_cv.wait(L, [this]{ return (fd_data > 0 && fd_metadata > 0);});
    } 
    int fd_data;
    int fd_metadata;
    void *addr_data;
    void *addr_metadata;
    sem_t *shm_semaphore;
    size_t data_size;
    size_t metadata_size;
    size_t m_writer_pos = 0;
    size_t m_reader_pos = 0;
    std::string m_data_store_name;
    std::string m_metadata_store_name;
    std::string m_semaphore_name;
    std::string m_mode;
    int rc;
    std::thread m_thread;
    std::condition_variable read_wait_notify_cv;
    std::mutex read_wait_notify_mutex; 
};
    
}
} 
