# Platform Transport Inventory

> **Purpose:** Catalog of all transport/output-pipeline implementations before removal and rebuild.  
> **Date:** 2026-07-22

---

## Architecture Note

LumaWave has two categories of "things that push pixels to hardware":

| Category | Base Class | Signature | Role |
|----------|-----------|-----------|------|
| **Transport** | `lw::transports::Transport` | `transmitBytes(span<uint8_t>)` | Receives bytes from `ProtocolTransportPipeline` and sends them over hardware |
| **Light Driver** | `lw::OutputPipeline` | `write(span<const Color>)` | Receives raw `Color` pixels directly (bypasses protocol encoding) and drives pins |

A `Transport` is paired with a `Protocol` inside `ProtocolTransportPipeline`. A light driver is used as a standalone `OutputPipeline` in a `PipelineRun`, typically for PWM/sigma-delta direct-drive scenarios.

---

## 1  Core Base Class & Truly Platform-Agnostic

### 1.1  `Transport` (base class)
- **File:** `src/transports/Transport.h`
- **Base:** standalone
- **Interface:**
  - `begin()` — initialize hardware
  - `beginTransaction()` — start a transmission session
  - `transmitBytes(span<uint8_t>)` — send protocol-encoded bytes
  - `endTransaction()` — end a transmission session
  - `isReadyToUpdate()` — true if hardware can accept next frame
  - `setRuntimeConfig(RuntimeConfig, void*)` — runtime parameter injection
- **Settings contract:** All transport settings must expose `public bool invert`
- **Shared settings base:** `TransportSettingsBase` — `invert`, `clockRateHz`, `bitOrder`, `dataMode`, `clockPin`, `dataPin`
- **Constants defined:** `BitOrderLsbFirst/MsbFirst`, `SpiMode0-3`

### 1.2  `NilTransport`
- **File:** `src/transports/NilTransport.h`
- **Base:** `Transport`
- **How it works:** No-op for all virtual methods. Used in tests and as a placeholder.
- **Settings:** `NilTransportSettings` — empty struct

---

## 2  Arduino-Abstraction Transports

These transports depend on the Arduino SDK (`Arduino.h`, `SPI.h`, `Print`) and are not platform-agnostic.

### 2.1  `SpiTransport`
- **File:** `src/transports/SpiTransport.h`
- **Base:** `Transport`
- **Conditional:** `LW_HAS_SPI_TRANSPORT`
- **How it works:**
  - Takes an optional Arduino `SPIClass*` pointer in settings
  - `begin()`: calls `spi->begin()`, sets clock/data pins as OUTPUT
  - `beginTransaction()`: calls `spi->beginTransaction(SPISettings(...))` with configured clock rate, bit order, and SPI mode
  - `transmitBytes()`: loops over `span<uint8_t>` calling `spi->transfer(byte)`; if `invert` is true, transmits `~byte`
  - `endTransaction()`: calls `spi->endTransaction()`
  - No DMA — byte-at-a-time SPI transfers
- **Settings:** `SpiTransportSettings` extends `TransportSettingsBase` with `SPIClass* spi`

### 2.2  `PrintTransport<TWritable>`
- **File:** `src/transports/PrintTransport.h`
- **Base:** `Transport`
- **Template param:** `TWritable` (defaults to Arduino `Print` or `NullWritable` on native)
- **How it works:**
  - Wraps any `Writable` (Arduino `Print`-compatible) object
  - `transmitBytes()` writes raw bytes to output; if `asciiOutput` mode, converts to hex chars
  - `debugOutput` mode prints `[BUS:id] begin/beginTransaction/bytes(N)/endTransaction` annotations
  - Formats unsigned decimals manually (no `printf` dependency)
  - Default output: `&Serial` on Arduino
- **Settings:** `PrintTransportSettingsT<TWritable>` — `output`, `asciiOutput`, `debugOutput`, `identifier`

---

## 3  Platform-Agnostic Light Drivers (OutputPipeline)

### 3.1  `PwmOutputPipeline`
- **File:** `src/transports/PwmOutputPipeline.h`
- **Base:** `OutputPipeline`
- **How it works:**
  - Abstract PWM driver — declares pure virtual platform hooks:
    - `platformAnalogWrite(int pin, uint16_t value)`
    - `platformPinMode(int pin, PinMode mode)`
    - `platformAnalogWriteFreq(uint32_t freq)`
    - `platformAnalogWriteRange(uint16_t range)`
  - `begin()`: calls platform hooks to configure PWM frequency/range, sets pins to OUTPUT
  - `write(span<const Color>)`: reads first color's channel components, scales to PWM range (`component * pwmMax / componentMax`), writes to 4 pins (R,G,B,W). Handles `invert` by subtracting from max.
  - Destructor: zeros PWM outputs, sets pins back to INPUT
  - Only processes the **first** color in the span (single-pixel PWM)
