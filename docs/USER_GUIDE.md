# ⚡ NOCKER: Official Comprehensive User Guide
> **"Bare-Metal Silicon Velocity · APFS Copy-on-Write · 100% Docker CLI Parity"**  
> **Author & Architect:** Wirot Chookeaw (`chin6700x`) | **License:** Apache License 2.0 &copy; 2026  
> 🌐 **Language:** [English](USER_GUIDE.md) | [🇹🇭 ภาษาไทย](../th-docs/docs/USER_GUIDE.md)

---

## 🧭 1. Architecture Overview

Before diving into commands, it is crucial to understand how **`nocker`** operates and how it differs from traditional container engines:

### 💡 The Reality of Docker
The standard Docker architecture is fundamentally split into two decoupled components:
1. **Docker CLI (Client):** The command-line interface (e.g., `/usr/local/bin/docker`). When you type `docker run` or `docker ps`, it does not execute containers itself; it simply sends HTTP REST requests over a UNIX Domain Socket (`~/.nocker/run/docker.sock`) to the daemon.
2. **Docker Engine / Daemon (Server):** The background service that listens to the socket, pulls images, unpacks filesystems, and manages CPU/memory isolation. Historically on macOS, this was **Docker Desktop**, which runs a heavy, battery-draining Linux Virtual Machine consuming 3–4 GB of RAM.

### ⚡ How `nocker` Changes Everything
We have rewritten the **Docker Engine Daemon** from scratch in native C11 and Objective-C specifically for Apple Silicon hardware:
- **No Docker Desktop Required:** Close and uninstall Docker Desktop entirely.
- **Bare-Metal Speed:** Uses Darwin's native APFS `clonefile()` system call to clone container root filesystems in **< 60 milliseconds** with **0 initial disk space overhead** (Zero-Copy).
- **Twin-Engine Architecture:**
  - **Engine 1 (Darwin Seatbelt Native):** Executes commands at raw silicon speeds (**< 2ms**) under hardware-isolated sandbox profiles.
  - **Engine 2 (Apple `Virtualization.framework` MicroVM):** High-speed Linux MicroVM bridge with VirtioFS and zero-copy page streaming.

---

## 🚀 2. Quickstart

### Verify System Health & Readiness
```bash
# Check daemon operational status
nocker status

# Run comprehensive Apple Silicon hardware diagnostics
nocker doctor

# Display system CPU, memory, and APFS storage driver metrics
nocker info
```

### Launch the Daemon in Background
If the daemon is not currently running, launch it with:
```bash
nocker daemon &
```
Once started, Nocker creates listening UNIX domain sockets at:
- `~/.nocker/run/nocker.sock` (with a companion `docker.sock` in the same directory)
- Globally accessible via `/var/run/docker.sock` when specified with `--global` (requires sudo) for universal external tool integration.

---

## 💻 3. Interaction Channels

You have complete flexibility to interact with Nocker through three distinct channels:

### Channel 1: The Native `nocker` CLI
Ideal for ultra-fast workflows and Apple Silicon-specific optimizations:
```bash
nocker run alpine:latest "echo 'Hello from Nocker'"
```

### Channel 2: Official `docker` CLI
External developer tools (VS Code Dev Containers, Cursor, CI/CD scripts) can use standard `docker` commands without modification:
```bash
docker run --rm library/alpine:latest uname -a
```

### Channel 3: REST API via UNIX Domain Socket
Custom automation scripts (Python, Go, Node.js) and desktop applications can interact directly over the socket using standard Docker Engine API v1.45:
```bash
curl --unix-socket ~/.nocker/run/docker.sock http://localhost/v1.45/containers/json
```
* **100% Docker Engine REST API v1.45 Compatibility**
* **< 4 MB RAM idle footprint**
* **Zero network latency: pure kernel IPC**

> 💡 **Need a Graphical Interface?**  
> We are developing **Nocker Studio Pro** as a standalone native desktop app (Rust + Tauri) that plugs directly into this socket!

