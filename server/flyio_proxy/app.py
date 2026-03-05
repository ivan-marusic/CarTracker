from flask import Flask, request
import os
import json

import firebase_admin
from firebase_admin import credentials, db

app = Flask(__name__)

creds_json = os.environ.get("FIREBASE_CREDS")
db_url = os.environ.get("DATABASE_URL")

if not creds_json or not db_url:
    raise RuntimeError("FIREBASE_CREDS and DATABASE_URL must be set")

cred = credentials.Certificate(json.loads(creds_json))
firebase_admin.initialize_app(cred, {"databaseURL": db_url})


@app.route("/update", methods=["POST"])
def update():
    payload = request.get_json(force=True, silent=True)
    if payload is None:
        return {"error": "Invalid JSON"}, 400
    try:
        db.reference("/location").set(payload)
        return {"status": "ok"}, 200
    except Exception as e:
      
        print(f"[ERROR] Firebase write failed: {e}", flush=True)
        return {"error": str(e)}, 500

@app.route("/", methods=["GET"])
def health():
    return "CarTracker Python proxy (Firebase Admin) running."
