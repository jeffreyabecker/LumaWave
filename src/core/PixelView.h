#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <type_traits>
#include <vector>

#include "colors/Color.h"`n #include "core/Compat.h"

namespace lw
{

class PixelView
{
public:
  using ColorType = lw::colors::Color;
  using ColorRef = std::add_lvalue_reference_t<lw::colors::Color>;
  using ConstColorRef = std::add_lvalue_reference_t<const std::remove_reference_t<lw::colors::Color>>;
  using ChunkType = span<lw::colors::Color>;

  class iterator;
  class const_iterator;

  PixelView() = default;

  explicit PixelView(ChunkType chunk) { initializeFlat(chunk, true); }

  explicit PixelView(span<ChunkType> chunks) { initializeFromChunks(chunks); }

  PixelView(const PixelView& other) { initializeFromOther(other); }

  PixelView(PixelView&& other) noexcept { initializeFromOther(std::move(other)); }

  PixelView& operator=(const PixelView& other)
  {
    if (this != &other)
    {
      initializeFromOther(other);
    }

    return *this;
  }

  PixelView& operator=(PixelView&& other) noexcept
  {
    if (this != &other)
    {
      initializeFromOther(std::move(other));
    }

    return *this;
  }

  [[nodiscard]] PixelView operator+(const PixelView& other) const { return concatenate(*this, other); }

  static PixelView concatenate(const PixelView& first) { return first; }

  template <typename... TOtherViews, typename = std::enable_if_t<(std::is_same_v<PixelView, std::remove_cv_t<std::remove_reference_t<TOtherViews>>> && ...)>>
  static PixelView concatenate(const PixelView& first, const TOtherViews&... others)
  {
    std::vector<ChunkType> concatenated;
    const size_t totalChunks = first.chunks().size() + (static_cast<size_t>(others.chunks().size()) + ... + 0u);
    concatenated.reserve(totalChunks);

    appendChunks(concatenated, first);
    (appendChunks(concatenated, others), ...);

    return PixelView(std::move(concatenated));
  }

  static PixelView concatenate(span<const std::reference_wrapper<PixelView>> views)
  {
    std::vector<ChunkType> concatenated;
    size_t totalChunks = 0;

    for (const auto& viewRef : views)
    {
      totalChunks += viewRef.get().chunks().size();
    }

    concatenated.reserve(totalChunks);

    for (const auto& viewRef : views)
    {
      appendChunks(concatenated, viewRef.get());
    }

    return PixelView(std::move(concatenated));
  }

  static PixelView concatenate(span<const std::reference_wrapper<const PixelView>> views)
  {
    std::vector<ChunkType> concatenated;
    size_t totalChunks = 0;

    for (const auto& viewRef : views)
    {
      totalChunks += viewRef.get().chunks().size();
    }

    concatenated.reserve(totalChunks);

    for (const auto& viewRef : views)
    {
      appendChunks(concatenated, viewRef.get());
    }

    return PixelView(std::move(concatenated));
  }

  [[nodiscard]] uint32_t size() const { return _size; }

  lw::colors::Color operator[](uint32_t index) const { return constRefAt(index); }

  ColorRef operator[](uint32_t index) { return refAt(index); }

  iterator begin() { return iterator(this, 0); }

  iterator end() { return iterator(this, size()); }

  const_iterator begin() const { return cbegin(); }

  const_iterator end() const { return cend(); }

  const_iterator cbegin() const { return const_iterator(this, 0); }

  const_iterator cend() const { return const_iterator(this, size()); }

  span<ChunkType> chunks() { return span<ChunkType>{_chunkData, _chunkCount}; }

  span<const ChunkType> chunks() const { return span<const ChunkType>{_chunkData, _chunkCount}; }

