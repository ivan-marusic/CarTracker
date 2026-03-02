# CarTracker App

CarTracker App is an Android application that displays the real-time location of a vehicle using data from Firebase.  
The vehicle's location is sent by a custom device built with an STM32F429 microcontroller and a SIM7600G-H CAT4 4G (LTE) Shield.

<p align="center">
    <img src="app/src/main/res/drawable/cartracker_background.jpeg" width="600" />
</p>

## Features

- Real-time location tracking
- Alerts when vehicle moves 30 km within a defined radius
- Location updates every 10 seconds

<div align="center">

<table>
  <tr>
    <td align="center"><strong>Start page</strong><br><img src="../images/start_page.jpg" width="250"></td>
    <td align="center"><strong>Sign-up page</strong><br><img src="../images/sign-up.jpg" width="250"></td>
  </tr>
  <tr>
    <td align="center"><strong>Car in 30km radius</strong><br><img src="../images/in_radius.jpg" width="250"></td>
    <td align="center"><strong>Car out of radius</strong><br><img src="../images/out_of_radius.jpg" width="250"></td>
  </tr>
</table>

</div>

## Technologies Used

- Kotlin (Android)
- Firebase Realtime Database
- Google Maps SDK
- STM32F429 + SIM7600G-H CAT4 4G (LTE) Shield (CarTracker device)

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
