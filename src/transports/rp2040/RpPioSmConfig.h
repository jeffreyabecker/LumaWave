#pragma once

#ifdef ARDUINO_ARCH_RP2040

#include <type_traits>

#include "hardware/pio.h"
#include "hardware/clocks.h"

namespace lw::transports::rp2040
{

class RpPioSmConfig
{
public:
  RpPioSmConfig() : _config{pio_get_default_sm_config()} {}

  RpPioSmConfig& setOutPinBase(uint outBase)
  {
    sm_config_set_out_pin_base(&_config, outBase);
    return *this;
  }

  RpPioSmConfig& setOutPinCount(uint outCount)
  {
    sm_config_set_out_pin_count(&_config, outCount);
    return *this;
  }

  RpPioSmConfig& setOutPins(uint outBase, uint outCount)
  {
    sm_config_set_out_pins(&_config, outBase, outCount);
    return *this;
  }

  RpPioSmConfig& setSetPinBase(uint setBase)
  {
    sm_config_set_set_pin_base(&_config, setBase);
    return *this;
  }

  RpPioSmConfig& setSetPinCount(uint setCount)
  {
    sm_config_set_set_pin_count(&_config, setCount);
    return *this;
  }

  RpPioSmConfig& setSetPins(uint setBase, uint setCount)
  {
    sm_config_set_set_pins(&_config, setBase, setCount);
    return *this;
  }

  RpPioSmConfig& setInPinBase(uint inBase)
  {
    sm_config_set_in_pin_base(&_config, inBase);
    return *this;
  }

  RpPioSmConfig& setInPins(uint inBase)
  {
    sm_config_set_in_pins(&_config, inBase);
    return *this;
  }

  RpPioSmConfig& setInPinCount(uint inCount)
  {
    sm_config_set_in_pin_count(&_config, inCount);
    return *this;
  }

  RpPioSmConfig& setSidesetPinBase(uint sidesetBase)
  {
    sm_config_set_sideset_pin_base(&_config, sidesetBase);
    return *this;
  }

  RpPioSmConfig& setSidesetPins(uint sidesetBase)
  {
    sm_config_set_sideset_pins(&_config, sidesetBase);
    return *this;
  }

  RpPioSmConfig& setSideset(uint bitCount, bool optional, bool pindirs)
  {
    sm_config_set_sideset(&_config, bitCount, optional, pindirs);
    return *this;
  }

  RpPioSmConfig& setClkdivIntFrac8(uint32_t divInt, uint8_t divFrac8)
  {
    sm_config_set_clkdiv_int_frac8(&_config, divInt, divFrac8);
    return *this;
  }

  RpPioSmConfig& setClkdivIntFrac(uint16_t divInt, uint8_t divFrac8)
  {
    sm_config_set_clkdiv_int_frac(&_config, divInt, divFrac8);
    return *this;
  }

  RpPioSmConfig& setClockDivisor(float div)
  {
    sm_config_set_clkdiv(&_config, div);
    return *this;
  }

  template <typename TIntegral, typename std::enable_if<std::is_integral<TIntegral>::value, int>::type = 0> RpPioSmConfig& setClockDivisor(TIntegral bitRateHz)
  {
    if (bitRateHz <= 0)
    {
      return *this;
    }

    const float div = static_cast<float>(clock_get_hz(clk_sys)) / static_cast<float>(bitRateHz);
    sm_config_set_clkdiv(&_config, div);
    return *this;
  }

  RpPioSmConfig& setWrap(uint wrapTarget, uint wrap)
  {
    sm_config_set_wrap(&_config, wrapTarget, wrap);
    return *this;
  }

  RpPioSmConfig& setJmpPin(uint pin)
  {
    sm_config_set_jmp_pin(&_config, pin);
    return *this;
  }

  RpPioSmConfig& setInShift(bool shiftRight, bool autopush, uint pushThreshold)
  {
    sm_config_set_in_shift(&_config, shiftRight, autopush, pushThreshold);
    return *this;
  }

  RpPioSmConfig& setOutShift(bool shiftRight, bool autopull, uint pullThreshold)
  {
    sm_config_set_out_shift(&_config, shiftRight, autopull, pullThreshold);
    return *this;
  }

  RpPioSmConfig& setFifoJoin(enum pio_fifo_join join)
  {
    sm_config_set_fifo_join(&_config, join);
    return *this;
  }

  RpPioSmConfig& setOutSpecial(bool sticky, bool hasEnablePin, uint enableBitIndex)
  {
    sm_config_set_out_special(&_config, sticky, hasEnablePin, enableBitIndex);
    return *this;
  }

  RpPioSmConfig& setMovStatus(enum pio_mov_status_type statusSel, uint statusN)
  {
    sm_config_set_mov_status(&_config, statusSel, statusN);
    return *this;
  }

  const pio_sm_config* raw() const { return &_config; }

  pio_sm_config* raw() { return &_config; }

  const pio_sm_config& get() const { return _config; }

private:
  pio_sm_config _config;
};

} // namespace lw::transports::rp2040

#endif // ARDUINO_ARCH_RP2040