---

## 🛠️ 4. Command Reference

### 1) System Diagnostics (`info` / `doctor`)
```bash
nocker info
# or
docker info
```
*Displays storage driver (`apfs-cow`), snapshotter (`clonefile`), macOS Darwin ARM64 architecture, and live CPU/RAM metrics.*

---

### 2) Running Containers (`run`)
```bash
# Standard one-off execution
nocker run alpine:latest "echo 'Bare-metal silicon speed'"

# Execution with a protective Watchdog timeout (in milliseconds)
nocker run --timeout 2000 alpine:latest "sleep 1"

# Force execution in Darwin Seatbelt Native mode (< 2ms)
nocker run --native host "echo 'Fastest execution on Apple Silicon'"

# Force execution via Apple Virtualization Linux MicroVM
nocker run --vz alpine:latest "uname -a"
```

---

### 3) Listing Containers (`ps`)
```bash
# List active containers
nocker ps

# List all containers (running and exited)
nocker ps -a
# or
docker ps -a
```

---

### 4) Viewing Logs (`logs`)
```bash
nocker logs <CONTAINER_NAME_OR_ID>
# or
docker logs <CONTAINER_NAME_OR_ID>
```

---

### 5) Detailed Inspection (`inspect`)
```bash
nocker inspect <CONTAINER_NAME_OR_ID>
# or
docker inspect <CONTAINER_NAME_OR_ID>
```

---

### 6) Container Cleanup (`rm`)
```bash
nocker rm <CONTAINER_NAME_OR_ID>
# or
docker rm <CONTAINER_NAME_OR_ID>
```

---

### 7) Image Management (`images` / `pull`)
```bash
# List locally cached images
nocker images

# Pull images from Docker Hub / OCI Registry
nocker pull alpine:latest
```

---

## 📂 5. Sharing Files & Programs with Containers

### Method 1: APFS Volume Mount (`-v /host:/container`) — *Recommended*
Mount directories directly from your Mac into the container without file copying:

```bash
# Example: Run a Python script located on your Mac inside an Alpine container
docker run --rm -v /tmp/my_app:/app library/alpine:latest python3 ./app/main.py

# Or via nocker CLI
nocker run -v /tmp/my_app:/app alpine:latest "sh ./app/run.sh"
```

---

### Method 2: Container File Copy (`docker cp`)

#### Copy files from macOS into a Container:
```bash
# 1. Create container
docker create --name app_box library/alpine:latest ./app/server

# 2. Copy binary into container's /app directory
docker cp /path/to/my_program app_box:/app/server

# 3. Start container
docker start app_box
```

#### Copy files from a Container to macOS:
```bash
docker cp app_box:/app/output.log /tmp/output.log
```

---

### Method 3: Direct APFS RootFS Inspection via macOS Finder
Because each container scratchpad lives on native APFS, you can inspect and modify container files directly in macOS Finder or VS Code at:
```bash
~/.nocker/storage/containers/<CONTAINER_ID>/rootfs/
```

---

## 📦 6. Custom Images, Databases, and Data Persistence

### 6.1 `Dockerfile` vs `docker-compose`
- **`Dockerfile`:** Recipe for building a single container image from base OS instructions, dependencies, and application code.
- **`docker-compose.yml`:** Multi-container orchestrator that spins up interdependent services (e.g., Web API + PostgreSQL + Redis) on a unified network bridge.

### 6.2 Writing a `Dockerfile`
Create a file named `Dockerfile` in your project root:
```dockerfile
# 1. Base runtime image
FROM library/alpine:latest

# 2. Install dependencies
RUN apk add --no-cache python3 py3-pip curl

# 3. Copy application code
COPY ./src /app

# 4. Entrypoint command
CMD ["python3", "/app/main.py"]
```
Build your image:
```bash
docker build -t my-app:v1.0 .
```

---

### 6.3 Running Production Databases (MySQL, PostgreSQL, Redis)
You do not need to build database images from scratch. Official OCI images work directly:

