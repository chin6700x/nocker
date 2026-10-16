# ⚡ AGENTS.MD: NOCKER MANIFESTO & ENGINEERING STANDARD
> **"Nocker: Not Docker · Bare-Metal Silicon Velocity · 100% OCI & REST Parity"**  
> **Author & Architect:** Wirot Chookeaw (`chin6700x`) | **License:** Apache License 2.0 &copy; 2026  
> **Workspace Root:** `/Volumes/Mac Backup/zero-tools/nocker/`  
> **Binary:** `~/.local/bin/nocker`

---

## 🧭 1. วิสัยทัศน์และเป้าหมายสูงสุด (The Vision)

**`nocker` (Not Docker)** ถูกออกแบบขึ้นเพื่อเป็น **Next-Generation Bare-Metal Container Engine** สำหรับสถาปัตยกรรม Apple Silicon Darwin:
1. **ปลุกพลัง Apple Silicon ที่แท้จริง:** ใช้ Apple `Virtualization.framework` บูต Linux MicroVM ในเวลาต่ำกว่า 250ms, ใช้ APFS `clonefile()` สร้าง Layer Snapshot ในเสี้ยววินาที (Zero-Copy), และใช้ ARM64 NEON Vector Memory Streaming
2. **100% Seamless Drop-in Parity (Without Subservience):** ให้บริการผ่าน UNIX Domain Socket `~/.nocker/run/nocker.sock` (และเชื่อมต่อกับ `/var/run/docker.sock` ในโหมด `--docker-compat`) ตอบสนอง Docker Engine REST API (v1.45) อย่างไร้รอยต่อ
3. **Twin-Engine Architecture:** สลับใช้ Bare-metal Darwin Seatbelt สำหรับงานด่วน (< 2ms) และ Apple Virtualization MicroVM สำหรับ Standard Linux OCI Images
4. **ความประหยัดเหนือชั้น:** ใช้ทรัพยากรขณะ Idle ต่ำกว่า 10 MB (ประหยัดแรมกว่า Docker Desktop ดั้งเดิมกว่า 99.7%)

---

## 🏛️ 2. กฎเหล็กของโครงการ (Core Engineering Rules)

### 📌 กฎเหล็กข้อที่ 1: Apache License 2.0 Header
ทุกไฟล์โค้ดใหม่ (`.c`, `.h`, `.m`, `.sh`) ต้องมี Header ระบุลิขสิทธิ์อย่างเคร่งครัด:
```c
/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */
```

### 📌 กฎเหล็กข้อที่ 2: Protocol Parity & Zero Learning Curve
* ตอบสนอง Docker Engine REST API มาตรฐาน (JSON schema ตรงตาม Docker v1.45)
* นักพัฒนาหรือเครื่องมือภายนอก (VS Code, Docker Compose, CI) สามารถชี้มาที่ socket ของ `nocker` แล้วใช้งานได้ทันทีโดยไม่ต้องแก้สคริปต์เดิม

### 📌 กฎเหล็กข้อที่ 3: Exhaustive Differential Testing
* มีชุดทดสอบ Shell Script (`.sh`) ครอบคลุมการทำงานทุกฟังก์ชัน
* เกณฑ์การผ่านคือ **100.0% PASS** ทั้ง Exit code และ Output Format

---

## 📁 3. แผนผังไดเรกทอรี (Directory Structure)

```
nocker/
├── Makefile             # Unified Makefile สำหรับคอมไพล์ bin/nocker
├── README.md            # คู่มือภาพรวมและ Benchmark เทียบกับ Docker Desktop
├── LICENSE              # สัญญาอนุญาต Apache License 2.0
├── AGENTS.md            # พิมพ์เขียวกฎเหล็กและมาตรฐานวิศวกรรม
├── entitlements.plist   # Apple Virtualization Entitlement
├── bin/                 # ไบนารี nocker
├── include/             # C Headers (nocker.h, nocker_oci.h, ...)
├── src/                 # ซอร์สโค้ดภาษา C11 และ Objective-C
│   ├── socket/          # UNIX Socket & Docker REST API v1.45 Router
│   ├── storage/         # APFS Copy-on-Write & OCI Registry Client
│   ├── engine/          # Twin-Engine Dispatcher & Container Lifecycle
│   ├── vz/              # Apple Virtualization.framework MicroVM
│   ├── ui/              # Cyberpunk Glassmorphic Web Studio Server
│   └── utils/           # HTTP Parser & Utilities
├── demo/                # Full-Stack Web + PHP + SQLite Live Demonstration
├── test/                # Parity Test Suites (100% Bit-Exact Match)
└── plan/                # Master Roadmaps & Migration Plans
```
