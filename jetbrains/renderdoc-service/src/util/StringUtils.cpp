#include "StringUtils.h"
#include <codecvt>
#include <locale>
#include <api/replay/renderdoc_replay.h>

decltype(StringUtils::convert) StringUtils::convert;

rdcstr StringUtils::WideToUtf8(const std::wstring &str)
{
  const auto bytes = convert.to_bytes(str);
  return {bytes.c_str(), bytes.length()};
}

std::wstring StringUtils::Utf8ToWide(const rdcstr &str)
{
  return convert.from_bytes(str.c_str());
}
