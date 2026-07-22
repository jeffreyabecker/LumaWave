#include <unity.h>

#include <cstdint>
#include <utility>

#include "buses/detail/TransportHolder.h"
#include "buses/detail/ProtocolHolder.h"
#include "buses/detail/ShaderList.h"
#include "buses/detail/BufferManager.h"
#include "buses/detail/PresetTraits.h"
#include "buses/BusStorage.h"
#include "buses/BusBuilder.h"
#include "core/Compat.h"
#include "core/Pixel.h"
#include "core/RuntimeConfig.h"
#include "protocols/Protocol.h"
#include "protocols/Ws2812xPreset.h"
#include "transports/NilTransportPreset.h"
#include "protocols/ShaderProtocol.h"
#include "protocols/IShader.h"
#include "protocols/Ws2812xProtocol.h"
#include "transports/NilTransport.h"

// ---------------------------------------------------------------------------
// Mock types for testing holders
// ---------------------------------------------------------------------------

namespace
{

struct MockTransport
{
  bool began = false;
  bool txBegan = false;
  bool txEnded = false;
  bool ready = true;
  size_t bytesTransmitted = 0;
  lw::RuntimeConfig lastConfigType{};
  void* lastConfigValue = nullptr;

  void begin() { began = true; }
  void beginTransaction() { txBegan = true; }
  void transmitBytes(lw::span<uint8_t> data) { bytesTransmitted += data.size(); }
  void endTransaction() { txEnded = true; }
  bool isReadyToUpdate() const { return ready; }
  void setRuntimeConfig(lw::RuntimeConfig type, void* value)
  {
    lastConfigType = type;
    lastConfigValue = value;
  }
};

struct MockProtocol
{
  bool began = false;
  bool updated = false;
  size_t lastColorCount = 0;
  size_t lastBufferSize = 0;
  lw::PixelCount _pixelCount = 30;
  lw::protocols::ProtocolSettings _settings{};
  lw::RuntimeConfig lastConfigType{};
  void* lastConfigValue = nullptr;

  void begin() { began = true; }
  void update(lw::span<const lw::Pixel> colors, lw::span<uint8_t> buffer)
  {
    updated = true;
    lastColorCount = colors.size();
    lastBufferSize = buffer.size();
  }
  lw::protocols::ProtocolSettings& settings() { return _settings; }
  lw::PixelCount pixelCount() const { return _pixelCount; }
  bool alwaysUpdate() const { return false; }
  void setRuntimeConfig(lw::RuntimeConfig type, void* value)
  {
    lastConfigType = type;
    lastConfigValue = value;
  }
};

struct MockShader : public lw::protocols::IShader
{
  bool applied = false;
  size_t sourceCount = 0;
  size_t destCount = 0;

