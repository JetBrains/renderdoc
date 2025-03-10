#ifndef RENDERDOCACTIONHELPERS_H
#define RENDERDOCACTIONHELPERS_H

#include "RenderDocModel/RdcAction.Generated.h"
#include <api/replay/renderdoc_replay.h>
#include <set>
#include <types/wrapper.h>

struct SDFile;
struct ActionDescription;
namespace jetbrains::renderdoc::model {
}

namespace jetbrains::renderdoc::helpers {
model::RdcActionFlags map_flags(const ActionFlags flags);
bool try_get_used_source_file_paths(const PipeState &pipeline, std::set<std::wstring> &entrypoints, std::set<std::wstring> &others);
rd::Wrapper<model::RdcSourceFilesInAction> get_used_source_file_paths(IReplayController* controller, const ActionDescription &action);
std::vector<rd::Wrapper<model::RdcAction>> get_actions_recursive(IReplayController* controller, const rdcarray<ActionDescription> &descriptions, const SDFile &file);
const ActionDescription *get_next_action(const ActionDescription *current);
const ActionDescription *find_action(const ActionDescription *begin, const std::function<bool(const ActionDescription &)> &predicate);
const ActionDescription *get_action(const rdcarray<ActionDescription> &actions, uint32_t event_id);
bool is_draw_call(const ActionDescription &action);

template<typename T>
rd::Wrapper<T> first_not_null_action(const ActionDescription *begin, const std::function<rd::Wrapper<T>(const ActionDescription &)> &transform)
{
  const ActionDescription *action = begin;
  while(action != nullptr)
  {
    if (auto result = transform(*action))
      return result;
    action = get_next_action(action);
  }
  return rd::Wrapper<T>(nullptr);
}
}

#endif //RENDERDOCACTIONHELPERS_H
