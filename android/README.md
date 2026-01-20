# CarTracker

CarTracker is an Android application that displays the real-time location of a vehicle using data from Firebase.  
The vehicle's location is sent by a custom device built with an STM32F429 microcontroller and a SIM7600G-H 4G shield.
   	      ![App_Image](app/src/main/res/drawable/cartracker_background.jpeg)

## Features

- Real-time location tracking
- Alerts when vehicle moves 30 km within a defined radius
- Location updates every 10 seconds

## Screenshots
#### Start page
![Start page](../../images/start_page.jpg)

#### Sign-up page
![Sign-Up](images/sign-up.jpg)

#### Car in 30km radius
![In Radius](images/in_radius.jpg)

#### Car out of radius
![Out of Radius](images/out_of_radius.jpg)

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