- **Settings:** `PwmOutputPipelineSettings` — `pins[4]`, `pwmRange` (default 1023), `pwmFrequencyHz` (default 1000), `invert`

### 3.2  `PrintOutputPipeline<TWritable>`
- **File:** `src/transports/PrintOutputPipeline.h`
- **Base:** `OutputPipeline`
- **How it works:**
  - Writes raw color data via a `Writable` (Arduino `Print`) interface
  - `write(span<const Color>)`: iterates all colors; binary mode writes R,G,B,W channel bytes; ASCII mode writes hex chars
  - `debugOutput` mode prints `[LIGHT:id] begin/write` annotations
  - Unlike `PrintTransport` (which receives protocol bytes), this receives `Color` pixels directly
- **Settings:** `PrintOutputPipelineSettings<TWritable>` — `output`, `asciiOutput`, `debugOutput`, `identifier`

---

## 4  RP2040 Transports (Pico/Pico2W)

### 4.1  `RpPioTransport`
- **File:** `src/transports/rp2040/RpPioTransport.h`
- **Base:** `Transport`
- **Conditional:** `ARDUINO_ARCH_RP2040`
- **How it works:**
  - Uses RP2040 PIO (Programmable I/O) state machines for bit-banged protocols (e.g., WS2812 one-wire)
  - Requires `dataPin` and `clockPin` (clock must be `dataPin + 1` for contiguous GPIO mapping)
  - Leases a PIO state machine via `RpPioManager` and a DMA channel via `RpDmaManager`
  - `begin()`: configures PIO SM with clock divider, programs output pins, loads PIO program
  - `transmitBytes()`: waits for previous DMA to complete (`isReadyToUpdate`), then starts new DMA transfer into PIO TX FIFO
  - `isReadyToUpdate()`: polls DMA state (Idle or DmaCompleted)
  - Destructor: clears FIFOs, releases DMA + SM leases, sets pins to INPUT
  - Holdoff timing computed from `computeBitPeriodNs(clockRateHz)` for inter-frame gap
- **Settings:** `RpPioTransportSettings` — `pioIndex` (0 or 1), inherits `TransportSettingsBase`
- **Dependencies:** `RpPioManager`, `RpDmaManager`, `RpPioSmConfig`, RP2040 SDK (`hardware/pio.h`, `hardware/dma.h`)

### 4.2  `RpSpiTransport`
- **File:** `src/transports/rp2040/RpSpiTransport.h`
- **Base:** `Transport`
- **Conditional:** `ARDUINO_ARCH_RP2040`
- **How it works:**
  - Uses RP2040 hardware SPI peripheral (not Arduino SPI library)
  - Leases a DMA channel via `RpDmaManager`
  - `begin()`: calls `spi_init()` with clock rate, format (CPOL/CPHA/bit-order), maps data/clock pins to `GPIO_FUNC_SPI`
  - `transmitBytes()`: waits for previous DMA, then starts DMA transfer from buffer to SPI TX FIFO
  - `isReadyToUpdate()`: polls DMA completion state
  - Destructor: releases DMA lease, sets pins to INPUT
  - Holdoff timing computed same as PIO
- **Settings:** `RpSpiTransportSettings` — `spiIndex` (0 or 1), inherits `TransportSettingsBase`
- **Dependencies:** `RpDmaManager`, RP2040 SDK (`hardware/spi.h`, `hardware/dma.h`)

### 4.3  `RpUartTransport`
- **File:** `src/transports/rp2040/RpUartTransport.h`
- **Base:** `Transport`
- **Conditional:** `ARDUINO_ARCH_RP2040`
- **How it works:**
  - Uses RP2040 hardware UART (fixed 8N1: 8 data bits, 1 stop bit, no parity)
  - `begin()`: calls `uart_init()` with baud rate derived from `clockRateHz`, maps data pin to `GPIO_FUNC_UART`
  - `transmitBytes()`: DMA transfer from buffer to UART TX FIFO
  - Only `dataPin` is used (UART is TX-only, no clock pin needed for most led protocols)
  - Destructor: calls `uart_deinit()`, releases DMA
- **Settings:** `RpUartTransportSettings` — `spiIndex` (misleading name, indexes UART), inherits `TransportSettingsBase`
- **Dependencies:** `RpDmaManager`, RP2040 SDK (`hardware/uart.h`, `hardware/dma.h`)

