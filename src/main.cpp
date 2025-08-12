#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// --- Pines TFT ---
#define TFT_CS   4
#define TFT_DC   3
#define TFT_RST  8
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// --- Pines DS1302 ---
#define DS1302_IO   7
#define DS1302_SCLK 6
#define DS1302_CE   5
ThreeWire myWire(DS1302_IO, DS1302_SCLK, DS1302_CE);
RtcDS1302<ThreeWire> rtc(myWire);

// --- Pines Ventilador ---
#define FAN_PWM_PIN 9
#define FAN_TACH_PIN 2

// --- relay---//

#define RELE_PIN 10

bool releEstado = false;
unsigned long releStart = 0;
const unsigned long releDuracion = 60000; // milisegundos (60 segundos)


// Variables ventilador
volatile unsigned long pulseCount = 0;
unsigned long lastMillis = 0;
unsigned long pulses = 0;
int rpm = 0;
float duty = 20;
const float pulsesPerRev = 2.0;

// Control de actualización
String ultimaHora = "";
int ultimoRPM = -1;

// Colores
#define COLOR_HORA    ST7735_GREEN
#define COLOR_RPM     ST7735_YELLOW
#define FONDO         ST7735_BLACK // ------> FONDO DE COLOR DEL TEXTO

// Posiciones en pantalla
int horaX = 0, horaY = 0;
int rpmX  = 0, rpmY  = 10;

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
  tft.fillScreen(FONDO);
  tft.setTextSize(1);

  // Ventilador
  pinMode(FAN_TACH_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FAN_TACH_PIN), countPulse, FALLING);

  lastMillis = millis();
 // Relay
 pinMode(RELE_PIN, OUTPUT);
 digitalWrite(RELE_PIN, LOW);
}

void loop() {
  // Mostrar hora en verde
  RtcDateTime now = rtc.GetDateTime();
  char buffer[9];
  snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", now.Hour(), now.Minute(), now.Second());
  String horaString = String(buffer);

  if (horaString != ultimaHora) {
    tft.fillRect(horaX, horaY, 60, 10, FONDO);
    tft.setCursor(horaX, horaY);
    tft.setTextColor(COLOR_HORA, FONDO);
    tft.print(horaString);
    ultimaHora = horaString;
  }
 /////////////////////////////DURACION RELAY/////////////////////////
  if ((now.Hour() == 5 && now.Minute() == 0 && now.Second() == 0) ||
      (now.Hour() == 12 && now.Minute() == 0 && now.Second() == 0) ||
      (now.Hour() == 18 && now.Minute() == 0 && now.Second() == 0)) {
    releStart = millis();
    releEstado = true;
  }

 // Apagar después del tiempo definido
  if (releEstado && millis() - releStart > releDuracion) {
    releEstado = false;
  }
  ///////////////////////////////////////////////////////////////////
  tft.setCursor(0, 30);
  tft.setTextColor(ST7735_WHITE, FONDO);
  tft.print("RELAY: ");
  tft.setTextColor(releEstado ? ST7735_GREEN : ST7735_RED, FONDO);
  tft.print(releEstado ? "ENCENDIDO " : "APAGADO");

  // Calcular y mostrar RPM en amarillo
  if (millis() - lastMillis >= 1000) {
    noInterrupts();
    pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    rpm = (pulses * 60) / pulsesPerRev;

    if (rpm != ultimoRPM) {
      tft.fillRect(rpmX, rpmY, 20, 20, FONDO);
      tft.setCursor(rpmX, rpmY);
      tft.setTextColor(COLOR_RPM, FONDO);
      tft.print("FAN01:");
      tft.print(rpm);
      tft.print("RPM");
      ultimoRPM = rpm;
    }

    Serial.print("Pulsos: ");
    Serial.print(pulses);
    Serial.print(" | RPM: ");
    Serial.println(rpm);

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
