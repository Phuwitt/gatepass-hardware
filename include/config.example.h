#pragma once
// Optional: copy to gatepass_config.h (ignored by git), or configure over setup Wi-Fi.
#define WIFI_SSID "YOUR_WIFI"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define API_BASE_URL "https://YOUR_PROJECT.vercel.app"
#define DEVICE_ID "R1_001"
#define ENROLLMENT_CODE "PASTE_ONE_TIME_CODE_FROM_WEB"
#define ALLOW_LOCAL_HTTP true
// Public Mozilla roots are bundled in TrustedRoots.h; never use setInsecure().
#define GP_VERCEL_PREVIEW_ORIGIN ""
#define GP_VERCEL_PREVIEW_BYPASS ""
// Confirmed by user: ILI9341 IPS 240x320, 2026-11-19.
#define GP_LCD_WIDTH 240
#define GP_LCD_HEIGHT 320
#define GP_LCD_ROTATION 0
#define GP_LCD_INVERT false
// R1 BL has a resistor, no dedicated transistor. Leave GPIO high-impedance until
// the panel BL pin is confirmed to be a logic input / safe low-current load.
#define GP_LCD_BL_GPIO_ENABLED 0
#define GP_CAMERA 1
// Contact levels depend on installed NO/NC switches. Omit door/tamper telemetry
// until calibrated rather than falsely reporting a closed door.
#define GP_DOOR_SENSOR_ENABLED 0
#define GP_DOOR_OPEN_LEVEL HIGH
#define GP_TAMPER_SENSOR_ENABLED 0
#define GP_TAMPER_ACTIVE_LEVEL LOW
#define GP_REX_ENABLED 0
#define GP_LOCK_PULSE_MS 3000
