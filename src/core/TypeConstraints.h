#pragma once

#include <cstdint>
#include <type_traits>

#include "protocols/Protocol.h"
#include "transports/Transport.h"

namespace lw
{

template <typename TProtocol, typename = void> struct ProtocolLikeImpl : std::false_type
{
};

template <typename TProtocol> struct ProtocolLikeImpl<TProtocol, std::void_t<typename TProtocol::ColorType, typename TProtocol::SettingsType>> : std::true_type
{
};

template <typename TProtocol> static constexpr bool ProtocolLike = ProtocolLikeImpl<TProtocol>::value;

template <typename TProtocol, typename TTransport>
static constexpr bool ProtocolSettingsConstructibleWithTransport = protocols::ProtocolPixelSettingsConstructible<TProtocol> || std::is_constructible_v<TProtocol, PixelCount, typename TProtocol::SettingsType, TTransport&>;

template <typename TProtocol, typename TTransport> static constexpr bool ProtocolTransportCompatible = ProtocolLike<TProtocol> && transports::TransportLike<TTransport>;

} // namespace lw