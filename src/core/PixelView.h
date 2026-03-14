#pragma once

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <functional>
#include <iterator>
#include <limits>
#include <type_traits>
#include <vector>

#include "core/Compat.h"

namespace lw
{

template <typename TColor> class PixelView
{
  public:
    using ColorType = TColor;
    using ColorRef = std::add_lvalue_reference_t<TColor>;
    using ConstColorRef = std::add_lvalue_reference_t<const std::remove_reference_t<TColor>>;
    using ChunkType = span<TColor>;

    class iterator;
    class const_iterator;

    PixelView() = default;

    explicit PixelView(span<ChunkType> chunks) : _chunks(chunks) {}

    [[nodiscard]] PixelView operator+(const PixelView& other) const { return concatenate(*this, other); }

    static PixelView concatenate(const PixelView& first) { return first; }

    template <typename... TOtherViews, typename = std::enable_if_t<std::conjunction<
                                           std::is_same<PixelView, lw::remove_cvref_t<TOtherViews>>...>::value>>
    static PixelView concatenate(const PixelView& first, const TOtherViews&... others)
    {
        std::vector<ChunkType> concatenated;
        const size_t totalChunks = first._chunks.size() + (static_cast<size_t>(others._chunks.size()) + ... + 0u);
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

    [[nodiscard]] uint32_t size() const
    {
        uint32_t total = 0;
        for (const auto chunk : _chunks)
        {
            const auto chunkSize = static_cast<uint32_t>(chunk.size());
            const uint32_t available = std::numeric_limits<uint32_t>::max() - total;
            total += (chunkSize < available) ? chunkSize : available;
            if (total == std::numeric_limits<uint32_t>::max())
            {
                break;
            }
        }
        return total;
    }

    TColor operator[](uint32_t index) const { return constRefAt(index); }

    ColorRef operator[](uint32_t index) { return refAt(index); }

    iterator begin() { return iterator(this, 0); }

    iterator end() { return iterator(this, size()); }

    const_iterator begin() const { return cbegin(); }

    const_iterator end() const { return cend(); }

    const_iterator cbegin() const { return const_iterator(this, 0); }

    const_iterator cend() const { return const_iterator(this, size()); }

    span<ChunkType> chunks() { return _chunks; }

    span<const ChunkType> chunks() const { return span<const ChunkType>{_chunks.data(), _chunks.size()}; }

    [[nodiscard]] PixelView slice(uint32_t startIndex, uint32_t endIndex)
    {
        const uint32_t totalSize = size();
        const uint32_t clampedStart = std::min(startIndex, totalSize);
        const uint32_t clampedEnd = std::min(endIndex, totalSize);
        const uint32_t normalizedEnd = (clampedEnd < clampedStart) ? clampedStart : clampedEnd;

        std::vector<ChunkType> sliced;
        sliced.reserve(_chunks.size());

        uint32_t globalOffset = 0;
        for (auto chunk : _chunks)
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
    explicit PixelView(std::vector<ChunkType>&& ownedChunks)
        : _ownedChunks(std::move(ownedChunks)), _chunks(_ownedChunks.data(), _ownedChunks.size())
    {
    }

    static void appendChunks(std::vector<ChunkType>& destination, const PixelView& source)
    {
        for (const auto chunk : source._chunks)
        {
            destination.push_back(chunk);
        }
    }

    ColorRef refAt(uint32_t index)
    {
        assert(index < size());

        uint32_t offset = index;
        for (auto chunk : _chunks)
        {
            const uint32_t chunkSize = static_cast<uint32_t>(chunk.size());
            if (offset < chunkSize)
            {
                return chunk[offset];
            }
            offset -= chunkSize;
        }

        // Unreachable with valid preconditions.
        return _chunks[0][0];
    }

    ConstColorRef constRefAt(uint32_t index) const
    {
        assert(index < size());

        uint32_t offset = index;
        for (const auto chunk : _chunks)
        {
            const uint32_t chunkSize = static_cast<uint32_t>(chunk.size());
            if (offset < chunkSize)
            {
                return chunk[offset];
            }
            offset -= chunkSize;
        }

        return _chunks[0][0];
    }

    std::vector<ChunkType> _ownedChunks;
    span<ChunkType> _chunks;

  public:
    class iterator
    {
      public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = TColor;
        using difference_type = std::ptrdiff_t;
        using reference = ColorRef;
        using pointer = std::add_pointer_t<TColor>;

        iterator() = default;

        iterator(PixelView* view, uint32_t index) : _view(view), _index(index) {}

        reference operator*() const { return _view->refAt(_index); }

        pointer operator->() const { return std::addressof(_view->refAt(_index)); }

        reference operator[](difference_type n) const { return _view->refAt(static_cast<uint32_t>(_index + n)); }

        iterator& operator++()
        {
            ++_index;
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
            return *this;
        }

        iterator& operator-=(difference_type n)
        {
            _index = static_cast<uint32_t>(_index - n);
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

        friend difference_type operator-(const iterator& a, const iterator& b)
        {
            return static_cast<difference_type>(a._index) - static_cast<difference_type>(b._index);
        }

        friend bool operator==(const iterator& a, const iterator& b)
        {
            return a._view == b._view && a._index == b._index;
        }

        friend bool operator!=(const iterator& a, const iterator& b) { return !(a == b); }

        friend bool operator<(const iterator& a, const iterator& b) { return a._index < b._index; }

        friend bool operator<=(const iterator& a, const iterator& b) { return a._index <= b._index; }

        friend bool operator>(const iterator& a, const iterator& b) { return a._index > b._index; }

        friend bool operator>=(const iterator& a, const iterator& b) { return a._index >= b._index; }

      private:
        friend class const_iterator;

        PixelView* _view{nullptr};
        uint32_t _index{0};
    };

    class const_iterator
    {
      public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = TColor;
        using difference_type = std::ptrdiff_t;
        using reference = ConstColorRef;
        using pointer = std::add_pointer_t<const std::remove_reference_t<TColor>>;

        const_iterator() = default;

        const_iterator(const PixelView* view, uint32_t index) : _view(view), _index(index) {}

        const_iterator(const iterator& it) : _view(it._view), _index(it._index) {}

        reference operator*() const { return _view->constRefAt(_index); }

        pointer operator->() const { return std::addressof(_view->constRefAt(_index)); }

        reference operator[](difference_type n) const { return _view->constRefAt(static_cast<uint32_t>(_index + n)); }

        const_iterator& operator++()
        {
            ++_index;
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
            return *this;
        }

        const_iterator& operator-=(difference_type n)
        {
            _index = static_cast<uint32_t>(_index - n);
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

        friend difference_type operator-(const const_iterator& a, const const_iterator& b)
        {
            return static_cast<difference_type>(a._index) - static_cast<difference_type>(b._index);
        }

        friend bool operator==(const const_iterator& a, const const_iterator& b)
        {
            return a._view == b._view && a._index == b._index;
        }

        friend bool operator!=(const const_iterator& a, const const_iterator& b) { return !(a == b); }

        friend bool operator<(const const_iterator& a, const const_iterator& b) { return a._index < b._index; }

        friend bool operator<=(const const_iterator& a, const const_iterator& b) { return a._index <= b._index; }

        friend bool operator>(const const_iterator& a, const const_iterator& b) { return a._index > b._index; }

        friend bool operator>=(const const_iterator& a, const const_iterator& b) { return a._index >= b._index; }

      private:
        const PixelView* _view{nullptr};
        uint32_t _index{0};
    };
};

template <typename TColor> inline void fillPixels(PixelView<TColor>& pixels, const TColor& solidColor)
{
    for (auto& pixel : pixels)
    {
        pixel = solidColor;
    }
}

template <typename TColor, typename TGenerator,
          typename = std::enable_if_t<std::is_invocable_r<TColor, TGenerator&, uint32_t>::value>>
inline void fillPixelsIndexed(PixelView<TColor>& pixels, TGenerator&& generator)
{
    uint32_t index = 0;
    for (auto& pixel : pixels)
    {
        pixel = static_cast<TColor>(generator(index));
        ++index;
    }
}

} // namespace lw
