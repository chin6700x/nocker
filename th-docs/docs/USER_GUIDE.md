# ⚡ NOCKER: คู่มือการใช้งานฉบับสมบูรณ์ (Official User Guide)
> **"Bare-Metal Silicon Velocity · APFS Copy-on-Write · 100% Docker CLI Parity"**  
> **ผู้พัฒนาและสถาปนิก:** Wirot Chookeaw (`chin6700x`) | **ลิขสิทธิ์:** Apache License 2.0 &copy; 2026  
> **ตำแหน่งติดตั้ง:** `/Volumes/Mac Backup/zero-tools/nocker/`  
> 🌐 **ภาษา:** [🇹🇭 ภาษาไทย](USER_GUIDE.md) | [🇬🇧 English Version](../../docs/USER_GUIDE.md)

---

## 🧭 1. ทำความเข้าใจสถาปัตยกรรม (Architecture Overview)

ก่อนเริ่มใช้งาน สิ่งสำคัญคือการเข้าใจว่า **`nocker`** ทำงานอย่างไร และแตกต่างจาก Docker ทั่วไปอย่างไร:

### 💡 ความจริงของ Docker
ในระบบ Docker ดั้งเดิม จะแบ่งออกเป็น 2 ชิ้นส่วนหลัก:
1. **Docker CLI (Client):** ตัวรับคำสั่ง (เช่น `/usr/local/bin/docker`) เมื่อคุณพิมพ์ `docker run` หรือ `docker ps` มันไม่ได้เป็นคนรันคอนเทนเนอร์เอง แต่จะส่งข้อความ JSON ผ่านสาย UNIX Socket (`~/.nocker/run/docker.sock`) ไปหาตัวแม่
2. **Docker Engine / Daemon (Server):** ตัวแม่ตัวจริงที่คอยฟังคำสั่ง ดึง Image แตกไฟล์ และสั่ง CPU ให้รันโปรเซส ในอดีตบน Mac ส่วนนี้คือ **Docker Desktop** ซึ่งต้องเปิด Virtual Machine ตัวยักษ์ กิน RAM 3–4 GB และหน่วงเครื่อง

### ⚡ สิ่งที่ `nocker` ทำ
เราได้เขียน **Docker Engine ตัวแม่ (Daemon)** ขึ้นมาใหม่ทั้งหมดด้วยภาษา C และ Objective-C บนชิป Apple Silicon โดยตรง:
- **ไม่ต้องเปิด Docker Desktop:** ปิดทิ้งได้เลย ไม่กิน RAM มหาศาล
- **ความเร็วเหนือระดับ (Bare-Metal Speed):** ใช้ Apple APFS `clonefile()` โคลน RootFS คอนเทนเนอร์เสร็จในเวลาเพียง **60 มิลลิวินาที** โดยกินเนื้อที่ดิสก์ **0 ไบต์** (Zero-Copy)
- **เครื่องยนต์คู่ (Twin-Engine Architecture):**
  - **Engine 1 (Darwin Seatbelt Native):** รันคำสั่งด้วยความเร็วระดับ Hardware Silicon **< 2ms**
  - **Engine 2 (Apple `Virtualization.framework` MicroVM):** ตรวจสอบและบูต Linux MicroVM ขนาดจิ๋วใน **4.10 ms**

---

## 🚀 2. การเริ่มต้นใช้งานอย่างรวดเร็ว (Quickstart)

### ตรวจสอบสถานะการทำงานของระบบ
```bash
# ตรวจสอบสถานะของ Daemon
nocker status

# ตรวจสอบความพร้อมของฮาร์ดแวร์ Apple Silicon
nocker doctor

# ดูข้อมูลระบบ สเปค CPU, RAM และ Storage Driver
nocker info
```

### การเปิด Daemon ให้ทำงานเป็นเบื้องหลัง (Background Daemon)
หาก Daemon ยังไม่ได้เปิด สามารถสั่งรันได้ด้วยคำสั่ง:
```bash
nocker daemon &
```
เมื่อ Daemon เริ่มทำงาน จะสร้าง Socket สำหรับรับคำสั่งไว้ที่:
- `~/.nocker/run/nocker.sock` (พร้อมเชื่อมต่อ `docker.sock` ภายในโฟลเดอร์เดียวกัน)
- และเชื่อมต่อเข้ากับ `/var/run/docker.sock` มาตรฐานของ macOS เมื่อระบุ `--global` เพื่อให้ Docker CLI ภายนอกคุยได้ทันที

