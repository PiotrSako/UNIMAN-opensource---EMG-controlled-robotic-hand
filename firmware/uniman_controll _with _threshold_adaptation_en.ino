#include <Servo.h>

const int EMG_PIN       = A0;
const int BUTTON_PIN    = 2;   // calibration button
const int MODE_PIN      = 3;   // mode switch
const int LED_NORMAL    = 4;   // normal mode LED
const int LED_LATCH     = 5;   // latch mode LED
const int SAMPLES       = 8;

const int SERVO_OPEN    = 500;
const int SERVO_CLOSE   = 2500;

int THRESHOLD_ON  = 450;
int THRESHOLD_OFF = 350;

Servo myServo;
bool active = false;
bool prevContraction = false;   // for edge detection in latch mode
bool prevMode = false;          // for mode change detection

int readEMGAveraged() {
  long sum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum += analogRead(EMG_PIN);
  }
  return sum / SAMPLES;
}

int median(int *arr, int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = 0; j < n - i - 1; j++) {
      if (arr[j] > arr[j + 1]) {
        int tmp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = tmp;
      }
    }
  }
  return arr[n / 2];
}

void calibration() {
  Serial.println("=== ADAPTIVE CALIBRATION ===");
  delay(1500);

  // --- PHASE 1: REST (10 sec) ---
  Serial.println(">>> RELAX THE MUSCLE FOR 10 SECONDS <<<");
  Serial.println("Just hold your arm loose...");
  delay(2000);
  Serial.println("Measurement START");

  const int N_REST = 400;
  int restSamples[N_REST];
  int maxRest = 0;
  for (int i = 0; i < N_REST; i++) {
    restSamples[i] = readEMGAveraged();
    if (restSamples[i] > maxRest) maxRest = restSamples[i];
    delay(25);
  }
  int medRest = median(restSamples, N_REST);
  THRESHOLD_OFF = medRest + 100;
  
  Serial.print("Rest median: ");
  Serial.println(medRest);
  Serial.print("Rest max: ");
  Serial.println(maxRest);
  Serial.print("THRESHOLD_OFF = ");
  Serial.println(THRESHOLD_OFF);

  delay(2000);

  // --- PHASE 2: CONTRACTIONS (15 sec) ---
  Serial.println(">>> PERFORM 3-5 STRONG CONTRACTIONS AT ANY PACE <<<");
  Serial.println("You have 15 seconds. Clench and relax.");
  delay(2000);
  Serial.println("Measurement START");

  int peakThreshold = maxRest + 50;
  
  const int N_FLEX = 600;
  const int MIN_PEAK_DIST = 40;
  const int MAX_PEAKS = 10;
  int peaks[MAX_PEAKS];
  int peakCount = 0;
  
  int currentPeak = 0;
  int samplesSinceLast = MIN_PEAK_DIST;
  bool inContraction = false;
  
  for (int i = 0; i < N_FLEX; i++) {
    int sample = readEMGAveraged();
    samplesSinceLast++;
    
    if (sample > peakThreshold && !inContraction && samplesSinceLast >= MIN_PEAK_DIST) {
      inContraction = true;
      currentPeak = sample;
    }
    
    if (inContraction) {
      if (sample > currentPeak) currentPeak = sample;
      
      if (sample < peakThreshold - 30) {
        if (peakCount < MAX_PEAKS) {
          peaks[peakCount] = currentPeak;
          peakCount++;
          Serial.print("Peak #");
          Serial.print(peakCount);
          Serial.print(": ");
          Serial.println(currentPeak);
        }
        inContraction = false;
        samplesSinceLast = 0;
      }
    }
    
    delay(25);
  }

  if (inContraction && peakCount < MAX_PEAKS) {
    peaks[peakCount] = currentPeak;
    peakCount++;
    Serial.print("Peak #");
    Serial.print(peakCount);
    Serial.print(": ");
    Serial.println(currentPeak);
  }

  if (peakCount < 2) {
    Serial.println("WARNING: Fewer than 2 contractions detected! Calibration failed.");
    return;
  }

  long peakSum = 0;
  for (int i = 0; i < peakCount; i++) {
    peakSum += peaks[i];
  }
  int peakAverage = peakSum / peakCount;
  THRESHOLD_ON = (peakAverage * 80L) / 100;

  Serial.print("Contractions detected: ");
  Serial.println(peakCount);
  Serial.print("Peak average: ");
  Serial.println(peakAverage);
  Serial.print("THRESHOLD_ON (80% of average) = ");
  Serial.println(THRESHOLD_ON);

  if (THRESHOLD_ON <= THRESHOLD_OFF) {
    Serial.println("WARNING: thresholds overlap! Repeat the calibration.");
  } else {
    int margin = THRESHOLD_ON - THRESHOLD_OFF;
    Serial.print("Margin between thresholds: ");
    Serial.println(margin);
  }

  Serial.println("=== CALIBRATION COMPLETE ===");
  delay(1000);
}

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(MODE_PIN, INPUT_PULLUP);
  pinMode(LED_NORMAL, OUTPUT);
  pinMode(LED_LATCH, OUTPUT);
  
  myServo.attach(9, SERVO_OPEN, SERVO_CLOSE);
  myServo.writeMicroseconds(SERVO_OPEN);
  
  // mode state initialization
  prevMode = (digitalRead(MODE_PIN) == LOW);
}

void loop() {
  // --- calibration button ---
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      calibration();
    }
  }

  // --- mode read (LOW = switch closed = LATCH mode) ---
  bool latchMode = (digitalRead(MODE_PIN) == LOW);
  
  // mode change detection -> reset to open position
  if (latchMode != prevMode) {
    active = false;
    prevContraction = false;
    myServo.writeMicroseconds(SERVO_OPEN);
    Serial.print(">>> MODE CHANGE: ");
    Serial.println(latchMode ? "LATCH" : "NORMAL");
    prevMode = latchMode;
    delay(100);  // short pause after the change
  }
  
  // --- LED indication ---
  digitalWrite(LED_NORMAL, latchMode ? LOW : HIGH);
  digitalWrite(LED_LATCH,  latchMode ? HIGH : LOW);

  // --- EMG read ---
  int emgRaw = readEMGAveraged();
  
  // contraction detection with hysteresis (common to both modes)
  bool contraction = prevContraction;
  if (emgRaw > THRESHOLD_ON)  contraction = true;
  if (emgRaw < THRESHOLD_OFF) contraction = false;

  // --- mode logic ---
  if (latchMode) {
    // LATCH MODE: rising edge toggles the state
    if (contraction && !prevContraction) {
      active = !active;
    }
  } else {
    // NORMAL MODE: state = current contraction
    active = contraction;
  }
  prevContraction = contraction;

  myServo.writeMicroseconds(active ? SERVO_CLOSE : SERVO_OPEN);

  Serial.print(emgRaw);
  Serial.print(",");
  Serial.print(THRESHOLD_ON);
  Serial.print(",");
  Serial.print(THRESHOLD_OFF);
  Serial.print(",");
  Serial.println(latchMode ? "LATCH" : "NORMAL");
  delay(20);
}
