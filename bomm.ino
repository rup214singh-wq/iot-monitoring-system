#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// =================================================
// PIN DEFINITIONS
// =================================================

#define DHTPIN D3
#define DHTTYPE DHT11

#define GAS_SENSOR D2
#define VIBRATION_SENSOR D1

#define ACS712_PIN A0

#define LED_PIN D4
#define BUZZER_PIN D7

#define LCD_SDA D5
#define LCD_SCL D6

// =================================================
// LCD
// =================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

// =================================================
// DHT
// =================================================

DHT dht(DHTPIN, DHTTYPE);

// =================================================
// LIMITS
// =================================================

#define MAX_TEMP 40.0
#define MAX_HUMIDITY 90.0
#define MAX_CURRENT_RAW 100

// =================================================
// TIMING
// =================================================

#define SENSOR_TIME 2000
#define ALARM_TIME 5000
#define BEEP_TIME 120

// =================================================
// VARIABLES
// =================================================

int sensorPage = 0;

unsigned long pageStart = 0;
unsigned long alarmStart = 0;
unsigned long beepStart = 0;
unsigned long blinkStart = 0;

bool alarmActive = false;
bool beepActive = false;
bool ledState = false;

String alarmReason = "";

// Sensor values

float temperature = 0;
float humidity = 0;

int gasValue = 0;
int currentRaw = 0;

// =================================================
// VIBRATION FILTER
// =================================================

bool vibrationDetected()
{
  static unsigned long lastTriggerTime = 0;

  // Ignore repeated triggers for 3 seconds
  if (millis() - lastTriggerTime < 3000)
    return false;

  int highCount = 0;

  for (int i = 0; i < 20; i++)
  {
    if (digitalRead(VIBRATION_SENSOR) == HIGH)
    {
      highCount++;
    }

    delay(2);
  }

  if (highCount >= 15)
  {
    lastTriggerTime = millis();
    return true;
  }

  return false;
}

// =================================================
// SETUP
// =================================================

void setup()
{
  Serial.begin(115200);

  Wire.begin(LCD_SDA, LCD_SCL);

  lcd.init();
  lcd.backlight();

  dht.begin();

  pinMode(GAS_SENSOR, INPUT);

  // IMPORTANT
  pinMode(VIBRATION_SENSOR, INPUT);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("INDUSTRIAL IoT");

  lcd.setCursor(0, 1);
  lcd.print("SYSTEM START");

  delay(2500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SYSTEM READY");

  lcd.setCursor(0, 1);
  lcd.print("MONITORING...");

  delay(1500);

  sensorPage = 0;
  pageStart = millis();

  shortBeep();
  showSensor();
}

// =================================================
// MAIN LOOP
// =================================================

void loop()
{
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  gasValue = digitalRead(GAS_SENSOR);
  currentRaw = analogRead(ACS712_PIN);

  Serial.print("Vibration = ");
  Serial.println(digitalRead(VIBRATION_SENSOR));

  // =================================================
  // ALARM MODE
  // =================================================

  if (alarmActive)
  {
    if (millis() - blinkStart >= 250)
    {
      blinkStart = millis();

      ledState = !ledState;

      digitalWrite(LED_PIN, ledState);
    }

    digitalWrite(BUZZER_PIN, HIGH);

    if (millis() - alarmStart >= ALARM_TIME)
    {
      alarmActive = false;

      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED_PIN, LOW);

      ledState = false;

      nextSensor();
    }

    return;
  }

  // =================================================
  // CHECK ALARMS
  // =================================================

  if (sensorPage == 0)
  {
    if (!isnan(temperature) && temperature > MAX_TEMP)
    {
      startAlarm("HIGH TEMP");
      return;
    }
  }

  else if (sensorPage == 1)
  {
    if (!isnan(humidity) && humidity > MAX_HUMIDITY)
    {
      startAlarm("HIGH HUMID");
      return;
    }
  }

  else if (sensorPage == 2)
  {
    if (gasValue == LOW)
    {
      startAlarm("GAS ALERT");
      return;
    }
  }

  else if (sensorPage == 3)
  {
    if (currentRaw > MAX_CURRENT_RAW)
    {
      startAlarm("HIGH CURRENT");
      return;
    }
  }

  else if (sensorPage == 4)
  {
    if (vibrationDetected())
    {
      startAlarm("VIBRATION");
      return;
    }
  }

  // =================================================
  // NEXT SCREEN
  // =================================================

  if (millis() - pageStart >= SENSOR_TIME)
  {
    nextSensor();
  }

  // =================================================
  // SHORT BEEP TIMER
  // =================================================

  if (beepActive)
  {
    if (millis() - beepStart >= BEEP_TIME)
    {
      beepActive = false;
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
}

// =================================================
// NEXT SENSOR
// =================================================

void nextSensor()
{
  sensorPage++;

  if (sensorPage > 4)
  {
    sensorPage = 0;
  }

  pageStart = millis();

  shortBeep();

  showSensor();
}

// =================================================
// SHORT BEEP
// =================================================

void shortBeep()
{
  beepActive = true;

  beepStart = millis();

  digitalWrite(BUZZER_PIN, HIGH);
}

// =================================================
// START ALARM
// =================================================

void startAlarm(String reason)
{
  alarmActive = true;

  alarmReason = reason;

  alarmStart = millis();

  blinkStart = millis();

  ledState = true;

  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("!!! WARNING !!!");

  lcd.setCursor(0, 1);
  lcd.print(reason);

  Serial.println();
  Serial.println("=================");
  Serial.print("ALARM: ");
  Serial.println(reason);
  Serial.println("=================");
}

// =================================================
// LCD DISPLAY
// =================================================

void showSensor()
{
  lcd.clear();

  if (sensorPage == 0)
  {
    lcd.setCursor(0, 0);
    lcd.print("TEMPERATURE");

    lcd.setCursor(0, 1);

    if (isnan(temperature))
    {
      lcd.print("ERROR");
    }
    else
    {
      lcd.print(temperature, 1);
      lcd.print((char)223);
      lcd.print("C");
    }
  }

  else if (sensorPage == 1)
  {
    lcd.setCursor(0, 0);
    lcd.print("HUMIDITY");

    lcd.setCursor(0, 1);

    if (isnan(humidity))
    {
      lcd.print("ERROR");
    }
    else
    {
      lcd.print(humidity, 1);
      lcd.print("%");
    }
  }

  else if (sensorPage == 2)
  {
    lcd.setCursor(0, 0);
    lcd.print("GAS SENSOR");

    lcd.setCursor(0, 1);

    if (gasValue == LOW)
      lcd.print("DETECTED");
    else
      lcd.print("NORMAL");
  }

  else if (sensorPage == 3)
  {
    lcd.setCursor(0, 0);
    lcd.print("CURRENT");

    lcd.setCursor(0, 1);

    lcd.print("RAW:");
    lcd.print(currentRaw);
  }

  else if (sensorPage == 4)
  {
    lcd.setCursor(0, 0);
    lcd.print("VIBRATION");

    lcd.setCursor(0, 1);

    if (digitalRead(VIBRATION_SENSOR) == HIGH)
      lcd.print("DETECTED");
    else
      lcd.print("NORMAL");
  }
}
