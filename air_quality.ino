#include <Tomoto_HM330X.h>

// Constants
const int RELAY_PIN = D6;          // Relay control pin
const int LOUDNESS_PIN = A0;       // Grove Loudness sensor pin
const int DELAY_MS = 100;
const int PM_WARMUP_READINGS = 10;
const int PM_READINGS = 10;
const int SOUND_READINGS = 10;      // Number of sound readings to average per cycle
#define uS_TO_S_FACTOR 1000000ULL  
#define TIME_TO_SLEEP  60          

RTC_DATA_ATTR int bootCount = 0;
Tomoto_HM330X sensor;

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup by ULP program"); break;
    default: Serial.printf("Wakeup not by deep sleep: %d\n", wakeup_reason); break;
  }
}

int getLoudnessReading() {
  long sum = 0;
  for(int i = 0; i < SOUND_READINGS; i++) {
    sum += analogRead(LOUDNESS_PIN);
    delay(10);  // Short delay between readings
  }
  return sum / SOUND_READINGS;
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LOUDNESS_PIN, INPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Turn on relay on wake
  delay(1000);
  
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();
  
  if (!sensor.begin()) {
    Serial.println("Failed to initialize HM330X");
    return;
  }
  
  Serial.println("HM330X initialized successfully!");
  Serial.println("\nStarting measurements...");
  
  // Warmup readings
  Serial.println("Warmup readings...");
  for (int i = 0; i < PM_WARMUP_READINGS; i++) {
    if (!sensor.readSensor()) {
      Serial.println("Failed to read during warmup");
      continue;
    }
    delay(DELAY_MS);
  }
  
  // Take readings and calculate averages
  int pm1_0_total = 0;
  int pm2_5_total = 0;
  int pm10_total = 0;
  int validReadings = 0;
  
  Serial.println("Taking measurements for average...");
  
  // Get loudness reading first
  int loudnessLevel = getLoudnessReading();
  Serial.print("Loudness Level: ");
  Serial.println(loudnessLevel);
  
  // Then get PM readings
  for (int i = 0; i < PM_READINGS; i++) {
    if (!sensor.readSensor()) {
      Serial.println("Failed to read sensor");
      continue;
    }
    
    pm1_0_total += sensor.std.getPM1();
    pm2_5_total += sensor.std.getPM2_5();
    pm10_total += sensor.std.getPM10();
    
    validReadings++;
    delay(DELAY_MS);
  }
  
  if (validReadings > 0) {
    Serial.println("\nAveraged PM Readings:");
    Serial.print("PM1.0 (ug/m^3): "); Serial.println(pm1_0_total / validReadings);
    Serial.print("PM2.5 (ug/m^3): "); Serial.println(pm2_5_total / validReadings);
    Serial.print("PM10  (ug/m^3): "); Serial.println(pm10_total / validReadings);
  }
  
  // Turn off relay before sleep
  Serial.println("Turning off relay...");
  digitalWrite(RELAY_PIN, LOW);
  delay(100);  // Small delay to ensure relay has time to switch
  
  // Configure deep sleep
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Going to sleep for " + String(TIME_TO_SLEEP) + " seconds");
  Serial.flush();
  
  esp_deep_sleep_start();
}

void loop() {
  // Never reached - everything is in setup()
}