  [[nodiscard]] PixelView slice(uint32_t startIndex, uint32_t endIndex)
  {
    const uint32_t totalSize = size();
    const uint32_t clampedStart = std::min(startIndex, totalSize);
    const uint32_t clampedEnd = std::min(endIndex, totalSize);
    const uint32_t normalizedEnd = (clampedEnd < clampedStart) ? clampedStart : clampedEnd;

    std::vector<ChunkType> sliced;
    sliced.reserve(_chunkCount);

    uint32_t globalOffset = 0;
    for (auto chunk : chunks())
    {
      const uint32_t chunkSize = static_cast<uint32_t>(chunk.size());
      const uint32_t chunkStart = globalOffset;
      const uint32_t chunkEnd = chunkStart + chunkSize;

      const uint32_t intersectionStart = std::max(clampedStart, chunkStart);
      const uint32_t intersectionEnd = std::min(normalizedEnd, chunkEnd);

      if (intersectionStart < intersectionEnd)
      {
        const size_t localStart = static_cast<size_t>(intersectionStart - chunkStart);
        const size_t localLength = static_cast<size_t>(intersectionEnd - intersectionStart);
        sliced.emplace_back(chunk.data() + localStart, localLength);
      }

      globalOffset = chunkEnd;
      if (globalOffset >= normalizedEnd)
      {
        break;
      }
    }

    return PixelView(std::move(sliced));
  }

private:
  enum class StorageMode : uint8_t
  {
    Flat,
    Chunked,
  };

  struct ChunkLookupEntry
  {
    uint32_t chunkStart{0};
    uint32_t chunkEnd{0};
    uint32_t chunkIndex{0};
  };

  struct ChunkLookupCache
  {
    size_t entryIndex{0};
    bool valid{false};
  };

  explicit PixelView(std::vector<ChunkType>&& ownedChunks) { initializeOwnedChunks(std::move(ownedChunks)); }

  static void appendChunks(std::vector<ChunkType>& destination, const PixelView& source)
  {
    for (const auto chunk : source.chunks())
    {
      destination.push_back(chunk);
    }
  }

  static uint32_t clampVisibleSize(size_t chunkSize, uint32_t start)
  {
    const uint32_t maxIndex = std::numeric_limits<uint32_t>::max();
    const uint32_t clampedChunkSize = (chunkSize < static_cast<size_t>(maxIndex)) ? static_cast<uint32_t>(chunkSize) : maxIndex;
    const uint32_t available = maxIndex - start;
    return (clampedChunkSize < available) ? clampedChunkSize : available;
  }

  void initializeFlat(ChunkType chunk, bool exposeChunk)
  {
    _mode = StorageMode::Flat;
    _flatChunkStorage[0] = chunk;
    _flatData = chunk.data();
    _size = clampVisibleSize(chunk.size(), 0);
    _ownedChunks.clear();
    _lookupEntries.clear();
    _chunkData = exposeChunk ? _flatChunkStorage.data() : nullptr;
    _chunkCount = exposeChunk ? 1u : 0u;
    clearLookupCache();
  }

