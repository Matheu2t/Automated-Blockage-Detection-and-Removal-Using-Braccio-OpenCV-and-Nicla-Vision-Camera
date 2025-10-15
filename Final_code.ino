/**************************************************************************************
 * BRACCIO++ PICK & PLACE — GPIO TRIGGER + ENTER-BUTTON FALLBACK (NANO RP2040 CONNECT)
 * - Camera/OpenMV pulses a pin -> Arduino D2 (RISING) to start a cycle
 * - OR press the Braccio keypad ENTER (id=6) to start manually
 **************************************************************************************/

#include <Arduino.h>
#include <Braccio++.h>

/*** USER TUNABLES ***********************************************************/
#define PRINT_BAUD         115200

// External trigger from OpenMV/Nicla:
#define TRIG_PIN           2            // Arduino D2 (interrupt-capable)
#define TRIG_DEBOUNCE_MS   100          // coarse ISR debounce
#define RUN_COOLDOWN_MS    1500         // rest after a cycle

// Extra filtering to remove unwanted triggers
#define TRIG_MIN_HIGH_MS   20           // HIGH must last at least this long
#define TRIG_REARM_LOW_MS  100          // require LOW for this long to re-arm

// Braccio keypad IDs
#define BUTTON_SELECT      3            // prints joints
#define BUTTON_ENTER       6            // manual start

// Motion timing
#define MOVE_MS_DEFAULT    900          // slower = gentler
#define STEP_DELAY_MARGIN  50
#define GRIP_DELAY_MS      500
#define LIFT_DWELL_MS      400

// Gripper angles 
#define GRIP_OPEN_ANG      150.0
#define GRIP_CLOSE_ANG     230.0

/*** POSES (order: gripper, wrist_roll, wrist_pitch, elbow, shoulder, base) ***/
static const float HOME_POS[6]     = {157.5,         157.5, 157.5, 157.5, 157.5,  90.0};
static const float PICK_ABOVE[6]   = {GRIP_OPEN_ANG, 157.5, 110.0, 110.0, 120.0,  90.0};
static const float PICK_DOWN[6]    = {GRIP_OPEN_ANG, 157.5, 130.0, 140.0, 130.0,  90.0};
static const float CARRY[6]        = {GRIP_CLOSE_ANG,157.5, 110.0, 110.0, 120.0,  90.0};
static const float DROP_ABOVE[6]   = {GRIP_CLOSE_ANG,157.5, 110.0, 110.0, 120.0,  20.0};
static const float DROP_DOWN[6]    = {GRIP_CLOSE_ANG,157.5, 130.0, 140.0, 120.0,  20.0};
static const float OPEN_AT_DROP[6] = {GRIP_OPEN_ANG, 157.5, 130.0, 140.0, 120.0,  20.0};

/*** GLOBALS ******************************************************************/
auto gripper    = Braccio.get(1);
auto wristRoll  = Braccio.get(2);
auto wristPitch = Braccio.get(3);
auto elbow      = Braccio.get(4);
auto shoulder   = Braccio.get(5);
auto base       = Braccio.get(6);

// Trigger filtering state
volatile bool trigPending = false;          // saw a rising edge
volatile unsigned long trigRiseMs = 0;
volatile unsigned long lastTrigMs  = 0;     // coarse debounce reference

bool  armedForNextPulse = true;             // require LOW window before new start
bool  busy              = false;
unsigned long cooldownUntil = 0;

/*** ISR **********************************************************************/
void onTrigPulse() {
  unsigned long now = millis();
  if (now - lastTrigMs > TRIG_DEBOUNCE_MS) { // coarse debounce
    trigRiseMs  = now;                        // when it went HIGH
    trigPending = true;                       // main loop will validate width
    lastTrigMs  = now;
  }
}

/*** PROTOTYPES ***************************************************************/
void moveAll(const float pose[6], uint16_t ms = MOVE_MS_DEFAULT);
void gripperOpen();
void gripperClose();
void pickPlaceCycle();
void printJoints();

/*** HELPERS ******************************************************************/
void moveAll(const float pose[6], uint16_t ms) {
  // Braccio.moveTo order: (gripper, wrist_roll, wrist_pitch, elbow, shoulder, base)
  Braccio.moveTo(pose[0], pose[1], pose[2], pose[3], pose[4], pose[5]);
  delay(ms + STEP_DELAY_MARGIN);
}

