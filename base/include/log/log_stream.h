// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_LOG_LOG_STREAM_H_
#define BASE_INCLUDE_LOG_LOG_STREAM_H_

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>  // for function memcpy
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include "base/include/base_export.h"

namespace lynx {
namespace base {
namespace logging {

constexpr int32_t kMaximumBufferSize = 4096;

// Cache logging data for the LogStream class, use the thread-local heap storage
// to helps to avoid stack overflow in recursive scenarios while also improving
// performance by reducing the frequency of heap memory allocation and
// destruction Example:
//    MixBuffer buffer;
//    const std::string contexnt = "hello world"
//    if (buffer.Available() > context.length()) {
//      buffer.Append(content.c_str(), context.length());
//    }
class MixBuffer {
 public:
  MixBuffer()
      : current_(Data()),
        end_(Data() + static_cast<size_t>(kMaximumBufferSize)) {}

  // uncopyable
  MixBuffer(const MixBuffer&) = delete;
  void operator=(const MixBuffer&) = delete;

  // If there is enough space left, append additional logging datas.
  void Append(const char* buffer, size_t length) {
    if (static_cast<size_t>(Available()) > length) {
      memcpy(current_, buffer, length);
      AddLength(length);
    }
  }

  // Initialize and access thread-local variables.
  static char* Data() {
    static thread_local std::unique_ptr<char[]> data =
        std::make_unique<char[]>(kMaximumBufferSize);
    return data.get();
  }

  int32_t Length() const { return static_cast<int32_t>(current_ - Data()); }
  char* Current() const { return current_; }
  int32_t Available() const { return static_cast<int32_t>(end_ - current_); }
  // write at the beginning of the buffer
  void Reset() { current_ = Data(); }
  // empty the buffer
  void Clear() {
    memset(Data(), 0, static_cast<size_t>(kMaximumBufferSize));
    Reset();
  }

 private:
  // upgate the length of buffer
  void AddLength(size_t length) {
    current_ += length;
    *current_ = '\0';
  }

  char* current_;
  const char* const end_;
};

// brief: Replace std::iostream with class LogStream, overwrite operator<< for
// base types, include: bool, char, int, int64, size_t, void*, float, double,
// string, ostream, ostringstream In case of int, double, char*, address type,
// It is faster and faster than snprintf and std::ostream Example:
//    LogStream os;
//    os << 2022 << "-" << 9 << "-" << 5 << "  " << "Welcome to the world of
//    lynx"; std::cout << os.str() << std::endl;
// !!!!!  Notice  !!!!!
// 1、class LogStream don`t support format string, such as std::hex,
//    std::setfill, std::setw. but you can do like this:
//        std::ostringstream buf;
//        LogStream os;
//        buf << std::setfill('0') << std::setw(10) << 123456;
//        os << buf;
// 2、when buffer size is longer than 4096 bytes(kSmallBuffer), do nothing.such
//    as ALog limits size to 4096 bytes
// 3、when convert address to hex string,
//    . if nullptr, the output is 0x00000000 in 32bits OS
//      and 0x0000000000000000 in 64bits OS
//    . have a fixed length output
// 4、 convert const char * to string
//    if nullptr, the output will be truncated

class BASE_EXPORT LogStream {
 public:
  LogStream() = default;
  ~LogStream() = default;

  // uncopyable
  LogStream(const LogStream&) = delete;
  void operator=(const LogStream&) = delete;

  LogStream& operator<<(bool);

  /**
   * replace snprintf with Milo based on branchlut scheme.[Written by Milo
   * Yip][Designed by Wojciech Muła] about 25x speedup faster than snprintf
   */
  // integer
  LogStream& operator<<(int8_t);
  LogStream& operator<<(uint8_t);
  LogStream& operator<<(int16_t);
  LogStream& operator<<(uint16_t);
  LogStream& operator<<(int32_t);
  LogStream& operator<<(uint32_t);
  LogStream& operator<<(int64_t);
  LogStream& operator<<(uint64_t);

// int64_t has different definition
// long long in 32bits while long in 64bits Android and linux
// long long in 64bits MacOS, iOS, Windows
#if defined(__LP64__) && (defined(OS_ANDROID) || defined(__linux__))
  LogStream& operator<<(long long value) {
    return operator<<(static_cast<int64_t>(value));
  }
  LogStream& operator<<(unsigned long long value) {
    return operator<<(static_cast<uint64_t>(value));
  }
#else
  LogStream& operator<<(long value) {
    return operator<<(static_cast<int64_t>(value));
  }
  LogStream& operator<<(unsigned long value) {
    return operator<<(static_cast<uint64_t>(value));
  }
#endif

