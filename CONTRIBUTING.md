# ⚡ Contributing to Nocker

Thank you for your interest in contributing to **Nocker (Not Docker)**!  
Nocker is an open-source, bare-metal Apple Silicon container runtime engineered for maximum performance, minimal memory footprint, and 100% Docker REST API interoperability.

---

## 🏛️ Guiding Philosophy & Standards

1. **Bare-Metal Silicon First:** All code must prioritize Apple Silicon hardware capabilities (ARM64 NEON, APFS CoW `clonefile`, Grand Central Dispatch, Apple Virtualization.framework).
2. **True Drop-in Compatibility:** Endpoints and CLI behavior must adhere to the Docker Engine REST API v1.45 standard.
3. **Zero Bloat:** Keep binaries lightweight, fast, and dependency-free. Core engine code is written in **C11** and **Objective-C**.

---

## 📜 Copyright & Licensing Header

All source files submitted to Nocker must be licensed under the **Apache License 2.0** and include the mandatory author attribution header:

```c
/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */
```

---

## 🛠️ Development Workflow

### 1. Prerequisites
- macOS 14 (Sonoma) or macOS 15 (Sequoia) running on Apple Silicon (M1 / M2 / M3 / M4)
- Xcode Command Line Tools (`clang`, `make`, `codesign`)

### 2. Building Nocker
```bash
make clean
make all
```

### 3. Running the Test Suite
All PRs must achieve a **100.0% pass rate** on the differential test suite:
```bash
# Run socket and API parity tests
make test

# Run multi-language compiler tests
make test-languages
```

---

## 🤝 Submitting Pull Requests

1. Fork the repository and create a new feature branch (`git checkout -b feature/my-feature`).
2. Implement your changes adhering to C11 standards and formatting conventions.
3. Verify that all tests pass (`make test`).
4. Commit your changes with clear, descriptive commit messages.
5. Push to your branch and submit a Pull Request to `main`.