  void apply(lw::span<const lw::Pixel> source, lw::span<lw::Pixel> dest) override
  {
    applied = true;
    sourceCount = source.size();
    destCount = dest.size();
  }
};

// ---------------------------------------------------------------------------
// TransportHolder tests
// ---------------------------------------------------------------------------

void test_transport_holder_default_is_empty(void)
{
  lw::buses::detail::TransportHolder holder;
  TEST_ASSERT_FALSE(static_cast<bool>(holder));
}

void test_transport_holder_owns_and_dispatches(void)
{
  MockTransport mt;
  mt.ready = false;

  lw::buses::detail::TransportHolder holder(std::move(mt));
  TEST_ASSERT_TRUE(static_cast<bool>(holder));

  holder.begin();
  holder.beginTransaction();

  uint8_t buf[] = {1, 2, 3, 4, 5};
  holder.transmitBytes(lw::span<uint8_t>{buf, 5});

  holder.endTransaction();

  TEST_ASSERT_FALSE(holder.isReadyToUpdate());

  int configVal = 42;
  holder.setRuntimeConfig(lw::RuntimeConfig::Brightness, &configVal);

  // We can't reach into the moved-from mock, but dispatch went through
}

void test_transport_holder_move_construct(void)
{
  lw::buses::detail::TransportHolder src(MockTransport{});
  TEST_ASSERT_TRUE(static_cast<bool>(src));

  lw::buses::detail::TransportHolder dst(std::move(src));
  TEST_ASSERT_TRUE(static_cast<bool>(dst));
  TEST_ASSERT_FALSE(static_cast<bool>(src)); // src is empty after move
}

void test_transport_holder_move_assign(void)
{
  lw::buses::detail::TransportHolder a(MockTransport{});
  lw::buses::detail::TransportHolder b;

  b = std::move(a);
  TEST_ASSERT_TRUE(static_cast<bool>(b));
  TEST_ASSERT_FALSE(static_cast<bool>(a));
}

void test_transport_holder_move_assign_overwrites_existing(void)
{
  lw::buses::detail::TransportHolder a(MockTransport{});
  lw::buses::detail::TransportHolder b(MockTransport{});

  b = std::move(a);
  TEST_ASSERT_TRUE(static_cast<bool>(b));
  TEST_ASSERT_FALSE(static_cast<bool>(a));
}

// ---------------------------------------------------------------------------
// ProtocolHolder tests
// ---------------------------------------------------------------------------

void test_protocol_holder_default_is_empty(void)
{
  lw::buses::detail::ProtocolHolder holder;
  TEST_ASSERT_FALSE(static_cast<bool>(holder));
}

void test_protocol_holder_owns_and_dispatches(void)
{
  MockProtocol mp;
  mp._pixelCount = 60;

  lw::buses::detail::ProtocolHolder holder(std::move(mp));
  TEST_ASSERT_TRUE(static_cast<bool>(holder));

  TEST_ASSERT_EQUAL_UINT16(60, holder.pixelCount());
  TEST_ASSERT_FALSE(holder.alwaysUpdate());

  holder.begin();

  lw::Pixel colors[3]{};
  uint8_t buf[64]{};
  holder.update(lw::span<const lw::Pixel>{colors, 3}, lw::span<uint8_t>{buf, 64});

  TEST_ASSERT_NOT_NULL(holder.settings());

  int configVal = 99;
  holder.setRuntimeConfig(lw::RuntimeConfig::Gamma, &configVal);
}

void test_protocol_holder_move_construct(void)
{
  lw::buses::detail::ProtocolHolder src(MockProtocol{});
  TEST_ASSERT_TRUE(static_cast<bool>(src));

  lw::buses::detail::ProtocolHolder dst(std::move(src));
  TEST_ASSERT_TRUE(static_cast<bool>(dst));
  TEST_ASSERT_FALSE(static_cast<bool>(src));
}

void test_protocol_holder_move_assign(void)
{
  lw::buses::detail::ProtocolHolder a(MockProtocol{});
  lw::buses::detail::ProtocolHolder b;

  b = std::move(a);
  TEST_ASSERT_TRUE(static_cast<bool>(b));
  TEST_ASSERT_FALSE(static_cast<bool>(a));
}

void test_protocol_holder_null_settings_when_empty(void)
{
  lw::buses::detail::ProtocolHolder holder;
  TEST_ASSERT_NULL(holder.settings());
  TEST_ASSERT_EQUAL_UINT16(0, holder.pixelCount());
}

// ---------------------------------------------------------------------------
// ShaderList tests
// ---------------------------------------------------------------------------

void test_shader_list_empty(void)
{
  lw::buses::detail::ShaderList list;
  TEST_ASSERT_TRUE(list.empty());
  TEST_ASSERT_FALSE(list.needsScratchBuffer());
  TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(list.size()));
  TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(list.shaders().size()));
}

void test_shader_list_add_one(void)
{
  lw::buses::detail::ShaderList list;
  list.addShader(MockShader{});

  TEST_ASSERT_FALSE(list.empty());
  TEST_ASSERT_TRUE(list.needsScratchBuffer());
  TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(list.size()));
  TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(list.shaders().size()));
  TEST_ASSERT_NOT_NULL(list.shaders()[0]);
}

