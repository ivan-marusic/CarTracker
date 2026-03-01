from flask import Flask, request
import requests
import os

app = Flask(__name__)

# Postavi svoj Firebase Realtime Database URL (.firebasedatabase.app + .json)
# Primjer:
# FIREBASE_URL = "https://cartracker-1c4f6-default-rtdb.europe-west1.firebasedatabase.app/location.json"
FIREBASE_URL = os.getenv("FIREBASE_URL", "https://<your-db>.firebasedatabase.app/location.json")

@app.route("/update", methods=["POST"])
def update():
    # Prihvati JSON od SIM7600 i proslijedi ga kao HTTPS PUT u Firebase
    payload = request.get_json(force=True, silent=True)
    if payload is None:
        return {"error": "Invalid JSON"}, 400

    r = requests.put(FIREBASE_URL, json=payload, timeout=10)
    return (r.text, r.status_code, {"Content-Type": "application/json"})

@app.route("/", methods=["GET"])
def health():
    return "CarTracker Python proxy running."
