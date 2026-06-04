// Sensores: Humedad de suelo (GPIO34), DHT11 (GPIO14)
// Salidas: LED RGB (R:27, V:26, A:25), LCD I2C (SDA:21, SCL:22), Bomba (GPIO32)
// Entradas: Botón Modo Auto/Manual (GPIO16)

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h> 

// Librerías para WiFi, Servidor Asíncrono y LittleFS
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// CONFIGURACIÓN WIFI (Reemplaza con tus datos reales)
const char* ssid = "NOVIEMBRE"; 
const char* password = "chester170607"; 

// Servidor asíncrono en el puerto 80
AsyncWebServer server(80);

//  DEFINICIÓN DE PINES
const int PIN_BOTON     = 16;  // Botón para alternar modo de riego

const int PIN_HUMEDAD   = 34;  // Sensor de humedad de suelo (ADC1_CH6)
const int PIN_DHT       = 14;  // Pin de datos del DHT11

const int PIN_LED_ROJO  = 27;   
const int PIN_LED_VERDE = 26;   
const int PIN_LED_AZUL  = 25;   

const int PIN_BOMBA     = 32;   

// CONFIGURACIÓN DHT11
#define DHTTYPE DHT11
DHT dht(PIN_DHT, DHTTYPE);

// PARÁMETROS CONFIGURABLES
const uint8_t LCD_DIRECCION_I2C = 0x27;

const unsigned long TIEMPO_LECTURA_SENSORES = 2000; 
const unsigned long TIEMPO_ACTUALIZAR_LCD   = 1000;
const unsigned long TIEMPO_ENVIO_SERIAL     = 2000;
const unsigned long TIEMPO_BOMBA_ON         = 15000; // 15 segundos de riego
const unsigned long TIEMPO_ENFRIAMIENTO     = 60000; // 1 minuto de espera

// AJUSTE ANTI BOUNCING (Antirrebote por software)
const unsigned long TIEMPO_ANTIRREBOTE = 200; 

const float TEMP_BAJA = 15.0;
const float TEMP_ALTA = 25.0;

const int HUMEDAD_IDEAL_FRIA = 40;
const int HUMEDAD_IDEAL_MEDIA = 55;
const int HUMEDAD_IDEAL_CALIDA = 70;

const int MARGEN_ALTO = 10;
const int MARGEN_MUY_ALTO = 20;

// VARIABLES GLOBALES DE ESTADO
LiquidCrystal_I2C lcd(LCD_DIRECCION_I2C, 16, 2);

float temperaturaC    = 0.0;
int   humedadPct      = 0;
int   humedadAmbiente = 0; 
int   humedadIdeal    = 0;
String estadoHumedad  = "";

bool  bombaEncendida      = false;
unsigned long tiempoInicioBomba  = 0;
unsigned long tiempoUltimoRiego  = 0;
bool  enfriamientoActivo  = false;

bool  riegoAutomaticoActivo = false;  
int   estadoBotonActual     = HIGH;
int   estadoBotonAnterior   = HIGH;
unsigned long ultimoTiempoRebote = 0;

unsigned long ultimaLectura    = 0;
unsigned long ultimaActLCD     = 0;
unsigned long ultimoEnvioSerial = 0;

// =============================================================================
// SETUP
// =============================================================================