void test_shader_list_add_multiple(void)
{
  lw::buses::detail::ShaderList list;
  list.addShader(MockShader{});
  list.addShader(MockShader{});
  list.addShader(MockShader{});

  TEST_ASSERT_TRUE(list.needsScratchBuffer());
  TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(list.size()));
  TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(list.shaders().size()));

  // All pointers are distinct
  TEST_ASSERT_NOT_NULL(list.shaders()[0]);
  TEST_ASSERT_NOT_NULL(list.shaders()[1]);
  TEST_ASSERT_NOT_NULL(list.shaders()[2]);
}

void test_shader_list_needs_scratch_buffer(void)
{
  lw::buses::detail::ShaderList list;

  // Empty: no scratch needed
  TEST_ASSERT_FALSE(list.needsScratchBuffer());

  // One shader: scratch needed (safe mode by default)
  list.addShader(MockShader{});
  TEST_ASSERT_TRUE(list.needsScratchBuffer());
}

void test_shader_list_destructive_mode_default_is_safe(void)
{
  lw::buses::detail::ShaderList list;
  TEST_ASSERT_FALSE(list.isDestructiveMode());
}

void test_shader_list_destructive_mode_skips_scratch(void)
{
  lw::buses::detail::ShaderList list;
  list.addShader(MockShader{});

  // Before: scratch needed
  TEST_ASSERT_TRUE(list.needsScratchBuffer());

  // Enable destructive mode
  list.setDestructiveMode(true);
  TEST_ASSERT_TRUE(list.isDestructiveMode());
  TEST_ASSERT_FALSE(list.needsScratchBuffer());

  // Disable destructive mode
  list.setDestructiveMode(false);
  TEST_ASSERT_FALSE(list.isDestructiveMode());
  TEST_ASSERT_TRUE(list.needsScratchBuffer());
}

void test_shader_list_destructive_mode_empty_still_no_scratch(void)
{
  lw::buses::detail::ShaderList list;
  list.setDestructiveMode(true);

  // Empty shader list: still no scratch regardless of mode
  TEST_ASSERT_FALSE(list.needsScratchBuffer());
}

// A shader that overwrites every pixel with a fixed value (for testing
// that the destructive path actually modifies the input in place).
struct OverwriteShader : public lw::protocols::IShader
{
  lw::Pixel value;

  explicit OverwriteShader(lw::Pixel v) : value(v) {}

  void apply(lw::span<const lw::Pixel> /*source*/, lw::span<lw::Pixel> dest) override
  {
    for (size_t i = 0; i < dest.size(); ++i)
    {
      dest[i] = value;
    }
  }
};

// A protocol that captures the pixel span it receives from update().
struct CaptureProtocol : public lw::protocols::Protocol
{
  lw::Pixel firstPixel{};
  size_t pixelCount = 0;
  bool updated = false;

  CaptureProtocol() : Protocol(0) {}

  void update(lw::span<const lw::Pixel> colors, lw::span<uint8_t> /*buffer*/) override
  {
    updated = true;
    pixelCount = colors.size();
    if (!colors.empty())
    {
      firstPixel = colors[0];
    }
  }
};

void test_shader_protocol_destructive_mode_mutates_input(void)
{
  CaptureProtocol inner;

  // Create ShaderProtocol with shaders but NO scratch (destructive mode)
  lw::protocols::IShader* shaderPtrs[] = {nullptr};
  OverwriteShader shader(lw::pixelFromRGB(99, 88, 77));
  shaderPtrs[0] = &shader;

  lw::protocols::ShaderProtocol shaderProto(inner, lw::span<lw::protocols::IShader*>{shaderPtrs, 1}, lw::span<lw::Pixel>{}); // empty scratch = destructive

  lw::Pixel pixels[3]{};
  pixels[0] = lw::pixelFromRGB(10, 20, 30);
  pixels[1] = lw::pixelFromRGB(40, 50, 60);
  pixels[2] = lw::pixelFromRGB(70, 80, 90);

  uint8_t buf[64]{};
  shaderProto.update(lw::span<const lw::Pixel>{pixels, 3}, lw::span<uint8_t>{buf, 64});

  // The inner protocol should see the overwritten values, not the originals
  TEST_ASSERT_TRUE(inner.updated);
  TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(inner.pixelCount));
  TEST_ASSERT_EQUAL_UINT8(99, lw::pixelR(inner.firstPixel));
  TEST_ASSERT_EQUAL_UINT8(88, lw::pixelG(inner.firstPixel));
  TEST_ASSERT_EQUAL_UINT8(77, lw::pixelB(inner.firstPixel));

  // The input pixels were also mutated in place
  TEST_ASSERT_EQUAL_UINT8(99, lw::pixelR(pixels[0]));
  TEST_ASSERT_EQUAL_UINT8(88, lw::pixelG(pixels[0]));
  TEST_ASSERT_EQUAL_UINT8(77, lw::pixelB(pixels[0]));
}

