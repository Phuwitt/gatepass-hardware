# GATEPASS R1 รุ่น 1.0.0

เฟิร์มแวร์สำหรับ **PCB R1 fixed_v3 วันที่ 24 ตุลาคม 2569 ที่สั่งมาแล้ว** ใช้ ESP32-S3-WROOM-1-N8R2, จอ **ILI9341 SPI 2.4 นิ้ว 240×320 (L0297)** และ **PN532 โหมด SPI** ตามรุ่นที่ผู้ใช้ยืนยันวันที่ 19 พฤศจิกายน 2569 ไม่ใช่ pin map ของ AI Thinker ESP32-CAM

## สิ่งที่ทำได้ในซอฟต์แวร์ชุดนี้

- อ่านบัตร ISO14443A UID 4/7/10 ไบต์ผ่าน PN532 และลงทะเบียนบัตรผ่านเว็บ
- อ่านข้อมูล GATEPASS Android HCE ผ่าน APDU เดิม แยกจาก UID ของบัตรจริง
- กดปุ่มที่ J9 หรือพิมพ์ Q ใน USB Serial เพื่อขอ QR จาก Backend แล้วแสดงบนจอ ให้แอปมือถือสแกน
- QR เป็นข้อมูลจากระบบจริง อายุ 60 วินาที ใช้ครั้งเดียว ตรวจผลผ่าน API ของเครื่องตนเอง ไม่สร้างสิทธิ์จำลอง
- แสดง READY, QR, ACCESS ALLOWED / DENIED และข้อผิดพลาดบนจอ ใช้ข้อความอังกฤษเพื่อให้อ่านได้ด้วยฟอนต์ในอุปกรณ์
- เปิดดูภาพกล้องผ่านเว็บเมื่อมีคำขอดูภาพ ส่งภาพ JPEG ประมาณทุก 2 วินาที ไม่ใช่วิดีโอเฟรมเรตสูง และ **ไม่มีการสแกนหน้า / ไม่เรียก API face**
- สัญญาณกลอนปิดตอนเริ่มเครื่อง/เครือข่ายขาด ใช้ timer แยกเพื่อปิดพัลส์ แม้ HTTP กำลังรอคำตอบ
- แยกสถานะผ่านสิทธิ์ พัลส์ที่ส่ง และเซ็นเซอร์ประตู ไม่อ้างว่าประตูเปิดจริงจากผลอนุญาต

## สถานะที่ตรวจแล้ว

PlatformIO build ผ่านทั้ง gatepass-r1 และ gatepass-r1-bench, pin map 32 สัญญาณตรงกับ pad/net ในไฟล์เดิมและไม่ชน GPIO USB19/20, ทดสอบเงื่อนไขพัลส์/คำขอซ้ำ/เวลา overflow บน host ผ่าน เว็บ typecheck และ build ผ่าน ทดสอบ API/policy รวม 14 รายการผ่าน

**ยังไม่ได้แฟลชบอร์ด และยังไม่ได้ทดสอบจอ/PN532/กล้อง/กลอนบน PCB จริง** การ build ไม่ได้ยืนยันไฟเลี้ยง การบัดกรี กล้อง FPC หรือการต่อโมดูล ส่วน Backend ที่เพิ่ม qr-status อยู่ในซอร์สเครื่องนี้ ต้องเผยแพร่ก่อนใช้ฟังก์ชันรับผล QR บนบอร์ดกับเว็บโฮสต์เดิม

## ไฟล์ที่เริ่มใช้

