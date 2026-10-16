# ⚡ Full-Stack Web + PHP + Database Live Demo (Nocker: Not Docker)
> **"Seamless Host-to-Container Interconnect · SQLite / PHP 8 · APFS Instant Volume Sync"**  
> **Author & Architect:** Wirot Chookeaw (`chin6700x`) | **License:** Apache License 2.0 &copy; 2026  
> 🌐 **Language:** [English](README.md) | [🇹🇭 ภาษาไทย](../th-docs/demo/README.md)

---

## 🎯 About This Demo

This demo simulates a production-grade full-stack container workload, demonstrating:
1. **Containerized Web Application:** Running PHP 8 + built-in web server inside a containerized sandbox.
2. **Persistent Embedded Database:** Real-time persistence with SQLite (`silicon_store.db`).
3. **Host-to-Container Interconnect:** Binding port `8080` for instant, zero-friction access from macOS.
4. **Data Persistence via APFS Volume Mount:** Host filesystem updates propagate to container storage instantly with Zero-Copy APFS overhead.
5. **Interactive UI Walkthrough:** An embedded web dashboard explaining the **Step 1 - 2 - 3 - 4** architecture in real time!

---

## 🚀 Quickstart (1-Click Run)

```bash
cd demo

# Launch demo instantly
bash run_demo.sh
```
The script will automatically launch the demo and open Safari or Chrome to **`http://localhost:8080`**!

---

## 🛠️ Step-by-Step Developer CLI Guide

```bash
# Step 1: Verify Daemon Status
nocker status

# Step 2: Run container with Docker CLI compatibility
docker run -d \
  --name silicon-web-demo \
  -p 8080:8080 \
  -v "$(pwd)/www:/app" \
  -v "$(pwd)/data:/data" \
  library/alpine:latest \
  sh -c "php -S 0.0.0.0:8080 -t /app"

# Step 3: Inspect active container and live logs
docker ps
docker logs silicon-web-demo

# Step 4: Open application in web browser
open http://localhost:8080
```

---

## ⏹️ Teardown & Cleanup

```bash
bash stop_demo.sh
```