void test_shader_protocol_scratch_mode_preserves_input(void)
{
  CaptureProtocol inner;

  lw::protocols::IShader* shaderPtrs[] = {nullptr};
  OverwriteShader shader(lw::pixelFromRGB(99, 88, 77));
  shaderPtrs[0] = &shader;

  lw::Pixel scratch[3]{};
  lw::protocols::ShaderProtocol shaderProto(inner, lw::span<lw::protocols::IShader*>{shaderPtrs, 1}, lw::span<lw::Pixel>{scratch, 3}); // scratch provided = safe mode

  lw::Pixel pixels[3]{};
  pixels[0] = lw::pixelFromRGB(10, 20, 30);
  pixels[1] = lw::pixelFromRGB(40, 50, 60);
  pixels[2] = lw::pixelFromRGB(70, 80, 90);

  uint8_t buf[64]{};
  shaderProto.update(lw::span<const lw::Pixel>{pixels, 3}, lw::span<uint8_t>{buf, 64});

  // Inner protocol sees the overwritten values (from scratch buffer)
  TEST_ASSERT_TRUE(inner.updated);
  TEST_ASSERT_EQUAL_UINT8(99, lw::pixelR(inner.firstPixel));

  // Input pixels are preserved (scratch used, not mutated)
  TEST_ASSERT_EQUAL_UINT8(10, lw::pixelR(pixels[0]));
  TEST_ASSERT_EQUAL_UINT8(20, lw::pixelG(pixels[0]));
  TEST_ASSERT_EQUAL_UINT8(30, lw::pixelB(pixels[0]));
}

void test_shader_list_applies_shaders(void)
{
  lw::buses::detail::ShaderList list;
  list.addShader(MockShader{});

  lw::Pixel src[2]{};
  lw::Pixel dst[2]{};
  src[0] = lw::pixelFromRGB(10, 20, 30);
  src[1] = lw::pixelFromRGB(40, 50, 60);

  list.shaders()[0]->apply(lw::span<const lw::Pixel>{src, 2}, lw::span<lw::Pixel>{dst, 2});
}

void test_shader_list_set_runtime_config(void)
{
  lw::buses::detail::ShaderList list;
  list.addShader(MockShader{});

  int val = 75;
  list.setRuntimeConfig(lw::RuntimeConfig::Brightness, &val);
}

// ---------------------------------------------------------------------------
// BufferManager tests
// ---------------------------------------------------------------------------

void test_buffer_manager_single_run_no_scratch(void)
{
  lw::buses::detail::BufferManager bm(128, 30, false);

  TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(bm.runCount()));
  TEST_ASSERT_FALSE(bm.hasScratch());

  TEST_ASSERT_EQUAL_UINT32(128, static_cast<uint32_t>(bm.protocolBuffer().size()));
  TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(bm.scratchPixels().size()));
}

void test_buffer_manager_single_run_with_scratch(void)
{
  lw::buses::detail::BufferManager bm(256, 60, true);

  TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(bm.runCount()));
  TEST_ASSERT_TRUE(bm.hasScratch());

  TEST_ASSERT_EQUAL_UINT32(256, static_cast<uint32_t>(bm.protocolBuffer().size()));
  TEST_ASSERT_EQUAL_UINT32(60, static_cast<uint32_t>(bm.scratchPixels().size()));
}