---

## 💻 3. วิธีการสั่งงาน (เลือกใช้ได้ 2 รูปแบบ)

คุณสามารถเลือกสั่งงานได้ 2 ช่องทางอย่างอิสระ:

### ช่องทางที่ 1: สั่งผ่านคำสั่ง `nocker` โดยตรง
เหมาะสำหรับการใช้งานแบบรวดเร็ว และใช้ความสามารถเฉพาะทางของ Apple Silicon:
```bash
nocker run alpine:latest "echo 'Hello from Nocker'"
```

### ช่องทางที่ 2: สั่งผ่านคำสั่ง `docker` ดั้งเดิมของเครื่อง Mac
เครื่องมือภายนอก (เช่น VS Code, Cursor, CI/CD Script) สามารถพิมพ์ `docker` ได้ตามปกติ โดยระบบจะส่งคำสั่งมาประมวลผลที่ `nocker` อัตโนมัติ:
```bash
docker run --rm library/alpine:latest uname -a
```

### ช่องทางที่ 3: สื่อสารผ่าน REST API & UNIX Domain Socket
โปรแกรมภายนอก, สคริปต์ Python, Go, Node.js หรือแอป Desktop สามารถยิง HTTP REST API ผ่าน UNIX Socket ได้โดยตรง:
```bash
curl --unix-socket ~/.nocker/run/docker.sock http://localhost/v1.45/containers/json
```
* สอดคล้องกับ Docker Engine API v1.45 แบบ 100%
* หน่วยความจำขณะ Daemon ทำงานไม่ถึง 4 MB RAM
* สื่อสารผ่าน Kernel IPC เร็วและปลอดภัยกว่า TCP Port

> 💡 **สำหรับผู้ใช้ที่ต้องการ GUI Dashboard:** สามารถใช้งานคู่กับ **Nocker Studio Pro** (Desktop App ที่กำลังพัฒนาด้วย Rust) ซึ่งจะเชื่อมต่อเข้ากับ Socket นี้โดยอัตโนมัติ!

---

## 🛠️ 4. สรุปคำสั่งพื้นฐานและการใช้งานจริง (Command Reference)

### 1) ดูข้อมูลระบบ (`info` / `doctor`)
```bash
nocker info
# หรือ
docker info
```
*ผลลัพธ์จะแสดง Storage Driver เป็น `apfs-cow`, Snapshotter เป็น `clonefile`, ระบบปฏิบัติการ macOS Darwin ARM64 พร้อมจำนวน CPU และ RAM จริงของเครื่อง*

---

### 2) การรันคอนเทนเนอร์ (`run`)
```bash
# รันแบบธรรมดา (One-off)
nocker run alpine:latest "echo 'Bare-metal silicon speed'"

# รันแบบจำกัดเวลา Watchdog ป้องกันค้าง (หน่วย ms)
nocker run --timeout 2000 alpine:latest "sleep 1"

# บังคับรันบน Bare-Metal Darwin Seatbelt (< 2ms)
nocker run --native host "echo 'Fastest execution on Apple Silicon'"

# บังคับรันบน Apple Virtualization Linux MicroVM
nocker run --vz alpine:latest "uname -a"
```

---

### 3) ดูคอนเทนเนอร์ทั้งหมด (`ps`)
```bash
# ดูเฉพาะคอนเทนเนอร์ที่กำลังทำงาน
nocker ps

# ดูคอนเทนเนอร์ทั้งหมด (ทั้งที่รันอยู่และจบไปแล้ว)
nocker ps -a
# หรือ
docker ps -a
```

---

### 4) ดูประวัติ Log ของคอนเทนเนอร์ (`logs`)
```bash
nocker logs <CONTAINER_NAME_OR_ID>
# หรือ
docker logs <CONTAINER_NAME_OR_ID>
```