- `bin/GATEPASS_R1_1.0.0_BENCH_FULL.bin` เริ่มตรวจจอ PN532 Wi-Fi กล้องและสิทธิ์ **ไม่ขับกลอน**
- `bin/GATEPASS_R1_1.0.0_CONTROL_FULL.bin` รองรับพัลส์กลอนเมื่อระบบหลังบ้านเปิดให้และสิทธิ์ผ่าน
- ทั้งสองไฟล์เป็น merged image สำหรับ ESP32-S3 **เริ่ม offset 0x0** ไม่ใช่ application image ที่ 0x10000
- `flash_r1.py` ช่วยเลือกไฟล์และตรวจ SHA-256 ก่อนแฟลช ระบุพอร์ตเอง ไม่มีการแฟลชอัตโนมัติ
- `PINMAP_R1.json` ผังขาที่ตรวจเทียบกับต้นฉบับ
- `firmware/` ซอร์ส PlatformIO และตัวอย่าง config ไม่มีรหัส Wi-Fi หรือ Supabase key จริง

## แฟลชบน Mac หลังประกอบพร้อมทดสอบ

1. เริ่มด้วยชุด BENCH ต่อ USB data ที่บอร์ด ถ้าไม่พบพอร์ต ให้กด BOOT ค้าง กด RESET หนึ่งครั้ง แล้วปล่อย BOOT เพื่อเข้า download mode
2. ดูชื่อพอร์ต `/dev/cu.usbmodem...` จากเครื่องจริง
3. ในโฟลเดอร์ชุดไฟล์นี้ รัน `python3 flash_r1.py --port /dev/cu.usbmodemXXXX --mode bench`
4. สคริปต์ใช้ esptool ที่มีใน PlatformIO ของ Mac นี้ได้ หากเครื่องอื่นไม่มี ให้ติดตั้ง esptool ใน Python environment ที่ใช้ก่อน
5. เปิด USB Serial Monitor 115200 (ใช้ USB CDC ไม่ใช่ UART GPIO43/44 ซึ่ง R1 นำไปใช้กับอุปกรณ์)
6. ครั้งแรกบอร์ดเปิด Wi-Fi `GATEPASS-SETUP-...` รหัสชั่วคราวแสดงใน USB Serial ต่อ Wi-Fi นี้แล้วเปิด `http://192.168.4.1/`
7. ใส่ Wi-Fi 2.4GHz, API URL เป็น origin ของเว็บที่ใช้จริง, Device ID และรหัสลงทะเบียนอุปกรณ์ 48 ตัวจากเว็บ แล้วบันทึก
8. กลับ Wi-Fi ปกติ ตรวจว่าเครื่อง Online และเช็กแยกว่ากล้อง/เครื่องอ่านพร้อมหรือไม่

เปลี่ยนการตั้งค่าด้วย W ใน Serial; R ล้าง device credential และเปิดหน้าตั้งค่าใหม่; Q ขอ QR รหัสลงทะเบียน 48 ตัวเป็นของอุปกรณ์ ไม่ใช่รหัส NFC มือถือ 64 ตัว ไม่ใช้ Supabase service key บนบอร์ด

HTTPS ตรวจใบรับรองด้วย public Mozilla CA bundle ที่แนบ ไม่ใช้ setInsecure; หากนาฬิกายังไม่ได้ NTP จะรอการเชื่อมต่อแบบ TLS ที่ถูกต้อง HTTP อนุญาตเฉพาะ IPv4 ใน LAN สำหรับโต๊ะทดลอง ชื่อ localhost บนบอร์ดไม่ใช่ Mac

## ต่อจอ ILI9341 กับ J4 ตามชื่อขา

**อย่าเสียบสาย 8 ขาตรงตามลำดับโดยไม่เทียบชื่อ** จอ L0297 มีขาเพิ่มสำหรับ MISO, touch และ SD ซึ่งไม่ได้ใช้ในงานนี้

| J4 บน R1 | Net | ต่อกับจอ | GPIO |
|---|---|---|---|
| 1 | GND | GND | — |
| 2 | +3V3 | VCC เมื่อโมดูลรองรับไฟ 3.3V | — |
| 3 | LCD_SCK | SCK / CLK | 14 |
| 4 | LCD_MOSI | SDI / MOSI | 21 |
| 5 | ESP_EN | RESET / RST | reset ร่วมกับ ESP32 ไม่ใช้ GPIO toggle |
| 6 | LCD_DC | DC / RS | 48 |
| 7 | LCD_CS_N | CS | 47 |
| 8 | LCD_BL | LED / BL **ตรวจวงจรโมดูลก่อน** | 37 ผ่าน R65 100Ω |