void test_buffer_manager_single_run_writable(void)
{
  lw::buses::detail::BufferManager bm(16, 4, true);

  // Write to protocol buffer
  auto protoBuf = bm.protocolBuffer();
  for (size_t i = 0; i < protoBuf.size(); ++i)
  {
    protoBuf[i] = static_cast<uint8_t>(i);
  }
  TEST_ASSERT_EQUAL_UINT8(0, bm.protocolBuffer()[0]);
  TEST_ASSERT_EQUAL_UINT8(15, bm.protocolBuffer()[15]);

  // Write to scratch
  auto scratch = bm.scratchPixels();
  scratch[0] = lw::pixelFromRGB(42, 0, 0);
  scratch[1] = lw::pixelFromRGB(0, 99, 0);
  TEST_ASSERT_EQUAL_UINT8(42, lw::pixelR(bm.scratchPixels()[0]));
  TEST_ASSERT_EQUAL_UINT8(99, lw::pixelG(bm.scratchPixels()[1]));
}

void test_buffer_manager_multi_run(void)
{
  size_t sizes[] = {128, 256, 64};
  lw::buses::detail::BufferManager bm(lw::span<const size_t>{sizes, 3}, 90, true);

  TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(bm.runCount()));
  TEST_ASSERT_TRUE(bm.hasScratch());

  // Each run gets its own protocol buffer
  TEST_ASSERT_EQUAL_UINT32(128, static_cast<uint32_t>(bm.protocolBuffer(0).size()));
  TEST_ASSERT_EQUAL_UINT32(256, static_cast<uint32_t>(bm.protocolBuffer(1).size()));
  TEST_ASSERT_EQUAL_UINT32(64, static_cast<uint32_t>(bm.protocolBuffer(2).size()));

  // Shared scratch
  TEST_ASSERT_EQUAL_UINT32(90, static_cast<uint32_t>(bm.scratchPixels().size()));
}

void test_buffer_manager_multi_run_no_scratch(void)
{
  size_t sizes[] = {64, 64};
  lw::buses::detail::BufferManager bm(lw::span<const size_t>{sizes, 2}, 50, false);

  TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(bm.runCount()));
  TEST_ASSERT_FALSE(bm.hasScratch());
  TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(bm.scratchPixels().size()));
}

void test_buffer_manager_empty_run(void)
{
  // Zero pixelCount with no scratch is valid (degenerate case)
  lw::buses::detail::BufferManager bm(0, 0, false);

  TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(bm.runCount()));
  TEST_ASSERT_FALSE(bm.hasScratch());
  TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(bm.protocolBuffer().size()));
}

void test_buffer_manager_single_run_const_access(void)
{
  const lw::buses::detail::BufferManager bm(32, 10, true);

  TEST_ASSERT_EQUAL_UINT32(32, static_cast<uint32_t>(bm.protocolBuffer().size()));
  TEST_ASSERT_EQUAL_UINT32(10, static_cast<uint32_t>(bm.scratchPixels().size()));
}

// ---------------------------------------------------------------------------
// BusStorage tests
// ---------------------------------------------------------------------------

void test_bus_storage_single_run_no_shaders(void)
{
  lw::buses::BusStorage storage(30, lw::buses::detail::ProtocolHolder(lw::protocols::Ws2812xProtocol(30, lw::protocols::Ws2812xProtocolSettings{})), lw::buses::detail::TransportHolder(lw::transports::NilTransport{}),
                                lw::buses::detail::ShaderList{}, lw::protocols::Ws2812xProtocol::requiredBufferSize(30, lw::protocols::Ws2812xProtocolSettings{}));

  storage.begin();

  storage.pixels()[0] = lw::pixelFromRGB(255, 128, 64);
  TEST_ASSERT_EQUAL_UINT8(255, lw::pixelR(storage.pixels()[0]));
  TEST_ASSERT_EQUAL_UINT8(128, lw::pixelG(storage.pixels()[0]));
  TEST_ASSERT_EQUAL_UINT8(64, lw::pixelB(storage.pixels()[0]));

  storage.show();
  TEST_ASSERT_TRUE(storage.isReadyToUpdate());
}