### 4.4  `RpPwmLightDriver`
- **File:** `src/transports/rp2040/RpPwmLightDriver.h`
- **Base:** `OutputPipeline` (light driver, not Transport)
- **Conditional:** `ARDUINO_ARCH_RP2040`
- **How it works:**
  - Uses RP2040 hardware PWM slices directly (not Arduino `analogWrite`)
  - `begin()`: for each pin, calls `gpio_set_function(GPIO_FUNC_PWM)`, configures PWM slice with `clockDiv` and `wrap` values
  - Avoids re-initializing PWM slices shared by multiple pins
  - `write(span<const Color>)`: reads first color, scales each channel from component range to `wrap` value, writes to PWM compare register. Handles `invert`.
  - Destructor: sets pins back to INPUT
- **Settings:** `RpPwmLightDriverSettings` — `pins[4]`, `wrap` (default 255), `clockDiv` (default 4.0f), `invert`
- **Dependencies:** RP2040 SDK (`hardware/pwm.h`, `hardware/gpio.h`)

### 4.5  `RpPioManager` (utility)
- **File:** `src/transports/rp2040/RpPioManager.h`
- **How it works:** Manages PIO state machine leasing. A `StateMachineLease` acquires a specific SM from a PIO block (0 or 1), loads a program, and configures it. Released via RAII or explicit `release()`.

### 4.6  `RpDmaManager` (utility)
- **File:** `src/transports/rp2040/RpDmaManager.h`
- **How it works:** Manages DMA channel leasing. A `ChannelLease` acquires a DMA channel, configures transfers. Provides `computeFifoCacheEmptyDeltaUs()` to calculate inter-frame holdoff timing based on bit period. Tracks state: `Sending`, `DmaCompleted`, `Idle`.

---

## 5  ESP32 Transports

### 5.1  `Esp32RmtTransport`
- **File:** `src/transports/esp32/Esp32RmtTransport.h`
- **Base:** `Transport`
- **Conditional:** `ARDUINO_ARCH_ESP32` (excludes ESP32-C6 and ESP32-H2)
- **How it works:**
  - Uses ESP32 RMT (Remote Control) peripheral — the standard approach for WS2812/NeoPixel one-wire protocols
  - `begin()`: configures RMT TX channel with clock divider, idle level (high if `invert`), installs driver with interrupt + memory block allocation
  - Translates byte data to RMT items (pulse pairs) using timing from `OneWireTiming` (WS2812x default)
  - `transmitBytes()`: copies data to RMT item buffer, calls `rmt_write_items()` to start transmission
  - `isReadyToUpdate()`: checks `rmt_wait_tx_done(0)` — returns immediately if done
  - Destructor: waits for TX done, uninstalls RMT driver, restores pin to GPIO
- **Settings:** `Esp32RmtTransportSettings` — `rmt_channel_t channel` (0-7), inherits `TransportSettingsBase`. Clock rate derived from `OneWireTiming`.
- **Dependencies:** ESP-IDF (`driver/rmt.h`, `soc/rmt_struct.h`), `OneWireTiming` protocol header

### 5.2  `Esp32DmaSpiTransport`
- **File:** `src/transports/esp32/Esp32DmaSpiTransport.h`
- **Base:** `Transport`
- **Conditional:** `ARDUINO_ARCH_ESP32` + IDF >= 4.4.1
- **How it works:**
  - Uses ESP-IDF `spi_master` driver with DMA
  - `begin()` / `beginTransaction()`: no-ops (initialization is lazy)
  - `transmitBytes()`: calls `ensureReadyForWrite()` which lazily initializes SPI bus + DMA, then queues a DMA SPI transaction via `spi_device_queue_trans()`. Handles `invert` by XOR-ing bytes before transmission.
  - Allocates from DMA-capable heap (`MALLOC_CAP_DMA`)
  - Default pins: `SCK`/`MOSI` if defined, otherwise -1
- **Settings:** `Esp32DmaSpiTransportSettings` — `spiHost` (default from `LW_ESP32_DMA_SPI_DEFAULT_HOST`), `ssPin`, inherits `TransportSettingsBase`
- **Dependencies:** ESP-IDF (`driver/spi_master.h`, `esp_heap_caps.h`)

### 5.3  `Esp32I2sTransport`
- **File:** `src/transports/esp32/Esp32I2sTransport.h`
- **Base:** `Transport`
- **Conditional:** `ARDUINO_ARCH_ESP32` (excludes ESP32-S3 and ESP32-C3)
- **How it works:**
  - Uses ESP32 I2S peripheral in parallel-output mode for high-speed LED data
  - Direct I2S register manipulation (`I2S0.conf`, `I2S0.fifo`, etc.) — not the ESP-IDF I2S driver
  - `begin()`: no-op (lazy init)
  - `transmitBytes()`: calls `ensureReadyForWrite()` which sets up I2S clock, configures DMA-linked descriptors, starts transmission. Translates bytes to I2S FIFO words with bit-level encoding (handles clock/data pin output routing via `gpio_matrix_out`).
  - DMA descriptors linked in a chain; data allocated from DMA-capable heap
  - Destructor: waits for DMA completion, de-inits I2S, restores pins to GPIO
