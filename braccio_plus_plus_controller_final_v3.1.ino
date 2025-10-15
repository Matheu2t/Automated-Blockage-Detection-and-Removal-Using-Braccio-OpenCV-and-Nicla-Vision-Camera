/*
  braccio_pick_place_button.ino
  Minimal Braccio++ pick & place (no OpenMV, no ROS, no WiFi).
  - Press ENTER button on the Braccio++ carrier to run: PICK -> PLACE -> HOME
  - Optional: set AUTO_RUN_ON_BOOT = true to run one full cycle automatically after boot.
*/

#include <Arduino.h>
#include <Braccio++.h>   // Make sure the Braccio++ library is installed

// ----------------- Config -----------------
static const bool AUTO_RUN_ON_BOOT = false;  // set true to auto-run once after boot

#define MOVE_SPEED        15
#define DOWN_SPEED        10
#define LIFT_DELAY_MS     300
#define STEP_DELAY_MS     250

// Gripper angles (larger is usually more open on Braccio++)
#define GRIP_OPEN_ANGLE   73
#define GRIP_CLOSE_ANGLE  45

// Pose = { base, shoulder, elbow, wrist_pitch, wrist_roll, gripper }
struct Pose { int b, s, e, wp, wr, g; };

// --------- TUNE THESE POSES to your table & bin ----------
Pose HOME       = { 90,  90,  90,  90,  90, GRIP_OPEN_ANGLE };

// Fixed pickup location (place your rock here by hand for now)
Pose ABOVE_PICK = { 90, 120,  80, 100,  90, GRIP_OPEN_ANGLE };
Pose PICK_Z     = { 90, 135,  70, 105,  90, GRIP_OPEN_ANGLE };

// Carry (after gripping)
Pose CARRY      = { 90, 100, 110,  80,  90, GRIP_CLOSE_ANGLE };

// Drop bin location
Pose DROP_ABOVE = { 20, 110,  90,  95,  90, GRIP_CLOSE_ANGLE };
Pose DROP_Z     = { 20, 125,  75, 100,  90, GRIP_CLOSE_ANGLE };
Pose OPEN_DROP  = { 20, 125,  75, 100,  90, GRIP_OPEN_ANGLE };

// ----------------- Helpers -----------------
static inline void fatal_blink() {
  pinMode(LED_BUILTIN, OUTPUT);
  for (;;) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(200);
  }
}

// Braccio++.moveTo expects order: (gripper, wrist_roll, wrist_pitch, elbow, shoulder, base)
static inline void movePose(const Pose &p, int speed) {
  Braccio.moveTo(p.g, p.wr, p.wp, p.e, p.s, p.b);
  delay(STEP_DELAY_MS);
}

static inline void gripperOpen()  { Pose t = HOME; t.g = GRIP_OPEN_ANGLE;  movePose(t, DOWN_SPEED); }
static inline void gripperClose() { Pose t = HOME; t.g = GRIP_CLOSE_ANGLE; movePose(t, DOWN_SPEED); }

// Comfortable initial posture (from your earlier code)
static void set_initial_position() {
  float base_pos        = 0.0f;
  float elbow_pos       = RAD_TO_DEG * 2.8f;
  float shoulder_pos    = RAD_TO_DEG * 2.8f;
  float wrist_pitch_pos = RAD_TO_DEG * 4.2f;
  float wrist_roll_pos  = RAD_TO_DEG * 2.93215f;
  float grippper_pos    = RAD_TO_DEG * 2.79253f;
  Braccio.moveTo(grippper_pos, wrist_roll_pos, wrist_pitch_pos,
                 elbow_pos, shoulder_pos, base_pos);
  delay(100);
}

static void pickFixedSquare() {
  movePose(ABOVE_PICK, MOVE_SPEED);
  movePose(PICK_Z,     DOWN_SPEED);

  Pose grip = PICK_Z; 
  grip.g = GRIP_CLOSE_ANGLE;
  movePose(grip, DOWN_SPEED);

  delay(LIFT_DELAY_MS);

  Pose carry = CARRY; 
  carry.g = GRIP_CLOSE_ANGLE;
  movePose(carry, MOVE_SPEED);
}

static void placeToBin() {
  movePose(DROP_ABOVE, MOVE_SPEED);
  movePose(DROP_Z,     DOWN_SPEED);

  movePose(OPEN_DROP,  DOWN_SPEED); // release
  delay(150);

  Pose lift = DROP_ABOVE; 
  lift.g = GRIP_OPEN_ANGLE;
  movePose(lift, MOVE_SPEED);
}

static void fullCycleFixed() {
  pickFixedSquare();
  placeToBin();
  movePose(HOME, MOVE_SPEED);
}

// ----------------- Arduino lifecycle -----------------
void setup() {
  // Init Braccio++
  if (!Braccio.begin()) {
    // Fatal error blink if init fails
    fatal_blink();
  }

  // Start in a comfy pose then move to HOME
  set_initial_position();
  movePose(HOME, MOVE_SPEED);

  // Optional: run one full cycle automatically after boot
  if (AUTO_RUN_ON_BOOT) {
    delay(1000);
    fullCycleFixed();
  }
}

void loop() {
  // Press ENTER button (on the Braccio++ carrier) to trigger a full cycle
  static unsigned long last_btn_ms = 0;
  if (Braccio.isButtonPressed_ENTER() && (millis() - last_btn_ms > 500)) {
    last_btn_ms = millis();
    fullCycleFixed();
  }

  // Idle
  delay(20);
}
