#pragma once

namespace lw::test
{
void resetArduinoFakes();
void installDefaultArduinoFakes();

inline void resetAndInstallDefaultArduinoFakes()
{
  resetArduinoFakes();
  installDefaultArduinoFakes();
}
} // namespace lw::test
