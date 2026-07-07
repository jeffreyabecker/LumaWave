#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>

#include "colors/Color.h"
#include "colors/ColorChannelIndexIterator.h"

namespace lw::colors
{

template <typename TValue = uint8_t> class ChannelMap
{
private:
  template <typename TSource> struct IsTaggedChannelSource;

public:
  using ValueType = TValue;
  static constexpr size_t ChannelCount = 4;
  using StorageType = std::array<ValueType, ChannelCount>;

  constexpr ChannelMap() = default;

  constexpr ChannelMap(std::initializer_list<ValueType> values) { assign(values); }

  template <typename TSource, typename = std::enable_if_t<IsTaggedChannelSource<TSource>::value>> explicit constexpr ChannelMap(const TSource& source) { assign(source); }

  template <typename TSource, typename = std::enable_if_t<IsTaggedChannelSource<TSource>::value>> constexpr ChannelMap& operator=(const TSource& source)
  {
    assign(source);
    return *this;
  }

  constexpr size_t size() const { return _values.size(); }

  ValueType& operator[](size_t index) { return _values[index]; }

  const ValueType& operator[](size_t index) const { return _values[index]; }

  ValueType& operator[](char channel) { return _values[channelToIndex(channel)]; }

  const ValueType& operator[](char channel) const { return _values[channelToIndex(channel)]; }

  ValueType* data() { return _values.data(); }

  const ValueType* data() const { return _values.data(); }

  typename StorageType::iterator begin() { return _values.begin(); }

  typename StorageType::iterator end() { return _values.end(); }

  typename StorageType::const_iterator begin() const { return _values.begin(); }

  typename StorageType::const_iterator end() const { return _values.end(); }

  typename StorageType::const_iterator cbegin() const { return _values.cbegin(); }

  typename StorageType::const_iterator cend() const { return _values.cend(); }

private:
  template <typename TMember> using IsIntegralMember = std::is_integral<std::remove_cv_t<std::remove_reference_t<TMember>>>;

  template <typename TSource, typename = void> struct HasIntegralR : std::false_type
  {
  };

  template <typename TSource> struct HasIntegralR<TSource, std::void_t<decltype(std::declval<const TSource&>().R)>> : IsIntegralMember<decltype(std::declval<const TSource&>().R)>
  {
  };

  template <typename TSource, typename = void> struct HasIntegralG : std::false_type
  {
  };

  template <typename TSource> struct HasIntegralG<TSource, std::void_t<decltype(std::declval<const TSource&>().G)>> : IsIntegralMember<decltype(std::declval<const TSource&>().G)>
  {
  };

  template <typename TSource, typename = void> struct HasIntegralB : std::false_type
  {
  };

  template <typename TSource> struct HasIntegralB<TSource, std::void_t<decltype(std::declval<const TSource&>().B)>> : IsIntegralMember<decltype(std::declval<const TSource&>().B)>
  {
  };

  template <typename TSource, typename = void> struct HasIntegralW : std::false_type
  {
  };

  template <typename TSource> struct HasIntegralW<TSource, std::void_t<decltype(std::declval<const TSource&>().W)>> : IsIntegralMember<decltype(std::declval<const TSource&>().W)>
  {
  };

  template <typename TSource> struct IsTaggedChannelSource : std::integral_constant<bool, HasIntegralR<TSource>::value && HasIntegralG<TSource>::value && HasIntegralB<TSource>::value && HasIntegralW<TSource>::value>
  {
  };

  static size_t channelToIndex(char channel)
  {
    assert(isSupportedChannelTag(channel));
    return channelIndex(channel);
  }

  constexpr void assign(std::initializer_list<ValueType> values)
  {
    if (values.size() == 0)
    {
      return;
    }

    if (values.size() == 1)
    {
      std::fill(_values.begin(), _values.end(), *values.begin());
      return;
    }

    const size_t copyCount = (values.size() < _values.size()) ? values.size() : _values.size();
    auto source = values.begin();
    for (size_t index = 0; index < copyCount; ++index, ++source)
    {
      _values[index] = *source;
    }
  }

  template <typename TSource, typename = std::enable_if_t<IsTaggedChannelSource<TSource>::value>> constexpr void assign(const TSource& source)
  {
    _values[0] = static_cast<ValueType>(source.R);
    _values[1] = static_cast<ValueType>(source.G);
    _values[2] = static_cast<ValueType>(source.B);
    _values[3] = static_cast<ValueType>(source.W);
  }

  StorageType _values{};
};

} // namespace lw::colors

namespace lw
{

template <typename TValue = uint8_t> using ChannelMap = colors::ChannelMap<TValue>;

} // namespace lw
