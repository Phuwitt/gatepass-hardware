#pragma once
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <qrcode.h>
SPIClass displaySpi(FSPI);
Adafruit_ILI9341 screen(&displaySpi,R1::lcdCs,R1::lcdDc,R1::lcdReset);
constexpr uint16_t navy=0x0926,teal=0x04B3,white=0xFFFF,muted=0xADB5;
String screenState;uint32_t screenUntil=0;bool displayReady=false,qrVisible=false;
void screenText(const String& title,const String& detail,uint16_t accent=teal,uint32_t hold=0) {
 if(!displayReady)return;qrVisible=false;screenState=title;screenUntil=hold?millis()+hold:0;
 screen.fillScreen(navy);screen.setTextWrap(true);screen.setCursor(16,18);screen.setTextSize(2);screen.setTextColor(white);screen.print("GATEPASS");
 screen.fillRoundRect(16,60,screen.width()-32,5,2,accent);screen.setCursor(16,90);screen.setTextColor(accent);screen.setTextSize(2);screen.println(title);
 screen.setCursor(16,135);screen.setTextSize(1);screen.setTextColor(white);screen.println(detail);screen.setCursor(16,screen.height()-28);screen.setTextColor(muted);screen.print(deviceId);
}
void startDisplay(){
 pinMode(R1::lcdCs,OUTPUT);digitalWrite(R1::lcdCs,HIGH);
 displaySpi.begin(R1::lcdSck,-1,R1::lcdMosi,R1::lcdCs);
 screen.begin(10000000);screen.setRotation(GP_LCD_ROTATION);screen.invertDisplay(GP_LCD_INVERT);displayReady=true;
 #if GP_LCD_BL_GPIO_ENABLED
 pinMode(R1::lcdBl,OUTPUT);digitalWrite(R1::lcdBl,HIGH);
 #else
 pinMode(R1::lcdBl,INPUT);
 #endif
 screenText("STARTING","Card / NFC / QR\nCamera preview only");
}
bool showQr(const String& value){
 // Fixed v10-L has room for normal HTTPS origins plus the 64-character token.
 // qrcode_initText reports overflow; no truncated payload is ever displayed.
 uint8_t buffer[ qrcode_getBufferSize(10) ];QRCode code;
 if(value.length()>250 || qrcode_initText(&code,buffer,10,ECC_LOW,value.c_str())!=0){screenText("QR TOO LONG","Shorten the public API hostname.",ILI9341_RED,5000);return false;}
 const int scale=(min(screen.width(),screen.height())-16)/(code.size+8);
 if(scale<2){screenText("PANEL TOO SMALL","Check LCD dimensions.",ILI9341_RED,5000);return false;}
 screen.fillScreen(navy);int size=(code.size+8)*scale,x=(screen.width()-size)/2,y=(screen.height()-size)/2;
 screen.fillRect(x,y,size,size,white);
 for(int row=0;row<code.size;row++)for(int col=0;col<code.size;col++)if(qrcode_getModule(&code,col,row))screen.fillRect(x+(col+4)*scale,y+(row+4)*scale,scale,scale,ILI9341_BLACK);
 screen.setTextSize(1);screen.setTextColor(white);screen.setCursor(12,8);screen.print("SCAN WITH GATEPASS APP");screen.setCursor(12,screen.height()-16);screen.print("One use / expires in 60s");qrVisible=true;screenUntil=0;return true;
}
