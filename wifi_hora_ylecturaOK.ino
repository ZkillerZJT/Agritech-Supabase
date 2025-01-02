#include "Credentials.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <random>
#include <OneWire.h>
#include <string.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 35 //se esta utilizando puerto GPIO35 para el onewire
// Crear un generador de números aleatorios TEMPORAL
    std::random_device rd;  // Obtiene un valor aleatorio desde el hardware
    std::mt19937 gen(rd()); // Inicializa el generador con el valor aleatorio

    // Definir el rango de valores (20 a 28)
    std::uniform_real_distribution<float> dis(20.0, 28.0);
//valores a insertar(en fase de pruebas serán fijos)
// 1 para humedad, 2 para temperatura y 3 para luz.
OneWire oneWire(ONE_WIRE_BUS); // inicializar sensor de temperatura
DallasTemperature sensors(&oneWire);
// arrays to hold device address
DeviceAddress insideThermometer;

float lightSensor = 300.2;  // Example light sensor value
float temperature;
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

int convertirHum(int limiteHumedad,int sensorReading){
  // usar limite seco
  float porcentaje = (100-(((float)sensorReading/limiteHumedad)*100));// relativo a humedad, muestra cantidad de humedad
  return porcentaje;
}
void obtainTempSensor(){
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
}

String sensorPayloadJson(int humidity, float temperature, int light = 300) {
  // Create the JSON with sensor data
  String jsonData = R"({
        "sensor_id": "Arduino1",
        "light": )" + String(light) + R"(,
        "humidity": )" + String(humidity) + R"(,
        "temperature": )" + String(temperature) + R"(
    })";

  return jsonData;
}
void printDeviceAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) Serial.print(":"); // Formato de dirección legible
  }
}
float getRandomTemperature() {// sin uso por ahora
    // Generar y devolver un número aleatorio en el rango especificado
    return dis(gen);
}
// function to print the temperature for a device
float printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  
  if(tempC == DEVICE_DISCONNECTED_C) 
  {
    Serial.println("Error: Could not read temperature data");
    return 0;
  }
  
  Serial.print("Temp C: ");
  Serial.print(tempC);
  
  Serial.print(" Temp F: ");
  Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
  return tempC;
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
    while  ((millis()-startTime) < timeoutDuration){
      if (httpResponseCode > 0) {
        //String response = http.getString();
        Serial.print("La respuesta es: ");
        Serial.println(String(httpResponseCode));
        Serial.println("Datos enviados: " + response);
      } 
      else {
        Serial.print("timeout!");
        Serial.println("Error al enviar datos: " + String(httpResponseCode));
        Serial.println("response code: " + response);
      }
      if((millis()-startTime) > timeoutDuration) {
        Serial.print("timeout!");
        Serial.println("codigo de respuesta: " + String(httpResponseCode));
        Serial.println("response: " + response);
        break;
      }
      delay(5000);
      break;
    }
    http.end();
  }
}
void wifiConnect(){
  unsigned long startTime = millis(); // Record the start time
  const unsigned long timeout = 8000; // Timeout duration in milliseconds (8 seconds)
  const unsigned long bigtimeout = 16000; // Timeout duration in milliseconds (8 seconds)
  WiFi.begin(ssid, pass);
  Serial.print("Intentando conectar a: ");
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startTime >= timeout) {
            Serial.println("\nTimeout reached. Retrying...");
            WiFi.disconnect();
            WiFi.begin(ssid,pass); // Restart the ESP32 if connection fails
            // Alternatively, you could call WiFi.disconnect() and WiFi.begin() again here instead of restarting.
    }
    if (millis() - startTime >= bigtimeout) {
            Serial.println("\nBIG Timeout reached. Restarting...");
            ESP.restart(); // Restart the ESP32 if connection fails
            // Alternatively, you could call WiFi.disconnect() and WiFi.begin() again here instead of restarting.
    }
  }
}
void setup() {
  Serial.begin(115200);
  wifiConnect();
  
  pinMode(humSensorID, INPUT);
  pinMode(humSensor2ID, INPUT);
  sensors.begin();// iniciar el onewire
  // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  
  // Search for devices on the bus and assign based on an index.
  if (!sensors.getAddress(insideThermometer, 0)) {
    Serial.println("Unable to find address for Device 0"); 
    return; // Salir si no se encuentra el dispositivo
  }

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printDeviceAddress(insideThermometer);
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 9);
 
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Serial.println();
}
void loop() {
  obtainData(humSensor, humSensorID);//puerto 32
  sensors.requestTemperatures(); // Send the command to get temperatures
  //obtainData(humSensor2, humSensor2ID); //puerto 35
  float temperature=printTemperature(insideThermometer); // imprime la temp. tambien se le puede asignar a un valor, función tipo float.
  showHumedad(humSensor,dry32,wet32); // esta wea no está imprimiendo nada
  int humedad = convertirHum(dry32,humSensor);
  //showHumedad(humSensor2,dry35,wet35);
  //String jsonPayload = sensorPayloadJson(humSensor);  // Solo incluye humedad
  Serial.print(String(sensorPayloadJson(humedad, 25 ,lightSensor)));//agregar la temperatura, en un valor previamente asociado
  httpSetup(String(sensorPayloadJson(humedad,25,lightSensor)), endpoint);
  //Serial.print(sensors.getTempCByIndex(0));// no muy seguro de si 0 sea el index adecuado
  Serial.print("finalizando bucle.");
  delay(60000);
  //Serial.print("fin de bucle");
}
