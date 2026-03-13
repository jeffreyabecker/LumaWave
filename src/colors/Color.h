#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <limits>
#include <type_traits>

#include "core/Compat.h"
#include "colors/ChannelOrder.h"
#include "colors/ColorChannelIndexIterator.h"
#include "colors/ColorHexCodec.h"

namespace lw::colors
{
static constexpr size_t ColorMinimumComponentCount = static_cast<size_t>(LW_COLOR_MINIMUM_COMPONENT_COUNT);
static constexpr size_t ColorMinimumComponentSizeBits = static_cast<size_t>(LW_COLOR_MINIMUM_COMPONENT_SIZE);

static_assert(ColorMinimumComponentCount >= 3 && ColorMinimumComponentCount <= 5,
              "LW_COLOR_MINIMUM_COMPONENT_COUNT must be in the range [3, 5].");
static_assert(ColorMinimumComponentSizeBits == 8 || ColorMinimumComponentSizeBits == 16,
              "LW_COLOR_MINIMUM_COMPONENT_SIZE must be 8 or 16.");

template <size_t ChannelCount>
static constexpr size_t InternalChannelCount =
    (ChannelCount < ColorMinimumComponentCount) ? ColorMinimumComponentCount : ChannelCount;

template <typename TComponent> struct InternalStorageComponent
{
    using type = std::conditional_t<(ColorMinimumComponentSizeBits > (sizeof(TComponent) * 8)), uint16_t, TComponent>;
};

template <size_t ChannelCount, typename TComponent>
static constexpr size_t DefaultInternalSize =
    InternalChannelCount<ChannelCount> * sizeof(typename InternalStorageComponent<TComponent>::type);

template <size_t ChannelCount, typename TComponent>
static constexpr size_t AliasInternalSize = DefaultInternalSize<ChannelCount, TComponent>;

template <size_t NChannels, typename TComponent = uint8_t,
          size_t InternalSize = NChannels * sizeof(typename InternalStorageComponent<TComponent>::type)>
class RgbBasedColor
{
  public:
    using ComponentType = TComponent;
    using InternalComponentType = typename InternalStorageComponent<TComponent>::type;

    class ComponentReference
    {
      public:
        explicit constexpr ComponentReference(InternalComponentType& value) : _value(value) {}

        constexpr ComponentReference& operator=(TComponent value)
        {
            _value = static_cast<InternalComponentType>(value);
            return *this;
        }

        constexpr ComponentReference& operator=(const ComponentReference& other)
        {
            _value = other._value;
            return *this;
        }

        constexpr operator TComponent() const { return static_cast<TComponent>(_value); }

      private:
        InternalComponentType& _value;
    };

    static_assert((InternalSize % sizeof(InternalComponentType)) == 0,
                  "RgbBasedColor InternalSize must be a multiple of component size.");
    static_assert(InternalSize >= (NChannels * sizeof(InternalComponentType)),
                  "RgbBasedColor InternalSize must be >= channel storage size.");

    static constexpr size_t ChannelCount = NChannels;
    static constexpr TComponent MaxComponent = std::numeric_limits<TComponent>::max();

    using ChannelIndexIterator = ColorChannelIndexIterator<NChannels>;
    using ChannelIndexRange = ColorChannelIndexRange<NChannels>;

    constexpr RgbBasedColor() = default;

    template <typename... Args,
              typename = std::enable_if_t<(sizeof...(Args) <= NChannels) &&
                                          std::conjunction<std::is_convertible<Args, TComponent>...>::value>>
    constexpr RgbBasedColor(Args... args) : Channels{}
    {
        constexpr size_t ArgCount = sizeof...(Args);
        const std::array<TComponent, ArgCount> values{static_cast<TComponent>(args)...};

        for (size_t idx = 0; idx < ArgCount; ++idx)
        {
            Channels[idx] = values[idx];
        }
    }

    constexpr TComponent operator[](char channel) const
    {
        return static_cast<TComponent>(Channels[ColorChannelIndexRange<NChannels>::indexFromChannel(channel)]);
    }

    template <typename T = InternalComponentType>
    std::enable_if_t<std::is_same<T, TComponent>::value, TComponent&> operator[](char channel)
    {
        return Channels[ColorChannelIndexRange<NChannels>::indexFromChannel(channel)];
    }

