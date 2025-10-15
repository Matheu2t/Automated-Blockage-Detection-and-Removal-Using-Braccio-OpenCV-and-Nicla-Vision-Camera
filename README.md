Automated Blockage Detection & Removal

Installation Guide & User Manual
Braccio++ • Arduino Nano RP2040 Connect • Nicla Vision (OpenMV + Edge Impulse)

Stable release for thesis: v1.0-thesis (recommended).
Code © You (MIT). Arduino pinout images © Arduino (CC BY-SA 4.0).

1) Overview

The system detects rock-like blockages using Nicla Vision running an Edge Impulse model in OpenMV, then triggers a single safe pick-and-place cycle on a Braccio++ arm controlled by an Arduino Nano RP2040 Connect.
A short GPIO pulse from Nicla D3 → Arduino D2 starts the cycle; you can also start manually with the ENTER keypad button.
2) Hardware & Wiring
Item	Details
Arm	Braccio++ 6-DoF
MCU (arm)	Arduino Nano RP2040 Connect
Camera/Edge	Nicla Vision running OpenMV + EI model
Trigger line	Nicla D3 → Arduino D2 (RISING edge)
Ground	Common GND between Nicla, Arduino, and the 5 V servo PSU
Power	External 5 V regulated supply for servos (do not power servos from USB)

Keep wiring short and secure; share ground or the trigger won’t be recognized.

2) Software Requirements

Arduino IDE 2.x, Braccio++ library, Nano RP2040 Connect board support

OpenMV IDE (latest)

Edge Impulse account to export the OpenMV bundle

(Optional) Python 3.10+ for host OpenCV testing

3) Installation (Step-by-Step)
3.1 Flash the Arduino (Braccio++ Controller)

Open Arduino IDE → File → Open… → Final_code.ino.

Install Braccio++ library and Nano RP2040 Connect boards package.

Select Board & Port → Verify → Upload.

Open Serial Monitor at 115200 baud to see logs.

Firmware defaults (from Final_code.ino):

Baud: 115200

Trigger input: D2 (interrupt, RISING)

ISR debounce: 100 ms (TRIG_DEBOUNCE_MS)

Run cooldown: 1500 ms (RUN_COOLDOWN_MS)

Gripper angles: open 150°, close 230°

Manual start: Braccio keypad ENTER (id=6)

SELECT prints current joint angles to Serial

4.0) Load the Nicla Vision (OpenMV + EI)

Open OpenMV IDE, connect Nicla Vision.

Copy your EI model files to the device (trained.tflite, labels.txt or EI bundle).

Open ei_object_detection.py and Save → Run on the device.

Confirm these script defaults (already set):

UART: UART(1) @ 115200 (for a “hello” log)

Trigger pin: D3 output, LOW at idle

Pulse width: 30 ms (trigger_arduino(30))

Confidence threshold: 0.70

Re-arm logic: waits a few frames before allowing the next trigger

When a detection ≥ 0.70 occurs and re-arm is satisfied, D3 pulses HIGH for ~30 ms → Arduino D2 interrupt → one full pick-and-place cycle.

5) How to Run
Edge-Only (recommended)

Power the 5 V servo PSU (arm on stable surface).

Upload & run ei_object_detection.py on Nicla from OpenMV IDE.

Ensure D3 → D2 trigger and common GND are connected.

Watch Arduino Serial Monitor (115200) for logs such as:

[ARM] Cycle start → moves through PICK, CARRY, DROP, open gripper, return to HOME.

To start manually (no detection), press ENTER (id=6) on the Braccio keypad.

(Optional) Host-assisted OpenCV

If you add opencv_host/, run:

cd opencv_host
python -m venv .venv
# Windows: .venv\Scripts\activate
# macOS/Linux: source .venv/bin/activate
pip install -r requirements.txt
python run.py


Send a serial trigger or a GPIO toggle equivalent to Nicla’s D3 pulse.

6) Configuration
6.1 Motion Poses & Limits (Arduino)

Edit in Final_code.ino:

Pose arrays (order gripper, wrist_roll, wrist_pitch, elbow, shoulder, base):
HOME_POS, PICK_ABOVE, PICK_DOWN, CARRY, DROP_ABOVE, DROP_DOWN, OPEN_AT_DROP

Timing: STEP_DELAY_MARGIN (default 50 ms), GRIP_DELAY_MS (500 ms), LIFT_DWELL_MS (400 ms)

Safety: keep angles within your arm’s safe range; test each pose individually.

6.2 Detection Thresholds (OpenMV)

Edit in ei_object_detection.py:

score >= 0.70 (raise to reduce false positives)

trigger_arduino(30) — pulse width; you can increase if your wire is long/noisy

Re-arm frames to avoid rapid re-triggers

7) Calibration Procedure

Manually find safe angles for HOME, PICK, PLACE locations.

Update each pose array and test single steps with long delays.

Tune gripper opening/closing (150° / 230° by default) for your object size.

Verify the cooldown (1500 ms) prevents immediate retriggers.

8) Troubleshooting
Symptom	Likely Cause	Fix
No cycle when object detected	D3→D2 wiring / no common GND	Share GND; check 30 ms pulse with LED or scope; raise pulse to 60–80 ms
Multiple cycles per detection	Cooldown too short / bounce	Increase RUN_COOLDOWN_MS; keep D3 pulse short; verify ISR debounce (100 ms)
Arm jerks or hits limits	Unsafe poses	Test pose arrays one by one; reduce angles/speeds
Camera detects nothing	Threshold too high / lighting	Lower threshold; add lighting; refocus lens
Serial busy/port errors	COM port in use	Close other apps/Serial Monitors; replug USB

9) Safety

Keep hands clear; have an E-Stop (power switch) within reach.

Never power servos from USB.

Secure cables out of the arm’s path.

Start with light dummy objects.

10) Credits & Licenses

Firmware & scripts © You, MIT (see LICENSE).

OpenMV example headers © OpenMV LLC (MIT).

Arduino pinout images © Arduino, CC BY-SA 4.0.

Quick Cheat-Sheet
Arduino baud: 115200
Trigger: Nicla D3 → Arduino D2 (RISING)
Debounce (Arduino): 100 ms
Cooldown between cycles: 1500 ms
Pulse width (Nicla): 30 ms
Gripper: open 150°, close 230°
Manual start: ENTER key (id=6)
SELECT key: print joint angles to Serial
