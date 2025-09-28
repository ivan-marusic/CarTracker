# CarTracker

CarTracker is an Android application that displays the real-time location of a vehicle using data from Firebase.  
The vehicle's location is sent by a custom device built with an STM32F429 microcontroller and a SIM7600G-H 4G shield.

## Features

- Real-time location tracking
- Display of vehicle path over the last 24 hours
- Alerts when vehicle moves 30 km within a defined radius
- Location updates every 10 seconds

## Technologies Used

- Kotlin (Android)
- Firebase Realtime Database
- Google Maps SDK
- STM32F429 + SIM7600G-H (CarTracker device)

## Getting Started

1. Clone the repo:
   ```bash
   git clone https://github.com/ivan-marusic/CarTracker.git
   ```
2. Open the project in Android Studio
3. Configure Firebase in the `google-services.json`
4. Run the app on your device or emulator

## License

This project is licensed under the MIT License.