  void initializeChunked(ChunkType* chunkData, size_t chunkCount)
  {
    _mode = StorageMode::Chunked;
    _flatChunkStorage[0] = ChunkType{};
    _flatData = nullptr;
    _chunkData = chunkData;
    _chunkCount = chunkCount;
    _lookupEntries.clear();

    uint32_t chunkStart = 0;
    for (size_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
    {
      const uint32_t chunkSize = clampVisibleSize(chunkData[chunkIndex].size(), chunkStart);
      if (chunkSize == 0)
      {
        continue;
      }

      const uint32_t chunkEnd = chunkStart + chunkSize;
      _lookupEntries.push_back(ChunkLookupEntry{chunkStart, chunkEnd, static_cast<uint32_t>(chunkIndex)});
      chunkStart = chunkEnd;

      if (chunkStart == std::numeric_limits<uint32_t>::max())
      {
        break;
      }
    }

    _size = chunkStart;
    clearLookupCache();
  }

  void initializeFromChunks(span<ChunkType> chunks)
  {
    if (chunks.empty())
    {
      initializeFlat(ChunkType{}, false);
      return;
    }

    if (chunks.size() == 1)
    {
      initializeFlat(chunks[0], true);
      return;
    }

    _ownedChunks.clear();
    initializeChunked(chunks.data(), chunks.size());
  }

  void initializeOwnedChunks(std::vector<ChunkType>&& ownedChunks)
  {
    _ownedChunks = std::move(ownedChunks);

    if (_ownedChunks.empty())
    {
      initializeFlat(ChunkType{}, false);
      return;
    }

    if (_ownedChunks.size() == 1)
    {
      const ChunkType chunk = _ownedChunks[0];
      _ownedChunks.clear();
      initializeFlat(chunk, true);
      return;
    }

    initializeChunked(_ownedChunks.data(), _ownedChunks.size());
  }

  void initializeOwnedChunksCopy(const std::vector<ChunkType>& ownedChunks)
  {
    _ownedChunks = ownedChunks;

    if (_ownedChunks.empty())
    {
      initializeFlat(ChunkType{}, false);
      return;
    }

    if (_ownedChunks.size() == 1)
    {
      const ChunkType chunk = _ownedChunks[0];
      _ownedChunks.clear();
      initializeFlat(chunk, true);
      return;
    }

    initializeChunked(_ownedChunks.data(), _ownedChunks.size());
  }

  void initializeFromOther(const PixelView& other)
  {
    if (other._mode == StorageMode::Flat)
    {
      const ChunkType chunk = (other._chunkCount > 0) ? other._chunkData[0] : ChunkType{};
      initializeFlat(chunk, other._chunkCount > 0);
      return;
    }

    if (!other._ownedChunks.empty())
    {
      initializeOwnedChunksCopy(other._ownedChunks);
      return;
    }

    initializeChunked(other._chunkData, other._chunkCount);
  }

  void initializeFromOther(PixelView&& other)
  {
    if (other._mode == StorageMode::Flat)
    {
      const ChunkType chunk = (other._chunkCount > 0) ? other._chunkData[0] : ChunkType{};
      initializeFlat(chunk, other._chunkCount > 0);
      return;
    }

    if (!other._ownedChunks.empty())
    {
      initializeOwnedChunks(std::move(other._ownedChunks));
      return;
    }

    initializeChunked(other._chunkData, other._chunkCount);
  }

  void clearLookupCache() const { _lookupCache = ChunkLookupCache{}; }

  bool tryResolveFromCache(uint32_t index, size_t& entryIndex) const
  {
    if (!_lookupCache.valid || _lookupCache.entryIndex >= _lookupEntries.size())
    {
      return false;
    }

    const ChunkLookupEntry& cached = _lookupEntries[_lookupCache.entryIndex];
    if (index >= cached.chunkStart && index < cached.chunkEnd)
    {
      entryIndex = _lookupCache.entryIndex;
      return true;
    }

    if (_lookupCache.entryIndex + 1u < _lookupEntries.size())
    {
      const ChunkLookupEntry& next = _lookupEntries[_lookupCache.entryIndex + 1u];
      if (index >= next.chunkStart && index < next.chunkEnd)
      {
        _lookupCache.entryIndex += 1u;
        entryIndex = _lookupCache.entryIndex;
        return true;
      }
    }

    if (_lookupCache.entryIndex > 0)
    {
      const ChunkLookupEntry& previous = _lookupEntries[_lookupCache.entryIndex - 1u];
      if (index >= previous.chunkStart && index < previous.chunkEnd)
      {
        _lookupCache.entryIndex -= 1u;
        entryIndex = _lookupCache.entryIndex;
        return true;
      }
    }

    return false;
  }

  size_t resolveLookupEntryIndex(uint32_t index) const
  {
    size_t entryIndex = 0;
    if (tryResolveFromCache(index, entryIndex))
    {
      return entryIndex;
    }

    size_t left = 0;
    size_t right = _lookupEntries.size();
    while (left < right)
    {
      const size_t mid = left + ((right - left) / 2u);
      if (index < _lookupEntries[mid].chunkEnd)
      {
        right = mid;
      }
      else
      {
        left = mid + 1u;
      }
    }

    _lookupCache.entryIndex = left;
    _lookupCache.valid = (left < _lookupEntries.size());
    return left;
  }

  ColorRef refAt(uint32_t index)
  {
    assert(index < size());

    if (_mode == StorageMode::Flat)
    {
      return _flatData[index];
    }

    const size_t entryIndex = resolveLookupEntryIndex(index);
    assert(entryIndex < _lookupEntries.size());
    const ChunkLookupEntry& entry = _lookupEntries[entryIndex];
    return _chunkData[entry.chunkIndex][static_cast<size_t>(index - entry.chunkStart)];

    // Unreachable with valid preconditions.
    return _chunkData[0][0];
  }

  ConstColorRef constRefAt(uint32_t index) const
  {
    assert(index < size());

    if (_mode == StorageMode::Flat)
    {
      return _flatData[index];
    }

    const size_t entryIndex = resolveLookupEntryIndex(index);
    assert(entryIndex < _lookupEntries.size());
    const ChunkLookupEntry& entry = _lookupEntries[entryIndex];
    return _chunkData[entry.chunkIndex][static_cast<size_t>(index - entry.chunkStart)];

    return _chunkData[0][0];
  }

  StorageMode _mode{StorageMode::Flat};
  std::array<ChunkType, 1> _flatChunkStorage{};
  lw::colors::Color* _flatData{nullptr};
  std::vector<ChunkType> _ownedChunks;
  ChunkType* _chunkData{nullptr};
  size_t _chunkCount{0};
  uint32_t _size{0};
  std::vector<ChunkLookupEntry> _lookupEntries;
  mutable ChunkLookupCache _lookupCache;

public:
  class iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = lw::colors::Color;
    using difference_type = std::ptrdiff_t;
    using reference = ColorRef;
    using pointer = std::add_pointer_t<lw::colors::Color>;

    iterator() = default;

    iterator(PixelView* view, uint32_t index) : _view(view) { seekToIndex(index); }

    reference operator*() const
    {
      if (_view->_mode == StorageMode::Flat)
      {
        return _view->_flatData[_index];
      }

      const ChunkLookupEntry& entry = _view->_lookupEntries[_entryIndex];
      return _view->_chunkData[entry.chunkIndex][_localOffset];
    }

    pointer operator->() const { return std::addressof(operator*()); }

    reference operator[](difference_type n) const { return *(*this + n); }

    iterator& operator++()
    {
      ++_index;
      advanceForward();
      return *this;
    }

    iterator operator++(int)
    {
      iterator copy = *this;
      ++(*this);
      return copy;
    }

    iterator& operator--()
    {
      --_index;
      seekToIndex(_index);
      return *this;
    }

    iterator operator--(int)
    {
      iterator copy = *this;
      --(*this);
      return copy;
    }

    iterator& operator+=(difference_type n)
    {
      _index = static_cast<uint32_t>(_index + n);
      seekToIndex(_index);
      return *this;
    }

    iterator& operator-=(difference_type n)
    {
      _index = static_cast<uint32_t>(_index - n);
      seekToIndex(_index);
      return *this;
    }

    friend iterator operator+(iterator it, difference_type n)
    {
      it += n;
      return it;
    }

    friend iterator operator+(difference_type n, iterator it)
    {
      it += n;
      return it;
    }

    friend iterator operator-(iterator it, difference_type n)
    {
      it -= n;
      return it;
    }

    friend difference_type operator-(const iterator& a, const iterator& b) { return static_cast<difference_type>(a._index) - static_cast<difference_type>(b._index); }

    friend bool operator==(const iterator& a, const iterator& b) { return a._view == b._view && a._index == b._index; }

    friend bool operator!=(const iterator& a, const iterator& b) { return !(a == b); }

    friend bool operator<(const iterator& a, const iterator& b) { return a._index < b._index; }

    friend bool operator<=(const iterator& a, const iterator& b) { return a._index <= b._index; }

    friend bool operator>(const iterator& a, const iterator& b) { return a._index > b._index; }

    friend bool operator>=(const iterator& a, const iterator& b) { return a._index >= b._index; }

  private:
    friend class const_iterator;

    void seekToIndex(uint32_t index)
    {
      _index = index;
      _entryIndex = 0;
      _localOffset = 0;

      if (_view == nullptr || _view->_mode == StorageMode::Flat)
      {
        return;
      }

      if (index >= _view->_size || _view->_lookupEntries.empty())
      {
        _entryIndex = _view->_lookupEntries.size();
        return;
      }

      _entryIndex = _view->resolveLookupEntryIndex(index);
      const ChunkLookupEntry& entry = _view->_lookupEntries[_entryIndex];
      _localOffset = static_cast<uint32_t>(index - entry.chunkStart);
    }

    void advanceForward()
    {
      if (_view == nullptr || _view->_mode == StorageMode::Flat)
      {
        return;
      }

      if (_entryIndex >= _view->_lookupEntries.size())
      {
        return;
      }

      ++_localOffset;
      const ChunkLookupEntry& entry = _view->_lookupEntries[_entryIndex];
      if ((entry.chunkStart + _localOffset) < entry.chunkEnd)
      {
        return;
      }

      ++_entryIndex;
      _localOffset = 0;
    }

    PixelView* _view{nullptr};
    uint32_t _index{0};
    size_t _entryIndex{0};
    uint32_t _localOffset{0};
  };

