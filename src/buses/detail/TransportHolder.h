#pragma once

#include <cstdint>
#include <utility>

#include "core/Compat.h"
#include "core/RuntimeConfig.h"

namespace lw::buses::detail
{

// ---------------------------------------------------------------------------
// Hand-rolled vtable for type-erased Transport dispatch.
// No RTTI, no exceptions, no C++ virtual dispatch.
// ---------------------------------------------------------------------------

struct TransportVTable
{
  void (*begin)(void* self);
  void (*beginTransaction)(void* self);
  void (*transmitBytes)(void* self, lw::span<uint8_t> data);
  void (*endTransaction)(void* self);
  bool (*isReadyToUpdate)(const void* self);
  void (*setRuntimeConfig)(void* self, lw::RuntimeConfig type, void* value);
  void (*destroy)(void* self);
};

// ---------------------------------------------------------------------------
// Per-type static vtable + dispatch stubs.
// ---------------------------------------------------------------------------

template <typename T> struct TransportModel
{
  static void begin(void* self) { static_cast<T*>(self)->begin(); }

  static void beginTransaction(void* self) { static_cast<T*>(self)->beginTransaction(); }

  static void transmitBytes(void* self, lw::span<uint8_t> data) { static_cast<T*>(self)->transmitBytes(data); }

  static void endTransaction(void* self) { static_cast<T*>(self)->endTransaction(); }

  static bool isReadyToUpdate(const void* self) { return static_cast<const T*>(self)->isReadyToUpdate(); }

  static void setRuntimeConfig(void* self, lw::RuntimeConfig type, void* value) { static_cast<T*>(self)->setRuntimeConfig(type, value); }

  static void destroy(void* self) { delete static_cast<T*>(self); }

  static constexpr TransportVTable vtable{&begin, &beginTransaction, &transmitBytes, &endTransaction, &isReadyToUpdate, &setRuntimeConfig, &destroy};
};

// ---------------------------------------------------------------------------
// TransportHolder — move-only, type-erased owner of any Transport.
// ---------------------------------------------------------------------------

class TransportHolder
{
public:
  TransportHolder() = default;

  /// Take ownership of a concrete Transport by move.
  template <typename T> explicit TransportHolder(T transport) : _storage(new T(std::move(transport))), _vtable(&TransportModel<T>::vtable) {}

  ~TransportHolder() { _destroy(); }

  // Move-only
  TransportHolder(TransportHolder&& other) noexcept : _storage(other._storage), _vtable(other._vtable)
  {
    other._storage = nullptr;
    other._vtable = nullptr;
  }

  TransportHolder& operator=(TransportHolder&& other) noexcept
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

  TransportHolder(const TransportHolder&) = delete;
  TransportHolder& operator=(const TransportHolder&) = delete;

  // --- Forwarding dispatch ---

  void begin()
  {
    if (_vtable)
      _vtable->begin(_storage);
  }

  void beginTransaction()
  {
    if (_vtable)
      _vtable->beginTransaction(_storage);
  }

  void transmitBytes(lw::span<uint8_t> data)
  {
    if (_vtable)
      _vtable->transmitBytes(_storage, data);
  }

  void endTransaction()
  {
    if (_vtable)
      _vtable->endTransaction(_storage);
  }

  bool isReadyToUpdate() const { return _vtable ? _vtable->isReadyToUpdate(_storage) : true; }

  void setRuntimeConfig(lw::RuntimeConfig type, void* value)
  {
    if (_vtable)
      _vtable->setRuntimeConfig(_storage, type, value);
  }

  /// True when a transport has been set.
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
  const TransportVTable* _vtable = nullptr;
};

} // namespace lw::buses::detail