บน R1 ไม่มีทรานซิสเตอร์ขับแบ็กไลต์แยก โค้ดจึงตั้ง GPIO37 เป็น input เป็นค่าเริ่มต้น จออาจมืดถ้า LED ต้องรับไฟจากขานี้ **ตอนตรวจจอ ให้แยกสาย LED ออกจาก J4.8 และจ่ายแบ็กไลต์ตามสเปกของโมดูลจริง** ถ้าเป็นโหลด LED ให้ใช้วงจรขับที่เหมาะสม ไม่ให้ GPIO แบกกระแส LED โดยตรง หากตรวจแล้วว่า BL เป็น logic input ที่กระแสปลอดภัย จึงเปิด `GP_LCD_BL_GPIO_ENABLED=1` ใน config และ build ใหม่ ไม่อาศัยคำว่า ILI9341 เพื่อตัดสินวงจรแบ็กไลต์ของแต่ละโมดูล

ไม่มีการใช้ LCD SDO/MISO, touch หรือ SD บน R1 ชุดนี้ หมุนภาพด้วย GP_LCD_ROTATION 0–3 ปัจจุบันตั้งแนวตั้ง 240×320

## ต่อ PN532 กับ J5

ตั้งโมดูลเป็น **SPI ด้วยสวิตช์/จัมเปอร์ของรุ่นนั้น** เทียบชื่อสัญญาณและระดับแรงดันก่อนต่อ

| J5 | Net | PN532 | GPIO |
|---|---|---|---|
| 1 | NFC_SCK | SCK | 38 |
| 2 | NFC_MISO | MISO | 40 |
| 3 | NFC_MOSI | MOSI | 39 |
| 4 | NFC_CS_CONN_N | SS / SSEL / CS | 43 |
| 5 | +3V3 | VCC ตามสเปกโมดูลที่รองรับ 3.3V | — |
| 6 | GND | GND | — |
| 7,8 | ไม่ต่อในต้นฉบับ | ไม่ใช้ | — |

ไม่อ่านบัตร 125kHz, ไม่เขียน UID มือถือให้เหมือนบัตร PN532 กับจอใช้ SPI คนละ peripheral เพื่อไม่สลับโหมด/ขาร่วมกัน

## QR บนบอร์ด

J9 มีสองขา QR_CONTACT/GND ใช้ปุ่มหน้าสัมผัสกดเข้ากราวด์แล้วขอ QR มันไม่ใช่ UART ของ QR scanner และไม่ได้รับข้อความ QR เข้ามาทางหัวต่อนี้

1. เครื่องลงทะเบียนกับเว็บและเชื่อม Wi-Fi
2. กด J9 หรือ Q → ขอ `/api/device/qr-session?format=text`
3. จอแสดง URL QR จริง → พนักงานเข้าสู่แอปและสแกน
4. ระบบตรวจวิธี QR/สถานะพนักงาน/สิทธิ์ประตู/ช่วงเวลา บันทึก event แล้วใช้ QR ซ้ำไม่ได้
5. บอร์ดเรียก `/api/device/qr-status` ทุก 4 วินาทีเพื่อรับผลเฉพาะ QR ของตัวเอง
6. หมดอายุหรือเครือข่ายเสียจะไม่ให้สิทธิ์เอง กดปุ่มเพื่อขอใหม่

## เปิดดูภาพกล้อง

ตั้งกล้องตามสาย FPC24 ของ R1 เดิม บอร์ดใช้ D0–D7=4,5,6,7,15,16,17,18; XCLK8/PCLK9/VSYNC10/HREF11/SDA12/SCL13 กล้อง PWDN/RESET ผูกสถานะด้วย resistor จึงตั้ง pin เป็น -1 ไม่ยกเลข GPIO จาก ESP32-CAM มาใช้

