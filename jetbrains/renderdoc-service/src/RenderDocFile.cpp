#include "RenderDocFile.h"

#include "api/replay/renderdoc_replay.h"
#include "util/StringUtils.h"

namespace jetbrains::renderdoc {

RenderDocFile::RenderDocFile(capture_file_ptr capture_file)
    : RdcCaptureFile(capture_file->LocalReplaySupport() == ReplaySupport::Supported, StringUtils::Utf8ToWide(capture_file->DriverName())), capture_file(std::move(capture_file)) {
    get_openCapture().set([this](const auto &) { return open_capture(); });
}

rd::Wrapper<RenderDocFile> RenderDocFile::open(const std::wstring &file_path) {
  capture_file_ptr capture_file(RENDERDOC_OpenCaptureFile(), [](ICaptureFile *file) { file->Shutdown(); });
  const auto result = capture_file->OpenFile(StringUtils::WideToUtf8(file_path), "rdc", nullptr);
  if (!result.OK())
    throw std::runtime_error(("Error: " + result.Message()).c_str());

  return rd::wrapper::make_wrapper<RenderDocFile>(std::move(capture_file));
}

rd::Wrapper<RenderDocReplay> RenderDocFile::open_capture() const {
  const ReplayOptions options;

  const auto result_and_controller = capture_file->OpenCapture(options, nullptr);
  if (!result_and_controller.first.OK())
    throw std::runtime_error(("Error: " + result_and_controller.first.Message()).c_str());

  return rd::wrapper::make_wrapper<RenderDocReplay>(result_and_controller.second);
}

} // namespace jetbrains::renderdoc
