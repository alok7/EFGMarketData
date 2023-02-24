#pragma once

#include <iostream>
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <cmath>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace EFG
{
namespace Core
{
namespace CSV
{

template<int columnCount>
class Reader
{
public:
  Reader(){}
  ~Reader()
  {
    if(mData)
    {
      munmap(const_cast<char*>(mData), mFileSize);
    }
    close(mFileHandle);
  }
  void setLineFilterFn(std::function<bool(const char*)>&& fn_){mLineFilter = fn_;}
  void setColumnFilerFn(const std::string& columnName, std::function<bool(const char*)>&& fn_)
  {
    mFilterColumn = columnName; mColumnFilter = fn_;
  }
  size_t getFileSize() const
  {
    return mFileSize;
  }
  const std::string &getLastError() const
  {
    return mLastError;
  }
  int getLineNo() const
  {
    return mLineNo;
  }
  template<class... ColNames>
  bool init(const std::string& filename_, ColNames... cols)
  {
    static_assert(sizeof...(ColNames) >=columnCount, "not enough columns");
    static_assert(sizeof...(ColNames) <=columnCount, "too many columns");
    setRequiredColumns(std::forward<ColNames>(cols)...);
    mFileName = filename_;
    mFileHandle = open(mFileName.c_str(), O_RDONLY);
    if(mFileHandle == (int)-1)
    {
      setError("failed to open file " + mFileName);
      return false;
    }
    mFileSize = lseek(mFileHandle, 0, SEEK_END);
    if(mFileSize==0 || (mFileSize==(off_t)-1))
    {
      mData = mReadPtr = mEndPtr = nullptr;
      setError("empty csv file " + mFileName);
      return false;
    }
    lseek(mFileHandle, 0, SEEK_SET);
    mData = (const char*)mmap(0, mFileSize, PROT_READ, MAP_PRIVATE, mFileHandle, 0);
    if(mData==MAP_FAILED)
    {
      setError("failed to mmap, file " + mFileName);
      return false;
    }
    mReadPtr = mData;
    mEndPtr = mData + mFileSize;
    mInited = readHeader();
    return mInited;
  }
  template<class... ColType>
  bool readRow(ColType &... cols)
  {
    static_assert(sizeof...(ColType) >=columnCount, "not enough columns");
    static_assert(sizeof...(ColType) <=columnCount, "too many columns");
    if(!mInited)
    {
      std::cerr << "ERROR: csvReader is not inited, " << mFileName << std::endl;
      return false;
    }
    try
    {
      do
      {
        mReadPtr = nextLine(mReadPtr);
        if(mReadPtr==nullptr)
        {
          return false;
        }
        mReadPtr = parseLine(mReadPtr);
        if(mColumnFilter)
        {
          std::string s;
          size_t columnIndex = mColumnMap[mFilterColumn];
          decode(mColumnInfo[columnIndex].first, mColumnInfo[columnIndex].second, s);
          if(!mColumnFilter(s)) continue;
        }
        readColumn(0, cols...);
        return true;
      }
      while(mReadPtr!=nullptr);

    }
    catch(...)
    {
      setError("error at file " + mFileName + " line " + std::to_string(mLineNo));
      return false;
    }
    return false;
  }
private:
  const char* nextLine(const char* ptr_)
  {
    do
    {
      // find line end
      while(ptr_ < mEndPtr && !isLineEnd(*ptr_))
        ++ptr_;
      // skip line end
      while(ptr_ < mEndPtr && isLineEnd(*ptr_))
        ++ptr_;
      // no more to read
      if(ptr_==mEndPtr)
        return nullptr;
      
      ++mLineNo;
      if(isLineCommentOrEmpty(ptr_)) 
      {
        continue;
      }
      if(mLineFilter) // if header has been read
      {
        if(mLineFilter(ptr_)) return ptr_;
      }
      else
        return ptr_;
    }
    while(true);
  }
  const char* parseLine(const char* lineStart_)
  {
    const char* start = lineStart_;
    const char* end = start;
    if(mColumnInfo.empty())
    {
      while(!isLineEnd(*end) && end < mEndPtr)
      {
        if(isColumnEnd(*end))
        {
          mColumnInfo.push_back({start, end});
          start = ++end;
        }
        else
        {
          ++end;
        }
      }
      mColumnInfo.push_back({start, end});
    }
    else
    {
      int index = 0;
      while(!isLineEnd(*end) && end < mEndPtr)
      {
        if(isColumnEnd(*end))
        {
          mColumnInfo[index].first = start;
          mColumnInfo[index].second = end;
          ++index;
          start = ++end;
        }
        else
          ++end;
      }
      mColumnInfo[index].first = start;
      mColumnInfo[index].second = end;
    }
    return end;
  }
  void decode(const char* start_, const char* end_, std::string &v_)
  {
    start_ = trimLeftSpace(start_);
    if(isColumnEnd(*start_))
    {
      v_ = "";
      return;
    }
    end_ = trimRightSpace(--end_);
    if(start_ > end_)
    {
      v_ = "";
    }
    else
    {
      v_ = std::string(start_, end_-start_+1);
    }
  }
  void decodeAsDefault(std::string& v_)
  {
    v_ = "";
  }
 
  void decode(const char* start_, const char* end_, uint16_t& v_)
  {
    start_ = trimLeftSpace(start_);
    v_ = 0;
    while(isdigit(*start_))
    {
      v_= v_*10 + (*start_ - '0');
      ++start_;
    }
  }
  void decodeAsDefault(uint16_t & v_)
  {
    v_ = 0;
  }

  void decode(const char* start_, const char* end_, uint32_t& v_)
  {
    start_ = trimLeftSpace(start_);
    v_ = 0;
    while(isdigit(*start_))
    {
      v_= v_*10 + (*start_ - '0');
      ++start_;
    }
  }
  void decodeAsDefault(uint32_t & v_)
  {
    v_ = 0;
  }

  void decode(const char* start_, const char* end_, uint64_t& v_)
  {
    start_ = trimLeftSpace(start_);
    v_ = 0;
    while(isdigit(*start_))
    {
      v_= v_*10 + (*start_ - '0');
      ++start_;
    }
  }
  void decodeAsDefault(uint64_t & v_)
  {
    v_ = 0;
  }

  void decode(const char* start_, const char* end_, int16_t& v_)
  {
    start_ = trimLeftSpace(start_);
    v_ = 0;
    bool isPositive = true;
    if(*start_=='-')
    {
      isPositive = false;
      ++start_;
    }
    else if(*start_=='+')
    {
      ++start_;
    }
    while(isdigit(*start_))
    {
      v_= v_*10 + (*start_ - '0');
      ++start_;
    }
    v_ = isPositive ? v_ : -v_;
  }
  void decodeAsDefault(int16_t & v_)
  {
    v_ = 0;
  }

  void decode(const char* start_, const char* end_, int32_t& v_)
  {
    start_ = trimLeftSpace(start_);
    v_ = 0;
    bool isPositive = true;
    if(*start_=='-')
    {
      isPositive = false;
      ++start_;
    }
    else if(*start_=='+')
    {
      ++start_;
    }
    while(isdigit(*start_))
    {
      v_= v_*10 + (*start_ - '0');
      ++start_;
    }
    v_ = isPositive ? v_ : -v_;
  }
  void decodeAsDefault(int32_t & v_)
  {
    v_ = 0;
  }

  void decode(const char* start_, const char* end_, int64_t& v_)
  {
    start_ = trimLeftSpace(start_);
    v_ = 0;
    bool isPositive = true;
    if(*start_=='-')
    {
      isPositive = false;
      ++start_;
    }
    else if(*start_=='+')
    {
      ++start_;
    }
    while(isdigit(*start_))
    {
      v_= v_*10 + (*start_ - '0');
      ++start_;
    }
    v_ = isPositive ? v_ : -v_;
  }
  void decodeAsDefault(int64_t & v_)
  {
    v_ = 0;
  }

  void decode(const char* start_, const char* end_, float& v_)
  {
    start_ = trimLeftSpace(start_);
    v_ = 0.0;
    bool isPositive = true;
    if(*start_=='-')
    {
      isPositive = false;
      ++start_;
    }
    else if(*start_=='+')
    {
      ++start_;
    }
    else if(*start_=='n')
    {
      if(*(start_+1)=='a' && *(start_+2)=='n')
        v_ = 0.0;
    }
    while(isdigit(*start_))
    {
      v_= v_*10 + (*start_ - '0');
      ++start_;
    }
    if(*start_=='.')
    {
      ++start_;
      int64_t pos = 10;
      while(isdigit(*start_))
      {
        v_ = (float)(*start_ - '0')/pos;
        pos *=10;
        ++start_;
      }
    }
    if(*start_=='e' || *start_=='E')
    {
      ++start_;
      int e;
      decode(start_, end_, e);
      if(e!=0)
      {
        float base;
        if(e < 0)
        {
          base = 0.1;
          e = -e;
        }
        else 
        {
          base = 10;
        }
        while(e!=1)
        {
          if((e&1)==0)
          {
            base = base * base;
            e >>=1;
          }
          else 
          {
            v_ *=base;
            --e;
          }
        }
        v_ *=base;
      }
    }
    if(!isPositive)
    {
      v_ = 0-v_;
    }
  }
  void decodeAsDefault(float & v_)
  {
    v_ = 0.0;
  }

  void decode(const char* start_, const char* end_, double& v_)
  {
    start_ = trimLeftSpace(start_);
    v_ = 0;
    bool isPositive = true;
    if(*start_=='-')
    {
      isPositive = false;
      ++start_;
    }
    else if(*start_=='+')
    {
      ++start_;
    }
    else if(*start_=='n')
    {
      if(*(start_+1)=='a' && *(start_+2)=='n')
        v_ = 0.0;
    }
    while(isdigit(*start_))
    {
      v_= v_*10 + (*start_ - '0');
      ++start_;
    }
    if(*start_=='.')
    {
      ++start_;
      int64_t pos = 10;
      while(isdigit(*start_))
      {
        v_ = (double)(*start_ - '0')/pos;
        pos *=10;
        ++start_;
      }
    }
    if(*start_=='e' || *start_=='E')
    {
      ++start_;
      int e;
      decode(start_, end_, e);
      if(e!=0)
      {
        double base;
        if(e < 0)
        {
          base = 0.1;
          e = -e;
        }
        else 
        {
          base = 10;
        }
        while(e!=1)
        {
          if((e&1)==0)
          {
            base = base * base;
            e >>=1;
          }
          else 
          {
            v_ *=base;
            --e;
          }
        }
        v_ *=base;
      }
    }
    if(!isPositive)
    {
      v_ = 0-v_;
    }
  }
  void decodeAsDefault(double & v_)
  {
    v_ = 0.0;
  }
  void decode(const char* start_, const  char* end_, unsigned char& v_)
  {
    start_ = trimLeftSpace(start_);
    if(start_ >=end_)
    {
      v_ = ' ';
    }
    else 
      v_ = (unsigned char)*start_;
  }
  void decodeDefault(unsigned char& v_)
  {
    v_ = ' ';
  }  

  void decode(const char* start_, const  char* end_, char& v_)
  {
    start_ = trimLeftSpace(start_);
    if(start_ >=end_)
    {
      v_ = ' ';
    }
    else 
      v_ = *start_;
  }
  void decodeDefault(char& v_)
  {
    v_ = ' ';
  }  

  void decode(const char* start_, const  char* end_, char*& v_)
  {
    char* p = v_;
    start_ = trimLeftSpace(start_);
    if(isColumnEnd(*start_) || isLineEnd(*start_))
    {
      *p = '\0';
      return;
    }
    end_ = trimRightSpace(end_-1);
    while(start_ <=end_)
    {
      *p = *start_;
      ++start_;
      ++p;
    }
    *p = '\0';
  }
  void decodeDefault(char*& v_)
  {
    v_ = '\0';
  } 

  template<size_t N, typename T>
  void decode(const char* start_, const char* end_, std::array<T, N>& v_)
  {
    v_.fill(0);
    start_ = trimLeftSpace(start_);
    if(isColumnEnd(*start_) || isLineEnd(*start_)) return;
    end_ = trimRightSpace(end_ -1);
    for(int i = 0; start_ <= end_ && i < v_.size(); ++i, ++start_)
    {
      v_[i] = *start_;
    }
  } 
  template<size_t N, typename T>
  void decodeAsDefault(std::array<T, N>& v_)
  {
    v_.fill(0);
  }
  
  void decode(const char* start_, const char* end_, bool& v_)
  {
    start_ = trimLeftSpace(start_);
    if(*start_=='1')
      v_ = true;
    else if(*start_=='0')
      v_ = false;
    else if(toupper(start_[0])=='Y' && toupper(start_[1])=='E' && toupper(start_[2])=='s')
      v_ = true;
    else if(toupper(start_[0])=='T' && toupper(start_[1])=='R' && toupper(start_[2])=='U' && toupper(start_[3])=='E')
      v_ = true;
    else
      v_ = false; 
  } 
  void decodeAsDefault(bool& v_)
  {
    v_ = false;
  }

  static const char* trimLeftSpace(const char* ptr_)
  {
    while(*ptr_==' ')
     ++ptr_;
    return ptr_;
  }  
  static const char* trimRightSpace(const char* ptr_)
  {
    while(*ptr_==' ')
      --ptr_;
    return ptr_;
  }

  static bool isLineCommentOrEmpty(const char* ptr_)
  {
    ptr_ = trimLeftSpace(ptr_);
    return isComment(*ptr_) || isLineEnd(*ptr_);
  }

  static bool isColumnEnd(char ch){ return ch==',';}
  static bool isLineEnd(char ch) { return ch== 0xD || ch == 0xA;}
  static bool isComment(char ch) { return ch == '#';}

  void setError(const std::string& errInfo_)
  {
    mLastError = errInfo_;
    if(mLineNo>=0)
      mLastError += " at line " + std::to_string(mLineNo);

    std::cerr << mLastError << std::endl;
  }
  
  template<class... ColNames>
  void setRequiredColumns(std::string s, ColNames... cols)
  {
    mRequiredColumns[columnCount - sizeof...(ColNames) -1] = std::move(s);
    setRequiredColumns(std::forward<ColNames>(cols)...);
  }
  void setRequiredColumns(){}

  bool readHeader()
  {
    if(!mReadPtr)
    {
      setError("empty csv file ");
      return false;
    }
    while(isLineCommentOrEmpty(mReadPtr))
    {
      setError("no valid line in csv file");
      return false;
    }
    mReadPtr = parseLine(mReadPtr);
    for(size_t i = 0; i < mColumnInfo.size(); ++i)
    {
      std::string name;
      decode(mColumnInfo[i].first, mColumnInfo[i].second, name);
      if(name.empty())
      {
        continue; // ignore column without name
      }
      mColumnMap[name] = i;
    }
    for(int i =0; i < columnCount; ++i)
    {
      if(mColumnMap.count(mRequiredColumns[i]) < 1)
      {
        setError("can't find column " + mRequiredColumns[i]);
        mRequiredColumnIndex[i] = columnCount*1000;
      }
      else
      {
        mRequiredColumnIndex[i] = mColumnMap[mRequiredColumns[i]];
      }
    }
    return true;
  }  
  void readColumn(size_t t){}
  
  template<class T, class... ColTypes>
  void readColumn(size_t index, T &t, ColTypes&... cols)
  {
    size_t columnIndex = mRequiredColumnIndex[index];
    if(columnIndex!=columnCount*1000)
    {
      decode(mColumnInfo[columnIndex].first, mColumnInfo[columnIndex].second, t);
    }
    else 
    {
      decodeAsDefault(t);
    }
    readColumn(index+1, cols...);
  } 
private:
  std::function<bool(const char*)> mLineFilter;
  std::function<bool(const std::string&)> mColumnFilter;
  std::string mFilterColumn;

  std::string mFileName;
  int mFileHandle = {-1};
  size_t mFileSize = {0};
  int mLineNo = {0};
  const char* mData = {nullptr};
  const char* mReadPtr = {nullptr};
  const char* mEndPtr; 
  bool mInited = {false};
  std::string mLastError;
  std::string mRequiredColumns[columnCount];
  int mRequiredColumnIndex[columnCount];
  
  std::unordered_map<std::string, int> mColumnMap;
  std::vector<std::pair<const char*, const char*>> mColumnInfo;

};
 
}
}
}
