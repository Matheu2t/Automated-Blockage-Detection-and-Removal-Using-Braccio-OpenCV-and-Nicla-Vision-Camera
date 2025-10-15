## System Overview

![Block diagram](docs/img/block-diagram.png)

**Pipeline summary**
- Nicla Vision runs OpenMV + Edge Impulse model for rock detection and raises a **GPIO pulse on D3** to trigger the cycle on Arduino.
- Arduino (Nano RP2040 Connect) handles **interrupt on D2**, gates a single pick-and-place **cycle**, and drives Braccio++ motion.

> OpenMV script uses Edge Impulse FOMO post-processing and emits a UART “hello” plus a D3 pulse for triggering. See `ei_object_detection.py` for details.

## How to Run (Edge-only)
1. Flash `Final_code.ino` to the Arduino board (Braccio++ attached).
2. In **OpenMV IDE**, load `ei_object_detection.py` on Nicla Vision, ensure `trained.tflite` and `labels.txt` are on the device.
3. Open Serial Monitor (115200) on the Arduino to watch cycle logs.
