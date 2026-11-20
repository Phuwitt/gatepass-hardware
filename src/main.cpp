#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <esp_camera.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <cJSON.h>
#include <memory>
#include <time.h>
#include "BoardR1.h"
#if __has_include("gatepass_config.h")
#include "gatepass_config.h"
#else
#include "config.example.h"
#endif
#include "TrustedRoots.h"
#include "AccessPolicy.h"
Preferences prefs;String token;
String wifiSsid=WIFI_SSID,wifiPassword=WIFI_PASSWORD,apiBase=API_BASE_URL,deviceId=DEVICE_ID,enrollmentCode=ENROLLMENT_CODE;
const char* firmwareVersion="gatepass-r1-1.0.0";
bool cameraReady=false,previewEnabled=false;uint32_t previewUntil=0,lastPreview=0,lastHeartbeat=0,lastWifi=0;
String cardSession;uint32_t cardSessionUntil=0;
String activeQrToken;uint32_t qrStarted=0,lastQrPoll=0,lastQrRequest=0;
esp_timer_handle_t lockTimer=nullptr,signalTimer=nullptr;bool localActuation=GP_ENABLE_ACTUATION;
String lastDecisionId;bool qrRequested=false;uint32_t signalUntil=0;

bool validApiUrl(const String& base){
 if(base.startsWith("https://"))return base.length()>8&&base.indexOf('@')<0&&base.indexOf('?')<0&&base.indexOf('#')<0;
 if(!ALLOW_LOCAL_HTTP||!base.startsWith("http://"))return false;
 String host=base.substring(7);int colon=host.indexOf(':');if(colon>=0)host=host.substring(0,colon);IPAddress ip;if(!ip.fromString(host))return false;
 return ip[0]==10||(ip[0]==192&&ip[1]==168)||(ip[0]==172&&ip[1]>=16&&ip[1]<=31);
}
bool allowedUrl(){return validApiUrl(apiBase);}
#include "Display.h"
#include "Provisioning.h"

