#pragma once
#include <type_traits>
#include <cstring>
#include <chrono>
#include <ctime>
#include <thread>
#include <tuple>
#include <atomic>
#include <queue>
#include <fstream>
#include <cstring>
#include <chrono>
#include <ctime>
#include <thread>
#include <tuple>
#include <atomic>
#include <queue>
#include <fstream>
#include <cstring>
#include <chrono>
#include <ctime>
#include <thread>
#include <tuple>
#include <atomic>
#include <queue>
#include <fstream>
#include <cstdint>
#include <memory>
#include <string>
#include <iosfwd>
#include <iomanip>
#include <iostream>
#include <cassert>

namespace MDLog
{
#ifndef likely
  #define likely(x) __builtin_expect((x), 1)
#endif

#ifndef unlikely
  #define unlikely(x) __builtin_expect((x), 0)
#endif   

   enum class LogLevel : uint8_t { INFO, WARN, CRIT};
    
    void set_log_level(LogLevel level);
    
    bool is_logged(LogLevel level);

    template<bool isDeterminstic>
    struct LoggerType;
    template<>
    struct LoggerType<false>
    {
	LoggerType(uint32_t ring_buffer_size_mb_) : ring_buffer_size_mb(ring_buffer_size_mb_) {}
	uint32_t ring_buffer_size_mb;
    };
    template<>
    struct LoggerType<true>
    {
       // using queue
    };

    void initialize(LoggerType<true> , std::string const & , std::string const & , uint32_t);
    void initialize(LoggerType<false> , std::string const & , std::string const & , uint32_t);
    
    class LogLine
    {
    public:
	LogLine(LogLevel level, char const * file, char const * function, uint32_t line);
	LogLine(char const * function, uint32_t line);
	~LogLine();

	LogLine(LogLine &&) = default;
	LogLine& operator=(LogLine &&) = default;
	LogLine& operator<<(char arg);
	LogLine& operator<<(int32_t arg);
	LogLine& operator<<(uint32_t arg);
	LogLine& operator<<(int64_t arg);
	LogLine& operator<<(uint64_t arg);
	LogLine& operator<<(double arg);
	LogLine& operator<<(long long arg);
	LogLine& operator<<(std::string const & arg);
	size_t writeBytes(char const * arg, size_t length);

	void stringify(std::ostream & os);
	
        template < size_t N >
	__attribute__((always_inline)) LogLine& operator<<(const char (&arg)[N])
	{
	    encode(string_literal_t(arg));
	    return *this;
	}

	template < typename Arg >
	__attribute__((always_inline)) typename std::enable_if < std::is_same < Arg, char const * >::value, LogLine& >::type
	operator<<(Arg const & arg)
	{
	    encode(arg);
	    return *this;
	}

	template < typename Arg >
	__attribute__((always_inline)) typename std::enable_if < std::is_same < Arg, char * >::value, LogLine& >::type
	operator<<(Arg const & arg)
	{
	    encode(arg);
	    return *this;
	}

	struct string_literal_t
	{
	    explicit string_literal_t(char const * s) : m_s(s) {}
	    char const * m_s;
	};

    private:	
	__attribute__((always_inline)) char * buffer()
        {
	  return !m_heap_buffer ? &m_stack_buffer[m_bytes_used] : &(m_heap_buffer.get())[m_bytes_used];
        }

	template < typename Arg >
	void encode(Arg arg);

	template < typename Arg >
	void encode(Arg arg, uint8_t type_id);

	void encode(char * arg);
	void encode(char const * arg);
	void encode(string_literal_t arg);
	void encode_c_string(char const * arg, size_t length);
	void resize_buffer_if_needed(size_t additional_bytes);
	void stringify(std::ostream & os, char * start, char const * const end);

    private:
	size_t m_bytes_used;
	size_t m_buffer_size;
	std::unique_ptr < char [] > m_heap_buffer;
	char m_stack_buffer[512 - 2 * sizeof(size_t) - sizeof(decltype(m_heap_buffer)) - 8 /* Reserved */];
    };

	struct BufferBase 
        {
          virtual ~BufferBase() = default;
        };

    struct SpinLock 
    {
       SpinLock(std::atomic_flag & flag) : m_flag(flag) { while (m_flag.test_and_set(std::memory_order_acquire)); }
       ~SpinLock() { m_flag.clear(std::memory_order_release); }

       private:
         std::atomic_flag & m_flag;
    };

    class RingBuffer : public BufferBase {
    public:
      struct alignas(64) Item 
      {
         Item() : flag{ ATOMIC_FLAG_INIT }, written(0), logline(LogLevel::INFO, nullptr, nullptr, 0) {}
	    
	 std::atomic_flag flag;
	 char written;
	 char padding[512 - sizeof(std::atomic_flag) - sizeof(char) - sizeof(LogLine)];
	 LogLine logline;
      };
	
      RingBuffer(size_t const size) 
            : m_size(size)
    	    , m_ring(static_cast<Item*>(std::malloc(size * sizeof(Item))))
    	    , m_write_index(0)
    	    , m_read_index(0) 
      {
    	    for (size_t i = 0; i < m_size; ++i) new (&m_ring[i]) Item();
	    	static_assert(sizeof(Item) == 512, "Expected size 512");
            assert(!(size == 0) && !(size & (size - 1)));
      }

      ~RingBuffer() 
      {
        for (size_t i = 0; i < m_size; ++i) m_ring[i].~Item();
    	    std::free(m_ring);
      }

