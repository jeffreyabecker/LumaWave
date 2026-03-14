#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>

#if __cplusplus >= 202002L
#include <compare>
#endif

#include "Color.h"

namespace lw::colors
{

// -----------------------------------------------------------------------
// ColorIteratorT ? wraps a std::function<TColor&(uint16_t)> + position
//
// Satisfies std::random_access_iterator.  The accessor returns a mutable
// TColor& so the same iterator type works for both reading (input) and
// writing (output) pixel data.
//
// The iterator does not track its own bounds ? that is the range's job.
// Source helpers (SolidColorSource, SpanColorSource) are proper ranges
// providing begin()/end() pairs that work with STL algorithms.
//
// Usage:
//   SolidColorSource fill{black, pixelCount};
//   std::copy(fill.begin(), fill.end(), dest);
//
//   SpanColorSource src{mySpan};
//   std::copy(src.begin(), src.end(), bus.pixelBuffer().begin());
//
//   // Subspan arithmetic:
//   auto it = src.begin() + 10;   // skip first 10 pixels
//
//   // Ad-hoc lambda:
//   std::vector<Color> buf(100);
//   ColorIterator begin{[&](uint16_t i) -> Color& { return buf[i]; }, 0};
//   ColorIterator end  {[&](uint16_t i) -> Color& { return buf[i]; }, 100};
// -----------------------------------------------------------------------
template <typename TColor> class ColorIteratorT
{
  public:
    using AccessorFn = std::function<TColor&(uint16_t idx)>;

    using iterator_category = std::random_access_iterator_tag;
    using value_type = TColor;
    using difference_type = std::ptrdiff_t;
    using pointer = TColor*;
    using reference = TColor&;

#if __cplusplus >= 202002L
    using iterator_concept = std::random_access_iterator_tag;
#endif

    // Default-constructed iterators compare equal (past-the-end)
    ColorIteratorT() = default;

    ColorIteratorT(AccessorFn accessor, uint16_t position) : _accessor(std::move(accessor)), _position(position) {}

    // Copyable / movable
    ColorIteratorT(const ColorIteratorT&) = default;
    ColorIteratorT& operator=(const ColorIteratorT&) = default;
    ColorIteratorT(ColorIteratorT&&) = default;
    ColorIteratorT& operator=(ColorIteratorT&&) = default;

    // -- Dereference ---------------------------------------------------

    TColor& operator*() const { return _accessor(_position); }

    TColor& operator[](difference_type n) const { return _accessor(static_cast<uint16_t>(_position + n)); }

    // -- Increment / decrement -----------------------------------------

    ColorIteratorT& operator++()
    {
        ++_position;
        return *this;
    }

    ColorIteratorT operator++(int)
    {
        auto tmp = *this;
        ++_position;
        return tmp;
    }

    ColorIteratorT& operator--()
    {
        --_position;
        return *this;
    }

    ColorIteratorT operator--(int)
    {
        auto tmp = *this;
        --_position;
        return tmp;
    }

    // -- Compound assignment -------------------------------------------

    ColorIteratorT& operator+=(difference_type n)
    {
        _position = static_cast<uint16_t>(_position + n);
        return *this;
    }

    ColorIteratorT& operator-=(difference_type n)
    {
        _position = static_cast<uint16_t>(_position - n);
        return *this;
    }

    // -- Arithmetic ----------------------------------------------------

    friend ColorIteratorT operator+(ColorIteratorT it, difference_type n)
    {
        it += n;
        return it;
    }

    friend ColorIteratorT operator+(difference_type n, ColorIteratorT it)
    {
        it += n;
        return it;
    }

    friend ColorIteratorT operator-(ColorIteratorT it, difference_type n)
    {
        it -= n;
        return it;
    }

    friend difference_type operator-(const ColorIteratorT& a, const ColorIteratorT& b)
    {
        return static_cast<difference_type>(a._position) - static_cast<difference_type>(b._position);
    }

    // -- Comparison ----------------------------------------------------

    friend bool operator==(const ColorIteratorT& a, const ColorIteratorT& b) { return a._position == b._position; }

#if __cplusplus >= 202002L
    friend auto operator<=>(const ColorIteratorT& a, const ColorIteratorT& b) { return a._position <=> b._position; }
#else
    friend bool operator!=(const ColorIteratorT& a, const ColorIteratorT& b) { return !(a == b); }

    friend bool operator<(const ColorIteratorT& a, const ColorIteratorT& b) { return a._position < b._position; }

    friend bool operator<=(const ColorIteratorT& a, const ColorIteratorT& b) { return a._position <= b._position; }

    friend bool operator>(const ColorIteratorT& a, const ColorIteratorT& b) { return a._position > b._position; }

    friend bool operator>=(const ColorIteratorT& a, const ColorIteratorT& b) { return a._position >= b._position; }
#endif

    // -- Observers -----------------------------------------------------

    uint16_t position() const { return _position; }

  private:
    AccessorFn _accessor;
    uint16_t _position{0};
};

// -----------------------------------------------------------------------
// SolidColorSource ? range that yields a constant color for N pixels
//
// Usage:
//   SolidColorSource fill{Color{}, 100};
//   std::copy(fill.begin(), fill.end(), bus.pixelBuffer().begin());
//   std::copy(fill.begin(), fill.end(), dest);
// -----------------------------------------------------------------------
template <typename TColor> struct SolidColorSourceT
{
    TColor color;
    PixelCount pixelCount;

    ColorIteratorT<TColor> begin()
    {
        return ColorIteratorT<TColor>{[this](uint16_t) -> TColor& { return color; }, 0};
    }

    ColorIteratorT<TColor> end()
    {
        return ColorIteratorT<TColor>{[this](uint16_t) -> TColor& { return color; }, pixelCount};
    }
};

template <typename TColor> using FillColorSourceT = SolidColorSourceT<TColor>;

// -----------------------------------------------------------------------
// SpanColorSource ? range that iterates over a span<Color>
//
// Because the reference is mutable, the same source can be used for
// writing into and reading from buffer-backed spans.
//
// Usage:
//   SpanColorSource src{myColors};
//   std::copy(src.begin(), src.end(), bus.pixelBuffer().begin() + destOffset);
//   std::copy(src.begin(), src.end(), dest);
// -----------------------------------------------------------------------
template <typename TColor> struct SpanColorSourceT
{
    span<TColor> data;
    SpanColorSourceT() = default;

    explicit SpanColorSourceT(span<TColor> span) : data(span) {}

    SpanColorSourceT(TColor* ptr, size_t size) : data(ptr, size) {}
    ColorIteratorT<TColor> begin()
    {
        return ColorIteratorT<TColor>{[this](uint16_t idx) -> TColor& { return data[idx]; }, 0};
    }

    ColorIteratorT<TColor> end()
    {
        return ColorIteratorT<TColor>{[this](uint16_t idx) -> TColor& { return data[idx]; },
                                      static_cast<uint16_t>(data.size())};
    }
};

} // namespace lw::colors

namespace lw
{

template <typename TColor> using ColorIteratorT = colors::ColorIteratorT<TColor>;

template <typename TColor> using SolidColorSourceT = colors::SolidColorSourceT<TColor>;

template <typename TColor> using FillColorSourceT = colors::FillColorSourceT<TColor>;

template <typename TColor> using SpanColorSourceT = colors::SpanColorSourceT<TColor>;

} // namespace lw
