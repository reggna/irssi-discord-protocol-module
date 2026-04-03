# Project Viability Analysis: irssi-discord

This document evaluates the current state of the `irssi-discord` project (last updated ~9 years ago) and determines whether it is a viable foundation for modern Irssi-Discord integration or if a complete rewrite is required.

## 1. Current State Overview
The project is a C-based protocol module for Irssi. It successfully implements the basic boilerplate for an Irssi protocol (`CHAT_PROTOCOL_REC`), including network, server, and channel management.

### Key Components:
- **Authentication:** Uses Hardcoded "Bot" style tokens (though the prefix "Bot " might be missing in some headers).
- **Communication:** Uses `libcurl` for synchronous HTTP REST requests.
- **JSON Parsing:** Uses `jansson`.
- **Message Fetching:** Pulls history on join/query start but **lacks real-time updates**.

## 2. Critical Issues & Technical Debt

### A. Discord API Obsolescence (High Severity)
- **Version Mismatch:** The code is hardcoded to use **Discord API v6** (`#define DISCORD_VERSION "6"` in `src/core.h`). As of 2026, v6 is deprecated/decommissioned. Modern Discord requires **v10**.
- **Header Format:** Discord v10 requires the `Authorization: Bot <token>` format. The current code uses `Authorization: <token>` (see `src/discord.c:16`), which will fail authentication.
- **Gateway Missing:** The current implementation has **no WebSocket/Gateway support**. Modern chat applications require real-time updates. Relying solely on REST for message history is not a functional chat experience.

### B. Irssi API Incompatibility (Medium Severity)
- **Connection Logic:** Irssi moved away from simple `net_connect` style logic to a more complex connection manager handling TLS and asynchronous DNS. The manual port/address handling in `src/servers.c` may conflict with modern Irssi core expectations.
- **Signal Signatures:** Several core signals (like `message join`) have added parameters over the last decade. The current signal handlers in `src/signals.c` and `src/channels.c` likely have incorrect signatures, which could lead to memory corruption or crashes in modern Irssi.
- **Include Paths:** Irssi's move to the Meson build system changed how headers are structured. The current `Makefile` and include statements (e.g., `#include <irssi/src/common.h>`) are brittle and likely need adjustment for standard dev environments.

### C. Architectural Limitations
- **Synchronous I/O:** Using `curl_easy_perform` inside Irssi's main thread (as seen in `src/discord.c`) will **block the entire UI** every time a message is sent or history is fetched. In a modern implementation, this must be asynchronous (using `curl_multi` or GLib's async tools).

## 3. Rewrite vs. Refactor Comparison

| Feature | Refactor Existing | Rewrite from Scratch |
| :--- | :--- | :--- |
| **Effort** | Medium (Fixing headers, versions, signals) | High (Starting from boilerplate) |
| **Boilerplate** | Already done (Registers protocol, channels) | Must be re-implemented |
| **Real-time Support** | Must be added (major task) | Must be added |
| **Async I/O** | Must be overhauled | Built-in from start |
| **Maintenance** | Inherits 9-year-old "TODOs" | Clean, modern codebase |

## 4. Final Verdict: **Incremental Evolution (Recommended)**

While the project is currently broken due to API versioning and lack of real-time updates, it provides a valuable **"Skeleton"** for an Irssi protocol module. Writing a new Irssi protocol from scratch is notoriously difficult due to the complexity of Irssi's internal structures (`SERVER_REC`, `CHANNEL_REC`, etc.).

### Recommended Action Plan:
1.  **Upgrade API Version:** Change `DISCORD_VERSION` to `"10"` and fix the `Authorization` header.
2.  **Fix Irssi Compatibility:** Update signal handlers and build system to match Irssi 1.4+.
3.  **Implement Asynchronous I/O:** Move `libcurl` calls to an asynchronous model to prevent UI lag.
4.  **Implement Gateway (WebSockets):** This is the largest task. Use a library like `libwebsockets` to implement a persistent connection for real-time events.

**Conclusion:** The project is **not usable as-is**, but it is a **better starting point than a blank file**. It contains the necessary "glue" code to talk to Irssi, which is the hardest part of the task.
