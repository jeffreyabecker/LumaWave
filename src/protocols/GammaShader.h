#pragma once

#include <array>
#include <cmath>
#include <cstdint>

#include "IShader.h"
#include "core/Color.h"

namespace lw::protocols
{

class GammaShader : public IShader
{
public:
  static constexpr float DefaultGamma = 2.6f;
  static constexpr float MinGamma = 0.1f;
  static constexpr float MaxGamma = 5.0f;

  GammaShader(float gamma = DefaultGamma) : _gammaValue(gamma) { recalculateTable(); }

  void apply(span<const lw::Color> source, span<lw::Color> dest) override
  {
    for (size_t i = 0; i < source.size(); ++i)
    {
      auto& d = dest[i];
      const auto& s = source[i];

      for (char channel : {'R', 'G', 'B', 'W'})
      {
        d[channel] = gamma8(static_cast<uint8_t>(s[channel]));
      }
    }
  }

  void setRuntimeConfig(RuntimeConfig type, void* value) override
  {
    if (type == RuntimeConfig::Gamma && value != nullptr)
    {
      setGamma(*static_cast<float*>(value));
    }
  }

  void setGamma(float gamma)
  {
    _gammaValue = gamma;
    recalculateTable();
  }

private:
  uint8_t gamma8(uint8_t value) const { return _table[value]; }

  void recalculateTable()
  {
    const float gamma = (_gammaValue < MinGamma || _gammaValue > MaxGamma) ? 1.0f : _gammaValue;

    _table[0] = 0;
    _table[255] = 255;

    for (uint16_t i = 1; i < 255; ++i)
    {
      _table[i] = static_cast<uint8_t>(static_cast<int>(std::pow(static_cast<float>(i) / 255.0f, gamma) * 255.0f + 0.5f));
    }
  }

  std::array<uint8_t, 256> _table{};
  float _gammaValue{DefaultGamma};
};

} // namespace lw::protocols
