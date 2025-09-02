package com.example.cartracker

import android.content.Intent
import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.ProgressBar
import android.widget.Toast
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.isGone
import com.google.firebase.auth.FirebaseAuth

class LoginActivity : AppCompatActivity() {
    private lateinit var auth: FirebaseAuth

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_login)

        auth = FirebaseAuth.getInstance()
        val emailLayout = findViewById<com.google.android.material.textfield.TextInputLayout>(R.id.emailLayout)
        val passwordLayout = findViewById<com.google.android.material.textfield.TextInputLayout>(R.id.passwordLayout)

        val progressBar = findViewById<ProgressBar>(R.id.progressBar)
        val emailField = findViewById<EditText>(R.id.emailEditText)
        val passwordField = findViewById<EditText>(R.id.passwordEditText)
        val loginButton = findViewById<Button>(R.id.loginButton)
        val registerButton = findViewById<Button>(R.id.registerButton)
        val resetPasswordButton = findViewById<Button>(R.id.resetPasswordButton)

        loginButton.setOnClickListener {
            // Show the fields if they are hidden
            if (emailLayout.isGone || passwordLayout.isGone || resetPasswordButton.isGone){
                emailLayout.visibility = View.VISIBLE
                passwordLayout.visibility = View.VISIBLE
                resetPasswordButton.visibility = View.VISIBLE
                return@setOnClickListener
            }
            val email = emailField.text.toString()
            val password = passwordField.text.toString()

            val emailRegex = "[a-zA-Z0-9._-]+@[a-z]+\\.+[a-z]+".toRegex()

            if (email.isEmpty() || password.isEmpty()) {
                Toast.makeText(this, "Unesite email i lozinku", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            if (!email.matches(emailRegex)) {
                Toast.makeText(this, "Email nije ispravan", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            if (password.length < 6) {
                Toast.makeText(this, "Lozinka mora imati najmanje 6 znakova", Toast.LENGTH_SHORT)
                    .show()
                return@setOnClickListener
            }

            progressBar.visibility = View.VISIBLE

            auth.signInWithEmailAndPassword(email, password)
                .addOnCompleteListener { task ->
                    if (task.isSuccessful) {
                        startActivity(Intent(this, MainActivity::class.java))
                        finish()
                    } else {
                        Toast.makeText(
                            this,
                            "Greška: ${task.exception?.message}",
                            Toast.LENGTH_SHORT
                        ).show()
                    }
                    progressBar.visibility = View.GONE
                }
        }

        registerButton.setOnClickListener {
            startActivity(Intent(this, RegisterActivity::class.java))
        }

        resetPasswordButton.setOnClickListener {
            startActivity(Intent(this, ResetPasswordActivity::class.java))
        }
    }

    override fun onStart() {
        super.onStart()
        if (auth.currentUser != null) {
            startActivity(Intent(this, MainActivity::class.java))
            finish()
        }
    }
}