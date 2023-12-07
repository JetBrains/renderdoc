#include "RenderDocService.h"

#include "RenderDocModel/RdcCapture.Generated.h"
#include "api/replay/renderdoc_replay.h"

#include "util/StringUtils.h"
#include <api/replay/pipestate.inl>
#include <api/replay/renderdoc_tostr.inl>

REPLAY_PROGRAM_MARKER()

rdcstr conv(const std::string &s)
{
  return {s.c_str(), s.size()};
}

std::string conv(const rdcstr &s)
{
  return {s.begin(), s.end()};
}

template <>
rdcstr DoStringise(const uint32_t &el)
{
  return conv(std::to_string(el));
}

namespace jetbrains::renderdoc
{

RenderDocService::RenderDocService()
{
  constexpr GlobalEnvironment environment;
  const rdcarray<rdcstr> args;

  RENDERDOC_InitialiseReplay(environment, args);
}

RenderDocService::~RenderDocService() { RENDERDOC_ShutdownReplay(); }

rd::Wrapper<RenderDocFile> RenderDocService::open_capture_file(const std::wstring &capture_path)
{
  return RenderDocFile::open(capture_path);
}

}
