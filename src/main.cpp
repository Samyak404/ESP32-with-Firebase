#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "007"
#define WIFI_PASSWORD "samyak000"

// Insert Firebase project API Key
#define API_KEY "AIzaSyDST6dAJKUZUcTG7tDX7r9c_XdPeolxqV0"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "h-drone-default-rtdb.firebaseio.com"

// PWM Values
const int frequency = 1000;
const int resolution = 8;

// Servo Motor define
Servo motor1;
Servo motor2;

int servoPinNumber1 = 13;
int servoPinNumber2 = 12;

//Define Firebase Data object
FirebaseData servoMotor;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

int count = 0;

uint32_t idleTimeForStream = 15000;

bool signupOK = false;

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (!Firebase.RTDB.beginStream(&servoMotor, "/Servo Motor"))
  {
    Serial.printf("stream begin error, %s\n\n", servoMotor.errorReason().c_str());
  }

  // For Servo motors
  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	motor1.setPeriodHertz(50);    // standard 50 hz servo
	motor1.attach(servoPinNumber1, 500, 2500);
	motor2.setPeriodHertz(50);
	motor2.attach(servoPinNumber2, 1400, 2425);

  
}

void loop() {
  if (Firebase.ready())
  {

    if (!Firebase.RTDB.readStream(&servoMotor))
      Serial.printf("stream read error, %s\n\n", servoMotor.errorReason().c_str());

    if (servoMotor.streamTimeout())
    {
      Serial.println("stream timed out, resuming...\n");

      if (!servoMotor.httpConnected())
        Serial.printf("error code: %d, reason: %s\n\n", servoMotor.httpCode(), servoMotor.errorReason().c_str());
    }

    if (servoMotor.streamAvailable())
    {
      if (servoMotor.intData() == 1)
      {
        motor1.write(84);
        motor2.write(0);
      }
      else if (servoMotor.intData() == 0)
      {
        motor1.write(0);
        motor2.write(180);
      }
    }
  }
}