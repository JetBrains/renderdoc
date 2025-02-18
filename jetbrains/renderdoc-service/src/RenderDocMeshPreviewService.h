#ifndef RENDERDOCMESHPREVIEWSERVICE_H
#define RENDERDOCMESHPREVIEWSERVICE_H

#include "types/wrapper.h"

#include <api/replay/renderdoc_replay.h>
#include <map>
#include <unordered_map>

namespace jetbrains::renderdoc {
namespace utils {
template <typename T>
inline T align_up(T x, T a)
{
  return (x + (a - 1)) & (~(a - 1));
}
}
namespace model {
class RdcVertexStageInOutputs;
}

struct BufferData {
  bytebuf buffer;
  std::size_t stride = 0;
};

struct BufferElementProperties {
  ResourceFormat format;
  int buffer = 0;
  ShaderBuiltin systemValue = ShaderBuiltin::Undefined;
  bool perinstance = false;
  bool perprimitive = false;
  bool floatCastWrong = false;
  int instancerate = 1;
};

class RenderDocMeshPreviewService {
  struct BufferConfig {
    uint32_t cur_instance = 0u; // need to support multiple instances, now only first one is supported
    int32_t base_vertex;
    uint32_t prim_restart = 0u;
    std::size_t rows_num;
    std::size_t expanded_rows_num;
    BufferData indices;
    std::vector<ShaderConstant> columns;
    std::vector<BufferElementProperties> properties;
    std::vector<BufferData> buffers;
  };

  IReplayController *controller;
  std::map<ResourceId, BufferDescription> buffers;
  std::unordered_map<uint32_t, rd::Wrapper<model::RdcVertexStageInOutputs>> stage_info_cache;

  static uint32_t calculate_index(const BufferData &data, uint32_t vertex_id, int32_t base_vertex, uint32_t prim_restart);

  void calculate_input_rows(const PipeState &pipe_state, const ActionDescription *action, BufferConfig &config) const;
  static void calculate_output_rows(const MeshFormat &post, const BufferConfig &in_config, BufferConfig &out_config) ;
  static void collect_input_columns(const PipeState &pipe_state, BufferConfig &config);
  static void collect_output_columns(const PipeState &pipe_state, BufferConfig &config);
  void fetch_buffers(const ActionDescription *action, const PipeState &pipe_state, const MeshFormat &post, BufferConfig &in_config, BufferConfig &out_config) const;
  static std::vector<std::vector<std::vector<float>>> translate_buffers_to_floats(const BufferConfig &config, std::vector<rd::Wrapper<std::wstring>> &columns, std::vector<uint32_t> &indices, uint32_t inst);
public:
  explicit RenderDocMeshPreviewService(IReplayController *controller);
  void calculate_vertices(const ActionDescription *action);
  rd::Wrapper<model::RdcVertexStageInOutputs> get_vertices(const ActionDescription *action);
  uint32_t get_vertex_index(const ActionDescription *action, uint32_t vertex_id);

};
} // namespace jetbrains::renderdoc

#endif // RENDERDOCMESHPREVIEWSERVICE_H
