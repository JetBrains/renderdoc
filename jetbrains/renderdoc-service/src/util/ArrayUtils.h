#ifndef ARRAYUTILS_H
#define ARRAYUTILS_H

#include <api/replay/renderdoc_replay.h>
#include <vector>

struct rdhalf;
class ArrayUtils final
{
public:
  template <typename From, typename To = From>
  inline static std::vector<To> CopyToVector(const rdcfixedarray<From, 16> &arr) {
    std::vector<To> v(16);
    for (std::size_t i = 0; i < 16; i++) {
      v[i] = static_cast<To>(arr[i]);
    }
    return v;
  }

  template <typename From, typename To>
  inline static std::vector<To> CopyToVector(const rdcarray<From> &arr, To (*f)(const From &)) {
    std::vector<To> v(arr.size());
    for (std::size_t i = 0; i < arr.size(); i++) {
      v[i] = f(arr[i]);
    }
    return v;
  }


  template <typename T>
  inline static std::vector<T> CopyToVector(const rdcarray<T> &arr) {
    std::vector<T> v(arr.size());
    for (std::size_t i = 0; i < arr.size(); i++) {
      v[i] = arr[i];
    }
    return v;
  }

  ArrayUtils() = delete;

};

#endif    // ARRAYUTILS_H