#### Example 1: MySQL Server 8.0
```bash
docker run -d \
  --name my-mysql \
  -e MYSQL_ROOT_PASSWORD=secret123 \
  -e MYSQL_DATABASE=app_db \
  -p 3306:3306 \
  -v ~/my_database_data:/var/lib/mysql \
  mysql:8.0
```

#### Example 2: PostgreSQL 16
```bash
docker run -d \
  --name my-postgres \
  -e POSTGRES_PASSWORD=secret123 \
  -e POSTGRES_DB=app_db \
  -p 5432:5432 \
  -v ~/postgres_data:/var/lib/postgresql/data \
  postgres:16-alpine
```

#### Example 3: Redis In-Memory Cache
```bash
docker run -d --name my-redis -p 6379:6379 redis:alpine
```

---

### 6.4 The Golden Rule: Persistent Storage (`-v`)
> [!IMPORTANT]
> **Always use volume mounts (`-v`) for databases to prevent data loss upon container deletion!**

When using `-v ~/my_database_data:/var/lib/mysql`:
1. **Zero Data Loss:** Database records are persisted directly to your Mac's APFS volume at `~/my_database_data`. Even if the container is removed (`docker rm`), spinning up a new container targeting the same folder recovers all records instantly.
2. **Apple Silicon NVMe Velocity:** Direct APFS storage achieves disk read/write throughput exceeding **3,000+ MB/s**, eliminating the heavy VM virtual disk bottleneck.

---

## 🧪 7. Test Suites & Quality Verification

Nocker includes 121 automated differential tests verifying 100% bit-for-bit compatibility against the official Docker engine:

```bash
# 1. Run socket & Docker REST API parity suite (116 tests)
make test

# 2. Run multi-language compiler & assembly sandbox suite (5 languages)
make test-languages
```
- **`test_nockersock.sh`**: REST protocol and UNIX domain socket verification (16/16 PASS)
- **`test_parity.sh`**: Exhaustive differential parity against Docker CLI (100/100 PASS)
- **`test_languages.sh`**: Multi-language compilation (C, C++, Zig, Go, PHP) with AST and ARM64 assembly inspection (5/5 PASS)

---

## 📦 8. Zero-Download Migration from Docker Desktop

Migrate existing images from Docker Desktop into APFS storage without internet bandwidth:
```bash
# Migrate specific images
nocker import-docker ubuntu:latest
nocker import-docker alpine:latest

# Migrate all local images in one command
nocker import-docker --all
```

---

## ❓ 9. Frequently Asked Questions (FAQs)

**Q1: Do I need Docker Desktop running in the background?**  
**A:** No. You can completely close or uninstall Docker Desktop. `nocker daemon` functions as the primary engine daemon while saving 3–4 GB of memory.

**Q2: Will VS Code Dev Containers and automation scripts work?**  
**A:** Yes. Simply ensure `export DOCKER_HOST="unix://$HOME/.nocker/run/docker.sock"` is set. External developer tools communicate with Nocker seamlessly.

**Q3: Will my database data be lost if a container is deleted?**  
**A:** If you use a volume mount (`-v ~/my_data:/var/lib/mysql`), your data is **100% safe** on your Mac's filesystem.

**Q4: Should I use a Dockerfile or Volume Mount during development?**  
**A:** During active development, use **Volume Mount (`-v`)** for instant code reloading without rebuilding images. Package your app into a **`Dockerfile`** once code is stable and ready for production deployment.

**Q5: What is the difference between `--native` mode and standard container mode?**  
**A:** `--native` mode bypasses container images entirely (0 MB disk space). It executes host binaries (Clang, Go, Zig, PHP) inside an Apple Seatbelt hardware-isolated sandbox in 15–38 ms. Ideal for fast test runners, build pipelines, and compiler explorers.

**Q6: What should I do after restarting my Mac?**  
**A:** Simply relaunch the daemon:
```bash
nocker daemon &
```
And verify connectivity with `nocker status`.
