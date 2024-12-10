#ifndef RENDERDOCACTIONHELPERS_H
#define RENDERDOCACTIONHELPERS_H

#include "RenderDocModel/RdcAction.Generated.h"
#include <api/replay/renderdoc_replay.h>
#include <types/wrapper.h>

struct SDFile;
struct ActionDescription;
namespace jetbrains::renderdoc::model {
}

namespace jetbrains::renderdoc::helpers {
model::RdcActionFlags map_flags(const ActionFlags flags);
std::vector<rd::Wrapper<model::RdcAction>> get_actions_recursive(const rdcarray<ActionDescription> &descriptions, const SDFile &file);
const ActionDescription *get_next_action(const ActionDescription *current);
const ActionDescription *find_action(const ActionDescription *begin, const std::function<bool(const ActionDescription &)> &predicate);
bool is_draw_call(const ActionDescription &action);
}

#endif //RENDERDOCACTIONHELPERS_H