#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Arduino_JSON.h>


//DHT
#define DHTPIN 7
#define DHTTYPE DHT11

//MQTT server conf
const char *MQTT_SERVER= "34.42.136.255";
const int MQTT_PORT = 1883;

// Wifi conf 
const char *WIFI_SSID = "cslab";
const char *WIFI_PASSWORD = "aksesg31";


const int MOISTURE_SENSOR_PIN = A3;

// Thresholds for watering 
const float MOISTURE_THRESHOLD = 0.4;

const int PIR_SENSOR = 42; // Pin connected to OUTPUT pin of PIR sensor
int pinStateCurrent   = LOW;  // current state of pin
int pinStatePrevious  = LOW;  // previous state of pin

///////////////////////////
// Class instantiation
///////////////////////////

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

//Variables
int DO = 0;
int tmp = 0;

// MQTT topics
const char *MQTT_TOPIC_SHELTER = "shelter";

void setup_wifi()
{
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
  delay(500);
  Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void setup()
{
  Serial.begin(9600);
  dht.begin();
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  pinMode(MOISTURE_SENSOR_PIN, INPUT);
  pinMode(PIR_SENSOR, INPUT);
  delay(2000);
}


void reconnect()
{
  while (!client.connected())
  {
      Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32Client"))
    {
      Serial.println("Connected to MQTT server");
    }
    else
    {
    Serial.print("Failed, rc=");
    Serial.print(client.state());
    Serial.println(" Retrying in 5 seconds...");
    delay(5000);
    }
  }
}

void loop()
{

///////////////////////////////////////
  delay(1000);
  if (!client.connected())
  {
    reconnect();
  } 
  client.loop();
  delay(5000);

///////////////////////////////////////

  //read the moisture content in %
  float humidity = dht.readHumidity();
  
  //read the temp in celcius
  float temperature = dht.readTemperature();

  float moistureLevel = 1.00 - (analogRead(MOISTURE_SENSOR_PIN) / 4095.00);

  pinStatePrevious = pinStateCurrent; // store old state
  pinStateCurrent = digitalRead(PIR_SENSOR);   // read new state

  if (pinStatePrevious == LOW && pinStateCurrent == HIGH) {   // pin state change: LOW -> HIGH
    Serial.println("Motion detected!");
    // TODO: turn on alarm, light or activate a device 
  }
  else
  if (pinStatePrevious == HIGH && pinStateCurrent == LOW) {   // pin state change: HIGH -> LOW
    Serial.println("Motion stopped!");
    // TODO: turn off alarm, light or deactivate a device 
  }

  //Check the moisture of the plant
  Serial.println(MOISTURE_THRESHOLD);
  if (moistureLevel < MOISTURE_THRESHOLD) {
    Serial.println("The plant is too dry!");
  } else {
    Serial.println("Plant have enough water");
  }

  //Check the temperature 
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed reception");
    return;
    //Returns an error if the ESP32 does not receive any measurements
  }



  // Print to serial////////////
  Serial.print("Humidity: "); Serial.print(humidity);
  Serial.print("%  Temperature: "); Serial.print(temperature); Serial.print("°C, ");
  Serial.print("Moisture Data: ");
  Serial.print(moistureLevel);
  Serial.print(" / ");


  // Construct payload to send to GCP//
  delay(500);
  JSONVar payload;

  payload["temperature"] = temperature;
  payload["humidity"] = humidity;
  payload["moisturelevel"] = MOISTURE_THRESHOLD;


  Serial.println(payload);

  // Send combined JSON data in one topic
  client.loop();
  client.publish(MQTT_TOPIC_SHELTER, JSON.stringify(payload).c_str());

}