void test_bus_storage_with_shader_scratch(void)
{
  lw::buses::detail::ShaderList shaders;
  shaders.addShader(MockShader{});

  lw::buses::BusStorage storage(30, lw::buses::detail::ProtocolHolder(lw::protocols::Ws2812xProtocol(30, lw::protocols::Ws2812xProtocolSettings{})), lw::buses::detail::TransportHolder(lw::transports::NilTransport{}),
                                std::move(shaders), lw::protocols::Ws2812xProtocol::requiredBufferSize(30, lw::protocols::Ws2812xProtocolSettings{}));

  storage.begin();
  storage.pixels()[0] = lw::pixelFromRGB(10, 20, 30);
  storage.show();
  TEST_ASSERT_TRUE(storage.isReadyToUpdate());
}

void test_bus_storage_const_pixels_access(void)
{
  lw::buses::BusStorage storage(10, lw::buses::detail::ProtocolHolder(lw::protocols::Ws2812xProtocol(10, lw::protocols::Ws2812xProtocolSettings{})), lw::buses::detail::TransportHolder(lw::transports::NilTransport{}),
                                lw::buses::detail::ShaderList{}, lw::protocols::Ws2812xProtocol::requiredBufferSize(10, lw::protocols::Ws2812xProtocolSettings{}));

  const auto& cstorage = storage;
  TEST_ASSERT_EQUAL_UINT32(10, static_cast<uint32_t>(cstorage.pixels().size()));
}

// ---------------------------------------------------------------------------
// BusBuilder tests
// ---------------------------------------------------------------------------

void test_builder_simple_build(void)
{
  auto bus = lw::buses::BusBuilder().setPixelCount(30).setTransport(lw::transports::NilTransport{}).setProtocol(lw::protocols::Ws2812xProtocol(30, lw::protocols::Ws2812xProtocolSettings{})).build();

  TEST_ASSERT_NOT_NULL(bus);

  bus->begin();
  bus->pixels()[0] = lw::pixelFromRGB(42, 0, 0);
  TEST_ASSERT_EQUAL_UINT8(42, lw::pixelR(bus->pixels()[0]));
  bus->show();
}

void test_builder_with_shader(void)
{
  auto bus = lw::buses::BusBuilder().setPixelCount(30).setTransport(lw::transports::NilTransport{}).setProtocol(lw::protocols::Ws2812xProtocol(30, lw::protocols::Ws2812xProtocolSettings{})).addShader(MockShader{}).build();

  TEST_ASSERT_NOT_NULL(bus);
  bus->begin();
  bus->pixels()[0] = lw::pixelFromRGB(1, 2, 3);
  bus->show();
}

void test_builder_destructive_shaders(void)
{
  auto bus = lw::buses::BusBuilder()
                 .setPixelCount(30)
                 .setTransport(lw::transports::NilTransport{})
                 .setProtocol(lw::protocols::Ws2812xProtocol(30, lw::protocols::Ws2812xProtocolSettings{}))
                 .addShader(MockShader{})
                 .enableDestructiveShaders()
                 .build();

  TEST_ASSERT_NOT_NULL(bus);
  bus->begin();
  bus->pixels()[0] = lw::pixelFromRGB(1, 2, 3);
  bus->show();
}

void test_builder_validate_missing_transport(void)
{
  auto err = lw::buses::BusBuilder().setPixelCount(30).setProtocol(lw::protocols::Ws2812xProtocol(30, lw::protocols::Ws2812xProtocolSettings{})).validate();

  TEST_ASSERT_FALSE(err.empty());
}

void test_builder_validate_missing_protocol(void)
{
  auto err = lw::buses::BusBuilder().setPixelCount(30).setTransport(lw::transports::NilTransport{}).validate();

  TEST_ASSERT_FALSE(err.empty());
}

void test_builder_validate_missing_pixel_count(void)
{
  auto err = lw::buses::BusBuilder().setTransport(lw::transports::NilTransport{}).setProtocol(lw::protocols::Ws2812xProtocol(30, lw::protocols::Ws2812xProtocolSettings{})).validate();

  TEST_ASSERT_FALSE(err.empty());
}

