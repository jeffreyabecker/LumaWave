#pragma once

#include <cstdint>
#include <utility>

#include "core/Compat.h"
#include "core/Pixel.h"
#include "core/RuntimeConfig.h"
#include "protocols/Protocol.h"

namespace lw::buses::detail
{

// ---------------------------------------------------------------------------
// Hand-rolled vtable for type-erased Protocol dispatch.
// No RTTI, no exceptions, no C++ virtual dispatch.
// ---------------------------------------------------------------------------

struct ProtocolVTable
{
  void (*begin)(void* self);
  void (*update)(void* self, lw::span<const lw::Pixel> colors, lw::span<uint8_t> buffer);
  void* (*settings)(void* self); // returns ProtocolSettings*
  lw::PixelCount (*pixelCount)(const void* self);
  bool (*alwaysUpdate)(const void* self);
  void (*setRuntimeConfig)(void* self, lw::RuntimeConfig type, void* value);
  void (*destroy)(void* self);
};

// ---------------------------------------------------------------------------
// Per-type static vtable + dispatch stubs.
// ---------------------------------------------------------------------------

template <typename T> struct ProtocolModel
{
  static void begin(void* self) { static_cast<T*>(self)->begin(); }

  static void update(void* self, lw::span<const lw::Pixel> colors, lw::span<uint8_t> buffer) { static_cast<T*>(self)->update(colors, buffer); }

  static void* settings(void* self) { return &static_cast<T*>(self)->settings(); }

  static lw::PixelCount pixelCount(const void* self) { return static_cast<const T*>(self)->pixelCount(); }

  static bool alwaysUpdate(const void* self) { return static_cast<const T*>(self)->alwaysUpdate(); }

  static void setRuntimeConfig(void* self, lw::RuntimeConfig type, void* value) { static_cast<T*>(self)->setRuntimeConfig(type, value); }

  static void destroy(void* self) { delete static_cast<T*>(self); }

  static constexpr ProtocolVTable vtable{&begin, &update, &settings, &pixelCount, &alwaysUpdate, &setRuntimeConfig, &destroy};
};

// ---------------------------------------------------------------------------
// ProtocolHolder — move-only, type-erased owner of any Protocol.
// ---------------------------------------------------------------------------

class ProtocolHolder
{
public:
  ProtocolHolder() = default;

  /// Take ownership of a concrete Protocol by move.
  template <typename T> explicit ProtocolHolder(T protocol) : _storage(new T(std::move(protocol))), _vtable(&ProtocolModel<T>::vtable) {}

  ~ProtocolHolder() { _destroy(); }

  // Move-only
  ProtocolHolder(ProtocolHolder&& other) noexcept : _storage(other._storage), _vtable(other._vtable)
  {
    other._storage = nullptr;
    other._vtable = nullptr;
  }

  ProtocolHolder& operator=(ProtocolHolder&& other) noexcept
  {
    if (this != &other)
    {
      _destroy();
      _storage = other._storage;
      _vtable = other._vtable;
      other._storage = nullptr;
      other._vtable = nullptr;
    }
    return *this;
  }

  ProtocolHolder(const ProtocolHolder&) = delete;
  ProtocolHolder& operator=(const ProtocolHolder&) = delete;

  // --- Forwarding dispatch ---

  void begin()
  {
    if (_vtable)
      _vtable->begin(_storage);
  }

  void update(lw::span<const lw::Pixel> colors, lw::span<uint8_t> buffer)
  {
    if (_vtable)
      _vtable->update(_storage, colors, buffer);
  }

  /// Returns a pointer to the concrete ProtocolSettings.
  /// Valid only as long as this holder is alive.
  lw::protocols::ProtocolSettings* settings()
  {
    if (!_vtable)
      return nullptr;
    return static_cast<lw::protocols::ProtocolSettings*>(_vtable->settings(_storage));
  }

  const lw::protocols::ProtocolSettings* settings() const
  {
    if (!_vtable)
      return nullptr;
    return static_cast<const lw::protocols::ProtocolSettings*>(_vtable->settings(const_cast<void*>(_storage)));
  }

  lw::PixelCount pixelCount() const { return _vtable ? _vtable->pixelCount(_storage) : 0; }

  bool alwaysUpdate() const { return _vtable ? _vtable->alwaysUpdate(_storage) : false; }

  void setRuntimeConfig(lw::RuntimeConfig type, void* value)
  {
    if (_vtable)
      _vtable->setRuntimeConfig(_storage, type, value);
  }

  /// True when a protocol has been set.
  explicit operator bool() const { return _vtable != nullptr; }

private:
  void _destroy()
  {
    if (_storage && _vtable)
    {
      _vtable->destroy(_storage);
      _storage = nullptr;
      _vtable = nullptr;
    }
  }

  void* _storage = nullptr;
  const ProtocolVTable* _vtable = nullptr;
};

} // namespace lw::buses::detail
