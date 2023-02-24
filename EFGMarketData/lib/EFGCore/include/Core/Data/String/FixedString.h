#pragma once

#include <cstring>
#include <cassert>
#include <stdexcept>
#include <string>

namespace EFG
{
namespace Core
{
namespace Data
{

template <std::size_t N>
struct FixedString
{
	char data_[N + 1];
	FixedString()
	{
	  std::memset(data_, '\0', sizeof(data_));
	}
	FixedString(const char * data)
	{
	  std::strncpy(data_, data, N);
	}
	template <std::size_t M>
	FixedString(FixedString<M> const & x)
	{
	  std::strncpy(data_, x.data(), N);
	}
	FixedString& operator=(const char * data)
	{
	  std::strncpy(data_, data, N);
          return *this;
	}
	FixedString& operator=(const std::string& data)
	{
	  std::strncpy(data_, data.c_str(), N);
          return *this;
	}
	bool operator==(const char * rhs)
	{
	  return std::strncmp(data_, rhs, N) == 0;
	}
	template <std::size_t M>
	bool operator==(FixedString<M> const & rhs)
	{
	  return std::strncmp(data_, rhs.data(), N) == 0;
	}
	bool operator!=(const char * rhs)
	{
	  return std::strncmp(data_, rhs, N) != 0;
	}
	template <std::size_t M>
	bool operator!=(FixedString<M> const & rhs)
	{
	  return std::strncmp(data_, rhs.data(), N) != 0;
	}
	char operator[](std::size_t position)
	{
	  assert(position < N && "Out of range");
	  return data_[position];
	}
	char at(std::size_t position)
	{
	  if (position >= N)
	  {
	    throw std::out_of_range("Out of range");
	  }
	  return data_[position];
	}
	char * data()
	{
	  return data_;
	}
	const char * data() const
	{
	  return data_;
	}
        std::string toString()
        {
          return std::string(data_);
        }
	operator bool()
	{
	  assert(N >= 1);
	  return data_[0];
	}
	std::size_t length()
	{
	  return std::strlen(data_);
	}
	char * begin()
	{
		return data_;
	}
	char * end()
	{
		return data_ + N;
	}
	bool empty()
	{
	  return data_[0]!='\0';
	}
};


}
}
}
