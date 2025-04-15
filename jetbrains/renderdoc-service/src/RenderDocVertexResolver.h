#ifndef RENDERDOCVARIABLESRESOLVER_H
#define RENDERDOCVARIABLESRESOLVER_H
#include <api/replay/renderdoc_replay.h>
#include <string>
#include <limits>

struct ResourceFormat;
namespace jetbrains::renderdoc {
namespace utils {
template <typename T> inline T read_obj(const byte *&data, const byte *end, bool &success) {
  if (data + sizeof(T) > end) {
    success = false;
    return {};
  }

  T ret = *reinterpret_cast<const T *>(data);
  data += sizeof(T);

  return ret;
}
} // namespace utils

class RenderDocVertexResolver {
public:
  struct VertexVar {
    static constexpr float INFINITY_FLOAT = std::numeric_limits<float>::infinity();
    static const float NAN_FLOAT;

    enum class Type { None, Double, Float, Int8, UInt8, Int16, UInt16, Int32, UInt32, Int64, UInt64 };

    explicit VertexVar();
    explicit VertexVar(double d);
    explicit VertexVar(float f);
    explicit VertexVar(int8_t i);
    explicit VertexVar(uint8_t ui);
    explicit VertexVar(int16_t i);
    explicit VertexVar(uint16_t ui);
    explicit VertexVar(int32_t i);
    explicit VertexVar(uint32_t ui);
    explicit VertexVar(int64_t i);
    explicit VertexVar(uint64_t ui);

    Type type;
    union {
      double d;
      float f;
      int8_t i8;
      uint8_t ui8;
      int16_t i16;
      uint16_t ui16;
      int32_t i32;
      uint32_t ui32;
      int64_t i64;
      uint64_t ui64;
    } value{};
  };

  static std::vector<VertexVar> get_variables(ResourceFormat format, const ShaderConstant &var, const byte *&data, const byte *end);

  template <typename T, typename UT = std::make_unsigned_t<T>>
  static VertexVar interpret(const ResourceFormat &f, UT comp) {
    if (f.compByteWidth != sizeof(T) / sizeof(byte) || f.compType == CompType::Float)
      return VertexVar();

    if (f.compType == CompType::SInt)
      return VertexVar(static_cast<T>(comp));

    if (f.compType == CompType::UInt)
      return VertexVar(comp);

    if (f.compType == CompType::SScaled)
      return VertexVar(static_cast<float>(static_cast<T>(comp)));

    if (f.compType == CompType::UScaled)
      return VertexVar(static_cast<float>(comp));

    if (f.compType == CompType::UNorm || f.compType == CompType::UNormSRGB)
      return VertexVar(static_cast<float>(comp) / static_cast<float>(std::numeric_limits<UT>::max()));

    if (f.compType == CompType::SNorm) {
      auto cast = static_cast<T>(comp);

      float ret = -1.0f;
      if (cast != std::numeric_limits<T>::min())
        ret = static_cast<float>(cast) / static_cast<float>(std::numeric_limits<T>::max());

      return VertexVar(ret);
    }

    return VertexVar();
  }
};
} // namespace jetbrains::renderdoc

#endif // RENDERDOCVARIABLESRESOLVER_H
