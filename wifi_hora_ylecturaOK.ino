#include "Credentials.h"
#include <time.h>
#include "sntp.h"
#include <WiFi.h>
#include <HTTPClient.h>

//valores a insertar(en fase de pruebas serán fijos)
int arduinoID = 1;  // Example Arduino ID
String sensorID = "Arduino1";
int plantID = 1;
int sensorTypeID = 1;
// 1 para humedad, 2 para temperatura y 3 para luz.
float lightSensor = 300.2;  // Example light sensor value
int humSensor;              // Example pH sensor value
int humSensor2;
int humSensorID = 32;
int humSensor2ID = 35;   //pin GPIO4 donde está conectado el sensor de temperatura
const int dry35 = 2625;  //valor de sequedad para sensor en puerto 35
const int wet35 = 930;   //limite de humedad max para sensor en puerto 35
const int dry32 = 2610;  //valor de sequedad para sensor en puerto 32
const int wet32 = 912;   //valor de humedad max para sensor en puerto 32
//este numero debe indicar el # de GPIO que se está utilizando, no el numero de pin, sino el numero logico de GPIO, por ejemplo para el GPIO14, la declaración debiera ser 14.

void obtainData(int &analogSensor, int sensorID) {
  analogSensor = analogRead(sensorID);
  Serial.print("Se lee: ");
  Serial.print(analogSensor);
  Serial.print(" en puerto: ");
  Serial.print(sensorID);
  Serial.print("\n");
}

void showHumedad(int analogSensor, int wet, int dry) {
  int intervals = (dry - wet) / 3;
  if (analogSensor > wet && analogSensor < (wet + intervals)) {
    Serial.println("Very Wet");
  } else if (analogSensor > (wet + intervals) && analogSensor < (dry - intervals)) {
    Serial.println("Wet");
  } else if (analogSensor < dry && analogSensor > (dry - intervals)) {
    Serial.println("Dry");
  }
}
String fecha;
String hora;
// NTP Server settings
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = -3 * 3600;  // Adjust for your timezone (e.g., -3 for GMT-3)
const int daylightOffset_sec = 0;

void obtenertlocal(String &fecha, String &hora) {  // funcion completada, pero sin usar
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Time not available yet...");
    return;
  }
  // Intento de formatear las variables fecha y hora
  fecha = String(timeinfo.tm_year + 1900) + "/" + String(timeinfo.tm_mon + 1) + "/" + String(timeinfo.tm_mday);
  // Asignar la hora en formato HH:MM:SS
  hora = String(timeinfo.tm_hour) + ":" + String(timeinfo.tm_min) + ":" + String(timeinfo.tm_sec);
  Serial.print("DEBUG: IN func() obtenertlocal \nFecha definida como: ");
  Serial.println(fecha);
  Serial.print("Hora como: ");
  Serial.println(hora);
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void timeavailable(struct timeval *t) {
  Serial.println("Got time adjustment from NTP!");
  obtenertlocal(fecha, hora);
}
int convertirHum(int limiteHumedad,int sensorReading){
  // usar limite seco
  float porcentaje = (100-(((float)sensorReading/limiteHumedad)*100));// relativo a humedad, muestra cantidad de humedad
  //Serial.print("Porcentaje de humedad: ");
  //Serial.print(porcentaje);
  return porcentaje;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000)
    ;
  WiFi.begin(ssid, pass);
  Serial.print("Intentando conectar a: ");
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

  }
  pinMode(humSensorID, INPUT);
  pinMode(humSensor2ID, INPUT);
  // print out info about the connection:
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  sntp_set_time_sync_notification_cb(timeavailable);
  sntp_servermode_dhcp(1);  // (optional)
  delay(3000);
}
/*String sensorReadingJson(int reading, int sensorID, int arduinoID, int plantID, int sensorTypeID) {
  // Obtener la fecha y hora actual en formato ISO 8601
  String createdAt = fecha + "T" + hora + ".280Z";  // Formato de fecha y hora
  String updatedAt = createdAt;  // Asumir que createdAt y updatedAt son iguales al momento de la creación

  // Crear el JSON con los datos del sensor
  String jsonData = "{";
  jsonData += "\"id\": " + String(sensorID) + ",";         // ID del sensor
  jsonData += "\"arduinoId\": " + String(arduinoID) + ","; // ID del Arduino
  jsonData += "\"plantId\": " + String(plantId) + ",";     // ID de la planta (puede ser un valor fijo o variable)
  jsonData += "\"sensorTypeId\": " + String(sensorTypeID) + ","; // ID del tipo de sensor
  jsonData += "\"createdAt\": \"" + createdAt + "\",";      // Fecha de creación
  jsonData += "\"updatedAt\": \"" + updatedAt + "\"";       // Fecha de actualización
  jsonData += "}";

  return jsonData;
}*/
String sensorPayloadJson(int humidity, float temperature = 28.0, int light = 300) {
  // Create the JSON with sensor data
  String jsonData = R"({
        "sensor_id": "Arduino1",
        "light": )" + String(light) + R"(,
        "humidity": )" + String(humidity) + R"(,
        "temperature": )" + String(temperature) + R"(
    })";

  return jsonData;
}
float getRandomTemperature() {// sin uso por ahora
    // Crear un generador de números aleatorios
    std::random_device rd;  // Obtiene un valor aleatorio desde el hardware
    std::mt19937 gen(rd()); // Inicializa el generador con el valor aleatorio

    // Definir el rango de valores (20 a 28)
    std::uniform_real_distribution<float> dis(20.0, 28.0);

    // Generar y devolver un número aleatorio en el rango especificado
    return dis(gen);
}

void httpSetup(String jsonData, const String link) {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(" llamado a httprequest exitoso...");
    HTTPClient http;
    // Crear la solicitud HTTP POST
    //String url = endpoint + "" + sensorID;//"?sensor_id=eq."
    http.begin(link.c_str());
    Serial.print(link);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", apiKey);
    // aqui se sube el json por cada sensor independiente
    //String jsonData = sensorJson(humSensor, humSensorID);
    //sensorJson(humSensor2, humSensor2ID);
    // Enviar la solicitud PUT
    unsigned long startTime = millis(); // Guardar el tiempo de inicio
    const unsigned long timeoutDuration = 6000; // Timeout de 6 segundos
    int httpResponseCode = http.PUT(jsonData);
    String response = http.getString();
    while  (millis()-startTime < timeoutDuration){
    if (httpResponseCode > 0) {

      //String response = http.getString();
      Serial.print("La respuesta es: ");
      Serial.println(String(httpResponseCode));
      
      Serial.println("Datos enviados: " + response);
    } else {
      Serial.print("timeout!");
      Serial.println("Error al enviar datos: " + String(httpResponseCode));
      Serial.println("response code: " + response);
    }
    }

    http.end();
  }
}
void loop() {
  obtainData(humSensor, humSensorID);//puerto 32
  //obtainData(humSensor2, humSensor2ID); //puerto 35
  showHumedad(humSensor,dry32,wet32);
  int humedad = convertirHum(dry32,humSensor);
  //showHumedad(humSensor2,dry35,wet35);
  //String jsonPayload = sensorPayloadJson(humSensor);  // Solo incluye humedad
  Serial.print(String(sensorPayloadJson(humedad)));
  httpSetup(String(sensorPayloadJson(humedad)), endpoint);
  Serial.print(String(sensorPayloadJson(humedad)));
  delay(10000);
  //Serial.print("fin de bucle");
}
