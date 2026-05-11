# PESP Host GUI

Local browser GUI for the two-board demo.

## Run

```bash
cd /Users/yiwenxu/Projects/coursework/PESP
python3 host-gui/app.py --serial /dev/cu.usbmodem1101 --port 8765
```

Then open:

```text
http://127.0.0.1:8765
```

## What It Shows

- Current environment values: temperature, humidity, pressure, light, gas status, and force.
- Interrupt state as a single lamp indicator. The lamp stays red briefly after an event so base-station auto-clear does not hide short interrupts.
- Raw serial data from the two-board run.

The GUI reads the sensor-node USB console. The base-station image stays USB-headless; its activity is visible through the framed commands logged by sensor-node.