    template <typename T = InternalComponentType>
    std::enable_if_t<!std::is_same<T, TComponent>::value, ComponentReference> operator[](char channel)
    {
        return ComponentReference(Channels[ColorChannelIndexRange<NChannels>::indexFromChannel(channel)]);
    }

    static constexpr ChannelIndexRange channelIndexes() { return makeColorChannelIndexRange<NChannels>(); }

    static constexpr size_t channelIndexFromTag(char channel)
    {
        return ColorChannelIndexRange<NChannels>::indexFromChannel(channel);
    }

    constexpr TComponent channelAtIndex(size_t index) const { return static_cast<TComponent>(Channels[index]); }

    template <typename T = InternalComponentType>
    std::enable_if_t<std::is_same<T, TComponent>::value, TComponent&> channelAtIndex(size_t index)
    {
        return Channels[index];
    }

    template <typename T = InternalComponentType>
    std::enable_if_t<!std::is_same<T, TComponent>::value, ComponentReference> channelAtIndex(size_t index)
    {
        return ComponentReference(Channels[index]);
    }

    constexpr bool operator==(const RgbBasedColor& other) const { return Channels == other.Channels; }

    constexpr bool operator!=(const RgbBasedColor& other) const { return !(*this == other); }

    constexpr bool operator<(const RgbBasedColor& other) const { return compareCanonical(other) < 0; }

    constexpr bool operator<=(const RgbBasedColor& other) const { return compareCanonical(other) <= 0; }

    constexpr bool operator>(const RgbBasedColor& other) const { return compareCanonical(other) > 0; }

    constexpr bool operator>=(const RgbBasedColor& other) const { return compareCanonical(other) >= 0; }

    template <typename T = TComponent,
              typename = std::enable_if_t<(NChannels >= 3 && NChannels <= 4) && std::is_same<T, uint8_t>::value>>
    constexpr RgbBasedColor& operator=(uint32_t packed)
    {
        (*this)['R'] = static_cast<TComponent>((packed >> (2u * 8u)) & 0xFFu);
        (*this)['G'] = static_cast<TComponent>((packed >> (1u * 8u)) & 0xFFu);
        (*this)['B'] = static_cast<TComponent>((packed >> (0u * 8u)) & 0xFFu);

        if constexpr (NChannels >= 4)
        {
            (*this)['W'] = static_cast<TComponent>((packed >> (3u * 8u)) & 0xFFu);
        }

        return *this;
    }

    template <typename T = TComponent,
              typename = std::enable_if_t<(NChannels >= 3 && NChannels <= 4) && std::is_same<T, uint16_t>::value>>
    constexpr RgbBasedColor& operator=(uint64_t packed)
    {
        (*this)['R'] = static_cast<TComponent>((packed >> (2u * 16u)) & 0xFFFFull);
        (*this)['G'] = static_cast<TComponent>((packed >> (1u * 16u)) & 0xFFFFull);
        (*this)['B'] = static_cast<TComponent>((packed >> (0u * 16u)) & 0xFFFFull);

        if constexpr (NChannels >= 4)
        {
            (*this)['W'] = static_cast<TComponent>((packed >> (3u * 16u)) & 0xFFFFull);
        }

        return *this;
    }

    template <typename T = TComponent,
              typename = std::enable_if_t<(NChannels >= 3 && NChannels <= 4) && std::is_same<T, uint16_t>::value>>
    constexpr RgbBasedColor& operator=(int64_t packed)
    {
        return operator=(static_cast<uint64_t>(packed));
    }

    template <typename T = TComponent,
              typename = std::enable_if_t<(NChannels >= 3 && NChannels <= 4) && std::is_same<T, uint8_t>::value>>
    constexpr operator uint32_t() const
    {
        const uint32_t r = static_cast<uint32_t>((*this)['R']);
        const uint32_t g = static_cast<uint32_t>((*this)['G']);
        const uint32_t b = static_cast<uint32_t>((*this)['B']);
        const uint32_t w = (NChannels >= 4) ? static_cast<uint32_t>((*this)['W']) : 0u;

        return (w << (3u * 8u)) | (r << (2u * 8u)) | (g << (1u * 8u)) | (b << (0u * 8u));
    }

    template <typename T = TComponent,
              typename = std::enable_if_t<(NChannels >= 3 && NChannels <= 4) && std::is_same<T, uint8_t>::value>>
    constexpr operator int32_t() const
    {
        return static_cast<int32_t>(static_cast<uint32_t>(*this));
    }