  LogStream& operator<<(const void*);

  /**
   * replace snprintf with Milo based on Grisu2.[Written by Milo Yip][Designed
   * by Florian Loitsch] about 9x speedup faster than snprintf 15 precision
   * default
   */
  // TODO(lipin): overload float func with precision
  LogStream& operator<<(float);
  LogStream& operator<<(double);

  LogStream& operator<<(const char&);

  BASE_EXPORT LogStream& operator<<(const char*);
  LogStream& operator<<(const unsigned char* value) {
    return operator<<(reinterpret_cast<const char*>(value));
  }
  LogStream& operator<<(unsigned char* const value) {
    return operator<<(reinterpret_cast<const char*>(value));
  }
  LogStream& operator<<(char* const value) {
    return operator<<(static_cast<const char*>(value));
  }

  LogStream& operator<<(const std::string&);
  LogStream& operator<<(const std::string_view&);

  // overload for wchar_t, std::wstring
#if defined(OS_WIN)
  LogStream& operator<<(wchar_t value);
  LogStream& operator<<(const wchar_t* value);
  LogStream& operator<<(const std::wstring& value);
  LogStream& operator<<(const std::wstring_view& value);
#endif

  // operator for std::ostream
  LogStream& operator<<(const std::ostringstream& output) {
    return operator<<(output.str());
  }

  friend std::ostream& operator<<(std::ostream& output,
                                  const LogStream& message) {
    output << message.c_str();
    return output;
  }

  LogStream& operator<<(const LogStream& output) {
    Append(output.c_str(), output.Buffer().Length());
    return *this;
  }

  // overload for std::shared_ptr
  template <typename T>
  LogStream& operator<<(const std::shared_ptr<T>& value) {
    return operator<<(value.get());
  }

  // overload for std::unique_ptr
  template <typename T>
  LogStream& operator<<(const std::unique_ptr<T>& value) {
    return operator<<(value.get());
  }

  // overload for std::weak_ptr
  template <typename T>
  LogStream& operator<<(const std::weak_ptr<T>& value) {
    return operator<<(value.lock().get());
  }

  // overload for std::atomic
  // type which is not trivially copyable is not supported
  // support UDT which must be trivially copyable, such as:
  // class SelfType {
  //   double value_;
  //   public:
  //    explicit SelfType(double value) : value_(value) {}
  //    inline friend LogStream& operator<<(LogStream& output, const SelfType&
  //    input) {
  //     output << input.value_;
  //     return output;
  //    }
  // };

  template <typename T>
  LogStream& operator<<(const std::atomic<T>& value) {
    *this << value.load();
    return *this;
  }

  // overload for std::endl
  // Function implementation:
  // *
  //   template <class _CharT, class _Traits>
  //   inline _LIBCPP_INLINE_VISIBILITY
  //   basic_ostream<_CharT, _Traits>&
  //   endl(basic_ostream<_CharT, _Traits>& __os)
  //   {
  //       __os.put(__os.widen('\n'));
  //       __os.flush();
  //       return __os;
  //   }
  // *
  // !!!NOTICE: !!!
  // Need to distinguish the same implementation between std::endl, std::ends
  // and std::flush
  using CharT_ = char;
  using TraitsT_ = std::char_traits<CharT_>;
  LogStream& operator<<(std::basic_ostream<CharT_, TraitsT_>& (*function_endl)(
      std::basic_ostream<CharT_, TraitsT_>&)) {
    if (function_endl == std::endl<CharT_, TraitsT_>) {
#if defined(OS_WIN)
      return operator<<("\r\n");
#else
      return operator<<("\n");
#endif
    }
// for debug, if not std::endl, than abort
// for release, do nothing
#ifndef NDEBUG
    else {
      // only support type std::endl;
      // you need to overload opertator<< for UDT
      abort();
    }
#endif
    return *this;
  }

  // overload
  // convert std::thread::id into a hexadecimal string in uppercase form
  LogStream& operator<<(const std::thread::id& value) {
    std::stringstream ss;
    ss << std::uppercase << std::hex << value;
    return operator<<(ss.str());
  }

  void Append(const char* buffer, size_t length) {
    buffer_.Append(buffer, length);
  }
  const MixBuffer& Buffer() const { return buffer_; }
  const char* c_str() const { return buffer_.Data(); }
  std::string str() const { return std::string(buffer_.Data()); }
  void Reset() { buffer_.Reset(); }
  void Clear() { buffer_.Clear(); }

 private:
  MixBuffer buffer_;
};
}  // namespace logging
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_LOG_LOG_STREAM_H_
