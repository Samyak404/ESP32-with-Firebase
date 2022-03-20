#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define WIFI_SSID "007"
#define WIFI_PASSWORD "samyak000"

#define API_KEY "AIzaSyDST6dAJKUZUcTG7tDX7r9c_XdPeolxqV0"

#define DATABASE_URL "h-drone-default-rtdb.firebaseio.com"

// PWM Values
const int frequency = 1000;
const int resolution = 8;

// Define Servo motors
FirebaseData servoMotor;

Servo motor1;
Servo motor2;

int servoPinNumber1 = 13;
int servoPinNumber2 = 12;

//Define Firebase Data object and esp32 pinNumbers
// 1st DC motor parameters
FirebaseData m1Rotation;    //For firebase
FirebaseData m1Speed;
FirebaseData m1State;

const int motor1PWM = 32;   //Pin Numbers For Arduino Framework
const int channel1 = 4;
const int m1CwDirection = 23;
const int m1CcwDirection = 22;

// 2st DC motor parameters
FirebaseData m2Rotation;    //For firebase
FirebaseData m2Speed;
FirebaseData m2State;

const int motor2PWM = 35;   //Pin Numbers For Arduino Framework
const int channel2 = 7;
const int m2CwDirection = 1;
const int m2CcwDirection = 3;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

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

  // DC Motor Decleration

  // For DC motor PWM 1
  ledcSetup(channel1, frequency, resolution);
  ledcAttachPin(motor1PWM, channel1);

  // For DC motor PWM 2
  ledcSetup(channel2, frequency, resolution);
  ledcAttachPin(motor2PWM, channel2);

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


void dcMotor1 (int direction, int speed, int state){

  speed = speed * 255 / 100;

  digitalWrite(direction, state);
  ledcWrite(channel1, speed);

}


void loop() {
 if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 50 || sendDataPrevMillis == 0)) 
 {
    sendDataPrevMillis = millis();

// For 1st dc motor

    if (Firebase.RTDB.getInt(&m1Rotation, "/DC Motor/Motor 1/Rotation") || 
        Firebase.RTDB.getInt(&m1Speed, "/DC Motor/Motor 1/Speed") || 
        Firebase.RTDB.getInt(&m1State, "/DC Motor/Motor 1/State")    ) 
    {
        if (m1Rotation.intData() == 1)
        {
          dcMotor1(m1CcwDirection, m1Speed.intData(), m1State.intData());
        }
        else if (m1Rotation.intData() == 0)
        {
          dcMotor1(m1CwDirection, m1Speed.intData(), m1State.intData());
        }
    }

// For 2nd dc motor

    if (Firebase.RTDB.getInt(&m2Rotation, "/DC Motor/Motor 2/Rotation") || 
        Firebase.RTDB.getInt(&m2Speed, "/DC Motor/Motor 2/Speed") || 
        Firebase.RTDB.getInt(&m2State, "/DC Motor/Motor 2/State")) 
    {
        if (m2Rotation.intData() == 1)
        {
          dcMotor1(m2CcwDirection, m2Speed.intData(), m2State.intData());
        }
        else if (m2Rotation.intData() == 0)
        {
          dcMotor1(m2CwDirection, m2Speed.intData(), m2State.intData());
        }
    }

// For Servo Motors

    if (Firebase.RTDB.getInt(&servoMotor, "/Servo Motor")) 
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