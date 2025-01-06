#include "RenderDocVertexResolver.h"
#include "RenderDocModel/RdcDebugVariableType.Generated.h"
#include <cmath>

namespace jetbrains::renderdoc {
using VertexVar = RenderDocVertexResolver::VertexVar;

const float VertexVar::NAN_FLOAT = std::nanf("");

VertexVar::VertexVar() : type(Type::None) {}
VertexVar::VertexVar(double d) : type(Type::Double) { value.d = d; }
VertexVar::VertexVar(float f) : type(Type::Float) { value.f = f; }
VertexVar::VertexVar(int8_t i) : type(Type::Int8) { value.i8 = i; }
VertexVar::VertexVar(uint8_t ui) : type(Type::UInt8) { value.ui8 = ui; }
VertexVar::VertexVar(int16_t i) : type(Type::Int16) { value.i16 = i; }
VertexVar::VertexVar(uint16_t ui) : type(Type::UInt16) { value.ui16 = ui; }
VertexVar::VertexVar(int32_t i) : type(Type::Int32) { value.i32 = i; }
VertexVar::VertexVar(uint32_t ui) : type(Type::UInt32) { value.ui32 = ui; }
VertexVar::VertexVar(int64_t i) : type(Type::Int64) { value.i64 = i; }
VertexVar::VertexVar(uint64_t ui) : type(Type::UInt64) { value.ui64 = ui; }

std::vector<VertexVar> RenderDocVertexResolver::get_variables(ResourceFormat format, const ShaderConstant &var, const byte *&data, const byte *end) {
  const auto &type = var.type;

  std::vector<VertexVar> vars;
  bool success = true;

  auto comp_type = format.compType;

  if (format.type == ResourceFormatType::R5G5B5A1 || format.type == ResourceFormatType::R5G6B5 || format.type == ResourceFormatType::R4G4B4A4) {
    auto packed = utils::read_obj<uint16_t>(data, end, success);
    if (!success)
      return {};

    if (format.type == ResourceFormatType::R5G5B5A1) {
      vars.emplace_back(static_cast<float>(packed >> 0 & 0x1f) / 31.0f);
      vars.emplace_back(static_cast<float>(packed >> 5 & 0x1f) / 31.0f);
      vars.emplace_back(static_cast<float>(packed >> 10 & 0x1f) / 31.0f);
      vars.emplace_back((packed & 0x8000) > 0 ? 1.0f : 0.0f);
    } else if (format.type == ResourceFormatType::R5G6B5) {
      vars.emplace_back(static_cast<float>(packed >> 0 & 0x1f) / 31.0f);
      vars.emplace_back(static_cast<float>(packed >> 5 & 0x3f) / 63.0f);
      vars.emplace_back(static_cast<float>(packed >> 11 & 0x1f) / 31.0f);
    } else {
      vars.emplace_back(static_cast<float>(packed >> 0 & 0xf) / 15.0f);
      vars.emplace_back(static_cast<float>(packed >> 4 & 0xf) / 15.0f);
      vars.emplace_back(static_cast<float>(packed >> 8 & 0xf) / 15.0f);
      vars.emplace_back(static_cast<float>(packed >> 12 & 0xf) / 15.0f);
    }

    if (format.BGRAOrder())
      std::swap(vars[0], vars[2]);
  } else if (format.type == ResourceFormatType::R10G10B10A2) {
    for (int i = 0; i < format.compCount / 4; ++i) {
      auto packed = utils::read_obj<uint32_t>(data, end, success);
      if (!success)
        return {};
      uint32_t r = packed >> 0 & 0x3ff;
      uint32_t g = packed >> 10 & 0x3ff;
      uint32_t b = packed >> 20 & 0x3ff;
      uint32_t a = packed >> 30 & 0x003;

      if (format.BGRAOrder())
        std::swap(r, b);

      if (comp_type == CompType::UInt) {
        vars.emplace_back(r);
        vars.emplace_back(g);
        vars.emplace_back(b);
        vars.emplace_back(a);
      } else if (comp_type == CompType::UScaled) {
        vars.emplace_back(static_cast<float>(r));
        vars.emplace_back(static_cast<float>(g));
        vars.emplace_back(static_cast<float>(b));
        vars.emplace_back(static_cast<float>(a));
      } else if (comp_type == CompType::SInt || comp_type == CompType::SScaled || comp_type == CompType::SNorm) {
        int int_r = static_cast<int>(r);
        if (r > 511)
          int_r -= 1024;
        int int_g = static_cast<int>(g);
        if (g > 511)
          int_g -= 1024;
        int int_b = static_cast<int>(b);
        if (b > 511)
          int_b -= 1024;

        int int_a = static_cast<int>(a);
        if (a > 1)
          int_a -= 4;

        if (comp_type == CompType::SInt) {
          vars.emplace_back(int_r);
          vars.emplace_back(int_g);
          vars.emplace_back(int_b);
          vars.emplace_back(int_a);
        } else if (comp_type == CompType::SScaled) {
          vars.emplace_back(static_cast<float>(int_r));
          vars.emplace_back(static_cast<float>(int_g));
          vars.emplace_back(static_cast<float>(int_b));
          vars.emplace_back(static_cast<float>(int_a));
        } else if (format.compType == CompType::SNorm) {
          if (int_r == -512)
            int_r = -511;
          if (int_g == -512)
            int_g = -511;
          if (int_b == -512)
            int_b = -511;
          if (int_a == -2)
            int_a = -1;

          vars.emplace_back(static_cast<float>(int_r) / 511.0f);
          vars.emplace_back(static_cast<float>(int_g) / 511.0f);
          vars.emplace_back(static_cast<float>(int_b) / 511.0f);
          vars.emplace_back(static_cast<float>(int_a) / 1.0f);
        }
      } else {
        vars.emplace_back(static_cast<float>(r) / 1023.0f);
        vars.emplace_back(static_cast<float>(g) / 1023.0f);
        vars.emplace_back(static_cast<float>(b) / 1023.0f);
        vars.emplace_back(static_cast<float>(a) / 3.0f);
      }
    }
  } else if (format.type == ResourceFormatType::R11G11B10) {
    auto packed = utils::read_obj<uint32_t>(data, end, success);
    if (!success)
      return {};

    static const int32_t lead_bit[] = {0x40, 0x40, 0x20};
    uint32_t mantissas[] = {packed >> 0 & 0x3f, packed >> 11 & 0x3f, packed >> 22 & 0x1f};
    int32_t exponents[] = {static_cast<int32_t>(packed >> 6) & 0x1f, static_cast<int32_t>(packed >> 17) & 0x1f, static_cast<int32_t>(packed >> 27) & 0x1f};

    for (uint8_t i = 0; i < 3; ++i) {
      if (mantissas[i] == 0 && exponents[i] == 0) {
        vars.emplace_back(0.0f);
      } else {
        if (exponents[i] == 0x1f) {
          vars.emplace_back(static_cast<float>(mantissas[i] == 0 ? VertexVar::INFINITY_FLOAT : VertexVar::NAN_FLOAT));
        } else if (exponents[i] != 0) {
          vars.emplace_back((static_cast<float>(lead_bit[i] | mantissas[i]) / static_cast<float>(lead_bit[i])) * std::pow(2.0f, static_cast<float>(exponents[i]) - 15.0f));
        } else {
          vars.emplace_back((static_cast<float>(mantissas[i]) / static_cast<float>(lead_bit[i])) * std::pow(2.0f, 1.0f - 15.0f));
        }
      }
    }
  } else {
    const byte *base = data;

    auto row_num = type.rows;
    auto col_num = type.columns;

    for (std::size_t row = 0; row < row_num; ++row) {
      for (std::size_t col = 0; col < col_num; ++col) {
        if (row_num == 1 || type.RowMajor())
          data = base + row * type.matrixByteStride + col * format.compByteWidth;
        else
          data = base + col * type.matrixByteStride + row * format.compByteWidth;

        if (comp_type == CompType::Float) {
          if (format.compByteWidth == 8)
            vars.emplace_back(utils::read_obj<double>(data, end, success));
          else if (format.compByteWidth == 4)
            vars.emplace_back(utils::read_obj<float>(data, end, success));
          else if (format.compByteWidth == 2)
            vars.emplace_back(static_cast<float>(rdhalf::make(utils::read_obj<uint16_t>(data, end, success))));
        } else if (comp_type == CompType::SInt) {
          if (var.bitFieldSize == 0) {
            if (format.compByteWidth == 8)
              vars.emplace_back(utils::read_obj<int64_t>(data, end, success));
            else if (format.compByteWidth == 4)
              vars.emplace_back(utils::read_obj<int32_t>(data, end, success));
            else if (format.compByteWidth == 2)
              vars.emplace_back(static_cast<int>(utils::read_obj<uint16_t>(data, end, success)));
            else if (format.compByteWidth == 1)
              vars.emplace_back(static_cast<int>(utils::read_obj<uint8_t>(data, end, success)));
          } else {
            uint64_t uval = 0;
            if (format.compByteWidth == 8)
              uval = utils::read_obj<uint64_t>(data, end, success);
            else if (format.compByteWidth == 4)
              uval = utils::read_obj<uint32_t>(data, end, success);
            else if (format.compByteWidth == 2)
              uval = utils::read_obj<uint16_t>(data, end, success);
            else if (format.compByteWidth == 1)
              uval = utils::read_obj<uint8_t>(data, end, success);

            if (!success)
              return {};

            int64_t val = 0;
            uval >>= var.bitFieldOffset;
            const uint64_t mask = (1ULL << var.bitFieldSize) - 1ULL;
            uval &= mask;

            if (uval & (1ULL << (var.bitFieldSize - 1)))
              uval |= ~0ULL ^ mask;

            memcpy(&val, &uval, sizeof(uval));

            if (format.compByteWidth == 8)
              vars.emplace_back(val);
            else if (format.compByteWidth == 4)
              vars.emplace_back(static_cast<int32_t>(val));
            else if (format.compByteWidth == 2)
              vars.emplace_back(static_cast<int16_t>(val));
            else if (format.compByteWidth == 1)
              vars.emplace_back(static_cast<int8_t>(val));
          }
        } else if (comp_type == CompType::UInt) {
          if (var.bitFieldSize == 0) {
            if (format.compByteWidth == 8)
              vars.emplace_back(utils::read_obj<uint64_t>(data, end, success));
            else if (format.compByteWidth == 4)
              vars.emplace_back(utils::read_obj<uint32_t>(data, end, success));
            else if (format.compByteWidth == 2)
              vars.emplace_back(static_cast<uint32_t>(utils::read_obj<uint16_t>(data, end, success)));
            else if (format.compByteWidth == 1)
              vars.emplace_back(static_cast<uint32_t>(utils::read_obj<uint8_t>(data, end, success)));
          } else {
            uint64_t val = 0;

            if (format.compByteWidth == 8)
              val = utils::read_obj<uint64_t>(data, end, success);
            else if (format.compByteWidth == 4)
              val = utils::read_obj<uint32_t>(data, end, success);
            else if (format.compByteWidth == 2)
              val = utils::read_obj<uint16_t>(data, end, success);
            else if (format.compByteWidth == 1)
              val = utils::read_obj<uint8_t>(data, end, success);

            if (!success)
              return {};

            val >>= var.bitFieldOffset;
            val &= (1ULL << var.bitFieldSize) - 1ULL;

            if (format.compByteWidth == 8)
              vars.emplace_back(val);
            else if (format.compByteWidth == 4)
              vars.emplace_back(static_cast<uint32_t>(val));
            else if (format.compByteWidth == 2)
              vars.emplace_back(static_cast<uint16_t>(val));
            else if (format.compByteWidth == 1)
              vars.emplace_back(static_cast<uint8_t>(val));
          }
        } else if (comp_type == CompType::UScaled) {
          if (format.compByteWidth == 4)
            vars.emplace_back(static_cast<float>(utils::read_obj<uint32_t>(data, end, success)));
          else if (format.compByteWidth == 2)
            vars.emplace_back(static_cast<float>(utils::read_obj<uint16_t>(data, end, success)));
          else if (format.compByteWidth == 1)
            vars.emplace_back(static_cast<float>(utils::read_obj<uint8_t>(data, end, success)));
        } else if (comp_type == CompType::SScaled) {
          if (format.compByteWidth == 4)
            vars.emplace_back(static_cast<float>(utils::read_obj<int32_t>(data, end, success)));
          else if (format.compByteWidth == 2)
            vars.emplace_back(static_cast<float>(utils::read_obj<int16_t>(data, end, success)));
          else if (format.compByteWidth == 1)
            vars.emplace_back(static_cast<float>(utils::read_obj<int8_t>(data, end, success)));
        } else if (comp_type == CompType::Depth) {
          if (format.compByteWidth == 4)
            vars.emplace_back(utils::read_obj<float>(data, end, success));
          else if (format.compByteWidth == 3) {
            auto f = utils::read_obj<uint32_t>(data, end, success);
            f &= 0x00ffffff;
            vars.emplace_back(static_cast<float>(f) / static_cast<float>(0x00ffffff));
          } else if (format.compByteWidth == 2) {
            auto f = static_cast<float>(utils::read_obj<uint16_t>(data, end, success));
            vars.emplace_back(f / static_cast<float>(0x0000ffff));
          }
        } else {
          if (format.compByteWidth == 4)
            vars.emplace_back(static_cast<float>(utils::read_obj<uint32_t>(data, end, success)) / static_cast<float>(0xffffffff));
          else if (format.compByteWidth == 2)
            vars.emplace_back(interpret<int16_t>(format, utils::read_obj<uint16_t>(data, end, success)));
          else if (format.compByteWidth == 1)
            vars.emplace_back(interpret<byte>(format, utils::read_obj<uint8_t>(data, end, success)));
        }
      }
    }

    if (format.BGRAOrder())
      std::swap(vars[0], vars[2]);
  }

  return success ? vars : std::vector<VertexVar>();
}
} // namespace jetbrains::renderdoc