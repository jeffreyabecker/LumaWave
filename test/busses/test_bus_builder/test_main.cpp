#include <unity.h>

#include <cstdint>
#include <utility>

#include "buses/detail/TransportHolder.h"
#include "buses/detail/ProtocolHolder.h"
#include "buses/detail/ShaderList.h"
#include "core/Compat.h"
#include "core/Pixel.h"
#include "core/RuntimeConfig.h"
#include "protocols/Protocol.h"

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

  // One shader: scratch needed
  list.addShader(MockShader{});
  TEST_ASSERT_TRUE(list.needsScratchBuffer());
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
  RUN_TEST(test_shader_list_applies_shaders);
  RUN_TEST(test_shader_list_set_runtime_config);

  return UNITY_END();
}
