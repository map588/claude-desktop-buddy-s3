#pragma once
#include <M5Unified.h>

using TFT_eSprite = M5Canvas;
using TFT_eSPI    = LovyanGFX;

namespace compat {
inline void setScreenBrightness0_100(uint8_t v) {
  if (v > 100) v = 100;
  M5.Display.setBrightness((uint16_t)v * 255 / 100);
}
inline void screenPower(bool on) {
  if (on) M5.Display.wakeup();
  else    M5.Display.sleep();
}
inline float batVoltageV() { return M5.Power.getBatteryVoltage() / 1000.0f; }
inline int   batCurrentMA() { return M5.Power.getBatteryCurrent(); }
inline float vbusVoltageV() {
  int mv = M5.Power.getVBUSVoltage();
  return mv > 0 ? mv / 1000.0f : 0.0f;
}
inline int chipTempC() { return (int)temperatureRead(); }
inline void getAccel(float* ax, float* ay, float* az) {
  M5.Imu.getAccel(ax, ay, az);
}
// tone() starts the speaker path (ES8311 codec, AW8737 amplifier) when it
// is off.
inline void beep(uint16_t freq, uint16_t durMs) {
  M5.Speaker.tone(freq, durMs);
}
// M5Unified keeps the speaker path on from begin() until end(). The
// AW8737 amplifier alone takes 10-15 mA when it is on and silent. On
// StickS3, end() turns off only the amplifier, so also set the ES8311
// analog and DAC power registers back to their reset values. The codec
// keeps its state through an ESP32 reset, so do this once at boot too.
// The next begin() powers the codec up again.
inline void speakerOff() {
  static bool codecOff = false;
  if (M5.Speaker.isRunning()) {
    if (M5.Speaker.isPlaying()) return;
    M5.Speaker.end();
    codecOff = false;
  }
  if (codecOff) return;
  const uint8_t ES8311 = 0x18;
  M5.In_I2C.writeRegister8(ES8311, 0x12, 0x02, 100000);  // DAC power down
  M5.In_I2C.writeRegister8(ES8311, 0x0D, 0xFC, 100000);  // analog power down
  codecOff = true;
}
inline void powerOff() { M5.Power.powerOff(); }
}
