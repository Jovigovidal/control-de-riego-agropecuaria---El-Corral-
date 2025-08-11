#include <Arduino.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// --- Pines DS1302 ---
#define DS1302_IO   7
#define DS1302_SCLK 6
#define DS1302_CE   5

// --- Pines Relé ---
#define RELE_PIN    10

// --- Pines Ventilador ---
#define FAN_PWM_PIN 9
#define FAN_TACH_PIN 2

// --- Pines TFT ---
#define TFT_CS   4
#define TFT_DC   3
#define TFT_RST  8

// TFT objeto
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Variables ventilador
volatile unsigned long pulseCount = 0;
unsigned long lastRPMTime = 0;
int rpm = 0;

// Objeto RTC
ThreeWire myWire(DS1302_IO, DS1302_SCLK, DS1302_CE);
RtcDS1302<ThreeWire> rtc(myWire);

// --- Interrupción para contar pulsos del ventilador ---
void countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);

  // --- Inicializar pines ---
  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(RELE_PIN, LOW);

  pinMode(FAN_PWM_PIN, OUTPUT);
  pinMode(FAN_TACH_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FAN_TACH_PIN), countPulse, FALLING);

  // --- Inicializar RTC ---
  rtc.Begin();
  if (!rtc.IsDateTimeValid()) {
    rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
  }
  rtc.SetIsWriteProtected(false);
  if (!rtc.GetIsRunning()) {
    rtc.SetIsRunning(true);
  }

  // --- Inicializar TFT ---
  tft.initR(INITR_BLACKTAB); // Inicialización estándar
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println("Sistema Iniciado");
}

void loop() {
  RtcDateTime now = rtc.GetDateTime();

  // ---- Control del relé ----
  if ((now.Hour() == 5 && now.Minute() == 0) ||
      (now.Hour() == 12 && now.Minute() == 0) ||
      (now.Hour() == 18 && now.Minute() == 0)) {
    digitalWrite(RELE_PIN, HIGH);
  } else {
    digitalWrite(RELE_PIN, LOW);
  }

  // ---- Control ventilador ----
  analogWrite(FAN_PWM_PIN, 50); // Velocidad fija

  // ---- Calcular RPM cada segundo ----
  if (millis() - lastRPMTime >= 1000) {
    lastRPMTime = millis();
    rpm = (pulseCount / 2) * 60; // 2 pulsos por vuelta
    pulseCount = 0;

    // --- Mostrar datos en Serial ---
    Serial.print("Hora: ");
    if (now.Hour() < 10) Serial.print('0');
    Serial.print(now.Hour());
    Serial.print(':');
    if (now.Minute() < 10) Serial.print('0');
    Serial.print(now.Minute());
    Serial.print(':');
    if (now.Second() < 10) Serial.print('0');
    Serial.print(now.Second());
    Serial.print(" | RPM: ");
    Serial.println(rpm);

    // --- Mostrar datos en TFT ---
    tft.fillScreen(ST77XX_BLACK); // Limpiar pantalla
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    tft.print("Hora:");
    tft.setCursor(0, 20);
    if (now.Hour() < 10) tft.print('0');
    tft.print(now.Hour());
    tft.print(':');
    if (now.Minute() < 10) tft.print('0');
    tft.print(now.Minute());
    tft.print(':');
    if (now.Second() < 10) tft.print('0');
    tft.print(now.Second());

    tft.setCursor(0, 50);
    tft.print("RPM:");
    tft.setCursor(0, 70);
    tft.print(rpm);
  }
}