void lockOff(void*){digitalWrite(R1::lock,LOW);}
void signalOff(void*){digitalWrite(R1::buzzer,LOW);digitalWrite(R1::ledR,LOW);digitalWrite(R1::ledG,LOW);}
void stopOutputs(){if(lockTimer)esp_timer_stop(lockTimer);digitalWrite(R1::lock,LOW);digitalWrite(R1::buzzer,LOW);digitalWrite(R1::ledR,LOW);digitalWrite(R1::ledG,LOW);}
void beginOutputs(){
 for(int pin:{R1::lock,R1::buzzer,R1::ledR,R1::ledG,R1::ledB}){digitalWrite(pin,LOW);pinMode(pin,OUTPUT);}
 for(int pin:{R1::door,R1::rex,R1::qrButton,R1::tamper})pinMode(pin,INPUT); // PCB has external pull-ups.
 esp_timer_create_args_t args={};args.callback=lockOff;args.name="r1-lock-off";
 if(esp_timer_create(&args,&lockTimer)!=ESP_OK){localActuation=false;Serial.println("Lock timer unavailable: outputs disabled.");}
 args.callback=signalOff;args.name="r1-signal-off";esp_timer_create(&args,&signalTimer);
}
int post(const char* path,const uint8_t* bytes,size_t length,const char* type,String& output,const String& rid="",bool auth=true,uint16_t timeout=5000){
 if(WiFi.status()!=WL_CONNECTED||!allowedUrl())return -1;
 std::unique_ptr<WiFiClient> network;
 if(apiBase.startsWith("https://")){if(time(nullptr)<1700000000)return -2;auto* tls=new WiFiClientSecure();tls->setCACert(ROOT_CA_PEM);network.reset(tls);}else network.reset(new WiFiClient());
 HTTPClient h;h.setTimeout(timeout);h.setConnectTimeout(4000);if(!h.begin(*network,apiBase+path))return -3;
 h.addHeader("Content-Type",type);if(auth){h.addHeader("X-Device-ID",deviceId);h.addHeader("Authorization","Bearer "+token);}if(rid.length())h.addHeader("X-Request-ID",rid);
 if(apiBase.startsWith("https://")&&apiBase==GP_VERCEL_PREVIEW_ORIGIN&&strlen(GP_VERCEL_PREVIEW_BYPASS))h.addHeader("x-vercel-protection-bypass",GP_VERCEL_PREVIEW_BYPASS);
 int status=h.POST(const_cast<uint8_t*>(bytes),length);output=status>0?h.getString():"";h.end();
 if(status==401||status==403){previewEnabled=false;activeQrToken="";stopOutputs();}
 return status;
}
String requestId(){uint8_t b[16];esp_fill_random(b,sizeof b);b[6]=(b[6]&15)|64;b[8]=(b[8]&63)|128;char s[37];snprintf(s,sizeof s,"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],b[10],b[11],b[12],b[13],b[14],b[15]);return String(s);}
String jsonString(cJSON* r,const char* key){auto* v=cJSON_GetObjectItemCaseSensitive(r,key);return cJSON_IsString(v)?String(v->valuestring):String();}
int64_t isoSeconds(const String& iso){struct tm t={};if(!strptime(iso.c_str(),"%Y-%m-%dT%H:%M:%S",&t))return 0;return mktime(&t);}
void previewResponse(const String& out){
 cJSON* r=cJSON_Parse(out.c_str());if(!r)return;
 auto* session=cJSON_GetObjectItemCaseSensitive(r,"card_enrollment");if(cJSON_IsObject(session)){cardSession=jsonString(session,"id");cardSessionUntil=millis()+17000;}else if(session)cardSession="";
 previewEnabled=cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(r,"preview_enabled"));if(previewEnabled)previewUntil=millis()+22000;cJSON_Delete(r);
}
void handleDecision(const String& out){
 cJSON* r=cJSON_Parse(out.c_str());if(!r)return;
 String id=jsonString(r,"request_id");bool allowed=cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(r,"allowed"));
 auto* duration=cJSON_GetObjectItemCaseSensitive(r,"unlock_ms");uint32_t pulse=cJSON_IsNumber(duration)?duration->valueint:0;
 int64_t expiry=isoSeconds(jsonString(r,"decision_expires_at")),now=time(nullptr);bool fresh=now>1700000000&&expiry>=now&&expiry<=now+15;
 bool repeated=id.isEmpty()||id==lastDecisionId;bool fire=permitPulse(allowed,cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(r,"actuation_enabled")),localActuation,fresh,repeated,pulse);
 // Save before actuation. Failed persistence refuses the pulse.
 if(!id.isEmpty()&&!repeated){lastDecisionId=id;if(prefs.putString("last_event",id)!=id.length())fire=false;}
 if(fire){esp_timer_stop(lockTimer);digitalWrite(R1::lock,HIGH);if(esp_timer_start_once(lockTimer,uint64_t(pulse)*1000)!=ESP_OK)digitalWrite(R1::lock,LOW);}
 digitalWrite(R1::ledG,allowed?HIGH:LOW);digitalWrite(R1::ledR,allowed?LOW:HIGH);digitalWrite(R1::buzzer,HIGH);signalUntil=millis()+150;if(signalTimer){esp_timer_stop(signalTimer);esp_timer_start_once(signalTimer,150000);}
 screenText(allowed?"ACCESS ALLOWED":"ACCESS DENIED",allowed?(fire?"Unlock pulse sent.\nDoor sensor is separate.":"Permission passed.\nLock output not enabled."):"Check employee / method / door time.",allowed?teal:ILI9341_RED,4000);
 Serial.printf("Decision: %s | output pulse: %s\r\n",allowed?"allowed":"denied",fire?"sent":"no");cJSON_Delete(r);
}
void enroll(){
 if(token.length()||!validEnrollment(enrollmentCode))return;cJSON* r=cJSON_CreateObject();cJSON_AddStringToObject(r,"device_id",deviceId.c_str());cJSON_AddStringToObject(r,"enrollment_code",enrollmentCode.c_str());char* j=cJSON_PrintUnformatted(r);String out;int status=post("/api/device/enroll",(uint8_t*)j,strlen(j),"application/json",out,"",false);memset(j,0,strlen(j));cJSON_free(j);cJSON_Delete(r);
 if(status==200){cJSON* v=cJSON_Parse(out.c_str());String saved=jsonString(v,"device_token");if(saved.length()==64&&prefs.putString("token",saved)==64){token=saved;prefs.remove("enroll");enrollmentCode="";screenText("REGISTERED","Waiting for first heartbeat.");}cJSON_Delete(v);}else screenText("ENROLL FAILED","Issue a new device enrollment code.",ILI9341_RED);
}
#include "ReaderR1.h"

