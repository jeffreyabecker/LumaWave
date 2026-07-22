#pragma once

#ifdef ARDUINO_ARCH_RP2040

#include <cstdint>
#include <cassert>

#include "hardware/pio.h"

#include "RpPioManager.h"

namespace lw::transports::rp2040
{

class RpPioSession
{
public:
  RpPioSession() = default;

  RpPioSession(const RpPioSession&) = delete;
  RpPioSession& operator=(const RpPioSession&) = delete;

  RpPioSession(RpPioSession&&) = default;
  RpPioSession& operator=(RpPioSession&&) = default;

  bool acquire(const pio_program* program, int8_t pioIndex = -1)
  {
    _lease = RpPioManager::requestStateMachine(program, pioIndex);
    return _lease.isValid();
  }

  bool isValid() const { return _lease.isValid(); }

  PIO pio() const
  {
    assert(isValid());
    return _lease.pio();
  }

  uint sm() const
  {
    assert(isValid());
    return _lease.sm();
  }

  uint programOffset() const
  {
    assert(isValid());
    return _lease.programOffset();
  }

  void configureDataPin(uint pin)
  {
    assert(isValid());
    pio_gpio_init(pio(), pin);
    pio_sm_set_consecutive_pindirs(pio(), sm(), pin, 1, true);
  }

  void initStateMachine(uint offset, const pio_sm_config* config)
  {
    assert(isValid());
    assert(config != nullptr);
    pio_sm_init(pio(), sm(), offset, config);
    pio_sm_set_enabled(pio(), sm(), true);
  }

  void disableStateMachine()
  {
    if (!isValid())
    {
      return;
    }
    pio_sm_set_enabled(pio(), sm(), false);
  }

  void clearStateMachineFifos()
  {
    if (!isValid())
    {
      return;
    }
    pio_sm_clear_fifos(pio(), sm());
  }

  void releaseStateMachine() { _lease.release(); }

  /// Performs PIO-only teardown in this order:
  /// disable SM -> clear FIFOs -> unclaim SM.
  ///
  /// If DMA is active for this state machine, DMA must be stopped by
  /// the caller before invoking this method.
  void shutdownPioHardware()
  {
    disableStateMachine();
    clearStateMachineFifos();
    releaseStateMachine();
  }

  volatile void* txFifoAddress() const
  {
    assert(isValid());
    return static_cast<volatile void*>(&pio()->txf[sm()]);
  }

  uint txDreq() const
  {
    assert(isValid());
    return pio_get_dreq(pio(), sm(), true);
  }

private:
  RpPioManager::StateMachineLease _lease;
};

} // namespace lw::transports::rp2040

#endif // ARDUINO_ARCH_RP2040
