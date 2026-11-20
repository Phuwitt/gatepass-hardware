#pragma once
// Uses only WebServer/DNSServer shipped with the ESP32 Arduino board package.
#include <WebServer.h>
#include <DNSServer.h>
WebServer setupWeb(80);
DNSServer setupDns;
bool portalActive=false;
uint32_t portalStarted=0,restartAfter=0;
String formToken;
String escapeHtml(String text){text.replace("&","&amp;");text.replace("<","&lt;");text.replace(">","&gt;");text.replace("\"","&quot;");text.replace("'","&#39;");return text;}
String randomHex(size_t count){String out;for(size_t i=0;i<count;i++){char b[3];snprintf(b,sizeof(b),"%02x",(uint8_t)esp_random());out+=b;}return out;}
bool validDeviceId(const String& value){if(value.length()<1||value.length()>40)return false;for(size_t i=0;i<value.length();i++){char c=value[i];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;}return true;}
bool validEnrollment(const String& value){if(value.length()!=48)return false;for(size_t i=0;i<value.length();i++){char c=value[i];if(!((c>='a'&&c<='f')||(c>='0'&&c<='9')))return false;}return true;}
void setupPage(){
 String html=F("<!doctype html><html lang='th'><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>GATEPASS Setup</title><style>body{font:16px system-ui;background:#eaf3f4;color:#163b40;margin:0;padding:24px}main{max-width:480px;margin:auto;background:white;padding:24px;border-radius:16px}label{display:block;margin:18px 0}input{display:block;box-sizing:border-box;width:100%;padding:12px;margin-top:6px;border:1px solid #a6c3c8;border-radius:8px;font:inherit}button{padding:14px;background:#006763;color:white;border:0;border-radius:8px;font:inherit}small,p{line-height:1.7}</style><main><h1>GATEPASS Setup</h1><p>ตั้งค่าเครือข่าย Wi-Fi 2.4 GHz และลงทะเบียนเครื่อง</p><form method='post' action='/save'><input type='hidden' name='csrf' value='");
 html+=formToken;html+=F("'><label>ชื่อ Wi-Fi (SSID)<input name='ssid' maxlength='32' required value='");html+=escapeHtml(wifiSsid=="YOUR_WIFI"?"":wifiSsid);
 html+=F("'></label><label>รหัส Wi-Fi<input type='password' name='password' maxlength='64' autocomplete='new-password'></label><small>กรอกรหัส Wi-Fi ใหม่ทุกครั้ง เว้นว่างเฉพาะเครือข่ายไม่มีรหัสผ่าน</small><label>API URL ของคอมพิวเตอร์<input name='api' maxlength='200' required value='");html+=escapeHtml(apiBase);
 html+=F("'></label><label>Device ID จากเว็บ<input name='device' maxlength='40' required value='");html+=escapeHtml(deviceId);
 html+=F("'></label><label>รหัสลงทะเบียนจากเว็บ (48 ตัว)<input type='password' name='code' maxlength='48' autocomplete='off'></label><small>ครั้งแรกต้องใส่รหัสลงทะเบียน เมื่อเปลี่ยนเฉพาะ Wi-Fi ของเครื่องที่ลงทะเบียนแล้ว เว้นช่องนี้ว่างได้ ไม่ใช่รหัส Admin</small><p><button>บันทึกและเชื่อมต่อ</button></p></form><p>Wi-Fi ตั้งค่าปิดเมื่อบันทึกสำเร็จหรือครบ 10 นาที กลับไปใช้ Wi-Fi เดิมหลังบอร์ดรีสตาร์ต</p></main></html>");
 setupWeb.sendHeader("Cache-Control","no-store");setupWeb.send(200,"text/html; charset=utf-8",html);
}
void saveSetup(){
 if(restartAfter||setupWeb.arg("csrf")!=formToken){setupWeb.send(403,"text/plain; charset=utf-8","หมดอายุ เปิดหน้าตั้งค่าใหม่");return;}
 String ssid=setupWeb.arg("ssid"),password=setupWeb.arg("password"),api=setupWeb.arg("api"),id=setupWeb.arg("device"),code=setupWeb.arg("code");api.trim();id.trim();code.trim();while(api.endsWith("/"))api.remove(api.length()-1);
 bool identityChanged=id!=deviceId||api!=apiBase;
 if(ssid.length()<1||ssid.length()>32||password.length()>64||(!password.isEmpty()&&password.length()<8)||!validDeviceId(id)||api.length()>200||!validApiUrl(api)||(!code.isEmpty()&&!validEnrollment(code))||((!token.length()||identityChanged)&&!validEnrollment(code))){setupWeb.send(400,"text/plain; charset=utf-8","ข้อมูลไม่ถูกต้อง: ตรวจ SSID, รหัส Wi-Fi อย่างน้อย 8 ตัว (หรือว่างสำหรับ open Wi-Fi), URL แบบ http://IP:3000, Device ID และรหัสลงทะเบียนใหม่ 48 ตัว");return;}
 // Mark incomplete until all settings have been written. Never echo secrets.
 prefs.putBool("configured",false);prefs.putString("ssid",ssid);prefs.putString("wifi_pass",password);prefs.putString("api",api);prefs.putString("device",id);
 if(identityChanged||code.length()){prefs.remove("token");prefs.putString("enroll",code);}else prefs.remove("enroll");
 prefs.putBool("configured",true);setupWeb.sendHeader("Cache-Control","no-store");setupWeb.send(200,"text/html; charset=utf-8","<meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><h2>บันทึกแล้ว กำลังรีสตาร์ต</h2><p>กลับไปต่อ Wi-Fi เดิม แล้วเปิดเว็บ GATEPASS บนคอมพิวเตอร์ ดูผลได้ใน Serial Monitor 115200</p>");restartAfter=millis()+1500;
}
void beginSetupPortal(){
 if(portalActive)return;
 if(cameraReady){esp_camera_deinit();cameraReady=false;}
 WiFi.disconnect();WiFi.mode(WIFI_AP);IPAddress ip(192,168,4,1);WiFi.softAPConfig(ip,ip,IPAddress(255,255,255,0));
 String name="GATEPASS-SETUP-"+String((uint32_t)(ESP.getEfuseMac()&0xffffff),HEX),password=randomHex(6);formToken=randomHex(16);
 if(!WiFi.softAP(name.c_str(),password.c_str(),1,0,1)){Serial.println("Setup AP failed. Reset to retry.");return;}
 setupDns.start(53,"*",ip);setupWeb.on("/",HTTP_GET,setupPage);setupWeb.on("/save",HTTP_POST,saveSetup);setupWeb.onNotFound([](){setupWeb.sendHeader("Location","http://192.168.4.1/",true);setupWeb.send(302,"text/plain","");});setupWeb.begin();portalActive=true;portalStarted=millis();restartAfter=0;
 Serial.printf("\r\nSETUP Wi-Fi: %s\r\nSETUP password: %s\r\nOpen http://192.168.4.1/ (expires in 10 minutes)\r\n",name.c_str(),password.c_str());
}
void handleSetupPortal(){setupDns.processNextRequest();setupWeb.handleClient();if((restartAfter&&(int32_t)(millis()-restartAfter)>=0)||millis()-portalStarted>600000){setupWeb.stop();setupDns.stop();WiFi.softAPdisconnect(true);ESP.restart();}}