bool startCamera(){
 camera_config_t c={};c.ledc_channel=LEDC_CHANNEL_0;c.ledc_timer=LEDC_TIMER_0;
 c.pin_d0=R1::camD0;c.pin_d1=R1::camD1;c.pin_d2=R1::camD2;c.pin_d3=R1::camD3;c.pin_d4=R1::camD4;c.pin_d5=R1::camD5;c.pin_d6=R1::camD6;c.pin_d7=R1::camD7;
 c.pin_xclk=R1::camXclk;c.pin_pclk=R1::camPclk;c.pin_vsync=R1::camVsync;c.pin_href=R1::camHref;c.pin_sccb_sda=R1::camSda;c.pin_sccb_scl=R1::camScl;c.pin_pwdn=R1::camPwdn;c.pin_reset=R1::camReset;
 c.xclk_freq_hz=10000000;c.pixel_format=PIXFORMAT_JPEG;c.frame_size=FRAMESIZE_QVGA;c.jpeg_quality=14;c.fb_count=1;c.fb_location=psramFound()?CAMERA_FB_IN_PSRAM:CAMERA_FB_IN_DRAM;c.grab_mode=CAMERA_GRAB_LATEST;
 esp_err_t e=esp_camera_init(&c);if(e!=ESP_OK){Serial.printf("Camera init error 0x%x; card/QR remain available.\r\n",e);return false;}return true;
}
void newQr(){
 lastQrRequest=millis();String out;int code=post("/api/device/qr-session?format=text",(const uint8_t*)"{}",2,"application/json",out);
 if(code!=200){screenText("QR UNAVAILABLE","Check network and backend.",ILI9341_RED,4000);return;}
 cJSON* r=cJSON_Parse(out.c_str());String url=jsonString(r,"url");cJSON_Delete(r);
 int i=url.indexOf("?qr=");if(i<0||url.length()-i-4!=64||!url.startsWith(apiBase+"/")){screenText("QR INVALID","Check public API origin.",ILI9341_RED,4000);return;}
 if(showQr(url)){activeQrToken=url.substring(i+4);qrStarted=millis();lastQrPoll=millis();}
}
void pollQr(){
 if(activeQrToken.isEmpty())return;if(elapsed(millis(),qrStarted,60000)){activeQrToken="";qrVisible=false;screenText("QR EXPIRED","Press QR button for a new code.",ILI9341_RED,3000);return;}
 if(!elapsed(millis(),lastQrPoll,4000))return;lastQrPoll=millis();String out,body="{\"token\":\""+activeQrToken+"\"}";
 int code=post("/api/device/qr-status",(const uint8_t*)body.c_str(),body.length(),"application/json",out);
 if(code==200){cJSON* r=cJSON_Parse(out.c_str());bool complete=jsonString(r,"status")=="allowed"||jsonString(r,"status")=="denied";cJSON_Delete(r);if(complete){activeQrToken="";handleDecision(out);}}
 else if(code==410||code==404){activeQrToken="";screenText("QR EXPIRED","Press QR button to try again.",ILI9341_RED,3000);}
}
struct Input {explicit Input(int p):pin(p){} int pin;bool stable=false,raw=false;uint32_t changed=0;bool update(){bool v=digitalRead(pin)==LOW;if(v!=raw){raw=v;changed=millis();}if(stable!=raw&&elapsed(millis(),changed,40)){stable=raw;return true;}return false;}};
Input qrInput{R1::qrButton},rexInput{R1::rex},doorInput{R1::door},tamperInput{R1::tamper};
void pollInputs(){
 if(qrInput.update()&&qrInput.stable)qrRequested=true;
 if(rexInput.update()&&rexInput.stable&&GP_REX_ENABLED&&localActuation&&lockTimer){esp_timer_stop(lockTimer);digitalWrite(R1::lock,HIGH);if(esp_timer_start_once(lockTimer,uint64_t(GP_LOCK_PULSE_MS)*1000)!=ESP_OK)digitalWrite(R1::lock,LOW);}
 doorInput.update();tamperInput.update();
}
void heartbeat(){
 lastHeartbeat=millis();cJSON* r=cJSON_CreateObject();cJSON_AddStringToObject(r,"firmware",firmwareVersion);cJSON_AddNumberToObject(r,"rssi",WiFi.RSSI());cJSON_AddBoolToObject(r,"camera_ready",cameraReady);cJSON_AddBoolToObject(r,"card_reader_ready",cardReaderReady());
 if(GP_DOOR_SENSOR_ENABLED)cJSON_AddBoolToObject(r,"door_open",doorInput.stable==(GP_DOOR_OPEN_LEVEL==LOW));if(GP_TAMPER_SENSOR_ENABLED)cJSON_AddBoolToObject(r,"tamper",tamperInput.stable==(GP_TAMPER_ACTIVE_LEVEL==LOW));
 char* j=cJSON_PrintUnformatted(r);String out;int code=post("/api/device/heartbeat",(uint8_t*)j,strlen(j),"application/json",out);cJSON_free(j);cJSON_Delete(r);
 if(code==200)previewResponse(out);else{previewEnabled=false;Serial.printf("Heartbeat HTTP %d\r\n",code);if(!qrVisible)screenText("BACKEND ERROR",String("HTTP ")+code+"\nNo access granted.",ILI9341_RED,3000);}
}
void setup(){
 setenv("TZ","UTC0",1);tzset();
 beginOutputs();Serial.begin(115200);delay(700);prefs.begin("gatepass",false);lastDecisionId=prefs.getString("last_event","");startDisplay();
 if(prefs.getBool("configured",false)){wifiSsid=prefs.getString("ssid","");wifiPassword=prefs.getString("wifi_pass","");apiBase=prefs.getString("api",apiBase);deviceId=prefs.getString("device",deviceId);enrollmentCode=prefs.getString("enroll","");}
 token=prefs.getString("token","");if(prefs.getString("device","")!=deviceId||prefs.getString("token_api",apiBase)!=apiBase){prefs.remove("token");token="";}prefs.putString("device",deviceId);prefs.putString("token_api",apiBase);
 if(wifiSsid.isEmpty()||wifiSsid=="YOUR_WIFI"||!allowedUrl()||(!token.length()&&!validEnrollment(enrollmentCode))){screenText("WIFI SETUP","See USB Serial Monitor 115200.\nConnect to GATEPASS-SETUP Wi-Fi.");beginSetupPortal();return;}
 WiFi.mode(WIFI_STA);WiFi.begin(wifiSsid.c_str(),wifiPassword.c_str());screenText("CONNECTING",wifiSsid);
 // Local lock timeout uses esp_timer, independent of blocking network requests.
 configTime(0,0,"pool.ntp.org","time.google.com");if(GP_CAMERA)cameraReady=startCamera();startReader();screenText("READY","Tap card / NFC\nPress QR button for QR");
}
void loop(){
 if(signalUntil&&(int32_t)(millis()-signalUntil)>=0){digitalWrite(R1::buzzer,LOW);digitalWrite(R1::ledR,LOW);digitalWrite(R1::ledG,LOW);signalUntil=0;}
 if(Serial.available()){char c=Serial.read();if(c=='W'||c=='R'){stopOutputs();if(c=='R'){prefs.remove("token");prefs.remove("enroll");token="";enrollmentCode="";}screenText("WIFI SETUP","See USB Serial Monitor.");beginSetupPortal();}if(c=='Q')qrRequested=true;}
 if(portalActive){handleSetupPortal();delay(2);return;}
 pollInputs();if(WiFi.status()!=WL_CONNECTED){stopOutputs();activeQrToken="";if(elapsed(millis(),lastWifi,10000)){lastWifi=millis();WiFi.reconnect();screenText("NO NETWORK","Waiting for Wi-Fi.\nNo access granted.");}delay(10);return;}
 if(!token.length()){if(elapsed(millis(),lastHeartbeat,10000)){lastHeartbeat=millis();enroll();}delay(10);return;}
 if(elapsed(millis(),lastHeartbeat,5000))heartbeat();
 if(qrRequested&&elapsed(millis(),lastQrRequest,5000)){qrRequested=false;newQr();}
 pollQr();pollReader();
 if(cardSession.length()&&(int32_t)(millis()-cardSessionUntil)>=0)cardSession="";
 if(previewEnabled&&(int32_t)(millis()-previewUntil)>=0)previewEnabled=false;
 if(cameraReady&&previewEnabled&&elapsed(millis(),lastPreview,2000)){lastPreview=millis();camera_fb_t* f=esp_camera_fb_get();if(f){String out;int code=post("/api/device/preview",f->buf,f->len,"image/jpeg",out);esp_camera_fb_return(f);if(code==200)previewResponse(out);else previewEnabled=false;}}
 if(screenUntil&&(int32_t)(millis()-screenUntil)>=0)screenText("READY","Tap card / NFC\nPress QR button for QR");delay(5);
}
