#include "Credentials.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <string.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 17 //se esta utilizando puerto GPIO4 para el onewire
OneWire oneWire(ONE_WIRE_BUS); // inicializar sensor de temperatura
DallasTemperature sensors(&oneWire);
// arrays to hold device address
DeviceAddress insideThermometer;
int numberOfDevices;
float lightSensor;  // Example light sensor value port 16
float temperature;
int humSensor;              // Example pH sensor value
//int humSensor2;
int humSensorID = 33;
int lightID = 32;
//int humSensor2ID=;
//const int wetx = 2625;  //valor de sequedad para sensor en puerto 35
//const int dryx = 930;   //limite de humedad max para sensor en puerto 35
const int wet32=4096;
const int dry32=930;
/*
Actualmente se están utilizando los puertos
35 para temperatura
32 para humedad
27 para luz
*/



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

String sensorPayloadJson(int humidity, float temperature, int light) {
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

void tempSerialPrint(){
  // Loop through each device, print out temperature data
  numberOfDevices = sensors.getDeviceCount();
  for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if(sensors.getAddress(insideThermometer, i)){
      // Output the device ID
      Serial.print("Temperature for device: ");
      Serial.println(i,DEC);
      // Print the data
      float tempC = sensors.getTempC(insideThermometer);
      Serial.print("Temp C: ");
      Serial.print(tempC);
      Serial.print(" Temp F: ");
      Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
    }
  }
}

void httpSetup(String jsonData, const String link) {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Llamado a httprequest exitoso...");
        HTTPClient http;

        // Initialize HTTP request
        http.begin(link.c_str());
        Serial.print(link);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("apikey", apiKey);

        // Start timing
        unsigned long startTime = millis();
        //const unsigned long timeoutDuration = 20000; // Timeout of 6 seconds
        Serial.print("Continuando flujo... ");
        // Send the HTTP PUT request
        int httpResponseCode = http.PUT(jsonData);
        Serial.println(httpResponseCode);
        Serial.print("arriba responsecode");
        if (httpResponseCode == 204){
          Serial.print("Solicitud exitosa a base de datos: ");
          Serial.println(httpResponseCode);
          http.end();
          return;
        }
        // Check if the request was successful
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.print("La respuesta es: ");
            Serial.println(httpResponseCode);
            Serial.println("Datos enviados: " + response);
        } else {
            // Handle error response
            Serial.print("Error al enviar datos: ");
            Serial.println(httpResponseCode);
            Serial.println("Response code: " + http.getString());
        }

        // Check for timeout
        /*while (millis() - startTime < timeoutDuration) {
            // If the response is received, break out of the loop
            if (httpResponseCode >= 200 && httpResponseCode < 300) {

            String response = http.getString();
            Serial.print("La respuesta es: ");
            Serial.println(httpResponseCode);
              http.end();
                break;
            }
            delay(5000); // Small delay to prevent busy-waiting
        }*/

        // Final check after timeout duration
        /*if ((millis() - startTime) >= timeoutDuration) {
            Serial.println("Timeout after 20 seconds!");
            Serial.println("Código de respuesta: " + String(httpResponseCode));
            Serial.println("Response: " + http.getString());
        }*/

        // Clean up
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
  
  pinMode(lightID, INPUT);
  pinMode(humSensorID, INPUT);
  sensors.begin();// iniciar el onewire
  numberOfDevices = sensors.getDeviceCount();
  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
  // Loop through each device, print out address
  for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if(sensors.getAddress(insideThermometer, i)){
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printDeviceAddress(insideThermometer);
      Serial.println();
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }
  wifiConnect();
}
void loop() {
  // Obtener datos del sensor de humedad
    obtainData(humSensor, humSensorID); // Puerto 32

    // Solicitar temperatura
    sensors.requestTemperatures(); // Conectados a puerto 4

    // Leer el valor del sensor de luz
    int lightSensor = analogRead(lightID); // Puerto 27 para la luz
    int lightPercentage = map(lightSensor, 3600, 0, 100, 0);
    int humSensor = analogRead(humSensorID);
    int humidityPercentage = map(humSensor, 0, 4096, 100, 0);

    // Mostrar humedad
    showHumedad(humSensor, wet32, dry32); // Esta función no está imprimiendo nada

    // Convertir y almacenar el valor de humedad

    // Obtener temperatura en grados Celsius
    float tempC = sensors.getTempC(insideThermometer);

    // Imprimir los valores de los sensores en el puerto serial
    Serial.print("Humedad = ");
    Serial.println(humidityPercentage);
    
    Serial.print("Luz = ");
    Serial.println(lightSensor);
    
    Serial.print("Temperatura = ");
    Serial.println(tempC);

    // Crear y enviar el payload JSON (opcional)
    Serial.print(String(sensorPayloadJson(humidityPercentage, tempC, lightPercentage)));

    // Mensaje de finalización del bucle
    Serial.println("Finalizando bucle.");
    httpSetup(sensorPayloadJson(humidityPercentage, tempC, lightPercentage), endpoint);

    
    delay(30000);
}
