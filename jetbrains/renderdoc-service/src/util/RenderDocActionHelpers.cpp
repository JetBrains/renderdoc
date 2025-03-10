#include "RenderDocActionHelpers.h"

#include "RenderDocConverterUtils.h"
#include "RenderDocSourceFileUtil.h"
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

bool try_get_used_source_file_paths(const PipeState &pipeline, std::set<std::wstring> &entrypoints, std::set<std::wstring> &others) {
  const ShaderReflection *vertex_shader = pipeline.GetShaderReflection(ShaderStage::Vertex);
  const ShaderReflection *pixel_shader = pipeline.GetShaderReflection(ShaderStage::Pixel);

  if (!vertex_shader && !pixel_shader)
    return false;

  const auto &vertex_entry = vertex_shader && vertex_shader->debugInfo.entryLocation.fileIndex >= 0 ? vertex_shader->debugInfo.entryLocation : LineColumnInfo();
  const auto &pixel_entry = pixel_shader && pixel_shader->debugInfo.entryLocation.fileIndex >= 0 ? pixel_shader->debugInfo.entryLocation : LineColumnInfo();

  const auto &files = vertex_shader ? vertex_shader->debugInfo.files : pixel_shader->debugInfo.files;
  std::size_t prev_line = 0;
  std::wstring prev_path;
  for (std::size_t i = 0; i < files.size(); ++i) {
    std::size_t j = 0;
    std::wistringstream stream(StringUtils::Utf8ToWide(files[i].contents));
    for (std::wstring line; std::getline(stream, line); ++j) {
      if (std::wsmatch matches; std::regex_match(line, matches, FILE_ENTRY_INFO_REGEX) && matches.size() >= 3 && matches[2].matched) {
        if (vertex_entry.fileIndex == i && vertex_entry.lineStart > prev_line && vertex_entry.lineStart < j ||
          pixel_entry.fileIndex == i && pixel_entry.lineStart > prev_line && pixel_entry.lineStart < j) {
          entrypoints.insert(prev_path);
          others.erase(prev_path);
        }
        others.insert(matches[2]);
        prev_line = j;
        prev_path = matches[2];
      } else if (j == 0) {
        break;
      }
    }
    if (vertex_entry.fileIndex == i && vertex_entry.lineStart > prev_line && vertex_entry.lineStart < j ||
          pixel_entry.fileIndex == i && pixel_entry.lineStart > prev_line && pixel_entry.lineStart < j) {
      entrypoints.insert(prev_path);
      others.erase(prev_path);
    }
  }
  if (entrypoints.empty() && others.empty())
    return false;
  return true;
}

rd::Wrapper<model::RdcSourceFilesInAction> get_used_source_file_paths(IReplayController* controller, const ActionDescription &action) {
  if (!action.children.empty() || !is_draw_call(action))
    return {};

  controller->SetFrameEvent(action.eventId, true);
  const auto &pipeline = controller->GetPipelineState();
  std::set<std::wstring> entrypoints;
  std::set<std::wstring> others;

  if (try_get_used_source_file_paths(pipeline, entrypoints, others))
    return rd::wrapper::make_wrapper<model::RdcSourceFilesInAction>(RenderDocConverterUtils::wrapStringsSet(entrypoints), RenderDocConverterUtils::wrapStringsSet(others));

  return rd::Wrapper<model::RdcSourceFilesInAction>(nullptr);
}

std::vector<rd::Wrapper<model::RdcAction>> get_actions_recursive(IReplayController* controller, const rdcarray<ActionDescription> &descriptions, const SDFile &file) { // NOLINT(*-no-recursion)
  std::vector<rd::Wrapper<model::RdcAction>> actions;
  for (const auto &it : descriptions) {
    const auto &children = get_actions_recursive(controller, it.children, file);

    actions.emplace_back(rd::wrapper::make_wrapper<model::RdcAction>(
      it.eventId,
      it.actionId,
      StringUtils::Utf8ToWide(it.GetName(file)),
      map_flags(it.flags),
      get_used_source_file_paths(controller, it),
      children
      ));
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

const ActionDescription *get_action(const rdcarray<ActionDescription> &actions, uint32_t event_id)
{
  for(const ActionDescription &a : actions)
  {
    if(!a.children.empty())
    {
      const ActionDescription *action = get_action(a.children, event_id);
      if(action != nullptr)
        return action;
    }

    if(a.eventId == event_id)
      return &a;
  }

  return nullptr;
}

bool is_draw_call(const ActionDescription &action) {
  return action.flags & ActionFlags::Drawcall || action.flags & ActionFlags::MeshDispatch;
}
}