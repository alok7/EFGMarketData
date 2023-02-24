#include <MarketData/Logger.h>

namespace
{

    uint64_t timestamp_now()
    {
    	//return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        int64_t epoch_nanos = (uint64_t)ts.tv_sec * 1000000000LL + (uint64_t)ts.tv_nsec;
        return epoch_nanos;
    }

    void format_timestamp(std::ostream & os, uint64_t timestamp)
    {
        using time_point = std::chrono::system_clock::time_point;
        time_point tp{std::chrono::duration_cast<time_point::duration>(std::chrono::nanoseconds(timestamp))};
        std::time_t tm = std::chrono::system_clock::to_time_t(tp);
	auto gmtime = std::gmtime(&tm);
	char buffer[32];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %T.", gmtime);
	char microseconds[7];
	sprintf(microseconds, "%06llu", (unsigned long long)(timestamp % 1000000));
        os << '[' << buffer << microseconds << ']';
    }

    std::thread::id current_thread_id()
    {
	static thread_local const std::thread::id id = std::this_thread::get_id();
	return id;
    }

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

} 

namespace MDLog
{
    typedef std::tuple < char, uint32_t, uint64_t, int32_t, int64_t, double, LogLine::string_literal_t, char *, long long> SupportedTypes;

    std::unique_ptr < Logger > logger;
    std::atomic < Logger * > atomic_logger;
    std::atomic < unsigned int > loglevel = {0};
    bool is_logged(LogLevel level) {return static_cast<unsigned int>(level) >= loglevel.load(std::memory_order_relaxed);}

    bool newLog::operator==(LogLine & logline)
    {
        atomic_logger.load(std::memory_order_acquire)->add(std::move(logline));
        return true;
    }
    void initialize(LoggerType<false> logger_type, std::string const & log_directory, std::string const & log_file_name, uint32_t log_file_roll_size_mb)
    {
        logger.reset(new Logger(logger_type, log_directory, log_file_name, log_file_roll_size_mb));
        atomic_logger.store(logger.get(), std::memory_order_seq_cst);
    }

    char const * to_string(LogLevel loglevel)
    {
	switch (loglevel)
	{
	case LogLevel::INFO:
	    return "INFO";
	case LogLevel::WARN:
	    return "WARN";
	
	}
	return "UNKNOWN";
    }

    template < typename Arg >
    void LogLine::encode(Arg arg)
    {
	*reinterpret_cast<Arg*>(buffer()) = arg;
	m_bytes_used += sizeof(Arg);
    }

    template < typename Arg >
    void LogLine::encode(Arg arg, uint8_t type_id)
    {
	resize_buffer_if_needed(sizeof(Arg) + sizeof(uint8_t));
	encode < uint8_t >(type_id);
	encode < Arg >(arg);
    }

    LogLine::LogLine(LogLevel level, char const * file, char const * function, uint32_t line)
	: m_bytes_used(0)
	, m_buffer_size(sizeof(m_stack_buffer))
    {
	encode < uint64_t >(timestamp_now());
//	encode < std::thread::id >(current_thread_id());
//	encode < string_literal_t >(string_literal_t(file));
	encode < string_literal_t >(string_literal_t(function));
	encode < uint32_t >(line);
	encode < LogLevel >(level);
    }
    LogLine::LogLine(char const * function, uint32_t line)
	: m_bytes_used(0)
	, m_buffer_size(sizeof(m_stack_buffer))
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t epoch_nanos = (uint64_t)ts.tv_sec * 1000000000LL + (uint64_t)ts.tv_nsec;

	*reinterpret_cast<uint64_t*>(buffer()) = epoch_nanos;
	m_bytes_used += sizeof(uint64_t);
	//encode < uint64_t >(epoch_nanos);

	encode < string_literal_t >(string_literal_t(function));