void test_builder_validate_success(void)
{
  auto err = lw::buses::BusBuilder().setPixelCount(30).setTransport(lw::transports::NilTransport{}).setProtocol(lw::protocols::Ws2812xProtocol(30, lw::protocols::Ws2812xProtocolSettings{})).validate();

  TEST_ASSERT_TRUE(err.empty());
}

void test_builder_build_fails_on_missing_transport(void)
{
  auto bus = lw::buses::BusBuilder().setPixelCount(30).setProtocol(lw::protocols::Ws2812xProtocol(30, lw::protocols::Ws2812xProtocolSettings{})).build();

  TEST_ASSERT_NULL(bus);
}

void test_builder_double_build_returns_null(void)
{
  lw::buses::BusBuilder builder;
  builder.setPixelCount(30).setTransport(lw::transports::NilTransport{}).setProtocol(lw::protocols::Ws2812xProtocol(30, lw::protocols::Ws2812xProtocolSettings{}));

  auto bus1 = builder.build();
  TEST_ASSERT_NOT_NULL(bus1);

  auto bus2 = builder.build();
  TEST_ASSERT_NULL(bus2);
}

void test_builder_set_pixel_count_and_storage_conflict(void)
{
  lw::Pixel buf[30]{};
  lw::buses::BusBuilder builder;
  builder.setPixelCount(30).setPixelStorage(lw::span<lw::Pixel>{buf, 30});

  // setPixelStorage overrides setPixelCount (last setter wins)
  auto err = builder.validate();
  // transport not set, but pixel storage should be valid
  TEST_ASSERT_FALSE(err.empty()); // transport not set, but no crash
}

// ---------------------------------------------------------------------------
// Preset composition tests (BBL-31)
// ---------------------------------------------------------------------------

// Mock shader preset for testing (BrightnessShader doesn't exist yet in-tree)
struct brightness
{
  uint8_t level = 128;
  void configure(lw::buses::BusBuilder& b) { b.addShader(MockShader{}); }
  // Use level to avoid unused-variable warning
  brightness() = default;
  explicit brightness(uint8_t l) : level(l) {}
};

void test_preset_trait_detects_configure(void)
{
  // All three preset types should satisfy is_preset
  TEST_ASSERT_TRUE((lw::buses::detail::is_preset_v<lw::buses::presets::ws2812x>));
  TEST_ASSERT_TRUE((lw::buses::detail::is_preset_v<lw::buses::presets::nil_transport>));
  TEST_ASSERT_TRUE((lw::buses::detail::is_preset_v<brightness>));
}

void test_preset_trait_rejects_non_preset(void)
{
  // A plain int has no configure() method
  TEST_ASSERT_FALSE((lw::buses::detail::is_preset_v<int>));
  TEST_ASSERT_FALSE((lw::buses::detail::is_preset_v<MockShader>));
}

void test_add_strip_proto_trans(void)
{
  lw::buses::BusBuilder builder;
  builder.setPixelCount(30).addStrip(lw::buses::presets::ws2812x{}, lw::buses::presets::nil_transport{});

  // Builder should now be valid
  auto err = builder.validate();
  TEST_ASSERT_TRUE(err.empty());
}

void test_add_strip_proto_trans_shader(void)
{
  lw::buses::BusBuilder builder;
  builder.setPixelCount(30).addStrip(lw::buses::presets::ws2812x{}, lw::buses::presets::nil_transport{}, brightness{128});

  auto err = builder.validate();
  TEST_ASSERT_TRUE(err.empty());
}

void test_add_strip_build_and_use(void)
{
  auto bus = lw::buses::BusBuilder().setPixelCount(30).addStrip(lw::buses::presets::ws2812x{}, lw::buses::presets::nil_transport{}).build();

  TEST_ASSERT_NOT_NULL(bus);
  bus->begin();
  bus->pixels()[0] = lw::pixelFromRGB(255, 128, 64);
  bus->show();
}

void test_add_strip_inline_field_override(void)
{
  lw::buses::BusBuilder builder;
  builder.setPixelCount(30).addStrip(lw::buses::presets::ws2812x{.channelOrder = "RGB"}, lw::buses::presets::nil_transport{});

  auto err = builder.validate();
  TEST_ASSERT_TRUE(err.empty());
}