  class const_iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = lw::colors::Color;
    using difference_type = std::ptrdiff_t;
    using reference = ConstColorRef;
    using pointer = std::add_pointer_t<const std::remove_reference_t<lw::colors::Color>>;

    const_iterator() = default;

    const_iterator(const PixelView* view, uint32_t index) : _view(view) { seekToIndex(index); }

    const_iterator(const iterator& it) : _view(it._view), _index(it._index), _entryIndex(it._entryIndex), _localOffset(it._localOffset) {}

    reference operator*() const
    {
      if (_view->_mode == StorageMode::Flat)
      {
        return _view->_flatData[_index];
      }

      const ChunkLookupEntry& entry = _view->_lookupEntries[_entryIndex];
      return _view->_chunkData[entry.chunkIndex][_localOffset];
    }

    pointer operator->() const { return std::addressof(operator*()); }

    reference operator[](difference_type n) const { return *(*this + n); }

    const_iterator& operator++()
    {
      ++_index;
      advanceForward();
      return *this;
    }

    const_iterator operator++(int)
    {
      const_iterator copy = *this;
      ++(*this);
      return copy;
    }

    const_iterator& operator--()
    {
      --_index;
      seekToIndex(_index);
      return *this;
    }

    const_iterator operator--(int)
    {
      const_iterator copy = *this;
      --(*this);
      return copy;
    }

