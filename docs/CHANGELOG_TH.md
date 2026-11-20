# บันทึกการเตรียมซอฟต์แวร์ R1

วันที่ 2026-11-19T23:16:41.921469+07:00

- ยืนยันจากผู้ใช้: ใช้ PCB R1 ตัวแรกที่สั่งมา จอ ILI9341 SPI 2.4 นิ้ว 240×320 รหัส L0297 และ PN532
- เพิ่มโปรเจกต์ gatepass-web-v2/firmware/gatepass-r1: pinmap เดิม จอ PN532/HCE QR และภาพกล้องแบบสั่งดู ไม่มี face recognition
- เพิ่ม API qr-status และผลตรวจสิทธิ์สำหรับพัลส์กลอนเฉพาะ R1 ในซอร์สเว็บ ไม่เปลี่ยนฐานข้อมูล Cloud
- เพิ่ม tests ของ permit expiry, retry, ownership, revocation และ compact QR
- บิลด์เฟิร์มแวร์ 2 environment และสร้าง merged images ผ่าน ตรวจ segment/hash ผ่าน
- Typecheck / web build / 14 tests ผ่าน เว็บแจ้ง Next.js security advisory ที่ยังไม่ได้อัปเดต
- ยังไม่ได้เผยแพร่เว็บ ไม่ได้แฟลชบอร์ด ไม่ได้ทดสอบฮาร์ดแวร์จริง

เวลาข้างต้นเป็นเวลาแก้ซอร์สและจัดไฟล์ ไม่ใช่เวลา deployment หรือการติดตั้งบนอุปกรณ์