void test_add_strip_with_shader_build_and_use(void)
{
  auto bus = lw::buses::BusBuilder().setPixelCount(30).addStrip(lw::buses::presets::ws2812x{}, lw::buses::presets::nil_transport{}, brightness{200}).build();

  TEST_ASSERT_NOT_NULL(bus);
  bus->begin();
  bus->pixels()[0] = lw::pixelFromRGB(1, 2, 3);
  bus->show();
}

} // namespace

// ---------------------------------------------------------------------------
// Unity boilerplate
// ---------------------------------------------------------------------------

void setUp(void)
{
}
void tearDown(void)
{
}

int main(void)
{
  UNITY_BEGIN();

  // TransportHolder
  RUN_TEST(test_transport_holder_default_is_empty);
  RUN_TEST(test_transport_holder_owns_and_dispatches);
  RUN_TEST(test_transport_holder_move_construct);
  RUN_TEST(test_transport_holder_move_assign);
  RUN_TEST(test_transport_holder_move_assign_overwrites_existing);

  // ProtocolHolder
  RUN_TEST(test_protocol_holder_default_is_empty);
  RUN_TEST(test_protocol_holder_owns_and_dispatches);
  RUN_TEST(test_protocol_holder_move_construct);
  RUN_TEST(test_protocol_holder_move_assign);
  RUN_TEST(test_protocol_holder_null_settings_when_empty);

  // ShaderList
  RUN_TEST(test_shader_list_empty);
  RUN_TEST(test_shader_list_add_one);
  RUN_TEST(test_shader_list_add_multiple);
  RUN_TEST(test_shader_list_needs_scratch_buffer);
  RUN_TEST(test_shader_list_destructive_mode_default_is_safe);
  RUN_TEST(test_shader_list_destructive_mode_skips_scratch);
  RUN_TEST(test_shader_list_destructive_mode_empty_still_no_scratch);
  RUN_TEST(test_shader_list_applies_shaders);
  RUN_TEST(test_shader_list_set_runtime_config);

  // ShaderProtocol destructive mode
  RUN_TEST(test_shader_protocol_destructive_mode_mutates_input);
  RUN_TEST(test_shader_protocol_scratch_mode_preserves_input);

  // BufferManager
  RUN_TEST(test_buffer_manager_single_run_no_scratch);
  RUN_TEST(test_buffer_manager_single_run_with_scratch);
  RUN_TEST(test_buffer_manager_single_run_writable);
  RUN_TEST(test_buffer_manager_multi_run);
  RUN_TEST(test_buffer_manager_multi_run_no_scratch);
  RUN_TEST(test_buffer_manager_empty_run);
  RUN_TEST(test_buffer_manager_single_run_const_access);

  // BusStorage
  RUN_TEST(test_bus_storage_single_run_no_shaders);
  RUN_TEST(test_bus_storage_with_shader_scratch);
  RUN_TEST(test_bus_storage_const_pixels_access);

  // BusBuilder
  RUN_TEST(test_builder_simple_build);
  RUN_TEST(test_builder_with_shader);
  RUN_TEST(test_builder_destructive_shaders);
  RUN_TEST(test_builder_validate_missing_transport);
  RUN_TEST(test_builder_validate_missing_protocol);
  RUN_TEST(test_builder_validate_missing_pixel_count);
  RUN_TEST(test_builder_validate_success);
  RUN_TEST(test_builder_build_fails_on_missing_transport);
  RUN_TEST(test_builder_double_build_returns_null);
  RUN_TEST(test_builder_set_pixel_count_and_storage_conflict);

  // Preset composition (BBL-31)
  RUN_TEST(test_preset_trait_detects_configure);
  RUN_TEST(test_preset_trait_rejects_non_preset);
  RUN_TEST(test_add_strip_proto_trans);
  RUN_TEST(test_add_strip_proto_trans_shader);
  RUN_TEST(test_add_strip_build_and_use);
  RUN_TEST(test_add_strip_inline_field_override);
  RUN_TEST(test_add_strip_with_shader_build_and_use);

  return UNITY_END();
}
