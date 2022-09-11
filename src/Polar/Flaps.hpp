#pragma once

#include "util/TrivialArray.hxx"
#include "util/StaticArray.hxx"
#include "util/StaticString.hxx"

struct FlapSpeeds : TrivialArray<double, 8> {

  constexpr std::size_t GetSetting(double speed) const {
    for (std::size_t i = 0; i < size(); i++) {
      if ((*this)[i] > speed) {
        return i > 0 ? i - 1 : 0;
      }
    }

    return size() - 1;
  }

  constexpr bool IsValid() const {
    return true;
  }
};

struct FlapSetting {
  StaticString<8> name;
  double minV;
};

/**
 * TODO: validation
 */
struct Flaps : TrivialArray<FlapSetting, FlapSpeeds::capacity()> {
  using TrivialArray::TrivialArray;

  FlapSpeeds GetSpeeds() const {
    FlapSpeeds speeds;
    speeds.clear();
    std::transform(begin(), end(), std::back_inserter(speeds),
        [](const auto& setting) { return setting.minV; });
    return speeds;
  }

};


