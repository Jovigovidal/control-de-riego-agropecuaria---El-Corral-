#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// --- Pines TFT ---
#define TFT_CS   4
#define TFT_DC   3
#define TFT_RST  8

// --- Pines DS1302 ---
#define DS1302_IO   7
#define DS1302_SCLK 6
#define DS1302_CE   5
ThreeWire myWire(DS1302_IO, DS1302_SCLK, DS1302_CE);
RtcDS1302<ThreeWire> rtc(myWire);

// --- Pines Ventilador ---
#define FAN_PWM_PIN 9
#define FAN_TACH_PIN 2

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Variables ventilador
volatile unsigned long pulseCount = 0;
unsigned long lastMillis = 0;
unsigned long pulses = 0;
int rpm = 0;
float duty = 20;
const float pulsesPerRev = 2.0;

String ultimaHora = "";
int posX = 10, posY = 20;

// ---- Contador de pulsos ----
void countPulse() {
  pulseCount++;
}


// ---- PWM a 25 kHz ----
void setupPWM25kHz() {
  TCCR1A = 0;
  TCCR1B = 0;
  pinMode(FAN_PWM_PIN, OUTPUT);
  TCCR1A = _BV(COM1A1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
  ICR1 = 640; // 25kHz
}

void setFanSpeed(float dutyPercent) {
  if (dutyPercent < 0) dutyPercent = 0;
  if (dutyPercent > 100) dutyPercent = 100;
  OCR1A = (ICR1 * dutyPercent) / 100.0;
}

void setup() {
  Serial.begin(9600);
  rtc.Begin();

  setupPWM25kHz();
  setFanSpeed(duty);

  if (!rtc.IsDateTimeValid()) {
    rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
  }
  rtc.SetIsWriteProtected(false);
  if (!rtc.GetIsRunning()) {
    rtc.SetIsRunning(true);
  }

  // TFT
  tft.initR(INITR_18BLACKTAB);
  tft.setRotation(0);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);

  // Ventilador
  pinMode(FAN_TACH_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FAN_TACH_PIN), countPulse, FALLING);

  lastMillis = millis();
}

void loop() {
  // Mostrar hora
  RtcDateTime now = rtc.GetDateTime();
  char buffer[9];
  snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", now.Hour(), now.Minute(), now.Second());
  String horaString = String(buffer);

  if (horaString != ultimaHora) {
    tft.fillRect(posX, posY - 0, 30, 10, ST7735_BLACK);
    tft.setCursor(posX, posY);
    tft.print(horaString);
    ultimaHora = horaString;
  }

  // Calcular y mostrar RPM cada 1 segundo
  if (millis() - lastMillis >= 1000) {
    noInterrupts();
    pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    rpm = (pulses * 60) / pulsesPerRev; // sin decimales

    Serial.print("Pulsos: ");
    Serial.print(pulses);
    Serial.print(" | RPM: ");
    Serial.println(rpm);

    // Mostrar en TFT
    tft.fillRect(0, 60, 120, 20, ST7735_BLACK); // Borrar solo área RPM
    tft.setCursor(0, 20);
    tft.print("RPM: ");
    tft.print(rpm);

    lastMillis = millis();
  }

  // Control PWM desde Serial
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'u') duty += 5;
    if (cmd == 'd') duty -= 5;
    setFanSpeed(duty);
    Serial.print("Nueva velocidad PWM: ");
    Serial.print(duty);
    Serial.println("%");
  }
}
