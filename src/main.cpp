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
Servo servoMotor1;
Servo servoMotor2;

const int servoPinNumber1 = 13;
const int servoPinNumber2 = 12;

// Define Firebase Data For Servo Motor
FirebaseData servoMotor;

// Define Firebase Data For 1st Dc motor
FirebaseData motor1Rotation;
FirebaseData motor1Speed;
FirebaseData motor1State;

//Define pinout for 1st Dc motor. Pin Numbers For Arduino Framework
const int motor1PWM = 32;
const int channel1 = 4;
const int m1CwDirection = 23;
const int m1CcwDirection = 22;

// Define Firebase Data For 1st Dc motor
FirebaseData motor2Rotation;
FirebaseData motor2Speed;
FirebaseData motor2State;

//Define pinout for 2st Dc motor. Pin Numbers For Arduino Framework
const int motor2PWM = 33;
const int channel2 = 5;
const int m2CwDirection = 3;
const int m2CcwDirection = 21;

FirebaseAuth auth;
FirebaseConfig config;

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


// For Servo Motors
  if (!Firebase.RTDB.beginStream(&servoMotor, "/Servo Motor"))
    {Serial.printf("stream begin error, %s\n\n", servoMotor.errorReason().c_str());}
  

// For 1st DC Motor
  if (!Firebase.RTDB.beginStream(&motor1Rotation, "/DC Motor/Motor 1/Rotation"))
    {Serial.printf("sream begin error, %s\n\n", motor1Rotation.errorReason().c_str());}

  if (!Firebase.RTDB.beginStream(&motor1Speed, "/DC Motor/Motor 1/Speed"))
    {Serial.printf("sream begin error, %s\n\n", motor1Speed.errorReason().c_str());}

  if (!Firebase.RTDB.beginStream(&motor1State, "/DC Motor/Motor 1/State"))
    {Serial.printf("sream begin error, %s\n\n", motor1State.errorReason().c_str());}



// For 2st DC Motor
  if (!Firebase.RTDB.beginStream(&motor2Rotation, "/DC Motor/Motor 2/Rotation"))
    {Serial.printf("sream begin error, %s\n\n", motor2Rotation.errorReason().c_str());}

  if (!Firebase.RTDB.beginStream(&motor2Speed, "/DC Motor/Motor 2/Speed"))
    {Serial.printf("sream begin error, %s\n\n", motor2Speed.errorReason().c_str());}

  if (!Firebase.RTDB.beginStream(&motor2State, "/DC Motor/Motor 2/State"))
    {Serial.printf("sream begin error, %s\n\n", motor2State.errorReason().c_str());}


  // For Servo motors
  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	servoMotor1.setPeriodHertz(50);    // standard 50 hz servo
	servoMotor1.attach(servoPinNumber1, 500, 2500);
	servoMotor2.setPeriodHertz(50);
	servoMotor2.attach(servoPinNumber2, 1400, 2425);

  // DC Motor Decleration

  // For DC motor PWM 1
  ledcSetup(channel1, frequency, resolution);
  ledcAttachPin(motor1PWM, channel1);

  pinMode(m1CwDirection, OUTPUT);
  pinMode(m1CcwDirection, OUTPUT);


  // For DC motor PWM 2
  ledcSetup(channel2, frequency, resolution);
  ledcAttachPin(motor2PWM, channel2);

  pinMode(m2CwDirection, OUTPUT);
  pinMode(m2CcwDirection, OUTPUT);

}

void loop() {
  if (Firebase.ready())
  {
    
//  For Servo Motors
      Firebase.RTDB.readStream(&servoMotor);
      servoMotor.httpConnected();

    if (servoMotor.streamAvailable())
    {
      if (servoMotor.intData() == 1)
      {
        servoMotor1.write(84);
        servoMotor2.write(0);
      }
      else if (servoMotor.intData() == 0)
      {
        servoMotor1.write(0);
        servoMotor2.write(180);
      }
    }

//  For DC Motor 1
      Firebase.RTDB.readStream(&motor1Rotation);
      motor1Rotation.httpConnected();

      Firebase.RTDB.readStream(&motor1Speed);
      motor1Speed.httpConnected();

      Firebase.RTDB.readStream(&motor1State);
      motor1State.httpConnected();


    if (motor1State.intData() == 1)
    {
      if (motor1Rotation.intData() == 1)
      {
        digitalWrite(m1CwDirection, LOW);
        digitalWrite(m1CcwDirection, HIGH);
        ledcWrite(channel1, motor1Speed.intData() * 255 / 100);
      }
      else if (motor1Rotation.intData() == 0)
      {
        digitalWrite(m1CcwDirection, LOW);
        digitalWrite(m1CwDirection, HIGH);
        ledcWrite(channel1, motor1Speed.intData() * 255 / 100);
      }
    }
    else if (motor1State.intData() == 0)
    {
      digitalWrite(m1CcwDirection, LOW);
      digitalWrite(m1CwDirection, LOW);
    }



//  For DC Motor 2
      Firebase.RTDB.readStream(&motor2Rotation);
      motor2Rotation.httpConnected();

      Firebase.RTDB.readStream(&motor2Speed);
      motor2Speed.httpConnected();

      Firebase.RTDB.readStream(&motor2State);
      motor2State.httpConnected();


    if (motor2State.intData() == 1)
    {
      if (motor2Rotation.intData() == 1)
      {
        digitalWrite(m2CwDirection, LOW);
        digitalWrite(m2CcwDirection, HIGH);
        ledcWrite(channel2, motor2Speed.intData() * 255 / 100);
      }
      else if (motor2Rotation.intData() == 0)
      {
        digitalWrite(m2CcwDirection, LOW);
        digitalWrite(m2CwDirection, HIGH);
        ledcWrite(channel2, motor2Speed.intData() * 255 / 100);
      }
    }
    else if (motor2State.intData() == 0)
    {
      digitalWrite(m2CcwDirection, LOW);
      digitalWrite(m2CwDirection, LOW);
    }
  }
}