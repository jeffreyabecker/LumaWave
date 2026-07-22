#pragma once

#ifdef ARDUINO_ARCH_RP2040

#include <cstdint>
#include <cassert>

#include "hardware/pio.h"
#include "RpPioSmConfig.h"

namespace lw::transports::rp2040
{

class RpPioManager
{
public:
  class StateMachineLease
  {
  public:
    StateMachineLease() = default;

    StateMachineLease(const StateMachineLease&) = delete;
    StateMachineLease& operator=(const StateMachineLease&) = delete;

    StateMachineLease(StateMachineLease&& other) noexcept : _pio{other._pio}, _sm{other._sm}, _programOffset{other._programOffset}, _smConfig{other._smConfig}
    {
      other._pio = nullptr;
      other._sm = -1;
      other._programOffset = NotLoaded;
    }

    StateMachineLease& operator=(StateMachineLease&& other) noexcept
    {
      if (this == &other)
      {
        return *this;
      }

      release();

      _pio = other._pio;
      _sm = other._sm;
      _programOffset = other._programOffset;
      _smConfig = other._smConfig;

      other._pio = nullptr;
      other._sm = -1;
      other._programOffset = NotLoaded;

      return *this;
    }

    ~StateMachineLease() { release(); }

    bool isValid() const { return _pio != nullptr && _sm >= 0; }

    PIO pio() const { return _pio; }

    uint sm() const { return static_cast<uint>(_sm); }

    uint programOffset() const { return _programOffset; }

    uint dreq(bool isTx) const
    {
      if (!isValid())
      {
        return 0;
      }

      return pio_get_dreq(_pio, static_cast<uint>(_sm), isTx);
    }

    volatile void* txFifoWriteAddress() const
    {
      if (!isValid())
      {
        return nullptr;
      }

      return static_cast<volatile void*>(&(_pio->txf[static_cast<uint>(_sm)]));
    }

    void clearFifos() const
    {
      if (!isValid())
      {
        return;
      }

      pio_sm_clear_fifos(_pio, static_cast<uint>(_sm));
    }

    void setEnabled(bool enabled) const
    {
      if (!isValid())
      {
        return;
      }

      pio_sm_set_enabled(_pio, static_cast<uint>(_sm), enabled);
    }

    void gpioInit(uint pin) const
    {
      if (!isValid())
      {
        return;
      }

      pio_gpio_init(_pio, pin);
    }

    void setConsecutivePindirs(uint pin, uint count, bool isOut) const
    {
      if (!isValid())
      {
        return;
      }

      pio_sm_set_consecutive_pindirs(_pio, static_cast<uint>(_sm), pin, count, isOut);
    }

    void init() const
    {
      if (!isValid())
      {
        return;
      }

      pio_sm_init(_pio, static_cast<uint>(_sm), _programOffset, _smConfig.raw());
    }

    RpPioSmConfig& smConfig() { return _smConfig; }

    const RpPioSmConfig& smConfig() const { return _smConfig; }

    void release()
    {
      if (!isValid())
      {
        return;
      }

      RpPioManager::releaseStateMachine(_pio, static_cast<uint>(_sm));

      _pio = nullptr;
      _sm = -1;
      _programOffset = NotLoaded;
    }

  private:
    friend class RpPioManager;

    static constexpr uint NotLoaded = static_cast<uint>(-1);

    StateMachineLease(PIO pio, uint sm, uint programOffset) : _pio{pio}, _sm{static_cast<int>(sm)}, _programOffset{programOffset} {}

    PIO _pio{nullptr};
    int _sm{-1};
    uint _programOffset{NotLoaded};
    RpPioSmConfig _smConfig{};
  };

  static StateMachineLease requestStateMachine(const pio_program* program, int8_t pioIndex = -1)
  {
    assert(program != nullptr);

    if (pioIndex < 0)
    {
      const int8_t resolved = resolveBestPioIndex(program);
      assert(resolved >= 0);
      if (resolved < 0)
      {
        return StateMachineLease{};
      }
      return requestStateMachine(program, resolved);
    }

    assert(isValidPioIndex(pioIndex));
    if (!isValidPioIndex(pioIndex))
    {
      return StateMachineLease{};
    }

    const uint index = static_cast<uint>(pioIndex);
    PIO pio = indexToPio(index);

    const uint activeCount = countClaimedStateMachines(pio);
    assert(activeCount <= MaxStateMachinesPerPio);

    if (s_program[index] != nullptr && s_program[index] != program)
    {
      assert(activeCount == 0);
      if (activeCount != 0)
      {
        return StateMachineLease{};
      }

      pio_remove_program(pio, s_program[index], s_programOffset[index]);
      s_program[index] = nullptr;
      s_programOffset[index] = NotLoaded;
    }

    if (s_program[index] == nullptr)
    {
      assert(pio_can_add_program(pio, program));
      s_programOffset[index] = pio_add_program(pio, program);
      s_program[index] = program;
    }

    const uint sm = pio_claim_unused_sm(pio, true);
    return StateMachineLease{pio, sm, s_programOffset[index]};
  }

  static void releaseStateMachine(PIO pio, uint sm) { pio_sm_unclaim(pio, sm); }

private:
  static constexpr uint MaxStateMachinesPerPio = 4;
  static constexpr uint NotLoaded = static_cast<uint>(-1);

  static inline const pio_program* s_program[NUM_PIOS] = {
#if NUM_PIOS == 2
      nullptr, nullptr
#elif NUM_PIOS == 3
      nullptr, nullptr, nullptr
#endif
  };

  static inline uint s_programOffset[NUM_PIOS] = {
#if NUM_PIOS == 2
      NotLoaded, NotLoaded
#elif NUM_PIOS == 3
      NotLoaded, NotLoaded, NotLoaded
#endif
  };

  static bool isValidPioIndex(int8_t index) { return index >= 0 && static_cast<uint>(index) < NUM_PIOS; }

  static int8_t resolveBestPioIndex(const pio_program* program)
  {
    for (uint index = 0; index < NUM_PIOS; ++index)
    {
      PIO pio = indexToPio(index);
      const uint activeCount = countClaimedStateMachines(pio);
      const bool hasCapacity = activeCount < MaxStateMachinesPerPio;
      const bool runningRequestedProgram = s_program[index] == program;
      const bool runningNoProgram = activeCount == 0;

      if (hasCapacity && (runningRequestedProgram || runningNoProgram))
      {
        return static_cast<int8_t>(index);
      }
    }

    return -1;
  }

  static uint countClaimedStateMachines(PIO pio)
  {
    uint count = 0;
    for (uint sm = 0; sm < MaxStateMachinesPerPio; ++sm)
    {
      if (pio_sm_is_claimed(pio, sm))
      {
        ++count;
      }
    }
    return count;
  }

  static PIO indexToPio(uint index)
  {
    switch (index)
    {
      case 0:
        return pio0;
#if NUM_PIOS >= 2
      case 1:
        return pio1;
#endif
#if NUM_PIOS >= 3
      case 2:
        return pio2;
#endif
      default:
        return pio0;
    }
  }
};

} // namespace lw::transports::rp2040

#endif // ARDUINO_ARCH_RP2040
