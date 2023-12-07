#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <codecvt>
#include <locale>
#include <string>
#include <sstream>

class rdcstr;

class StringUtils final
{
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
  // ReSharper disable CppDeprecatedEntity
  static std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
  template <typename T>
  inline static void WriteToStingStream(std::stringstream& ss, T arg) {
    ss << arg;
  }

  template <typename T, typename ...Args>
  inline static void WriteToStingStream(std::stringstream& ss, T arg, Args... args) {
    WriteToStingStream<T>(ss, arg);
    WriteToStingStream<Args...>(ss, std::forward<Args>(args)...);
  }
public:
  StringUtils() = delete;

  static rdcstr WideToUtf8(const std::wstring &str);
  static std::wstring Utf8ToWide(const rdcstr &str);

  template <typename ...Args>
  inline static std::string BuildString(Args...args) {
    std::stringstream ss;
    WriteToStingStream<Args...>(ss, std::forward<Args>(args)...);
    return ss.str();
  }
};

#endif    // STRINGUTILS_H