void setup() {
  delay(3000);
  Serial.begin(9600);
  Serial.println(F("Iniciando ESP32 con Servidor LittleFS..."));

  // Inicializar sistema de archivos LittleFS
  if(!LittleFS.begin(true)){
    Serial.println("Error montando LittleFS. Asegúrate de haber subido la carpeta 'data'.");
    return;
  }
  Serial.println("LittleFS montado correctamente.");

  // Conectar al WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando al WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado! Dirección IP para ver la página: ");
  Serial.println(WiFi.localIP());

  dht.begin(); 

  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_LED_ROJO,  OUTPUT);
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_AZUL,  OUTPUT);
  
  digitalWrite(PIN_BOMBA, LOW); 
  pinMode(PIN_BOMBA, OUTPUT);

  setLED(LOW, LOW, LOW);

  Wire.begin(21, 22); 
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(F("WiFi OK. IP:"));
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(3000); 
  lcd.clear(); 

  // --- CONFIGURACIÓN DEL SERVIDOR WEB ---
  
  // Ruta raíz: Entregar los archivos estáticos desde LittleFS
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // Ruta /datos: Entregar el JSON para los gráficos y los estados
  server.on("/datos", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"temperatura\":" + String(temperaturaC) + ",";
    json += "\"humedad_amb\":" + String(humedadAmbiente) + ",";
    json += "\"humedad_suelo\":" + String(humedadPct) + ",";
    json += "\"modo\":\"" + String(riegoAutomaticoActivo ? "AUTOMATICO" : "MANUAL") + "\",";
    json += "\"bomba\":\"" + String(bombaEncendida ? "ENCENDIDA" : "APAGADA") + "\"";
    json += "}";
    
    request->send(200, "application/json", json);
  });

  // Ruta nueva para el botón de la web
  server.on("/toggle-bomba", HTTP_GET, [](AsyncWebServerRequest *request){
    if (bombaEncendida) {
      // Si estaba prendida, la apagamos
      digitalWrite(PIN_BOMBA, LOW);
      bombaEncendida = false;
      enfriamientoActivo = true;
      tiempoUltimoRiego = millis();
      Serial.println(F("[Web] Bomba APAGADA manualmente"));
    } else {
      // Si estaba apagada, la prendemos y forzamos modo MANUAL
      riegoAutomaticoActivo = false; 
      digitalWrite(PIN_BOMBA, HIGH);
      bombaEncendida = true;
      tiempoInicioBomba = millis();
      Serial.println(F("[Web] Bomba ENCENDIDA manualmente"));
    }
    // Le respondemos a la web que salió todo bien
    request->send(200, "text/plain", "OK");
  });

  // Iniciar el servidor
  server.begin();
  Serial.println("Servidor Web Asíncrono iniciado.");
}

// =============================================================================
// LOOP
// =============================================================================

void loop() {
  // Nota: Con ESPAsyncWebServer ya no necesitas server.handleClient();
  
  unsigned long ahora = millis();

  gestionarBoton(ahora);

  if (ahora - ultimaLectura >= TIEMPO_LECTURA_SENSORES) {
    ultimaLectura = ahora;
    leerSensores();
    calcularHumedadIdeal();
    clasificarEstado();
    controlarLED();
    controlarBomba(ahora);
  }

  if (ahora - ultimaActLCD >= TIEMPO_ACTUALIZAR_LCD) {
    ultimaActLCD = ahora;
    actualizarLCD();
  }

  gestionarBomba(ahora);
}

// =============================================================================
// FUNCIONES (Se mantienen iguales)
// =============================================================================

void gestionarBoton(unsigned long ahora) {
  int lectura = digitalRead(PIN_BOTON);

  if (lectura != estadoBotonAnterior) {
    ultimoTiempoRebote = ahora;
  }

  if ((ahora - ultimoTiempoRebote) > TIEMPO_ANTIRREBOTE) {
    if (lectura != estadoBotonActual) {
      estadoBotonActual = lectura;

      if (estadoBotonActual == LOW) {
        riegoAutomaticoActivo = !riegoAutomaticoActivo; 
        
        Serial.print(F("[Modo] Cambiado a: "));
        Serial.println(riegoAutomaticoActivo ? F("AUTOMATICO") : F("MANUAL"));

        if (!riegoAutomaticoActivo && bombaEncendida) {
          digitalWrite(PIN_BOMBA, LOW); // APAGAR RELÉ
          bombaEncendida      = false;
          enfriamientoActivo  = true; 
          tiempoUltimoRiego   = ahora;
          Serial.println(F("[Bomba] APAGADA - Riego interrumpido manualmente"));
        }

        actualizarLCD(); 
      }
    }
  }
  
  estadoBotonAnterior = lectura;
}

void leerSensores() {
  float lecturaTempDHT = dht.readTemperature();
  int lecturaHumDHT = dht.readHumidity();
  
  if (!isnan(lecturaTempDHT)) {
    temperaturaC = lecturaTempDHT;
  }
  if (!isnan(lecturaHumDHT)) {
    humedadAmbiente = lecturaHumDHT;
  }

  if (bombaEncendida == false) {
    int lecturaADC_hum = analogRead(PIN_HUMEDAD);
    humedadPct = map(lecturaADC_hum, 4095, 0, 0, 100);
    humedadPct = constrain(humedadPct, 0, 100);
  }
}