    template <typename T = TComponent,
              typename = std::enable_if_t<(NChannels >= 3 && NChannels <= 4) && std::is_same<T, uint16_t>::value>>
    constexpr operator uint64_t() const
    {
        const uint64_t r = static_cast<uint64_t>((*this)['R']);
        const uint64_t g = static_cast<uint64_t>((*this)['G']);
        const uint64_t b = static_cast<uint64_t>((*this)['B']);
        const uint64_t w = (NChannels >= 4) ? static_cast<uint64_t>((*this)['W']) : 0ull;

        return (w << (3u * 16u)) | (r << (2u * 16u)) | (g << (1u * 16u)) | (b << (0u * 16u));
    }

    template <typename T = TComponent,
              typename = std::enable_if_t<(NChannels >= 3 && NChannels <= 4) && std::is_same<T, uint16_t>::value>>
    constexpr operator int64_t() const
    {
        return static_cast<int64_t>(static_cast<uint64_t>(*this));
    }

  private:
    constexpr int compareCanonical(const RgbBasedColor& other) const
    {
        constexpr std::array<char, 5> CanonicalChannels = {'R', 'G', 'B', 'C', 'W'};

        for (char channel : CanonicalChannels)
        {
            if (!ColorChannelIndexRange<NChannels>::isSupportedChannelTag(channel))
            {
                continue;
            }

            const auto left = (*this)[channel];
            const auto right = other[channel];
            if (left < right)
            {
                return -1;
            }

            if (left > right)
            {
                return 1;
            }
        }

        return 0;
    }

