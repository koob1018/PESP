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
- Last interrupt time only. FSR is displayed as a raw value; the GUI does not judge whether the value is triggered.
- Runtime configuration controls for FSR threshold, sampling interval, and interrupt enable.
- Raw serial data from the two-board run.

The GUI reads the sensor-node USB console. The base-station image stays USB-headless; its activity is visible through the framed commands logged by sensor-node.