- **Settings:** `Esp32I2sTransportSettings` — `busNumber` (0 or 1), inherits `TransportSettingsBase`
- **Dependencies:** ESP-IDF (`rom/lldesc.h`, `soc/i2s_struct.h`, `soc/gpio_sig_map.h`, `esp_heap_caps.h`)

### 5.4  `Esp32LedcLightDriver`
- **File:** `src/transports/esp32/Esp32LedcLightDriver.h`
- **Base:** `OutputPipeline` (light driver)
- **Conditional:** `ARDUINO_ARCH_ESP8266` or `ARDUINO_ARCH_ESP32`
- **How it works:**
  - Uses ESP LEDC (LED PWM Controller) peripheral — 4 independent PWM channels
  - `begin()`: allocates LEDC timer + channels (one per pin); configures frequency and resolution
  - `write(span<const Color>)`: reads first color, scales each channel to resolution-bit duty cycle, calls `ledc_set_duty()` + `ledc_update_duty()`. Handles `invert`.
  - Destructor: zeros duty cycles, deallocates channels/timer, sets pins to INPUT
  - ESP8266 support: resolution 1-16 bits; ESP32: 1-14 bits
- **Settings:** `Esp32LedcLightDriverSettings` — `pins[4]`, `frequencyHz` (default 5000), `resolutionBits` (default 8), `invert`
- **Dependencies:** Arduino, ESP-IDF `driver/ledc.h`

### 5.5  `Esp32SigmaDeltaLightDriver`
- **File:** `src/transports/esp32/Esp32SigmaDeltaLightDriver.h`
- **Base:** `OutputPipeline` (light driver)
- **Conditional:** `ARDUINO_ARCH_ESP32`
- **How it works:**
  - Uses ESP32 sigma-delta modulation peripheral for analog-like LED driving
  - Supports both new `driver/sdm.h` and legacy `driver/sigmadelta.h`
  - `begin()`: configures SDM channels (one per pin) with sample rate and prescale
  - `write(span<const Color>)`: reads first color, scales each channel from component range to duty (-128 to 127 for SDM), writes via `sdm_channel_set_duty()`. Handles `invert`.
  - Destructor: disables SDM channels, sets pins to INPUT
- **Settings:** `Esp32SigmaDeltaLightDriverSettings` — `pins[4]`, `sampleRateHz` (default 1MHz), `prescale` (default 80), `invert`
- **Dependencies:** Arduino, ESP-IDF `driver/sdm.h` or `driver/sigmadelta.h`, `soc/soc_caps.h`

---

## 6  Key Implementation Patterns (for rebuild)

### Transport pattern (byte-oriented)
All transports follow the same lifecycle:
1. **Construct:** accept settings struct, store config
2. **`begin()`:** initialize hardware peripheral (SPI, PIO, RMT, UART, etc.)
3. **`beginTransaction()`:** prepare for a transmission burst
4. **`transmitBytes(span<uint8_t>)`:** send protocol-encoded bytes (possibly via DMA)
5. **`endTransaction()`:** finalize burst
6. **`isReadyToUpdate()`:** non-blocking check if hardware can accept next frame (DMA done, FIFO empty, etc.)
7. **Destructor:** clean up hardware resources, restore pin states

### Light driver pattern (pixel-oriented)
Light drivers bypass protocol encoding:
1. **Construct:** accept settings, store config
2. **`begin()`:** initialize PWM/SDM/LEDC hardware, set pin modes
3. **`write(span<const Color>)`:** directly drive pins from color channel values (typically first pixel only for PWM)
4. **Destructor:** zero outputs, restore pin states

### DMA pattern (RP2040 + ESP32)
- RP2040: `RpDmaManager` provides channel leasing; transports configure DMA to feed hardware FIFOs (PIO TX, SPI TX, UART TX)
- ESP32: DMA via ESP-IDF drivers (`spi_master`, I2S linked descriptors); data allocated from `MALLOC_CAP_DMA` heap
- Both platforms compute holdoff timing to respect inter-frame gaps required by LED protocols

### Pin management
- Most transports restore pins to INPUT on destruction (high-impedance, safe state)
- RP2040 uses `gpio_set_function()` to switch pin mux
- ESP32 uses `gpio_matrix_out()` + `pinMode()` to route and restore signals