void calcularHumedadIdeal() {
  if (temperaturaC < TEMP_BAJA) {
    humedadIdeal = HUMEDAD_IDEAL_FRIA;
  } else if (temperaturaC <= TEMP_ALTA) {
    humedadIdeal = HUMEDAD_IDEAL_MEDIA;
  } else {
    humedadIdeal = HUMEDAD_IDEAL_CALIDA;
  }

  if (humedadAmbiente > 0) { 
    if (humedadAmbiente < 30) {
      humedadIdeal += 10; 
    } else if (humedadAmbiente > 60) {
      humedadIdeal -= 10; 
    }
  }
  humedadIdeal = constrain(humedadIdeal, 0, 100);
}

void clasificarEstado() {
  int diferencia = humedadPct - humedadIdeal;

  if (diferencia > MARGEN_MUY_ALTO) {
    estadoHumedad = "Muy humeda";
  } else if (diferencia > MARGEN_ALTO) {
    estadoHumedad = "Humeda";
  } else if (diferencia < -MARGEN_MUY_ALTO) {
    estadoHumedad = "Muy seca";
  } else if (diferencia < -MARGEN_ALTO) {
    estadoHumedad = "Seca";
  } else {
    estadoHumedad = "Optima";
  }
}

void controlarLED() {
  if (estadoHumedad == "Optima") {
    setLED(LOW, HIGH, LOW);
  } else if (estadoHumedad == "Humeda" || estadoHumedad == "Seca") {
    setLED(HIGH, HIGH, LOW);
  } else {
    setLED(HIGH, LOW, LOW);
  }
}

void setLED(int r, int g, int b) {
  digitalWrite(PIN_LED_ROJO,  r);
  digitalWrite(PIN_LED_VERDE, g);
  digitalWrite(PIN_LED_AZUL,  b);
}

void controlarBomba(unsigned long ahora) {
  if (enfriamientoActivo && (ahora - tiempoUltimoRiego >= TIEMPO_ENFRIAMIENTO)) {
    enfriamientoActivo = false;
    Serial.println(F("[Bomba] Enfriamiento terminado."));
  }

  if (riegoAutomaticoActivo && estadoHumedad == "Muy seca" && !bombaEncendida && !enfriamientoActivo) {
    digitalWrite(PIN_BOMBA, HIGH); 
    bombaEncendida     = true;
    tiempoInicioBomba  = ahora;
    Serial.println(F("[Bomba] ENCENDIDA - Regando (Modo Auto)"));
  }
}

void gestionarBomba(unsigned long ahora) {
  if (bombaEncendida && (ahora - tiempoInicioBomba >= TIEMPO_BOMBA_ON)) {
    digitalWrite(PIN_BOMBA, LOW); 
    bombaEncendida      = false;
    enfriamientoActivo  = true;
    tiempoUltimoRiego   = ahora;

    Serial.print(F("Esperando para volver a regar en "));
    Serial.print(TIEMPO_ENFRIAMIENTO / 1000); 
    Serial.println(F(" seg."));
  }
}

void actualizarLCD() {
  lcd.setCursor(0, 0);
  lcd.print(F("T:"));
  lcd.print(temperaturaC, 1);
  lcd.print(F("C Ha:"));
  lcd.print(humedadAmbiente);
  lcd.print(F("%   ")); 

  lcd.setCursor(0, 1);
  lcd.print(F("S:"));
  lcd.print(humedadPct);
  lcd.print(F("% "));
  lcd.print(estadoHumedad);
  
  int caracteresOcupados = 4 + String(humedadPct).length() + estadoHumedad.length();
  for (int i = caracteresOcupados; i < 13; i++) {
    lcd.print(F(" "));
  }

  lcd.setCursor(13, 1); 
  if (riegoAutomaticoActivo) {
    lcd.print(F("[A]"));
  } else {
    lcd.print(F("[M]")); 
  }
}
