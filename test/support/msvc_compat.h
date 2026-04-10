#pragma once

// MSVC does not support GCC __attribute__ annotations.
// Force-include this file before ArduinoFake headers to silently discard them.
#ifndef __GNUC__
#define __attribute__(x)
#endif

#ifdef _MSC_VER
#include <stdlib.h> // _ultoa, _gcvt
#include <stdio.h>  // snprintf

// MSVC has no utoa; redirect to the unsigned long variant from the CRT.
static inline char* utoa(unsigned int value, char* str, int radix)
{
  return _ultoa(static_cast<unsigned long>(value), str, radix);
}

// MSVC has no dtostrf (Arduino/AVR double-to-string with width and precision).
// Implement using snprintf; caller is responsible for providing a large enough buffer.
static inline char* dtostrf(double val, signed char width, unsigned char prec, char* sout)
{
  char fmt[32];
  snprintf(fmt, sizeof(fmt), "%%%d.%df", static_cast<int>(width), static_cast<int>(prec));
  snprintf(sout, 64, fmt, val);
  return sout;
}
#endif
