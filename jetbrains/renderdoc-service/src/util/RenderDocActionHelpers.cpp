#include "RenderDocActionHelpers.h"

#include "StringUtils.h"

namespace jetbrains::renderdoc::helpers {

model::RdcActionFlags map_flags(const ActionFlags flags) {
  model::RdcActionFlags rdc_flags = {};
  if (flags & ActionFlags::Drawcall)
    rdc_flags |= model::RdcActionFlags::Drawcall;
  if (flags & ActionFlags::MeshDispatch)
    rdc_flags |= model::RdcActionFlags::MeshDispatch;
  return rdc_flags;
}

std::vector<rd::Wrapper<model::RdcAction>> get_actions_recursive(const rdcarray<ActionDescription> &descriptions, const SDFile &file) { // NOLINT(*-no-recursion)
  std::vector<rd::Wrapper<model::RdcAction>> actions;
  for (const auto &it : descriptions) {
    actions.emplace_back(rd::wrapper::make_wrapper<model::RdcAction>(it.eventId, it.actionId, StringUtils::Utf8ToWide(it.GetName(file)), map_flags(it.flags), get_actions_recursive(it.children, file)));
  }
  return actions;
}

const ActionDescription *get_next_action(const ActionDescription *current) {
  if (current == nullptr)
    return nullptr;
  const ActionDescription *action = current;
  if(!action->children.empty())
    return action->children.begin();
  if(!action->next && action->parent)
    action = action->parent;
  return action->next;
}

const ActionDescription *find_action(const ActionDescription *begin, const std::function<bool(const ActionDescription &)> &predicate)
{
  const ActionDescription *action = begin;
  while(action != nullptr)
  {
    if(predicate(*action))
      break;
    action = get_next_action(action);
  }
  return action;
}

bool is_draw_call(const ActionDescription &action) {
  return action.flags & ActionFlags::Drawcall || action.flags & ActionFlags::MeshDispatch;
}
}