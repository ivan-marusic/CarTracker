# CarTracker

**CarTracker** is an end‑to‑end, privacy‑preserving GPS vehicle tracking system built from scratch using embedded hardware, a lightweight cloud proxy, Firebase Realtime Database, and a Android application.
The system consists of:

- [**Embedded firmware**](firmware/stm32f429_sim7600_rawtcp) (STM32F429 + SIM7600G‑H) — obtains GNSS location and sends compact JSON via raw TCP/HTTP.
- [**Fly.io Python Proxy (Flask)**](server/flyio_proxy) — receives POST /update from the embedded device and pushes the data to Firebase Realtime Database.
- [**Firebase Realtime Database**] — stores the most recent location and enables real‑time sync.
- [**CarTracker Android app**](android) (Kotlin + Google Maps SDK) — displays real‑time vehicle location from Firebase on Google Maps with geofencing behavior.

## System Architecture
<p align="center">
  <img src="images/system.drawio.png" width="250" />
</p>

## Getting Started

### 1. Clone the repository
```bash
git clone https://github.com/ivan-marusic/CarTracker.git
```
---

## Features

- **Live location** updates to Firebase Realtime Database
- **Lightweight modem stack**: SIM7600 AT‑sockets
    - NETOPEN, CIPOPEN, CIPSEND
    - Manual HTTP POST assembly with correct Content-Length
- **Minimal data usage** (~120 bytes per update)
- **Low‑latency proxy** hosted on Fly.io with auto‑sleep
- **Android client** with:
    - Google Maps
    - Geofencing (radius check)
- **Fully offline‑capable MCU** (no Linux, no PPP required)
- **Extensible** to historical tracks, alerts, and telemetry (speed, sats, battery)

## Hardware

- STM32F429 microcontroller board
- SIM7600G-H CAT4 4G (LTE) Shield
- SIM7600G‑H module requires antennas
    - MAIN (LTE antenna)
    - GNSS antenna
- Female to Female jumper wires (UART wiring between STM32 and SIM7600G-H)
- PmodUSBUART module for debugging
- USB-A to Mini USB cable (STM32F429)
- USB-A to Micro USB cable (SIM7600G-H and PmodUSBUART)
- Machine-to-Machine (M2M) SIM card
    -> Tested with **Onomondo** network operator

<p align="center">
  <img src="images/STM32F429+SIM7600G-H.jpg" width="400" />
  <img src="images/STM32F429+SIM7600G-H+PmodUSBUART.jpg" width="400" />
</p>

## Software & Tools

- **STM32CubeIDE** – firmware development and flashing
- **PuTTY/Minicom** – UART debugging and AT command testing
- **Android Studio** – building and running the Android app
- **Python 3 + Flask + Gunicorn** — Fly.io proxy
- **Firebase Console** – monitoring Realtime Database
- draw.io — system diagrams

## Embedded Firmware (STM32F429 + SIM7600G‑H)
The firmware communicates with the SIM7600G‑H modem using UART2 and performs AT commands:

- Powering and initializing the SIM7600 modem
- Enabling GNSS (`AT+CGPS=1`)
- GNSS location retrieval (`AT+CGPSINFO`)
- Configuring APN (`AT+CGDCONT=1,"IP","<APN>"`)
- Modem IP stack activation (`AT+NETOPEN`)
- Creating TCP sockets (`AT+CIPOPEN=0,"TCP","cartracker-proxy.fly.dev",80`)
- JSON telemetry upload (`AT+CIPSEND=0,<len>
<HTTP headers>
<JSON body>`)

Telemetry JSON Format
```
{  
  "latitude": 45.110000,
  "longitude": 17.400000,
  "timestamp": "2025-08-09T16:30:00Z"
}
```
This structure matches the Firebase Realtime Database schema used by the Android app.
Full details, including setup and code structure, are located in: [firmware/stm32f429_sim7600_rawtcp](firmware/stm32f429_sim7600_rawtcp/)

## CarTracker Android app (Kotlin)
The Android app uses Firebase Realtime Database and Google Maps SDK to:
- Connects to Firebase Realtime Database
- Observes the /location node for live updates
- Displays the vehicle position on a map
- Shows whether the vehicle is inside/outside a configured radius
- Includes UI screens shown in [/images/](images)
<p align="center">
  <img src="images/start_page.jpg" width="200" />
  <img src="images/in_radius.jpg" width="200" /> 
  <img src="images/out_of_radius.jpg" width="200" />
</p>

To run:

1. Open in Android Studio
2. Add Firebase configuration
3. Build and deploy. [See here](android)

## Testing the System

1. Start your Fly.io proxy (auto-starts on request)
2. Power on the embedded device (STM32F429 + SIM7600G-H)
3. Watch debug UART:
    NETOPEN OK
    CIPOPEN OK
    SEND OK

4. Check Firebase Console -> Realtime Database -> /location

5. Open the Android app:
   -> see real‑time map update

## License
This project is MIT licensed unless stated otherwise in sub‑folders.
