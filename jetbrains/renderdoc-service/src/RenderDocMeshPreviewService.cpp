#include "RenderDocMeshPreviewService.h"

#include "RenderDocCaptureContext.h"
#include "RenderDocModel/RdcVertexStageInOutputs.Generated.h"
#include "RenderDocVertexResolver.h"
#include "util/StringUtils.h"

namespace jetbrains::renderdoc {
RenderDocMeshPreviewService::RenderDocMeshPreviewService(IReplayController *controller, const std::shared_ptr<RenderDocCaptureContext> &capture_context) : controller(controller), capture_context(capture_context) {}

uint32_t RenderDocMeshPreviewService::calculate_index(const BufferData &data, uint32_t vertex_id, int32_t base_vertex, uint32_t prim_restart) {
  const auto it = data.buffer.data() + vertex_id * sizeof(uint32_t);
  if(it + sizeof(uint32_t) > data.buffer.end())
    return ~0U;

  uint32_t idx = *reinterpret_cast<const uint32_t *>(it);

  // check for primitive restart *before* adding base vertex
  if(prim_restart && idx == prim_restart)
    return idx;

  // apply base vertex but clamp to 0 if subtracting
  if(base_vertex < 0)
  {
    auto subtract = static_cast<uint32_t>(-base_vertex);

    if(idx < subtract)
      idx = 0;
    else
      idx -= subtract;
  }
  else if(base_vertex > 0)
  {
    idx += static_cast<uint32_t>(base_vertex);
  }

  return idx;
}

void RenderDocMeshPreviewService::calculate_input_rows(const PipeState &pipe_state, const ActionDescription *action, BufferConfig &config) const {
  config.rows_num = action->numIndices;
  config.expanded_rows_num = 0;

  uint32_t rows_upper_bound = 0;

  if (action->flags & ActionFlags::Indexed) {
    const auto &i_buffer = pipe_state.GetIBuffer();
    uint32_t available_bytes = i_buffer.byteSize;

    if (available_bytes == ~0U) {
      if (const auto it = capture_context->try_get_buffer(i_buffer.resourceId); it != nullptr) {
        uint64_t offset = i_buffer.byteOffset + action->indexOffset * i_buffer.byteStride;
        if(offset > it->length)
          available_bytes = 0;
        else
          available_bytes = it->length - offset;
      }
      else
        available_bytes = 0;
    }
    rows_upper_bound = available_bytes / std::max(1U, i_buffer.byteStride);
  } else {
    const auto &v_buffers = pipe_state.GetVBuffers();

    for (const auto &buf: v_buffers) {
      if (buf.byteStride == 0) continue;
      uint32_t available_bytes = buf.byteSize;

      if (available_bytes == ~0U) {
        if (const auto it = capture_context->try_get_buffer(buf.resourceId); it != nullptr) {
          if(buf.byteOffset > it->length)
            available_bytes = 0;
          else
            available_bytes = it->length - buf.byteOffset;
        }
        else
          available_bytes = 0;
      }
      rows_upper_bound = std::max(rows_upper_bound, available_bytes / std::max(1U, buf.byteStride));
    }
  }

  if (rows_upper_bound != ~0U && rows_upper_bound + 100 < config.rows_num) {
    config.expanded_rows_num = config.rows_num;
    config.rows_num = rows_upper_bound + 100;
  }

  if(action->flags & ActionFlags::Instanced && action->numInstances == 0)
  {
    config.rows_num = config.expanded_rows_num = 0;
  }
}

void RenderDocMeshPreviewService::calculate_output_rows(const MeshFormat &post, const BufferConfig &in_config, BufferConfig &out_config) {
  if(post.numIndices <= in_config.rows_num)
  {
    out_config.rows_num = post.numIndices;
    out_config.expanded_rows_num = 0;
  }
  else
  {
    out_config.rows_num = in_config.rows_num;
    out_config.expanded_rows_num = in_config.expanded_rows_num;
  }
}

void RenderDocMeshPreviewService::collect_input_columns(const PipeState &pipe_state, BufferConfig &config) {
  const auto &inputs = pipe_state.GetVertexInputs();
  for (const VertexInputAttribute &in : inputs) {
    if (!in.used)
      continue;
    ShaderConstant column;
    column.name = in.name;
    column.byteOffset = in.byteOffset;
    column.type.columns = in.format.compCount;
    column.type.rows = 1;
    column.type.arrayByteStride = column.type.matrixByteStride = in.format.ElementSize();

    BufferElementProperties prop;
    prop.buffer = in.vertexBuffer;
    prop.perinstance = in.perInstance;
    prop.instancerate = in.instanceRate;
    prop.floatCastWrong = in.floatCastWrong;
    prop.format = in.format;

    config.columns.push_back(column);
    config.properties.push_back(prop);
  }
}

void RenderDocMeshPreviewService::collect_output_columns(const PipeState &pipe_state, BufferConfig &config) {
  const ShaderReflection *reflection = pipe_state.GetShaderReflection(ShaderStage::Vertex);
  if (reflection == nullptr) {
    return;
  }

  for (const SigParameter &param: reflection->outputSignature) {
    if (param.stream != 0 || param.systemValue == ShaderBuiltin::OutputIndices)
      continue;
    ShaderConstant column;
    column.name = !param.varName.isEmpty() ? param.varName : param.semanticIdxName;
    if (param.perPrimitiveRate)
      column.name += " (per primitive)";
    column.type.rows = 1;
    column.type.columns = param.compCount;

    BufferElementProperties prop;
    prop.buffer = 0;
    prop.perinstance = false;
    prop.perprimitive = param.perPrimitiveRate;
    prop.instancerate = 1;
    prop.systemValue = param.systemValue;
    prop.format.type = ResourceFormatType::Regular;
    prop.format.compByteWidth = std::max<uint32_t>(sizeof(float), VarTypeByteSize(param.varType));
    prop.format.compCount = param.compCount;
    prop.format.compType = VarTypeCompType(param.varType);

    column.type.arrayByteStride = prop.format.compByteWidth * prop.format.compCount;

    if(param.systemValue == ShaderBuiltin::Position) {
      config.columns.insert(config.columns.begin(), column);
      config.properties.insert(config.properties.begin(), prop);
    }
    else {
      config.columns.push_back(column);
      config.properties.push_back(prop);
    }
  }
  std::size_t n = config.columns.size();
  uint32_t perPrimOffset = 0, perVertOffset = 0;
  for(std::size_t i = 0; i < n; i++) {
    auto &constant = config.columns[i];
    const auto &prop = config.properties[i];
    uint8_t type_columns = constant.type.columns;
    uint8_t size = prop.format.compByteWidth > 4 ? 8U : 4U;

    MeshDataStage outStage = MeshDataStage::VSOut;

    switch(reflection->stage)
    {
      case ShaderStage::Vertex: outStage = MeshDataStage::VSOut; break;
      case ShaderStage::Hull:
      case ShaderStage::Domain:
      case ShaderStage::Geometry: outStage = MeshDataStage::GSOut; break;
      case ShaderStage::Task: outStage = MeshDataStage::TaskOut; break;
      case ShaderStage::Mesh: outStage = MeshDataStage::MeshOut; break;
      default: break;
    }

    uint32_t &offset = prop.perprimitive ? perPrimOffset : perVertOffset;

    if(type_columns >= 2 && pipe_state.HasAlignedPostVSData(outStage))
    {
      unsigned int mult = type_columns == 2 ? 2U : 4U;
      offset = utils::align_up(offset, mult * size);
    }

    constant.byteOffset = offset;

    offset += type_columns * size;
  }
}

void RenderDocMeshPreviewService::fetch_buffers(const ActionDescription *action, const PipeState &pipe_state, const MeshFormat &post, BufferConfig &in_config, BufferConfig &out_config) const {
  auto i_buffer = pipe_state.GetIBuffer();

  uint32_t indices_num = action->numIndices;
  bytebuf data;
  const auto is_indexed = action->flags & ActionFlags::Indexed;
  if (i_buffer.resourceId != ResourceId::Null() && is_indexed) {
    uint32_t offset = action->indexOffset * i_buffer.byteStride;
    uint64_t bytes = i_buffer.byteSize > offset ? std::min<uint64_t>(i_buffer.byteSize - offset, indices_num * i_buffer.byteStride) : 0;
    if (bytes > 0) {
      data = controller->GetBufferData(i_buffer.resourceId, i_buffer.byteOffset + offset, bytes);
    }
  }

  uint32_t mult = 0;
  if (i_buffer.byteStride != 0 && !data.isEmpty())
    mult = std::min(indices_num, (static_cast<uint32_t>(data.size()) + i_buffer.byteStride - 1) / i_buffer.byteStride);
  else if (is_indexed)
    mult = 1;
  if (mult > 0) {
    in_config.indices.buffer.resize(sizeof(uint32_t) * mult);
  }

  auto *indices = reinterpret_cast<uint32_t *>(in_config.indices.buffer.begin());
  uint32_t max_index = std::max(1U, indices_num) - 1;
  if (!data.isEmpty()) {
    max_index = 0;
    if (i_buffer.byteStride == 1) {
      uint8_t prim_restart = in_config.prim_restart & 0xff;
      for (std::size_t i = 0; i < std::min<std::size_t>(data.size(), indices_num); ++i) {
        indices[i] = static_cast<uint32_t>(data[i]);

        if(prim_restart && indices[i] == prim_restart) continue;
        max_index = std::max(max_index, indices[i]);
      }
    } else if (i_buffer.byteStride == 2) {
      uint16_t prim_restart = in_config.prim_restart & 0xffff;
      auto *src = reinterpret_cast<uint16_t *>(data.data());
      for (std::size_t i = 0; i < std::min<std::size_t>(data.size() / sizeof(uint16_t), indices_num); ++i) {
        indices[i] = static_cast<uint32_t>(src[i]);

        if(prim_restart && indices[i] == prim_restart) continue;
        max_index = std::max(max_index, indices[i]);
      }
    } else if (i_buffer.byteStride == 4) {
      uint32_t prim_restart = in_config.prim_restart;
      memcpy(indices, data.data(), std::min<std::size_t>(data.size(), indices_num * sizeof(uint32_t)));

      for(uint32_t i = 0; i < std::min<std::size_t>(data.size() / sizeof(uint32_t), indices_num); i++)
      {
        if(prim_restart && indices[i] == prim_restart) continue;
        max_index = std::max(max_index, indices[i]);
      }
    }
  }

  const auto v_buffers = pipe_state.GetVBuffers();
  int v_buffer_idx = 0;
  for (const auto &v_buf : v_buffers) {
    bool used = false;
    bool per_inst = false;
    bool per_vert = false;
    uint32_t max_attr_offset = 0;

    for (int64_t i = 0; i < in_config.columns.size(); ++i) {
      const auto &column = in_config.columns[i];
      const auto &prop = in_config.properties[i];

      if (prop.buffer == v_buffer_idx) {
        used = true;
        max_attr_offset = std::max(max_attr_offset, column.byteOffset);
        if (prop.perinstance)
          per_inst = true;
        else
          per_vert = true;
      }
    }

    ++v_buffer_idx;

    uint32_t max_idx = 0;
    uint32_t offset = 0;

    if (used) {
      if (per_inst) {
        max_idx = std::max(1U, action->numInstances) - 1;
        offset = action->instanceOffset;
      }
      if (per_vert) {
        max_idx = std::max(max_idx, max_index);
        offset = action->vertexOffset;

        if (uint32_t base_vertex = action->baseVertex; base_vertex > 0)
          max_idx = std::max(max_idx, max_idx + base_vertex);
      }
    }

    BufferData buf;
    if (used) {
      uint64_t bytes_num = std::max(max_idx, max_idx + 1) * v_buf.byteStride + max_attr_offset;

      if (v_buf.byteStride == 0)
        bytes_num += 16;

      offset *= v_buf.byteStride;

      if (v_buf.byteSize > offset)
        bytes_num = std::min(v_buf.byteSize - offset, bytes_num);
      else
        bytes_num = 0;

      if (bytes_num > 0)
        buf.buffer = controller->GetBufferData(v_buf.resourceId, v_buf.byteOffset + offset, bytes_num);
      buf.stride = v_buf.byteStride;
    }
    in_config.buffers.push_back(buf);
  }

  if (post.indexResourceId != ResourceId::Null() && is_indexed)
    data = controller->GetBufferData(post.indexResourceId, post.indexByteOffset, indices_num * post.indexByteStride);

  indices = nullptr;
  if (i_buffer.byteStride != 0 && !data.isEmpty()) {
    out_config.indices.buffer.resize(sizeof(uint32_t) * indices_num);
    indices = reinterpret_cast<uint32_t *>(out_config.indices.buffer.begin());

    if (i_buffer.byteStride == 1) {
      for (std::size_t i = 0; i < std::min<std::size_t>(data.size(), indices_num); ++i)
        indices[i] = static_cast<uint32_t>(data[i]);
    }
    else if (i_buffer.byteStride == 2) {
      const auto src = reinterpret_cast<uint16_t *>(data.data());
      for (std::size_t i = 0; i < std::min<std::size_t>(data.size() / sizeof(uint16_t), indices_num); ++i)
        indices[i] = static_cast<uint32_t>(src[i]);
    }
    else if (i_buffer.byteStride == 4) {
      memcpy(indices, data.data(), std::min(data.size(), indices_num * sizeof(uint32_t)));
    }
  }

  if (post.vertexResourceId != ResourceId::Null()) {
    BufferData post_vs;
    post_vs.buffer = controller->GetBufferData(post.vertexResourceId, post.vertexByteOffset, 0);
    post_vs.stride = post.vertexByteStride;

    out_config.buffers.push_back(post_vs);
  }
}

std::vector<std::vector<std::vector<float>>> RenderDocMeshPreviewService::translate_buffers_to_floats(const BufferConfig &config, std::vector<rd::Wrapper<std::wstring>> &columns, std::vector<uint32_t> &indices, uint32_t inst) {
  struct ColumnInfo {
    uint32_t inst_index = 0;
    std::size_t stride = 0;
    const byte * data = nullptr;
    const byte * end = nullptr;
  };

  const std::size_t row_num = config.rows_num;
  const std::size_t col_num = config.columns.size();

  std::vector<std::vector<std::vector<float>>> result(row_num);

  columns.resize(col_num);
  std::vector<ColumnInfo> column_info(col_num);
  for (std::size_t col = 0; col < col_num; ++col) {
    auto &info = column_info[col];
    const auto &column = config.columns[col];
    const auto &prop = config.properties[col];
    columns[col] = { StringUtils::Utf8ToWide(column.name) };

    info.inst_index = prop.instancerate > 0 ? inst / prop.instancerate : 0;

    if (prop.buffer < config.buffers.size()) {
      info.data = config.buffers[prop.buffer].buffer.data() + column.byteOffset;
      info.end = config.buffers[prop.buffer].buffer.end();
      info.stride = config.buffers[prop.buffer].stride;

      if (prop.perinstance)
        info.data += info.stride * info.inst_index;
    }

    if(prop.perprimitive) info.end = info.data;
  }

  for (std::size_t row = 0; row < row_num; ++row) {
    uint32_t idx = row;
    if (!config.indices.buffer.isEmpty()) {
      idx = calculate_index(config.indices, row, config.base_vertex, config.prim_restart);
      if (idx == ~0U || config.prim_restart && idx == config.prim_restart)
        continue;
    }

    indices.push_back(idx);
    result[row].resize(col_num);
    for (std::size_t col = 0; col < col_num; ++col) {
      const auto &info = column_info[col];
      const auto &column = config.columns[col];
      const auto &prop = config.properties[col];

      if(info.data)
      {
        const byte *bytes = info.data;

        if(!prop.perinstance)
          bytes += info.stride * idx;

        const auto list = RenderDocVertexResolver::get_variables(prop.format, column, bytes, info.end);

        const std::size_t comp_num = std::min<std::size_t>(list.size(), 4);
        result[row][col].resize(comp_num);
        for(std::size_t comp = 0; comp < comp_num; comp++)
        {
          const RenderDocVertexResolver::VertexVar &v = list[comp];

          float f_val = 0.0f;

          using Type = RenderDocVertexResolver::VertexVar::Type;

          if (v.type == Type::Double)
            f_val = static_cast<float>(v.value.d);
          else if (v.type == Type::Float)
            f_val = v.value.f;
          else if (v.type == Type::UInt32 || v.type == Type::UInt16 || v.type == Type::UInt8)
            f_val = static_cast<float>(v.value.ui32);
          else if (v.type == Type::Int32 || v.type == Type::Int16 || v.type == Type::Int8)
            f_val = static_cast<float>(v.value.i32);
          else
            continue;

          result[row][col][comp] = f_val;
        }
      }
    }
  }

  return result;
}

void RenderDocMeshPreviewService::calculate_vertices(const ActionDescription *action) {
  const auto &pipe_state = controller->GetPipelineState();

  // TODO: support buffer preview for MeshDispatch actions
  auto is_mesh_dispatch = action->flags & ActionFlags::MeshDispatch;
  auto is_drawcall = action->flags & ActionFlags::Drawcall;
  if (!is_drawcall) {
    stage_info_cache.try_emplace(action->eventId, rd::Wrapper<model::RdcVertexStageInOutputs>(nullptr));
    return;
  }

  BufferConfig input_config;
  BufferConfig output_config;
  if (pipe_state.IsRestartEnabled() && action->flags & ActionFlags::Indexed) {
    input_config.prim_restart = pipe_state.GetRestartIndex();
    const auto byte_stride = pipe_state.GetIBuffer().byteStride;
    if(byte_stride == 1)
      input_config.prim_restart &= 0xff;
    else if(byte_stride == 2)
      input_config.prim_restart &= 0xffff;

    output_config.prim_restart = input_config.prim_restart;
  }

  input_config.base_vertex = action->baseVertex;
  calculate_input_rows(pipe_state, action, input_config);
  collect_input_columns(pipe_state, input_config);

  collect_output_columns(pipe_state, output_config);
  constexpr int instance = 0; // TODO: handle multiple instances
  auto post_data = controller->GetPostVSData(instance, 0, MeshDataStage::VSOut);
  output_config.base_vertex = post_data.baseVertex;
  calculate_output_rows(post_data, input_config, output_config);

  fetch_buffers(action, pipe_state,post_data, input_config, output_config);

  std::vector<uint32_t> in_indices, out_indices;
  std::vector<rd::Wrapper<std::wstring>> in_columns, out_columns;
  stage_info_cache.try_emplace(action->eventId, rd::wrapper::make_wrapper<model::RdcVertexStageInOutputs>(
    translate_buffers_to_floats(input_config, in_columns, in_indices, instance),
    in_columns,
    in_indices,
    translate_buffers_to_floats(output_config, out_columns, out_indices, instance),
    out_columns,
    out_indices
    ));
}

uint32_t RenderDocMeshPreviewService::get_vertex_index(const ActionDescription *action, uint32_t vertex_id) {
  if (stage_info_cache.find(action->eventId) == stage_info_cache.end()) {
    calculate_vertices(action);
  }
  const auto &stage_info = stage_info_cache.at(action->eventId);
  const auto &indices = stage_info ? stage_info->get_input_indices() : std::vector<uint32_t>();
  if (vertex_id >= indices.size())
    return ~0U;
  return stage_info->get_input_indices().at(vertex_id);
}

rd::Wrapper<model::RdcVertexStageInOutputs> RenderDocMeshPreviewService::get_vertices(const ActionDescription *action) {
  const uint32_t event_id = action->eventId;
  if (stage_info_cache.find(event_id) == stage_info_cache.end()) {
    calculate_vertices(action);
  }
  const auto &stage_info = stage_info_cache.at(event_id);
  if (stage_info.has_value())
    return stage_info;
  return rd::Wrapper<model::RdcVertexStageInOutputs>(nullptr);
}

} // namespace jetbrains::renderdoc