    const_iterator& operator+=(difference_type n)
    {
      _index = static_cast<uint32_t>(_index + n);
      seekToIndex(_index);
      return *this;
    }

    const_iterator& operator-=(difference_type n)
    {
      _index = static_cast<uint32_t>(_index - n);
      seekToIndex(_index);
      return *this;
    }

    friend const_iterator operator+(const_iterator it, difference_type n)
    {
      it += n;
      return it;
    }

    friend const_iterator operator+(difference_type n, const_iterator it)
    {
      it += n;
      return it;
    }

    friend const_iterator operator-(const_iterator it, difference_type n)
    {
      it -= n;
      return it;
    }

    friend difference_type operator-(const const_iterator& a, const const_iterator& b) { return static_cast<difference_type>(a._index) - static_cast<difference_type>(b._index); }

    friend bool operator==(const const_iterator& a, const const_iterator& b) { return a._view == b._view && a._index == b._index; }

    friend bool operator!=(const const_iterator& a, const const_iterator& b) { return !(a == b); }

    friend bool operator<(const const_iterator& a, const const_iterator& b) { return a._index < b._index; }

    friend bool operator<=(const const_iterator& a, const const_iterator& b) { return a._index <= b._index; }

    friend bool operator>(const const_iterator& a, const const_iterator& b) { return a._index > b._index; }

    friend bool operator>=(const const_iterator& a, const const_iterator& b) { return a._index >= b._index; }

  private:
    void seekToIndex(uint32_t index)
    {
      _index = index;
      _entryIndex = 0;
      _localOffset = 0;

      if (_view == nullptr || _view->_mode == StorageMode::Flat)
      {
        return;
      }

      if (index >= _view->_size || _view->_lookupEntries.empty())
      {
        _entryIndex = _view->_lookupEntries.size();
        return;
      }

      _entryIndex = _view->resolveLookupEntryIndex(index);
      const ChunkLookupEntry& entry = _view->_lookupEntries[_entryIndex];
      _localOffset = static_cast<uint32_t>(index - entry.chunkStart);
    }

    void advanceForward()
    {
      if (_view == nullptr || _view->_mode == StorageMode::Flat)
      {
        return;
      }

      if (_entryIndex >= _view->_lookupEntries.size())
      {
        return;
      }

      ++_localOffset;
      const ChunkLookupEntry& entry = _view->_lookupEntries[_entryIndex];
      if ((entry.chunkStart + _localOffset) < entry.chunkEnd)
      {
        return;
      }

      ++_entryIndex;
      _localOffset = 0;
    }

    const PixelView* _view{nullptr};
    uint32_t _index{0};
    size_t _entryIndex{0};
    uint32_t _localOffset{0};
  };
};

inline void fillPixels(PixelView& pixels, const lw::colors::Color& solidColor)
{
  for (auto& pixel : pixels)
  {
    pixel = solidColor;
  }
}

template <typename TGenerator, typename = std::enable_if_t<std::is_invocable_r<lw::colors::Color, TGenerator&, uint32_t>::value>> inline void fillPixelsIndexed(PixelView& pixels, TGenerator&& generator)
{
  uint32_t index = 0;
  for (auto& pixel : pixels)
  {
    pixel = static_cast<lw::colors::Color>(generator(index));
    ++index;
  }
}

} // namespace lw
