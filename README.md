# GATEPASS Hardware R1 / Firmware

โครงงาน CPE23267 · ชุดส่งซอร์ส 20 พฤศจิกายน 2569

**สถานะส่งงาน: ซอฟต์แวร์บิวด์ผ่าน แต่ยังบัดกรีบอร์ดไม่เสร็จและยังไม่ผ่านการทดสอบฮาร์ดแวร์จริงครบระบบ**

โค้ดสำหรับ PCB R1 fixed_v3 วันที่ 24 ตุลาคม 2569 ที่สั่งผลิต ใช้ ESP32-S3-WROOM-1-N8R2 (8 MB flash / 2 MB quad PSRAM), จอ ILI9341 SPI 2.4 นิ้ว 240×320 และ PN532 SPI ไม่ใช่ pin map ของ AI Thinker ESP32-CAM

## ขอบเขตซอฟต์แวร์

อ่าน UID บัตร 4/7/10 ไบต์, Android HCE APDU, แสดง QR จาก server บนจอ, ตรวจ QR ใช้ครั้งเดียว, ส่ง heartbeat/สถานะเครื่อง, เปิดดูภาพ JPEG ตามคำขอประมาณทุก 2 วินาที ไม่มีการสแกนหน้า

## โครงสร้าง

- `src/main.cpp`, `include/`: เฟิร์มแวร์ R1
- `platformio.ini`: pinned PlatformIO / libraries และสอง build environments
- `tools/audit_pinmap.py`: เทียบ 32 สัญญาณกับ pad/net R1 และตรวจ GPIO ซ้ำ
- `tools/policy_test.cpp`: host test สำหรับเงื่อนไขการสั่งกลอน
- `reference/pcb-r1/`: PCB ต้นฉบับแบบอ่านอย่างเดียว และแผนผังสำหรับดูตอนบัดกรี
- `mechanical/`: กล่อง V1 สำหรับตรวจแบบ 3D ยังไม่ยืนยัน fit กับโมดูลจริง
- `bin/`: merged images BENCH/CONTROL, offset 0x0, ตรวจ hash ใน MANIFEST.json
- `docs/`: pin map และหลักฐานการตรวจวันที่เตรียมเฟิร์มแวร์

## เปิดและบิวด์

เปิดโฟลเดอร์นี้ใน VS Code + PlatformIO หรือใช้ CLI:

```sh
pio run -e gatepass-r1-bench
pio run -e gatepass-r1
python3 tools/audit_pinmap.py
c++ -std=c++17 tools/policy_test.cpp -o /tmp/gatepass-policy-test
/tmp/gatepass-policy-test
```

`gatepass-r1-bench` ปิดการสั่งกลอนฝั่ง firmware ส่วน `gatepass-r1` ยังต้องได้ผลอนุญาตสดและ server opt-in ก่อนสั่งพัลส์ จึงไม่เปิดกลอนเพียงเพราะเปิดเว็บ Production

## ไฟล์พร้อมแฟลช (หลังตรวจบอร์ดพร้อมเท่านั้น)

```sh
python3 flash_r1.py --port /dev/cu.usbmodemXXXX --mode bench
```

พอร์ตเป็นตัวอย่าง ต้องเลือกจากบอร์ดจริง สคริปต์ไม่แฟลชอัตโนมัติเมื่อเปิด repo และตรวจ SHA-256 ก่อนเขียน เริ่ม BENCH ก่อน CONTROL ทั้งสอง binary เริ่ม offset **0x0**

ครั้งแรกตั้งค่า Wi-Fi 2.4 GHz / API URL / Device ID / enrollment code 48 ตัวผ่านหน้าตั้งค่าของบอร์ด Device token 64 ตัวเก็บใน NVS ไม่ใส่ Supabase service key ลงเฟิร์มแวร์

## งานที่เหลือหลังส่งซอร์ส

1. บัดกรีและตรวจ short/แรงดัน/GND ก่อนจ่ายไฟ
2. ตรวจ backlight จอ (R1 ใช้ R65 100Ω; ห้ามสรุปว่าขา GPIO ขับกระแส backlight ได้โดยไม่ตรวจโมดูล)
3. ตรวจไฟเลี้ยง/ภาพกล้อง/PN532/jumper SPI/ขาจอตามชื่อขาจริง
4. ทดสอบบัตร/HCE/QR กับ backend และมือถือจริง
5. ปรับ polarity เซนเซอร์ก่อนใช้ DOOR/REX แล้วตรวจพัลส์กลอนและเงื่อนไขตัดไฟ

รายงาน build/host checks แยกจากการตรวจ EasyEDA ERC/DRC, RF, วงจรไฟเลี้ยง, กล้องจริงและการเปิดประตูจริง ไม่มีการอ้างว่าบัดกรีเสร็จแล้ว

Web/API: https://github.com/Phuwitt/gatepass-web
Mobile: https://github.com/Phuwitt/gatepass-mobile
