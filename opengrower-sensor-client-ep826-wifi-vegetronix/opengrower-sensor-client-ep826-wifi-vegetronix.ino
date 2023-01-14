
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define DEBUG true

#define SERVER_ADDRESS "192.168.1.100:8080"
#define STASSID ""
#define STAPSK  ""

// digital pins for LEDs
#define LED_BUILTIN D0
#define LED_BUILTIN_2 D4

// digital pins to turn sensors on/off
#define TEMPERATURE_D D1
#define MOISTURE_D D2
#define HUMIDITY_D D3
#define LIGHT_D D5

// analog pins for reading sensor data
#define TEMPERATURE_A A0
#define MOISTURE_A A0
#define HUMIDITY_A A0
#define LIGHT_A A0

const char* ssid = STASSID;
const char* password = STAPSK;

const char* host = SERVER_ADDRESS;

class Sensor {
  private:
    String id;
    float temperature; // degrees farenheit
    float moisture;    // % volumetric water content
    float humidity;    // % relative humidity
    float light;       // lux

  public:
    Sensor(String i) {
      id = i;
      temperature = 0.0;
      moisture = 0.0;
      humidity = 0.0;
      light = 0.0;
    }

    void set_temperature(float t) {
      temperature = t;
    }
    void set_moisture(float m) {
      moisture = m;
    }
    void set_humidity(float h) {
      humidity = h;
    }
    void set_light(float l) {
      light = l;
    }

    float get_temperature() {
      return temperature;
    }
    float get_moisture() {
      return moisture;
    }
    float get_humidity() {
      return humidity;
    }
    float get_light() {
      return light;
    }

    String to_json() {
      return "{\"sensor\":\"" + id + "\","
             + "\"temperature\":" + temperature + ","
             + "\"moisture\":" + moisture + ","
             + "\"humidity\":" + humidity + ","
             + "\"light\":" + light
             + "}";
    }

    String to_string() {
      return "sensor" + id + ", "
             + "temperature: " + temperature + ", "
             + "moisture: " + moisture + ", "
             + "humidity: " + humidity + ", "
             + "light: " + light;
    }
};

class SensorReader {
  private:
    bool READ_TEMPERATURE = false;
    bool READ_MOISTURE = true;
    bool READ_HUMIDITY = false;
    bool READ_LIGHT = false;

  public:
    SensorReader(Sensor &sensor) {
      if (READ_TEMPERATURE) {
        digitalWrite(TEMPERATURE_D, HIGH);
        delay(500);
        float temperature = analogRead(TEMPERATURE_A) * (5.0 / 1023.0);
        sensor.set_temperature(temperature * 75.006 - 40);
        digitalWrite(TEMPERATURE_D, LOW);
      }

      if (READ_MOISTURE) {
        digitalWrite(MOISTURE_D, HIGH);
        delay(500);
        float moisture = analogRead(MOISTURE_A) * (5.0 / 1023.0);
        if (moisture >= 0.0 && moisture < 1.1) {
          sensor.set_moisture(10 * moisture - 1);
        } else if (moisture >= 1.1 && moisture < 1.3) {
          sensor.set_moisture(25 * moisture - 17.5);
        } else if (moisture >= 1.3 && moisture < 1.82) {
          sensor.set_moisture(48.08 * moisture - 47.5);
        } else if (moisture >= 1.82 && moisture < 2.2) {
          sensor.set_moisture(26.32 * moisture - 7.89);
        } else if (moisture >= 2.2 && moisture <= 3.0) {
          sensor.set_moisture(62.5 * moisture - 87.5);
        } else {
          sensor.set_moisture(0.0);
        }
        digitalWrite(MOISTURE_D, LOW);
      }

      if (READ_HUMIDITY) {
        digitalWrite(HUMIDITY_D, HIGH);
        delay(500);
        float humidity = analogRead(HUMIDITY_A) * (5.0 / 1023.0);
        sensor.set_humidity(humidity * 33.33);
        digitalWrite(HUMIDITY_D, LOW);
      }

      if (READ_LIGHT) {
        digitalWrite(LIGHT_D, HIGH);
        delay(500);
        float light = analogRead(LIGHT_A)  * (5.0 / 1023.0);
        sensor.set_light(light * 50000);
        digitalWrite(LIGHT_D, LOW);
      }

    }
};

Sensor sensor("sensor001");

void setup() {

  if (DEBUG) {
    Serial.begin(9600);
  }

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_BUILTIN_2, OUTPUT);
  pinMode(TEMPERATURE_D, OUTPUT);
  pinMode(MOISTURE_D, OUTPUT);
  pinMode(HUMIDITY_D, OUTPUT);
  pinMode(LIGHT_D, OUTPUT);

  pinMode(TEMPERATURE_A, INPUT_PULLUP);
  pinMode(MOISTURE_A, INPUT_PULLUP);
  pinMode(HUMIDITY_A, INPUT);
  pinMode(LIGHT_A, INPUT);

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LED_BUILTIN_2, HIGH);
  digitalWrite(TEMPERATURE_D, LOW);
  digitalWrite(MOISTURE_D, LOW);
  digitalWrite(HUMIDITY_D, LOW);
  digitalWrite(LIGHT_D, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

}

void loop() {
  SensorReader sensorReader(sensor);

  if ((WiFi.status() == WL_CONNECTED)) {
    digitalWrite(LED_BUILTIN_2, HIGH);
    WiFiClient client;
    HTTPClient http;
    http.begin(client, "http://" SERVER_ADDRESS "/measurement");
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(sensor.to_json());
    if (httpCode != 200) {
      if (DEBUG) {
        Serial.println("server error code: " + String(httpCode));
      }
    }
    http.end();
    digitalWrite(LED_BUILTIN_2, LOW);
  }

  if (DEBUG) {
    Serial.println(sensor.to_json());
  }

}