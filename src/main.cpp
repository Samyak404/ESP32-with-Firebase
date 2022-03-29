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
FirebaseData dcMotor1;

//Define pinout for 1st Dc motor. Pin Numbers For Arduino Framework
#define motor1PWM 32
#define channel1 4
#define m1CwDirection 27
#define m1CcwDirection 14

String parentPath = "/DC Motor/Motor 1";
String childPath[3] = {"/Rotation", "/Speed", "/State"};

// For On Board LED
const int led = 2;

FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

int count = 0;

volatile bool dataChanged = false;

volatile bool rotation, state;

int speed;


void streamCallback(MultiPathStream dcMotor1)
{
  size_t numChild = sizeof(childPath) / sizeof(childPath[0]);

  for (size_t i = 0; i < numChild; i++)
  {
    if (dcMotor1.get(childPath[i]))
    {
      if (i == 0)
      {
        dcMotor1.value.toInt() == 1 ? rotation = true : rotation = false;
      }

      if (i == 1)
      {
        speed = dcMotor1.value.toInt();
      }

      if (i == 2)
      {
        dcMotor1.value.toInt() == 1 ? state = true : state = false;
      }
    }
  }

  Serial.println();

  Serial.printf("Received stream payload size: %d (Max. %d)\n\n", dcMotor1.payloadLength(), dcMotor1.maxPayloadLength());

  dataChanged = true;
}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
    Serial.println("stream timed out, resuming...\n");

  if (!dcMotor1.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", dcMotor1.httpCode(), dcMotor1.errorReason().c_str());
}


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
  if (!Firebase.RTDB.beginMultiPathStream(&dcMotor1, parentPath))
    Serial.printf("sream begin error, %s\n\n", dcMotor1.errorReason().c_str());

  Firebase.RTDB.setMultiPathStreamCallback(&dcMotor1, streamCallback, streamTimeoutCallback);




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

  pinMode(led, OUTPUT);
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
  }

  if (dataChanged)
  {
    dataChanged = false;
    Serial.printf("Rotation: %d \nSpeed: %d \nState: %d \n\n", rotation, speed, state);
    
    if (state)
    {
      if (rotation)
      {
        ledcWrite(channel1, speed * 255 / 100);
        digitalWrite(m1CcwDirection, LOW);
        digitalWrite(m1CwDirection, HIGH);
      }
      else if (!rotation)
      {
        ledcWrite(channel1, speed * 255 / 100);
        digitalWrite(m1CwDirection, LOW);
        digitalWrite(m1CcwDirection, HIGH);
      }
    }
    else if(!state)
    {
      digitalWrite(m1CwDirection, LOW);
      digitalWrite(m1CcwDirection, LOW);
    }
  }
}