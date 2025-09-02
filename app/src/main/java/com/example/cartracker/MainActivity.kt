package com.example.cartracker

import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.widget.Toast
import android.widget.ImageButton
import android.location.Location
import android.widget.ImageView
import android.widget.TextView
import android.media.AudioManager
import android.media.ToneGenerator

import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity

import com.google.android.gms.maps.CameraUpdateFactory
import com.google.android.gms.maps.GoogleMap
import com.google.android.gms.maps.OnMapReadyCallback
import com.google.android.gms.maps.SupportMapFragment
import com.google.android.gms.maps.model.LatLng
import com.google.android.gms.maps.model.MarkerOptions
import com.google.android.gms.maps.model.*

import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.DatabaseReference
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener
import com.google.firebase.auth.FirebaseAuth
import java.util.Locale

class MainActivity : AppCompatActivity(), OnMapReadyCallback {

    private lateinit var mMap: GoogleMap
    private lateinit var database: DatabaseReference

    private var lowVoltageAlertShown = false

    private lateinit var batteryIcon: ImageView
    private lateinit var batteryPercentage: TextView

    private var geofenceCircle: Circle? = null

    private lateinit var geofenceStatusText: TextView


    // Geofencing
    private val referenceLocation = LatLng(44.1058067, 15.3793184) // Općina Zemunik Donji
    private val radiusKm = 30.0
    private var wasInsideRadius: Boolean? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Provjera prijave
        val auth = FirebaseAuth.getInstance()
        if (auth.currentUser == null) {
            startActivity(Intent(this, LoginActivity::class.java))
            finish()
            return
        }

        setContentView(R.layout.activity_main)

        batteryIcon = findViewById(R.id.batteryIcon)
        batteryPercentage = findViewById(R.id.batteryPercentage)


        val signOutButton = findViewById<ImageButton>(R.id.signOutButton)
        signOutButton.setOnClickListener {
            FirebaseAuth.getInstance().signOut()
            Toast.makeText(this, "Uspješno ste se odjavili", Toast.LENGTH_SHORT).show()
            startActivity(Intent(this, LoginActivity::class.java))
            finish()
        }

        geofenceStatusText = findViewById(R.id.geofenceStatusText)

        val mapFragment = supportFragmentManager
            .findFragmentById(R.id.map) as SupportMapFragment
        mapFragment.getMapAsync(this)

        database = FirebaseDatabase.getInstance().getReference("location")

    }

    override fun onMapReady(p0: GoogleMap) {
        mMap = p0

        // Prikaz geofencing kruga
        geofenceCircle = mMap.addCircle(
            CircleOptions()
                .center(referenceLocation)
                .radius(radiusKm * 1000) // u metrima
                .strokeColor(Color.BLUE)
                .fillColor(Color.argb(100, 255, 0, 0)) // poluprozirna crvena ispuna
                .strokeWidth(2f)
        )

        database.addValueEventListener(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                val latitude = snapshot.child("latitude").getValue(Double::class.java)
                val longitude = snapshot.child("longitude").getValue(Double::class.java)

                val voltage = snapshot.child("voltage").getValue(Double::class.java)


                if (voltage != null) {
                    // Pretvori napon u postotak (npr. 3.0V = 0%, 4.2V = 100%)
                    val percentage = voltageToPercentage(voltage)
                    batteryPercentage.text = getString(R.string.battery_percentage, percentage)

                    val iconRes = when {
                        percentage >= 90 -> R.drawable.battery_full
                        percentage >= 70 -> R.drawable.battery_80
                        percentage >= 50 -> R.drawable.battery_60
                        percentage >= 30 -> R.drawable.battery_40
                        percentage >= 10 -> R.drawable.battery_20
                        else -> R.drawable.battery_empty
                    }
                    batteryIcon.setImageResource(iconRes)

                    // Upozorenje ako je napon prenizak
                    if (voltage < 3.4) {
                        if (!lowVoltageAlertShown) {
                            val toneGen = ToneGenerator(AudioManager.STREAM_ALARM, 100)
                            toneGen.startTone(ToneGenerator.TONE_CDMA_ALERT_CALL_GUARD, 500)

                            AlertDialog.Builder(this@MainActivity)
                                .setTitle("Upozorenje")
                                .setMessage(
                                    "Napon baterije je nizak (${
                                        String.format(
                                            Locale.getDefault(),
                                            "%.2f",
                                            voltage
                                        )
                                    } V). Preporučuje se punjenje uređaja."
                                )
                                .setPositiveButton("U redu") { dialog, _ ->
                                    dialog.dismiss()
                                }
                                .show()

                            lowVoltageAlertShown = true
                        }

                        batteryPercentage.setTextColor(Color.RED)

                    } else {

                        lowVoltageAlertShown = false

                        batteryPercentage.setTextColor(Color.BLACK)
                    }
                }


                if (latitude != null && longitude != null) {
                    val carLocation = LatLng(latitude, longitude)
                    mMap.clear()

                    // Dodaj ponovno geofence krug
                    geofenceCircle = mMap.addCircle(
                        CircleOptions()
                            .center(referenceLocation)
                            .radius(radiusKm * 1000)
                            .strokeColor(Color.BLUE)
                            .fillColor(Color.argb(50, 255, 0, 0))
                            .strokeWidth(2f)
                    )
                    mMap.addMarker(MarkerOptions().position(carLocation).title("Car Location"))
                    mMap.moveCamera(CameraUpdateFactory.newLatLngZoom(carLocation, 15f))

                    // Geofencing logic
                    val currentDistance = distanceInKm(carLocation, referenceLocation)
                    val isInsideRadius = currentDistance <= radiusKm

                    if (wasInsideRadius == null || wasInsideRadius != isInsideRadius) {
                        if (isInsideRadius) {
                            Toast.makeText(
                                this@MainActivity, "Vozilo je ušlo u područje od $radiusKm.",
                                Toast.LENGTH_SHORT
                            ).show()

                            geofenceCircle?.strokeColor = Color.GREEN
                            geofenceCircle?.fillColor = Color.argb(80, 0, 255, 0) // zelena ispuna
                            geofenceStatusText.text = getString(R.string.geofence_status_inside)
                            geofenceStatusText.setTextColor(Color.GREEN)

                        } else {
                            Toast.makeText(
                                this@MainActivity, "Vozilo je izašlo iz područja od $radiusKm km.",
                                Toast.LENGTH_SHORT
                            ).show()

                            geofenceCircle?.strokeColor = Color.RED
                            geofenceCircle?.fillColor = Color.argb(80, 255, 0, 0) // crvena ispuna

                            geofenceStatusText.text = getString(R.string.geofence_status_outside)
                            geofenceStatusText.setTextColor(Color.RED)

                        }
                        wasInsideRadius = isInsideRadius
                    }
                }
            }

            override fun onCancelled(error: DatabaseError) {
                Toast.makeText(this@MainActivity, "Failed to load location", Toast.LENGTH_SHORT)
                    .show()
            }
        })
    }

    private fun voltageToPercentage(voltage: Double): Int {
        val minVoltage = 3.0
        val maxVoltage = 4.2
        val percentage = ((voltage - minVoltage) / (maxVoltage - minVoltage)) * 100
        return percentage.coerceIn(0.0, 100.0).toInt()
    }

    private fun distanceInKm(from: LatLng, to: LatLng): Double {
        val results = FloatArray(1)
        Location.distanceBetween(
            from.latitude, from.longitude,
            to.latitude, to.longitude,
            results
        )
        return results[0] / 1000.0 // u kilometrima
    }
}
