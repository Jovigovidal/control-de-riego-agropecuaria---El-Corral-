#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ILI9341.h>
#include <Fonts/FreeSans9pt7b.h> 
#include <SPI.h>

// Pines (ajusta según tu conexión)
#define TFT_CS   4
#define TFT_DC   3
#define TFT_RST  8

// --- Pines DS1302 ---
#define DS1302_IO   7
#define DS1302_SCLK 6
#define DS1302_CE   5
ThreeWire myWire(DS1302_IO, DS1302_SCLK, DS1302_CE);
RtcDS1302<ThreeWire> rtc(myWire);
int posX = 10;
int posY = 60;
String ultimaHora = "";
//////////////////////////////////////////////////

// --- Pines Relé ---
#define RELE_PIN    10

// --- Pines Ventilador ---
#define FAN_PWM_PIN 9
#define FAN_TACH_PIN 2

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// --- Constantes ventilador ---
volatile unsigned long pulseCount = 0;
unsigned long lastMillis = 0;
int rpm = 0;

// Parámetros ventilador
int duty = 50;
unsigned long pulses = 0;
const float pulsesPerRev = 2.0; 

void countPulse() {
  pulseCount++;
}

void setupPWM25kHz() {
  // Configurar PWM a 25kHz en FAN_PWM_PIN (timer1)
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
  setFanSpeed(duty); // -------> 100% de velocidad inicial
  ///////////////////////////////////////////
  if (!rtc.IsDateTimeValid()) {
    rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
  }
  rtc.SetIsWriteProtected(false);
  if (!rtc.GetIsRunning()) {
    rtc.SetIsRunning(true);
  }
  Serial.println("Iniciando TFT ST7735S...");
  
  // Inicializar según tu tipo de pantalla
  tft.initR(INITR_18BLACKTAB); // Cambia a GREENTAB o REDTAB si ves colores raros
  tft.setRotation(0); // rotacion de pantalla
  tft.fillScreen(ST7735_BLACK);
  

  // Mostrar texto
  int16_t x1, y1;
  uint16_t w, h;
  String texto = "Hola Mundo";
  tft.getTextBounds(texto, 0, 0, &x1, &y1, &w, &h);

  int16_t posX = (tft.width() - w) / 2;
  posY = 20; // Centrado vertical

  tft.setCursor(posX, posY);
  tft.print(texto);
  delay(2000);

  tft.fillScreen(ST7735_BLACK);
  rtc.Begin();


  pinMode(FAN_PWM_PIN, OUTPUT);
  pinMode(FAN_TACH_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FAN_TACH_PIN), countPulse, FALLING);

  lastMillis = millis();
}

void loop() {
  RtcDateTime now = rtc.GetDateTime();

  char buffer[9];
  snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", now.Hour(), now.Minute(), now.Second());
  String horaString = String(buffer);

  // Solo actualizar si cambia
  if (horaString != ultimaHora) {
    // Borrar hora anterior
    tft.fillRect(posX, posY - 2, 80, 20, ST7735_BLACK);

    // Dibujar hora nueva
    tft.setCursor(posX, posY);
    tft.setTextColor(ST7735_BLUE);
    tft.setTextSize(1);
    tft.print(horaString);

    ultimaHora = horaString;
  }
  ///////////////////////////////////////VENTILADOR/////////////////////////
  // Cada 1 segundo calcular RPM
  if (millis() - lastMillis >= 1000) {
    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    rpm = (pulses * 60.0) / pulsesPerRev;

    Serial.print("Pulsos: ");
    Serial.print(pulses);
    Serial.print(" | RPM: ");
    Serial.println(rpm);

    lastMillis = millis();
  }
    // Mostrar en TFT
    // --- Mostrar en TFT sin parpadeo ---
    tft.setTextSize(1);

    // Pulsos
   /* tft.setCursor(10, 60);
    tft.fillRect(10, 60, 200, 20, ILI9341_BLACK); // Borrar solo esta área
    tft.print("Pulsos: ");
    tft.print(pulses);*/

    // RPM
  tft.setCursor(0, 60);
  tft.print("RPM: ");
  tft.print((int)rpm);// Espacios para limpiar

    lastMillis = millis(); 
////////////////////////////////////////////////
  if (Serial.available()) {
    char cmd = Serial.read();
    static float duty = 50;
    if (cmd == 'u') duty += 5;
    if (cmd == 'd') duty -= 5;
    setFanSpeed(duty);
    Serial.print("Nueva velocidad PWM: ");
    Serial.print(duty);
    Serial.println("%");
  }
  delay(200);
}
