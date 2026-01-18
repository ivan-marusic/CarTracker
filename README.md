# CarTracker

**CarTracker** is an end‑to‑end, privacy‑preserving vehicle tracking system composed of:

- **CarTracker Android app** (Kotlin) — displays real‑time vehicle location from Firebase on Google Maps.
- **Embedded firmware** (STM32F429 + SIM7600G‑H) — acquires GNSS position and streams compact JSON over raw TCP.
- **Python TCP relay (Oracle VM)** — receives JSON over TCP and updates Firebase Realtime Database via HTTPS.

> The repository originally contains the Android project structure (e.g., `app/`, Gradle files). Firmware and server live in their own folders under `firmware/` and `server/`. [1](https://github.com/ivan-marusic/CarTracker)

---

##  Features

- **Live location** updates to Firebase Realtime Database
- **Lightweight modem stack**: SIM7600 AT‑sockets (`NETOPEN`/`CIPOPEN`/`CIPSEND`) — **no PPP/LwIP** on the MCU
- **Minimal bandwidth JSON**: `{"latitude":..., "longitude":..., "timestamp":"..."}` (newline‑delimited)
- **Clean separation of concerns**: firmware ↔ relay ↔ mobile app
- **Extensible** to historical tracks, alerts, and telemetry (speed, sats, battery)

---

##  System Architecture