void gripperOpen()  { gripper.move().to(GRIP_OPEN_ANG);  delay(GRIP_DELAY_MS); }
void gripperClose() { gripper.move().to(GRIP_CLOSE_ANG); delay(GRIP_DELAY_MS); }

/*** MAIN TASK ****************************************************************/
void pickPlaceCycle() {
  Serial.println(F("[ARM] Cycle start"));

  // Approach above rock
  moveAll(PICK_ABOVE);

  // Descend with gripper open, then close
  moveAll(PICK_DOWN);
  gripperClose();
  delay(LIFT_DWELL_MS);

  // Lift to safe carry
  moveAll(CARRY);

  // Move over bin, descend, release
  moveAll(DROP_ABOVE);
  moveAll(DROP_DOWN);
  moveAll(OPEN_AT_DROP);

  // Lift away and return home
  moveAll(DROP_ABOVE);
  moveAll(HOME_POS);

  Serial.println(F("[ARM] Cycle done"));
}

/*** OPTIONAL: PRINT CURRENT JOINTS *******************************************/
void printJoints() {
  float a[6];
  Braccio.positions(a);
  Serial.println(F("---- Joint Angles (1..6) ----"));
  Serial.print(F("Gripper: "));  Serial.println(a[0], 1);
  Serial.print(F("W-Roll : "));  Serial.println(a[1], 1);
  Serial.print(F("W-Pitch: "));  Serial.println(a[2], 1);
  Serial.print(F("Elbow  : "));  Serial.println(a[3], 1);
  Serial.print(F("Should : "));  Serial.println(a[4], 1);
  Serial.print(F("Base   : "));  Serial.println(a[5], 1);
  Serial.println(F("-----------------------------"));
}

/*** SETUP / LOOP *************************************************************/
void setup() {
  Serial.begin(PRINT_BAUD);
  while (!Serial) { /* wait for USB */ }

  if (!Braccio.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.println(F("[ERR] Braccio.begin() failed — blinking LED"));
    for (;;) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); delay(200); }
  }

  // Home pose on boot
  moveAll(HOME_POS);

  // External trigger input from OpenMV
  pinMode(TRIG_PIN, INPUT_PULLUP); // OpenMV drives a HIGH pulse
  attachInterrupt(digitalPinToInterrupt(TRIG_PIN), onTrigPulse, RISING);

  Serial.println(F("[SYS] Ready — D2 pulse OR ENTER button will start pick→place"));
}

void loop() {
  // 1) Camera GPIO trigger (one-shot, debounced, re-arm on LOW)
  if (!busy && millis() >= cooldownUntil) {
    // If we saw a rising edge in the ISR, verify pulse width in the main loop:
    if (trigPending && armedForNextPulse) {
      // confirm the pin remained HIGH long enough
      if (digitalRead(TRIG_PIN) == HIGH &&
          (millis() - trigRiseMs) >= TRIG_MIN_HIGH_MS) {

        trigPending        = false;     // consume the event
        armedForNextPulse  = false;     // disarm until we see a solid LOW

        busy = true;
        pickPlaceCycle();
        busy = false;

        cooldownUntil = millis() + RUN_COOLDOWN_MS;
      }
      // If it bounced low quickly, just clear pending; wait for a clean next edge
      else if (digitalRead(TRIG_PIN) == LOW) {
        trigPending = false;
      }
    }

    // Re-arm logic: require the line to be LOW for TRIG_REARM_LOW_MS continuously
    static unsigned long lowStartMs = 0;
    if (!armedForNextPulse) {
      if (digitalRead(TRIG_PIN) == LOW) {
        if (lowStartMs == 0) lowStartMs = millis();
        if (millis() - lowStartMs >= TRIG_REARM_LOW_MS) {
          armedForNextPulse = true;   // ready for the next real pulse
          lowStartMs = 0;
        }
      } else {
        lowStartMs = 0; // went high again; restart the LOW timer
      }
    }
  }

  // 2) Manual ENTER button fallback
  int key = Braccio.getKey();
  if (key == BUTTON_ENTER && !busy && millis() >= cooldownUntil) {
    while (Braccio.getKey() == BUTTON_ENTER) { delay(10); } // wait for release
    busy = true;
    pickPlaceCycle();
    busy = false;
    cooldownUntil = millis() + RUN_COOLDOWN_MS;
  }

  // 3) Optional: SELECT prints joint angles
  if (key == BUTTON_SELECT) {
    printJoints();
    while (Braccio.getKey() == BUTTON_SELECT) { delay(10); }
  }

  delay(2);
}

