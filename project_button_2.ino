// === Two-Button Two-LED Reaction Loop ===
// Pins
const uint8_t BTN_PINS[2] = {2, 3};    // Button1, Button2  (INPUT_PULLUP)
const uint8_t LED_PINS[2] = {8, 9};    // LED1, LED2        (OUTPUT, active HIGH)
const uint8_t BUZZER_PIN  = 6;         // Buzzer (tone pin)

// Timings (ms)
const unsigned long RESPOND_WINDOW_MS = 5000;  // 亮燈後等待按下的時間
const unsigned long BLINK_DURATION_MS = 3000;  // 閃爍總長度
const unsigned long BLINK_INTERVAL_MS = 200;   // 閃爍頻率 (200ms ≈ 5Hz)
const unsigned long ITI_MS            = 1000;  // 試次間隔

// Debounce
const unsigned long DEBOUNCE_MS = 15;

enum State { WAIT_ITI, IDLE, ON, BLINK };
State state = IDLE;

int activeLed = -1;                 // 0 or 1; -1 means none
bool ledVisible = false;
unsigned long stateStart = 0;
unsigned long lastBlinkToggle = 0;

// debounce bookkeeping
bool lastRaw[2]        = {true, true};  // using pullup: HIGH = not pressed
bool stablePressed[2]  = {false, false};
unsigned long lastEdgeMs[2] = {0,0};

void beepOnce() {
  // 無源蜂鳴器：短促提示
  tone(BUZZER_PIN, 900, 80); // 80ms
}

void setLed(int idx, bool on) {
  digitalWrite(LED_PINS[idx], on ? HIGH : LOW);
  ledVisible = on;
}

void allLedsOff() {
  for (int i=0;i<2;i++) digitalWrite(LED_PINS[i], LOW);
  ledVisible = false;
}

void startTrial() {
  activeLed = random(0, 2);     // 0 or 1
  setLed(activeLed, true);
  state = ON;
  stateStart = millis();
  beepOnce();
  Serial.print("TRIAL:START:led=");
  Serial.println(activeLed + 1); // human 1-based
}

void finishTrial(const char* reason, bool sendTriggerIfPressed) {
  // 熄燈 & 通知
  if (activeLed >= 0) setLed(activeLed, false);
  Serial.print("EVENT:LED_OFF:led=");
  Serial.print(activeLed + 1);
  Serial.print(":reason=");
  Serial.println(reason);

  if (sendTriggerIfPressed && strcmp(reason, "pressed") == 0) {
    Serial.print("TRIGGER:led=");
    Serial.println(activeLed + 1);
  }

  activeLed = -1;
  state = WAIT_ITI;
  stateStart = millis();
}

void setup() {
  for (int i=0;i<2;i++) {
    pinMode(BTN_PINS[i], INPUT_PULLUP);
    pinMode(LED_PINS[i], OUTPUT);
  }
  pinMode(BUZZER_PIN, OUTPUT);
  allLedsOff();

  // 隨機種子
  randomSeed(analogRead(A0));

  Serial.begin(9600);
  while(!Serial){;}
  // 先休息一下再開始第一回合
  state = WAIT_ITI;
  stateStart = millis();
}

void updateButtons() {
  unsigned long now = millis();
  for (int i=0;i<2;i++) {
    bool raw = digitalRead(BTN_PINS[i]);  // HIGH = released, LOW = pressed
    if (raw != lastRaw[i]) {
      lastRaw[i] = raw;
      lastEdgeMs[i] = now;
    } else if ((now - lastEdgeMs[i]) >= DEBOUNCE_MS) {
      bool pressed = (raw == LOW);
      stablePressed[i] = pressed;
    }
  }
}

void loop() {
  updateButtons();
  unsigned long now = millis();

  switch (state) {
    case WAIT_ITI:
      if (now - stateStart >= ITI_MS) {
        state = IDLE;
      }
      break;

    case IDLE:
      startTrial();  // 直接開始下一回合
      break;

    case ON:
      // 5 秒內如果按了正確按鈕 → 立即結束（pressed）
      if (activeLed >= 0 && stablePressed[activeLed]) {
        finishTrial("pressed", /*sendTriggerIfPressed=*/true);
        break;
      }
      // 超時 → 進入閃爍 3 秒
      if (now - stateStart >= RESPOND_WINDOW_MS) {
        state = BLINK;
        stateStart = now;
        lastBlinkToggle = now;
        ledVisible = true; // 目前亮著
      }
      break;

    case BLINK:
      // 閃爍切換
      if (now - lastBlinkToggle >= BLINK_INTERVAL_MS) {
        lastBlinkToggle = now;
        ledVisible = !ledVisible;
        setLed(activeLed, ledVisible);
      }
      // 閃爍期間有按正確按鈕 → 立即結束（pressed）
      if (activeLed >= 0 && stablePressed[activeLed]) {
        finishTrial("pressed", /*sendTriggerIfPressed=*/true);
        break;
      }
      // 閃爍時長到 → 自動結束（timeout）
      if (now - stateStart >= BLINK_DURATION_MS) {
        finishTrial("timeout", /*sendTriggerIfPressed=*/false);
      }
      break;
  }
}
