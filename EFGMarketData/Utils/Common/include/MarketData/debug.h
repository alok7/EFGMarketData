#include <mutex>
#include <iostream>

namespace EFG {
  template<bool>
  class Debugger {
    template<typename... Args>
    inline void debug(Args... args) {}
  };

  template<>
  class Debugger<true> {
    private:
    std::mutex print_lock;
    
    public:
    template<typename Arg, typename... Args>
    inline void debug(const Arg& arg, const Args&... args) {
      print_lock.lock();
      MD_LOG_INFO << arg;
      int dummy[sizeof...(args)] = { (MD_LOG_INFO << ' ' << args, 0)... };
      MD_LOG_INFO << "\n";
      print_lock.unlock();
    }
  };
}