	*reinterpret_cast<uint32_t*>(buffer()) = line;
	m_bytes_used += sizeof(uint32_t);
	//encode < uint32_t >(line);
    }
    LogLine::~LogLine() = default;

    void LogLine::stringify(std::ostream & os)
    {
	char * b = !m_heap_buffer ? m_stack_buffer : m_heap_buffer.get();
	char const * const end = b + m_bytes_used;
	uint64_t timestamp = *reinterpret_cast < uint64_t * >(b); b += sizeof(uint64_t);
//	std::thread::id threadid = *reinterpret_cast < std::thread::id * >(b); b += sizeof(std::thread::id);
//	string_literal_t file = *reinterpret_cast < string_literal_t * >(b); b += sizeof(string_literal_t);
	string_literal_t function = *reinterpret_cast < string_literal_t * >(b); b += sizeof(string_literal_t);
	uint32_t line = *reinterpret_cast < uint32_t * >(b); b += sizeof(uint32_t);
	//LogLevel loglevel = *reinterpret_cast < LogLevel * >(b); b += sizeof(LogLevel);
	format_timestamp(os, timestamp);
        os << '[' << function.m_s << ':' << line << "] ";
	stringify(os, b, end);
    }

    template < typename Arg >
    char * decode(std::ostream & os, char * b, Arg * dummy)
    {
	Arg arg = *reinterpret_cast < Arg * >(b);
	os << arg;
	return b + sizeof(Arg);
    }

    template <>
    char * decode(std::ostream & os, char * b, LogLine::string_literal_t * dummy)
    {
	LogLine::string_literal_t s = *reinterpret_cast < LogLine::string_literal_t * >(b);
	os << s.m_s;
    // os.flush();
	return b + sizeof(LogLine::string_literal_t);
    }

    template <>
    char * decode(std::ostream & os, char * b, char ** dummy)
    {
	size_t length = *reinterpret_cast<size_t*>(b);
	b += sizeof(length);
	while (length--)
	{
	    os << *b;
	    ++b;
	}
	return b;
    }
    
    size_t LogLine::writeBytes(char const* arg, size_t length) {
      encode_c_string(arg, length);
      return length;
    }

    void LogLine::stringify(std::ostream & os, char * start, char const * const end)
    {
	if (start == end)
	    return;

	int type_id = static_cast < int >(*start); start++;
	
	switch (type_id)
	{
	case 0:
	    stringify(os, decode(os, start, static_cast<std::tuple_element<0, SupportedTypes>::type*>(nullptr)), end);
	    return;
	case 1:
	    stringify(os, decode(os, start, static_cast<std::tuple_element<1, SupportedTypes>::type*>(nullptr)), end);
	    return;
	case 2:
	    stringify(os, decode(os, start, static_cast<std::tuple_element<2, SupportedTypes>::type*>(nullptr)), end);
	    return;
	case 3:
	    stringify(os, decode(os, start, static_cast<std::tuple_element<3, SupportedTypes>::type*>(nullptr)), end);
	    return;
	case 4:
	    stringify(os, decode(os, start, static_cast<std::tuple_element<4, SupportedTypes>::type*>(nullptr)), end);
	    return;
	case 5:
	    stringify(os, decode(os, start, static_cast<std::tuple_element<5, SupportedTypes>::type*>(nullptr)), end);
	    return;
	case 6:
	    stringify(os, decode(os, start, static_cast<std::tuple_element<6, SupportedTypes>::type*>(nullptr)), end);
	    return;
	case 7:
	    stringify(os, decode(os, start, static_cast<std::tuple_element<7, SupportedTypes>::type*>(nullptr)), end);
	    return;
    case 8:
	    stringify(os, decode(os, start, static_cast<std::tuple_element<8, SupportedTypes>::type*>(nullptr)), end);
	    return;
    // case 9:
	//     stringify(os, decode(os, start, static_cast<std::tuple_element<9, SupportedTypes>::type*>(nullptr)), end);
	//     return;
	}
    }
    
    void LogLine::resize_buffer_if_needed(size_t additional_bytes)
    {
	size_t const required_size = m_bytes_used + additional_bytes;

	if (likely(required_size <= m_buffer_size))
	    return;

	if (!m_heap_buffer)
	{
	    m_buffer_size = std::max(static_cast<size_t>(512), required_size);
	    m_heap_buffer.reset(new char[m_buffer_size]);
	    memcpy(m_heap_buffer.get(), m_stack_buffer, m_bytes_used);
	    return;
	}
	else
	{
	    m_buffer_size = std::max(static_cast<size_t>(2 * m_buffer_size), required_size);
	    std::unique_ptr < char [] > new_heap_buffer(new char[m_buffer_size]);
	    memcpy(new_heap_buffer.get(), m_heap_buffer.get(), m_bytes_used);
	    m_heap_buffer.swap(new_heap_buffer);
	}
    }

    void LogLine::encode(char const * arg)
    {
	if (arg != nullptr)
	    encode_c_string(arg, strlen(arg));
    }

    void LogLine::encode(char * arg)
    {
	if (arg != nullptr)
	    encode_c_string(arg, strlen(arg));
    }

    void LogLine::encode_c_string(char const * arg, size_t length)
    {
	if (length == 0)
	    return;
	
	resize_buffer_if_needed(1 + sizeof(length) + length);
	char * b = buffer();
	auto type_id = TupleIndex < char *, SupportedTypes >::value;
	*reinterpret_cast<uint8_t*>(b++) = static_cast<uint8_t>(type_id);
	*reinterpret_cast<size_t*>(b) = length;
	b += sizeof(length);
	memcpy(b, arg, length);
	m_bytes_used += 1 + sizeof(length) + length;
    }

    void LogLine::encode(string_literal_t arg)
    {
	encode < string_literal_t >(arg, TupleIndex < string_literal_t, SupportedTypes >::value);
    }

    LogLine& LogLine::operator<<(std::string const & arg)
    {
	encode_c_string(arg.c_str(), arg.length());
	return *this;
    }

    LogLine& LogLine::operator<<(int32_t arg)
    {
	encode < int32_t >(arg, TupleIndex < int32_t, SupportedTypes >::value);
	return *this;
    }

    LogLine& LogLine::operator<<(uint32_t arg)
    {
	encode < uint32_t >(arg, TupleIndex < uint32_t, SupportedTypes >::value);
	return *this;
    }

    LogLine& LogLine::operator<<(int64_t arg)
    {
	encode < int64_t >(arg, TupleIndex < int64_t, SupportedTypes >::value);
	return *this;
    }

    LogLine& LogLine::operator<<(uint64_t arg)
    {
	encode < uint64_t >(arg, TupleIndex < uint64_t, SupportedTypes >::value);
	return *this;
    }

    LogLine& LogLine::operator<<(double arg)
    {
	encode < double >(arg, TupleIndex < double, SupportedTypes >::value);
	return *this;
    }

    LogLine& LogLine::operator<<(char arg)
    {
	encode < char >(arg, TupleIndex < char, SupportedTypes >::value);
	return *this;
    }

    LogLine& LogLine::operator<<(long long arg)
    {
	encode < long long >(arg, TupleIndex < long long, SupportedTypes >::value);
	return *this;
    }

    void FileWriter::write(LogLine & logline) 
    {
      auto pos = m_os->tellp();
      logline.stringify(*m_os);
      m_bytes_written += m_os->tellp() - pos;
    }

    void FileWriter::roll_file() {
      
      if (m_os) {
        m_os->flush();
	m_os->close();
      }

      m_bytes_written = 0;
      m_os.reset(new std::ofstream());
      std::string log_file_name = m_name;
      if(m_file_number>0)
      {
        log_file_name.append(".");
        log_file_name.append(std::to_string(++m_file_number));
        log_file_name.append(".log");
      }
      else
      {
        log_file_name.append(".log");
      }
      m_os->open(log_file_name, std::ios::out | std::ios::app | std::ios::binary);
   }
   void Logger::pop() {
     while (m_state.load(std::memory_order_acquire) == State::INIT)
       std::this_thread::sleep_for(std::chrono::microseconds(50));
	    
     LogLine logline(LogLevel::INFO, nullptr, nullptr, 0);

     while (likely(m_state.load() == State::READY)) 
     {
       if (likely(m_buffer_base->try_pop(logline))) 
       {
           m_file_writer.write(logline);
       }
       else std::this_thread::sleep_for(std::chrono::microseconds(50));
     }	    
     // Pop and log all remaining entries
     while (m_buffer_base->try_pop(logline)) {
       m_file_writer.write(logline);
     }
   }


} 
