# ⚡ FULL-STACK WEB + PHP + DATABASE DEMO (NOCKER: NOT DOCKER)
> **"Seamless Host-to-Container Interconnect · SQLite / PHP 8 · APFS Instant Volume Sync"**  
> **Author & Architect:** Wirot Chookeaw (`chin6700x`) | **License:** Apache License 2.0 &copy; 2026

---

## 🎯 เกี่ยวกับ Demo นี้ (About This Demo)

ชุด Demo นี้จำลองการทำงานจริงของโปรเจกต์ระดับ Production โดยแสดงให้เห็น:
1. การรันเว็บแอปพลิเคชัน (PHP 8 + Web Server) ภายในคอนเทนเนอร์
2. การเชื่อมต่อฐานข้อมูลจริง (SQLite Database: `silicon_store.db`)
3. การเปิดพอร์ตเครือข่าย `8080` ให้เครื่อง Mac เข้าถึงได้แบบไร้รอยต่อ
4. การเก็บรักษาข้อมูล (Data Persistence) ผ่าน APFS Volume Mount
5. หน้าจอเว็บที่สรุป **Step 1 - 2 - 3 - 4** วางไว้บนหน้าเว็บ ให้ผู้ใช้มองเห็นสถาปัตยกรรมทั้งระบบได้ทันที!

---

## 🚀 วิธีการรันอย่างรวดเร็ว (1-Click Run)

```bash
cd "/Volumes/Mac Backup/zero-tools/nocker/demo"

# สั่งรัน Demo ทันที
bash run_demo.sh
```
สคริปต์จะเปิด Safari/Chrome ไปที่ **`http://localhost:8080`** อัตโนมัติ!

---

## 🛠️ ขั้นตอนแบบ CLI (Step-by-Step for Developers)

```bash
# ขั้นที่ 1: ตรวจสอบ Daemon
nocker status

# ขั้นที่ 2: รัน Container ด้วย Docker CLI
docker run -d \
  --name silicon-web-demo \
  -p 8080:8080 \
  -v "$(pwd)/www:/app" \
  -v "$(pwd)/data:/data" \
  library/alpine:latest \
  sh -c "php -S 0.0.0.0:8080 -t /app"

# ขั้นที่ 3: ตรวจดูสถานะ
docker ps
docker logs silicon-web-demo

# ขั้นที่ 4: เปิดดูผลลัพธ์ผ่านเบราว์เซอร์
open http://localhost:8080
```

---

## ⏹️ วิธีหยุดการทำงาน (Stop Demo)

```bash
bash stop_demo.sh
```
