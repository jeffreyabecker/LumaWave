#include "FakeArduinoSetup.h"

#include <ArduinoFake.h>

namespace lw::test
{
void resetArduinoFakes()
{
  ArduinoFakeReset();
}

void installDefaultArduinoFakes()
{
  using namespace fakeit;

  When(Method(ArduinoFake(Function), millis)).AlwaysReturn(0UL);
  When(Method(ArduinoFake(Function), micros)).AlwaysReturn(0UL);
  When(Method(ArduinoFake(Function), digitalRead)).AlwaysReturn(LOW);
  When(Method(ArduinoFake(Function), analogRead)).AlwaysReturn(0);

  When(Method(ArduinoFake(Function), pinMode)).AlwaysDo([](uint8_t, uint8_t) {});

  When(Method(ArduinoFake(Function), digitalWrite)).AlwaysDo([](uint8_t, uint8_t) {});

  When(Method(ArduinoFake(Function), delay)).AlwaysDo([](unsigned long) {});

  When(Method(ArduinoFake(Function), delayMicroseconds)).AlwaysDo([](unsigned int) {});

  When(Method(ArduinoFake(Function), yield)).AlwaysDo([]() {});
}
} // namespace lw::test