---

### 5) ตรวจสอบสเปคคอนเทนเนอร์แบบละเอียด (`inspect`)
```bash
nocker inspect <CONTAINER_NAME_OR_ID>
# หรือ
docker inspect <CONTAINER_NAME_OR_ID>
```

---

### 6) ลบคอนเทนเนอร์ที่ไม่ใช้งาน (`rm`)
```bash
nocker rm <CONTAINER_NAME_OR_ID>
# หรือ
docker rm <CONTAINER_NAME_OR_ID>
```

---

### 7) ดู Image ที่มีอยู่ในแคชเครื่อง (`images`)
```bash
nocker images
# หรือ
docker images
```

---

### 8) ดาวน์โหลด Image เพิ่มเติมจาก Docker Hub (`pull`)
```bash
nocker pull alpine:latest
```

---

## 📂 5. การนำไฟล์และโปรแกรมเข้าสู่ Container (3 วิธี)

### วิธีที่ 1: การใช้ Volume Mount (`-v /host:/container`) — *แนะนำที่สุด*
Mount โฟลเดอร์จากเครื่อง Mac เข้าสู่ Container ทันทีโดยไม่ต้องก๊อปปี้ไฟล์:

```bash
# ตัวอย่าง: รันสคริปต์ Python ในโฟลเดอร์ของ Mac ผ่าน Container
docker run --rm -v /tmp/my_app:/app library/alpine:latest python3 ./app/main.py

# หรือใช้ผ่าน nocker
nocker run -v /tmp/my_app:/app alpine:latest "sh ./app/run.sh"
```

---

### วิธีที่ 2: การใช้คำสั่ง `docker cp` — *ก๊อปปี้ไฟล์เข้า/ออก Container*

#### ก๊อปปี้ไฟล์จาก Mac เข้าสู่ Container:
```bash
# 1. สร้างคอนเทนเนอร์รอไว้
docker create --name app_box library/alpine:latest ./app/server

# 2. ก๊อปปี้ไฟล์โปรแกรม binary เข้าไปในโฟลเดอร์ /app ของ Container
docker cp /path/to/my_program app_box:/app/server

# 3. สั่งเริ่มทำงาน
docker start app_box
```

#### ก๊อปปี้ไฟล์จาก Container ออกมายัง Mac:
```bash
docker cp app_box:/app/output.log /tmp/output.log
```

---

### วิธีที่ 3: เปิดดูไฟล์ใน RootFS ของ Container โดยตรงผ่าน Mac Finder
เนื่องจากคอนเทนเนอร์ทุกตัวสร้างบน APFS จริง จึงสามารถเปิดดูไฟล์ของ Container ได้ทันทีที่:
```bash
~/.nocker/storage/containers/<CONTAINER_ID>/rootfs/
```
คุณสามารถเปิดผ่าน Finder, แก้ไขไฟล์ด้วย VS Code หรือดูด้วย Terminal ได้แบบสดๆ ครับ!

---

## 📦 6. การสร้าง Custom Image (`Dockerfile`), ระบบฐานข้อมูล (Database) และความปลอดภัยของข้อมูล

ในหัวข้อนี้จะอธิบายสิ่งที่นักพัฒนาต้องใช้งานเป็นประจำ: การสร้าง Image ของเราเอง, การรัน Database ยอดนิยม เช่น MySQL/Postgres/Redis, และการรักษาข้อมูลไม่ให้สูญหาย

### 6.1 ความแตกต่างระหว่าง `Dockerfile` กับ `docker-compose`
- **`Dockerfile` (สูตรสร้าง Image เดี่ยว):** เปรียบเสมือน "สูตรทำอาหาร" ใช้สร้าง Image ก้อนใหม่จากศูนย์ โดยระบุ OS เริ่มต้น, คำสั่งติดตั้งโปรแกรม และไฟล์โค้ด
- **`docker-compose.yml` (ผู้จัดการวง / Orchestrator):** เปรียบเสมือน "ผู้จัดการร้าน" ที่สั่งให้รันหลาย Container พร้อมกัน (เช่น รันเว็บแอป + รัน MySQL + รัน Redis) และต่อสายเชื่อมโยงหากันเป็นระบบ

