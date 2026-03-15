#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#if defined(__cpp_lib_span) && (__cpp_lib_span >= 202002L)
#include <span>
#define LW_HAS_STD_SPAN 1
#elif defined(__has_include)
#if __has_include(<span>)
#include <span>
#if defined(__cpp_lib_span) && (__cpp_lib_span >= 202002L)
#define LW_HAS_STD_SPAN 1
#endif
#endif
#endif

#if defined(ARDUINO)
#define LW_HAS_ARDUINO 1
#elif defined(__has_include)
#if __has_include(<Arduino.h>)
#define LW_HAS_ARDUINO 1
#else
#define LW_HAS_ARDUINO 0
#endif
#else
#define LW_HAS_ARDUINO 0
#endif

#if defined(__has_include)
#if __has_include(<SPI.h>)
#define LW_HAS_SPI_TRANSPORT 1
#endif
#endif

#ifndef LW_SPI_CLOCK_DEFAULT_HZ
#define LW_SPI_CLOCK_DEFAULT_HZ 10000000UL
#endif

#ifndef LW_ESP32_DMA_SPI_CLOCK_DEFAULT_HZ
#define LW_ESP32_DMA_SPI_CLOCK_DEFAULT_HZ 10000000UL
#endif

#ifndef LW_ESP32_DMA_SPI_DEFAULT_HOST
#if defined(SPI2_HOST)
#define LW_ESP32_DMA_SPI_DEFAULT_HOST SPI2_HOST
#else
#define LW_ESP32_DMA_SPI_DEFAULT_HOST 1
#endif
#endif

#ifndef LW_RP_DMA_IRQ_INDEX
#define LW_RP_DMA_IRQ_INDEX 1
#endif

#ifndef LW_COLOR_MINIMUM_COMPONENT_COUNT
#define LW_COLOR_MINIMUM_COMPONENT_COUNT 4
#endif

#ifndef LW_COLOR_MINIMUM_COMPONENT_SIZE
#define LW_COLOR_MINIMUM_COMPONENT_SIZE 8
#endif

#ifndef LW_USE_PLATFORM_RANDOM
#define LW_USE_PLATFORM_RANDOM 0
#endif

#ifndef LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES
#define LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES 0
#endif

#if LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES
#define LW_HAS_TEMPLATE_COMBINATORIAL_TYPES 0
#else
#define LW_HAS_TEMPLATE_COMBINATORIAL_TYPES 1
#endif

#if !defined(LW_HAS_STD_SPAN)
#include "third_party/tcb/span.hpp"
#endif

namespace lw
{

using PixelCount = uint16_t;

using ssize_t = std::ptrdiff_t;

#if defined(__cpp_lib_remove_cvref) && (__cpp_lib_remove_cvref >= 201711L)
template <typename T> using remove_cvref_t = std::remove_cvref_t<T>;
#else
template <typename T> using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
#endif

#if defined(LW_HAS_STD_SPAN)
static constexpr std::size_t dynamic_extent = std::dynamic_extent;
template <typename T, std::size_t Extent = std::dynamic_extent> using span = std::span<T, Extent>;
#else
static constexpr std::size_t dynamic_extent = tcb::dynamic_extent;

template <typename T, std::size_t Extent = dynamic_extent> using span = tcb::span<T, Extent>;
#endif

} // namespace lw