    std::array<InternalComponentType, InternalSize / sizeof(InternalComponentType)>
        Channels; // no {} here so we're trivially constructable
};

using Rgb8Color = RgbBasedColor<3, uint8_t, AliasInternalSize<3, uint8_t>>;
using Rgbw8Color = RgbBasedColor<4, uint8_t, AliasInternalSize<4, uint8_t>>;
using Rgbcw8Color = RgbBasedColor<5, uint8_t, AliasInternalSize<5, uint8_t>>;

using Rgb16Color = RgbBasedColor<3, uint16_t, AliasInternalSize<3, uint16_t>>;
using Rgbw16Color = RgbBasedColor<4, uint16_t, AliasInternalSize<4, uint16_t>>;
using Rgbcw16Color = RgbBasedColor<5, uint16_t, AliasInternalSize<5, uint16_t>>;

using DefaultColorType = std::conditional_t<
    (ColorMinimumComponentSizeBits == 16),
    std::conditional_t<(ColorMinimumComponentCount >= 5), Rgbcw16Color,
                       std::conditional_t<(ColorMinimumComponentCount >= 4), Rgbw16Color, Rgb16Color>>,
    std::conditional_t<(ColorMinimumComponentCount >= 5), Rgbcw8Color,
                       std::conditional_t<(ColorMinimumComponentCount >= 4), Rgbw8Color, Rgb8Color>>>;

using Color = DefaultColorType;
template <typename TColor, typename = void> struct ColorTypeImpl : std::false_type
{
};

template <typename TColor>
struct ColorTypeImpl<TColor, std::void_t<typename TColor::ComponentType, decltype(TColor::ChannelCount)>>
    : std::integral_constant<bool, std::is_convertible<decltype(TColor::ChannelCount), size_t>::value>
{
};

template <typename TColor> static constexpr bool ColorType = ColorTypeImpl<TColor>::value;

template <typename TColor, size_t NChannels>
static constexpr bool ColorChannelsExactly = ColorType<TColor> && (TColor::ChannelCount == NChannels);

template <typename TColor, size_t MinChannels>
static constexpr bool ColorChannelsAtLeast = ColorType<TColor> && (TColor::ChannelCount >= MinChannels);

template <typename TColor, size_t MaxChannels>
static constexpr bool ColorChannelsAtMost = ColorType<TColor> && (TColor::ChannelCount <= MaxChannels);

template <typename TColor, size_t MinChannels, size_t MaxChannels>
static constexpr bool ColorChannelsInRange =
    ColorType<TColor> && (TColor::ChannelCount >= MinChannels) && (TColor::ChannelCount <= MaxChannels);

template <typename TColor, typename TComponent>
static constexpr bool ColorComponentTypeIs =
    ColorType<TColor> && std::is_same<typename TColor::ComponentType, TComponent>::value;

template <typename TColor, size_t BitDepth>
static constexpr bool ColorComponentBitDepth =
    ColorType<TColor> && ((sizeof(typename TColor::ComponentType) * 8) == BitDepth);

template <typename TColor, size_t NChannels>
using RequireColorChannelsExactly = std::enable_if_t<ColorChannelsExactly<TColor, NChannels>, int>;

template <typename TColor, size_t MinChannels, size_t MaxChannels>
using RequireColorChannelsInRange = std::enable_if_t<ColorChannelsInRange<TColor, MinChannels, MaxChannels>, int>;

template <typename TColor, size_t BitDepth>
using RequireColorComponentBitDepth = std::enable_if_t<ColorComponentBitDepth<TColor, BitDepth>, int>;

template <typename TLeftColor, typename TRightColor>
static constexpr bool ColorComponentAtLeastAsLarge =
    ColorType<TLeftColor> && ColorType<TRightColor> &&
    (sizeof(typename TLeftColor::ComponentType) >= sizeof(typename TRightColor::ComponentType));

template <typename TLeftColor, typename TRightColor>
static constexpr bool ColorChannelAtLeastAsLarge =
    ColorType<TLeftColor> && ColorType<TRightColor> && (TLeftColor::ChannelCount >= TRightColor::ChannelCount);

template <typename TLeftColor, typename TRightColor>
static constexpr bool ColorAtLeastAsLarge =
    ColorType<TLeftColor> && ColorType<TRightColor> &&
    ((sizeof(typename TLeftColor::ComponentType) > sizeof(typename TRightColor::ComponentType)) ||
     ((sizeof(typename TLeftColor::ComponentType) == sizeof(typename TRightColor::ComponentType)) &&
      (TLeftColor::ChannelCount >= TRightColor::ChannelCount)));

template <typename TLeftColor, typename TRightColor, typename = void> struct LargerColorType
{
};

template <typename TLeftColor, typename TRightColor>
struct LargerColorType<TLeftColor, TRightColor, std::enable_if_t<ColorType<TLeftColor> && ColorType<TRightColor>>>
{
    using type = std::conditional_t<ColorAtLeastAsLarge<TLeftColor, TRightColor>, TLeftColor, TRightColor>;
};

template <typename TLeftColor, typename TRightColor>
using LargerColorTypeT = typename LargerColorType<TLeftColor, TRightColor>::type;

template <size_t N, size_t InternalSize>
constexpr RgbBasedColor<N, uint16_t> widen(const RgbBasedColor<N, uint8_t, InternalSize>& src)
{
    RgbBasedColor<N, uint16_t> result;
    for (auto channel : RgbBasedColor<N, uint8_t, InternalSize>::channelIndexes())
    {
        const uint8_t value = src[channel];
        result[channel] = static_cast<uint16_t>((static_cast<uint16_t>(value) << 8) | value);
    }
    return result;
}

template <size_t N, size_t InternalSize>
constexpr RgbBasedColor<N, uint8_t> narrow(const RgbBasedColor<N, uint16_t, InternalSize>& src)
{
    RgbBasedColor<N, uint8_t> result;
    for (auto channel : RgbBasedColor<N, uint16_t, InternalSize>::channelIndexes())
    {
        result[channel] = static_cast<uint8_t>(src[channel] >> 8);
    }
    return result;
}

template <size_t N, size_t M, typename T, size_t SrcInternalSize, typename std::enable_if<(N > M), int>::type = 0>
constexpr RgbBasedColor<N, T> expand(const RgbBasedColor<M, T, SrcInternalSize>& src)
{
    RgbBasedColor<N, T> result{};
    for (auto channel : RgbBasedColor<M, T, SrcInternalSize>::channelIndexes())
    {
        result[channel] = src[channel];
    }
    return result;
}

template <size_t N, size_t M, typename T, size_t SrcInternalSize, typename std::enable_if<(N < M), int>::type = 0>
constexpr RgbBasedColor<N, T> compress(const RgbBasedColor<M, T, SrcInternalSize>& src)
{
    RgbBasedColor<N, T> result;
    for (auto channel : RgbBasedColor<N, T>::channelIndexes())
    {
        result[channel] = src[channel];
    }
    return result;
}

} // namespace lw::colors