### 6.2 วิธีเขียน `Dockerfile` สำหรับแอปของคุณ (4 คำสั่งหลัก)
สร้างไฟล์ข้อความชื่อ `Dockerfile` (ไม่มีนามสกุล) ในโฟลเดอร์โปรเจกต์ของคุณ:
```dockerfile
# 1. เลือก OS / รันไทม์ตั้งต้น
FROM library/alpine:latest

# 2. ติดตั้งโปรแกรมและแพ็กเกจที่ต้องการ
RUN apk add --no-cache python3 py3-pip curl

# 3. คัดลอกโค้ดจากเครื่อง Mac เข้าไปใน Container
COPY ./src /app

# 4. กำหนดคำสั่งเริ่มต้นให้ทำงานเมื่อ Container บูตขึ้นมา
CMD ["python3", "/app/main.py"]
```
- สั่งสร้าง Image ด้วยคำสั่ง:
  ```bash
  docker build -t my-app:v1.0 .
  ```

---

### 6.3 การรันระบบฐานข้อมูล (MySQL, PostgreSQL, Redis)
ในการทำงานจริง 99% เรา **ไม่ต้องสร้าง Image MySQL เองจากศูนย์** เพราะผู้พัฒนาได้สร้าง **Official Image** คุณภาพสูงไว้ให้เราดึงมาใช้ได้ทันที:

#### ตัวอย่าง 1: การรัน MySQL Server 8.0
```bash
docker run -d \
  --name my-mysql \
  -e MYSQL_ROOT_PASSWORD=secret123 \
  -e MYSQL_DATABASE=app_db \
  -p 3306:3306 \
  -v ~/my_database_data:/var/lib/mysql \
  mysql:8.0
```

#### ตัวอย่าง 2: การรัน PostgreSQL 16
```bash
docker run -d \
  --name my-postgres \
  -e POSTGRES_PASSWORD=secret123 \
  -e POSTGRES_DB=app_db \
  -p 5432:5432 \
  -v ~/postgres_data:/var/lib/postgresql/data \
  postgres:16-alpine
```

#### ตัวอย่าง 3: การรัน Redis In-Memory Cache
```bash
docker run -d --name my-redis -p 6379:6379 redis:alpine
```

---

### 6.4 กฎเหล็กของ Database: การเก็บรักษาข้อมูล (Data Persistence)
> [!IMPORTANT]
> **หากไม่ใส่ `-v` ข้อมูลใน Database จะสูญหายเมื่อ Container ถูกลบ!**

เมื่อใส่ `-v ~/my_database_data:/var/lib/mysql`:
1. **ข้อมูลไม่สูญหาย (Persistent Storage):** ไฟล์ฐานข้อมูลจริงจะถูกบันทึกลงบนฮาร์ดดิสก์ Mac ที่โฟลเดอร์ `~/my_database_data` ต่อให้ Container ถูกลบทิ้ง (`docker rm`) หรืออัปเกรดเวอร์ชัน เพียงแค่สร้าง Container ใหม่แล้วชี้มาที่โฟลเดอร์เดิม ข้อมูลจะกลับมา 100% ทันที
2. **ความเร็วระดับ Apple Silicon NVMe:** บน APFS การอ่านเขียนไฟล์ Database จะทะยานระดับ 3,000+ MB/s เร็วกว่า Docker Desktop ดั้งเดิมอย่างมาก เพราะไม่ต้องส่งข้อมูลข้าม Network VM เสมือน

---

## 🧪 7. การทดสอบความถูกต้องและเสถียรภาพ (Test Suites)

ในโปรเจกต์มีชุดทดสอบอัตโนมัติครอบคลุม 121 ข้อ เพื่อรับประกันว่าสามารถใช้งานแทน Docker ได้ 100% บิตต่อบิต:

