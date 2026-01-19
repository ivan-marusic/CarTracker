# CarTracker

**CarTracker** is an end‑to‑end, privacy‑preserving vehicle tracking system composed of:

- [**CarTracker Android app**](android) (Kotlin) — displays real‑time vehicle location from Firebase on Google Maps.
- [**Embedded firmware**](firmware/stm32f429_sim7600_rawtcp) (STM32F429 + SIM7600G‑H) — acquires GNSS position and streams compact JSON over raw TCP.
- [**Python TCP relay (Oracle VM)**](server/python) — receives JSON over TCP and updates Firebase Realtime Database via HTTPS.

> The repository originally contains the Android project structure (e.g., [`app/`](android/app), Gradle files). Firmware and server live in their own folders under [`firmware/`](firmware/stm32f429_sim7600_rawtcp) and [`server/`](server).

---

##  Features

- **Live location** updates to Firebase Realtime Database
- **Lightweight modem stack**: SIM7600 AT‑sockets (`NETOPEN`/`CIPOPEN`/`CIPSEND`) — **no PPP/LwIP** on the MCU
- **Minimal bandwidth JSON**: `{"latitude":..., "longitude":..., "timestamp":"..."}` (newline‑delimited)
- **Clean separation of concerns**: firmware ↔ relay ↔ mobile app
- **Extensible** to historical tracks, alerts, and telemetry (speed, sats, battery)

---

## Firmware (STM32F429 + SIM7600G‑H)
The firmware communicates with the SIM7600G‑H modem via UART and performs:

GNSS activation (AT+CGNSPWR=1)
Location retrieval (AT+CGNSINF)
APN setup (AT+CGDCONT=1,"IP","<APN>")
Modem IP stack activation (AT+NETOPEN)
TCP socket creation (AT+CIPOPEN=0,"TCP","<HOST>",<PORT>)
JSON telemetry upload using AT+CIPSEND

Telemetry JSON Format
{  "latitude": 44.110000,  
"longitude": 15.400000,  
"timestamp": "2025-08-09T16:30:00Z"
}
This structure matches the Firebase Realtime Database schema used by the Android app.
Full details, including setup and code structure, are located in:
[firmware] (firmware/stm32f429_sim7600_rawtcp/README.md)

## Python TCP Relay (Oracle VM)
The relay server acts as a secure gateway between the embedded device and Firebase:

Accepts incoming raw TCP connections from devices
Parses newline‑delimited JSON
Issues HTTPS PUT requests to Firebase
Writes to the database path:

/location

Steps for running the server, environment variables, and firewall setup are documented in:
[server] (server/python)

##  Android App
The Android app (Kotlin + Google Maps SDK):

Connects to Firebase Realtime Database
Observes the /location node for live updates
Displays the vehicle position on a map
Shows whether the vehicle is inside/outside a configured radius
Includes UI screens shown in /images/

Open the project in Android Studio, configure your Firebase credentials, and build.

## Testing the System

1. Start the Python relay on the VM (ensure port is open)

2. Power on the embedded device

3. Watch serial logs for:
    NETOPEN OK
    CIPOPEN OK
    SEND OK

4. Check Firebase Console:
Realtime Database → /location

5. Open the Android app:
Map updates live with the transmitted coordinates

## License
This project is MIT licensed unless stated otherwise in sub‑folders.