namespace lw
{

inline constexpr size_t ColorMinimumComponentCount = colors::ColorMinimumComponentCount;
inline constexpr size_t ColorMinimumComponentSizeBits = colors::ColorMinimumComponentSizeBits;

template <size_t ChannelCount>
inline constexpr size_t InternalChannelCount = colors::InternalChannelCount<ChannelCount>;

template <typename TComponent> using InternalStorageComponent = colors::InternalStorageComponent<TComponent>;

template <size_t ChannelCount, typename TComponent>
inline constexpr size_t DefaultInternalSize = colors::DefaultInternalSize<ChannelCount, TComponent>;

template <size_t ChannelCount, typename TComponent>
inline constexpr size_t AliasInternalSize = colors::AliasInternalSize<ChannelCount, TComponent>;

template <size_t NChannels, typename TComponent = uint8_t,
          size_t InternalSize = NChannels * sizeof(typename InternalStorageComponent<TComponent>::type)>
using RgbBasedColor = colors::RgbBasedColor<NChannels, TComponent, InternalSize>;

using Rgb8Color = colors::Rgb8Color;
using Rgbw8Color = colors::Rgbw8Color;
using Rgbcw8Color = colors::Rgbcw8Color;
using Rgb16Color = colors::Rgb16Color;
using Rgbw16Color = colors::Rgbw16Color;
using Rgbcw16Color = colors::Rgbcw16Color;
using DefaultColorType = colors::DefaultColorType;
using Color = colors::Color;

template <typename TColor, typename Enable = void> using ColorTypeImpl = colors::ColorTypeImpl<TColor, Enable>;

template <typename TColor> inline constexpr bool ColorType = colors::ColorType<TColor>;

template <typename TColor, size_t NChannels>
inline constexpr bool ColorChannelsExactly = colors::ColorChannelsExactly<TColor, NChannels>;

template <typename TColor, size_t MinChannels>
inline constexpr bool ColorChannelsAtLeast = colors::ColorChannelsAtLeast<TColor, MinChannels>;

template <typename TColor, size_t MaxChannels>
inline constexpr bool ColorChannelsAtMost = colors::ColorChannelsAtMost<TColor, MaxChannels>;

template <typename TColor, size_t MinChannels, size_t MaxChannels>
inline constexpr bool ColorChannelsInRange = colors::ColorChannelsInRange<TColor, MinChannels, MaxChannels>;

template <typename TColor, typename TComponent>
inline constexpr bool ColorComponentTypeIs = colors::ColorComponentTypeIs<TColor, TComponent>;

template <typename TColor, size_t BitDepth>
inline constexpr bool ColorComponentBitDepth = colors::ColorComponentBitDepth<TColor, BitDepth>;

template <typename TColor, size_t NChannels>
using RequireColorChannelsExactly = colors::RequireColorChannelsExactly<TColor, NChannels>;

template <typename TColor, size_t MinChannels, size_t MaxChannels>
using RequireColorChannelsInRange = colors::RequireColorChannelsInRange<TColor, MinChannels, MaxChannels>;

template <typename TColor, size_t BitDepth>
using RequireColorComponentBitDepth = colors::RequireColorComponentBitDepth<TColor, BitDepth>;

template <typename TLeftColor, typename TRightColor>
inline constexpr bool ColorComponentAtLeastAsLarge = colors::ColorComponentAtLeastAsLarge<TLeftColor, TRightColor>;

template <typename TLeftColor, typename TRightColor>
inline constexpr bool ColorChannelAtLeastAsLarge = colors::ColorChannelAtLeastAsLarge<TLeftColor, TRightColor>;

template <typename TLeftColor, typename TRightColor>
inline constexpr bool ColorAtLeastAsLarge = colors::ColorAtLeastAsLarge<TLeftColor, TRightColor>;

template <typename TLeftColor, typename TRightColor, typename Enable = void>
using LargerColorType = colors::LargerColorType<TLeftColor, TRightColor, Enable>;

template <typename TLeftColor, typename TRightColor>
using LargerColorTypeT = colors::LargerColorTypeT<TLeftColor, TRightColor>;

using colors::compress;
using colors::expand;
using colors::narrow;
using colors::widen;

} // namespace lw