      // mask = m_size -1//
      __attribute__((always_inline)) void push(LogLine && logline)
      {
        unsigned int write_index = m_write_index.fetch_add(1, std::memory_order_relaxed) & (m_size-1);
        Item & item = m_ring[write_index];
        SpinLock spinlock(item.flag);
        item.logline = std::move(logline);
        item.written = 1;
      }
      __attribute__((always_inline)) bool try_pop(LogLine & logline)
      {
        Item & item = m_ring[m_read_index & (m_size-1)];
        SpinLock spinlock(item.flag);
        if (item.written == 1) {
          logline = std::move(item.logline);
	  item.written = 0;
          ++m_read_index;
	  return true;
        }
        return false;
      }

      RingBuffer(RingBuffer const &) = delete;	
      RingBuffer& operator=(RingBuffer const &) = delete;

      private:
    	size_t const m_size;
    	Item * m_ring;
    	std::atomic < unsigned int > m_write_index;
        char pad[64];
    	unsigned int m_read_index;
    };

    class Buffer {
      public:
        struct Item 
        {
	  Item(LogLine && logline) : logline(std::move(logline)) {}
	  char padding[512 - sizeof(LogLine)];
	   LogLine logline;
    	};

        static constexpr const size_t size = 32768; // 8MB

    	Buffer() : m_buffer(static_cast<Item*>(std::malloc(size * sizeof(Item)))) {
    	    for (size_t i = 0; i <= size; ++i) m_write_state[i].store(0, std::memory_order_relaxed);
	    	static_assert(sizeof(Item) == 512, "Unexpected size != 512");
    	}
    	~Buffer() {
	    unsigned int write_count = m_write_state[size].load();
    	    for (size_t i = 0; i < write_count; ++i) m_buffer[i].~Item();
    	    std::free(m_buffer);
    	}

    	Buffer(Buffer const &) = delete;	
    	Buffer& operator=(Buffer const &) = delete;

    	private:
    	Item * m_buffer;
		std::atomic < unsigned int > m_write_state[size + 1];
    };

    class FileWriter 
    {
      public:
	FileWriter(std::string const & log_directory, std::string const & log_file_name, uint32_t log_file_roll_size_mb)
	    : m_log_file_roll_size_bytes(log_file_roll_size_mb * 1024 * 1024)
	     , m_name(log_directory + log_file_name) 
       {
         roll_file(); // first time for create one file - theen roll every 24hr 
         m_os->precision(15); 
       }
	void write(LogLine &);
	std::unique_ptr < std::ofstream > m_os;

     private:
       void roll_file();

       uint32_t m_file_number = 0;
       std::streamoff m_bytes_written = 0;
       uint32_t const m_log_file_roll_size_bytes;
       std::string const m_name;
       
    };

	class Logger {
		public:
		Logger(LoggerType<false> logger_type, std::string const & log_directory, std::string const & log_file_name, uint32_t log_file_roll_size_mb)
			: m_state(State::INIT)
			, m_buffer_base(new RingBuffer(std::max(4u, logger_type.ring_buffer_size_mb) * 1024 * 4))
			, m_file_writer(log_directory, log_file_name, std::max(1u, log_file_roll_size_mb))
			, m_thread(&Logger::pop, this) {
			m_state.store(State::READY, std::memory_order_release);
		}
                ~Logger() {
			m_state.store(State::SHUTDOWN);
			m_thread.join();
		}

		__attribute__((always_inline)) void add(LogLine && logline) { m_buffer_base->push(std::move(logline)); }
		void pop();
		private:
		enum class State { INIT, READY, SHUTDOWN };
		std::atomic < State > m_state;
		std::unique_ptr < RingBuffer > m_buffer_base;
		FileWriter m_file_writer;
		std::thread m_thread;
	};
    
	template<bool isDeterministic>
        struct Log {
		public:
		Log(LoggerType<isDeterministic> logger_type, std::string const & log_directory, std::string const & log_file_name, uint32_t log_file_roll_size_mb) {
			logger.reset(new Logger(logger_type, log_directory, log_file_name, log_file_roll_size_mb));
			atomic_logger.store(logger.get(), std::memory_order_seq_cst);
		}
		void set_log_level(LogLevel level) {loglevel.store(static_cast<unsigned int>(level), std::memory_order_release);}
		bool is_logged(LogLevel level) {return static_cast<unsigned int>(level) >= loglevel.load(std::memory_order_relaxed);}
		__attribute__((always_inline)) bool operator==(LogLine & logline) {
			atomic_logger.load(std::memory_order_acquire)->add(std::move(logline));
			return true;
		}
		
		private:
		std::unique_ptr < Logger > logger;
		std::atomic < Logger * > atomic_logger;
		std::atomic < unsigned int > loglevel = {0};
        };

	struct newLog {
		bool operator==(LogLine &);
	};

	class Duration
        {
        private:
            using clock = std::chrono::high_resolution_clock;
            using sec = std::chrono::duration<double, std::ratio<1> >;        
            std::chrono::time_point<clock> now;        
        public:
            Duration() : now(clock::now())
            {
            }        
            void reset()
            {
                now = clock::now();
            }        
            double elapsed() const
            {
                return std::chrono::duration_cast<sec>(clock::now() - now).count();
            }
        };
 
} 

#define MD_LOG_LEVEL(LEVEL) MDLog::newLog() == MDLog::LogLine(__func__, __LINE__)
#define MD_LOG_INFO MD_LOG_LEVEL(MDLog::LogLevel::INFO)
#define MD_LOG_WARN MDLog::is_logged(MDLog::LogLevel::WARN) && MD_LOG_LEVEL(MDLog::LogLevel::WARN)
