#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

// Configuración del servidor web
ESP8266WebServer server(80);

// Pines de los relés
const int relayPins[] = {16, 5, 4, 2, 14, 12, 13, 15}; // GPIO16, GPIO5, GPIO4, etc.
bool relayStates[8] = {false, false, false, false, false, false, false, false}; // Estados iniciales de los relés

// Variables para manejar la hora de encendido y apagado
int hourOn = 6;     // Hora de encendido por defecto (6:00 AM)
int minuteOn = 0;   // Minuto de encendido por defecto
int hourOff = 22;   // Hora de apagado por defecto (10:00 PM)
int minuteOff = 0;  // Minuto de apagado por defecto

// Variables para evitar imprimir varias veces la hora de apagado
bool lightsTurnedOff = false;  // Para evitar imprimir varias veces la hora de apagado

// Credenciales Wi-Fi
const char* ssid = "INFINITUM1ABB";       // Cambia por tu SSID
const char* password = "ecykj4tDxu"; // Cambia por tu contraseña

// Configuración de NTP para sincronización horaria (hora estándar de la Ciudad de México: UTC -6)
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 0, 60000); // UTC sin ajuste para la zona horaria

// Página HTML con formulario para cambiar horarios
String getHTML() {
  String html = "<!DOCTYPE html><html lang='es'><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Panel de Control Focos Col. Vallejo</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; padding: 20px; background-image: url('img/lldm_vallejo_iluminado.jpg'); background-size: cover; background-position: center; background-repeat: no-repeat; height: 100vh; margin: 0; color: #333; }";
  html += ".title-container { background-color: rgba(128, 128, 128, 0.4); padding: 20px; border-radius: 5px; margin-bottom: 40px; }";
  html += "h1 { color: #ffffff; margin: 0; }";
  html += ".button-panel { display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px; justify-content: center; }";
  html += "button { padding: 10px; font-size: 18px; border: none; cursor: pointer; border-radius: 5px; transition: background-color 0.3s, transform 0.3s; opacity: 0.7; }";
  html += "button:hover { transform: scale(1.05); opacity: 1; }";
  html += ".on { background-color: green; color: white; }";
  html += ".off { background-color: red; color: white; }";
  html += ".form-container { margin-top: 40px; }";
  html += "input { padding: 8px; font-size: 16px; margin: 5px; }";
  html += "</style>";
  html += "</head><body>";

  html += "<div class='title-container'><h1>Panel de Control Focos Col. Vallejo</h1></div>";
  
  html += "<div class='button-panel'>";
  for (int i = 0; i < 8; i++) {
    html += "<button id='relay" + String(i) + "' class='" + (relayStates[i] ? "on" : "off") + "' onclick='toggleRelay(" + String(i) + ")'>Foco " + String(i + 1) + "</button>";
  }
  html += "</div>";

  // Formulario para configurar horarios
  html += "<div class='form-container'>";
  html += "<h2>Configurar Horarios de Encendido/Apagado</h2>";
  html += "<form action='/setTime' method='get'>";
  html += "<label>Hora Encendido:</label><input type='number' name='hourOn' value='" + String(hourOn) + "' min='0' max='23' required>";
  html += "<label>Minuto Encendido:</label><input type='number' name='minuteOn' value='" + String(minuteOn) + "' min='0' max='59' required><br>";
  html += "<label>Hora Apagado:</label><input type='number' name='hourOff' value='" + String(hourOff) + "' min='0' max='23' required>";
  html += "<label>Minuto Apagado:</label><input type='number' name='minuteOff' value='" + String(minuteOff) + "' min='0' max='59' required><br>";
  html += "<button type='submit'>Guardar</button>";
  html += "</form>";
  html += "</div>";

  html += "<script>";
  html += "function toggleRelay(relay) {";
  html += "fetch('/toggle?relay=' + relay);"; // Se agrega un parámetro para el número del relé
  html += "setTimeout(() => location.reload(), 500);";  // Refrescar la página al cambiar el estado
  html += "}</script>";

  html += "</body></html>";
  return html;
}

// Maneja la página principal
void handleRoot() {
  server.send(200, "text/html", getHTML());
}

// Maneja el encendido/apagado de relés
void handleToggleRelay() {
  if (server.hasArg("relay")) {
    int relay = server.arg("relay").toInt();
    if (relay >= 0 && relay < 8) {
      relayStates[relay] = !relayStates[relay];  // Cambiar el estado del relé
      digitalWrite(relayPins[relay], relayStates[relay] ? LOW : HIGH); // LOW enciende los relés de low level trigger
    }
  }
  server.send(200, "text/plain", "OK");
}

// Maneja la configuración de la hora de encendido y apagado
void handleSetTime() {
  if (server.hasArg("hourOn")) {
    hourOn = server.arg("hourOn").toInt();
  }
  if (server.hasArg("minuteOn")) {
    minuteOn = server.arg("minuteOn").toInt();
  }
  if (server.hasArg("hourOff")) {
    hourOff = server.arg("hourOff").toInt();
  }
  if (server.hasArg("minuteOff")) {
    minuteOff = server.arg("minuteOff").toInt();
  }
  
  server.send(200, "text/html", "<h2>Horarios actualizados. <a href='/'>Regresar</a></h2>");
}

void setup() {
  // Inicia la comunicación serial
  Serial.begin(115200);  // Asegúrate de que el Monitor Serial esté configurado a 115200 baudios

  // Configuración de los pines
  for (int i = 0; i < 8; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH); // Inicialmente apagados (relés desactivados con HIGH)
  }

  // Conexión Wi-Fi
  Serial.println("Conectando a Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Conectado a Wi-Fi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Inicializar NTPClient
  timeClient.begin();
  Serial.println("NTPClient iniciado");

  // Configuración del servidor web
  server.on("/", handleRoot);  // Página principal
  server.on("/toggle", handleToggleRelay);  // Control manual de relés
  server.on("/setTime", handleSetTime);  // Configuración de horarios
  server.begin();
}

void loop() {
  // Actualizar hora
  timeClient.update();
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();

  // Ajuste de hora para zona horaria de Ciudad de México (UTC -6)
  currentHour = (currentHour - 6) % 24; 

  // Controla los relés según la hora configurada
  if (currentHour == hourOn && currentMinute == minuteOn) {
    for (int i = 0; i < 8; i++) {
      if (!relayStates[i]) {  // Si el relé está apagado, encenderlo
        relayStates[i] = true;
        digitalWrite(relayPins[i], LOW); 
      }
    }
  } else if (currentHour == hourOff && currentMinute == minuteOff && !lightsTurnedOff) {
    for (int i = 0; i < 8; i++) {
      if (relayStates[i]) {  // Si el relé está encendido, apagarlo
        relayStates[i] = false;
        digitalWrite(relayPins[i], HIGH);
      }
    }
    lightsTurnedOff = true;
  } else if (currentHour != hourOff || currentMinute != minuteOff) {
    lightsTurnedOff = false;
  }

  server.handleClient();
}
