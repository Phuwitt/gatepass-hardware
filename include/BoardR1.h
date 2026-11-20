#pragma once
// Derived from the ORIGINAL fixed_v3 R1 U1 module pads, 2026-10-24.
// ESP32-S3-WROOM-1-N8R2: 8MB flash, 2MB QUAD PSRAM. Do not use N8R8 settings.
namespace R1 {
constexpr int camD0=4,camD1=5,camD2=6,camD3=7,camD4=15,camD5=16,camD6=17,camD7=18;
constexpr int camXclk=8,camPclk=9,camVsync=10,camHref=11,camSda=12,camScl=13;
// Camera PWDN/reset are resistor-strapped on R1, not routed to U1 GPIO.
constexpr int camPwdn=-1,camReset=-1;
constexpr int lcdSck=14,lcdMosi=21,lcdCs=47,lcdDc=48,lcdBl=37,lcdReset=-1;
constexpr int nfcSck=38,nfcMosi=39,nfcMiso=40,nfcCs=43;
constexpr int door=41,rex=42,qrButton=35,tamper=36;
constexpr int ledR=45,ledG=46,ledB=44,buzzer=2,lock=1;
// USB D-/D+ GPIO19/20 remain reserved. GPIO43/44 cannot also be UART0 console.
}