เมื่อเปิดดูภาพจากเว็บที่เครื่องถูกผูกไว้ Backend ส่ง preview_enabled แล้วบอร์ดจึงส่ง JPEG ไป `/api/device/preview` หยุดเมื่อหมดคำขอ ถ้ากล้อง init ไม่ผ่าน จอ/QR/PN532 ยังทำงานได้ รูปจากกล้องใช้ดูหน้าประตู ไม่วิเคราะห์ใบหน้า ทั้งนี้ต้องตรวจรุ่นกล้อง หน้าสัมผัส FPC และขั้วจ่ายไฟกับโมดูลจริง

## กลอน เซ็นเซอร์ และ Backend

เริ่มทดสอบไฟล์ BENCH ก่อน ชุด CONTROL จะส่งพัลส์เฉพาะเมื่อ Backend คืน `allowed:true`, `actuation_enabled:true`, request_id ใหม่ และผลตรวจยังไม่เกิน 10 วินาที บันทึก request_id ก่อนสั่งกลอน พัลส์ 3000ms ปิดด้วย esp_timer แยกจาก network loop

Backend ปิดการขับกลอนเป็นค่าเริ่มต้น เปิดเฉพาะเมื่อทดสอบภาคกลอนแล้วด้วย environment **GATEPASS_R1_ACTUATION_ENABLED=true** และเผยแพร่ Backend ที่แก้ไว้ ใช้เฉพาะ firmware gatepass-r1-1.0.0 รุ่นเดิมยังไม่ขับโหลด การเรียกคำขอเก่าไม่ต่ออายุคำอนุญาตให้พัลส์ใหม่

อินพุต J7 ประตู=GPIO41, J8 ปุ่มขอออก=GPIO42, J10 tamper=GPIO36 มี debounce ซอฟต์แวร์ ปิดการรายงาน sensor/สั่ง REX เป็นค่าเริ่มต้นจนกำหนด NO/NC จากการติดจริง: GP_DOOR_SENSOR_ENABLED, GP_DOOR_OPEN_LEVEL, GP_TAMPER_SENSOR_ENABLED, GP_TAMPER_ACTIVE_LEVEL, GP_REX_ENABLED การไม่ทราบสถานะไม่ถูกส่งเป็น “ประตูปิด” กลอนใช้ GPIO1 ผ่าน MOSFET บน R1 ไม่ใช่คอยล์ต่อ GPIO โดยตรง

มีการแก้เฉพาะซอร์ส Backend ในเครื่องนี้ ยังไม่ได้อัปเดต deployment ออนไลน์ และไม่ได้แก้ฐานข้อมูล Cloud ต้องเผยแพร่ API ส่วนเพิ่มก่อนทดสอบรับผล QR บนฮาร์ดแวร์ เว็บยังแจ้งเตือน dependency Next.js ที่มี security advisory; ควรอัปเดตก่อนเปิดใช้งานจริง

## Build ซอร์ส

`pio run -e gatepass-r1 -e gatepass-r1-bench` ในโฟลเดอร์ firmware ใช้ espressif32 6.13.0 และ Arduino core 2.0.17 ตั้ง Quad PSRAM สำหรับ N8R2 ไม่ใช้ Octal PSRAM ของ N8R8 การเปลี่ยน config หลังแจกไฟล์ต้อง build และ merge image ใหม่ ไฟล์ bin เดิมไม่เปลี่ยนตาม config ที่แก้

## แหล่งอ้างอิง

- PCB/BOM ที่ผู้ใช้ส่ง: GatePassR1 fixed_v3 2026-10-24; pin mapping verified against original PAD_NET_MAP.csv
- https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf (module pad ↔ GPIO)
- https://github.com/adafruit/Adafruit_ILI9341 (SPI constructor, native 240×320)
- https://github.com/adafruit/Adafruit-PN532 (ISO14443A / APDU driver)
- https://github.com/espressif/esp32-camera (JPEG capture)