```bash
# 1. รันชุดทดสอบ Socket และ Docker REST API Parity (116 ข้อ)
make test

# 2. รันชุดทดสอบ Multi-Language AST & Assembly Sandbox (5 ภาษา)
make test-languages
```
- **`test_nockersock.sh`**: ทดสอบโปรโตคอล REST และ UNIX Domain Socket (16/16 ผ่าน)
- **`test_parity.sh`**: ทดสอบความเหมือนกับ Docker CLI ตัวจริง 100 เคส (100/100 ผ่าน)
- **`test_languages.sh`**: ทดสอบการคอมไพล์ C, C++, Zig, Go, PHP สตรีม AST และ ARM64 Assembly (5/5 ผ่าน)

---

## 📦 8. การย้าย Image จาก Docker Desktop (Zero-Download Migration)

คุณสามารถย้าย Image ที่มีอยู่ใน Docker Desktop เข้ามายัง APFS Storage ของ Nocker ได้โดยไม่ต้องต่อเน็ต:
```bash
# ย้ายทีละตัว
nocker import-docker ubuntu:latest
nocker import-docker alpine:latest

# หรือย้ายทั้งหมดในเครื่องในครั้งเดียว
nocker import-docker --all
```

---

## ❓ 9. คำถามที่พบบ่อย (FAQs)

**Q1: ฉันต้องเปิดโปรแกรม Docker Desktop ควบคู่ไปด้วยไหม?**  
**A:** ไม่ต้องเปิดครับ ปิด Docker Desktop ทิ้งได้เลย `nocker` ทำหน้าที่เป็น Engine ตัวแม่แทนอย่างสมบูรณ์ และประหยัด RAM ไปได้ถึง 3-4 GB!

**Q2: โปรแกรมอื่นในเครื่อง เช่น VS Code หรือสคริปต์อัตโนมัติ จะใช้งานได้ไหม?**  
**A:** ใช้งานได้ทันทีครับ เพียงตั้งค่า `export DOCKER_HOST="unix://$HOME/.nocker/run/docker.sock"` ซึ่งเป็นช่องทางมาตรฐานที่เครื่องมือทุกตัวในโลกเชื่อมต่ออยู่แล้ว

**Q3: ถ้าเผลอลบ Container ที่รัน MySQL ทิ้ง ข้อมูลในฐานข้อมูลจะหายไหม?**  
**A:** ถ้าคุณรันโดยใส่ Volume Mount (`-v ~/my_data:/var/lib/mysql`) ข้อมูลจะ **ไม่หาย 100%** ครับ เพราะข้อมูลจริงถูกเซฟอยู่บนเครื่อง Mac ของคุณอย่างปลอดภัย

**Q4: ระหว่างเขียน Dockerfile สร้าง Image ใหม่ กับใช้ Volume Mount (`-v`) แบบไหนดีกว่ากัน?**  
**A:** ในช่วงกำลังพัฒนาโค้ด (Development) แนะนำให้ใช้ **Volume Mount (`-v`)** เพราะแก้โค้ดบน Mac ปุ๊บ ใน Container อัปเดตทันที ไม่ต้องเสียเวลา Build ใหม่ทุกครั้ง! แต่เมื่อพัฒนาเสร็จและต้องการส่งให้คนอื่นหรือนำขึ้น Production ค่อยเขียน **`Dockerfile`** เพื่อแพ็คเป็น Image ครับ

**Q5: โหมด `--native` แตกต่างจากโหมด Container ปกติอย่างไร?**  
**A:** โหมด `--native` จะไม่ใช้ Image เลยแม้แต่ตัวเดียว (พื้นที่ 0 MB) แต่จะนำคอมไพเลอร์ในเครื่อง Mac ของคุณเอง (เช่น Clang, Go, Zig) มาประมวลผลภายใต้การกักกันความปลอดภัยของ Apple Seatbelt รันเสร็จใน 15–38 ms เหมาะสำหรับการสร้างระบบ Code Runner หรือ Godbolt Compiler Explorer ส่วนตัวครับ!

**Q6: ถ้าเครื่อง Mac รีสตาร์ทใหม่ ต้องทำอย่างไร?**  
**A:** เพียงสั่งเปิด Daemon อีกครั้งด้วยคำสั่ง:
```bash
nocker daemon &
```
หรือตรวจสอบสถานะด้วย `nocker status` ได้ทันทีครับ!
