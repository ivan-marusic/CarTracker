
# Python TCP Relay (Oracle Cloud VM)

This server receives raw TCP JSON from the STM32F429 + SIM7600 device
and updates Firebase Realtime Database over HTTPS.

## Run:
sudo ufw allow 4000/tcp
python3 server.py

