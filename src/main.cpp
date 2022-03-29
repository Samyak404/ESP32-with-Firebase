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

#define servoPinNumber1 13
#define servoPinNumber2 12

// Define Firebase Data For Servo Motor
FirebaseData servoMotor;

// Define Firebase Data For 1st Dc motor
FirebaseData dcMotor1;

//Define pinout for 1st Dc motor. Pin Numbers For Arduino Framework
#define motor1PWM 32
#define channel1 4
#define m1CwDirection 27
#define m1CcwDirection 14

String parentPathForM1 = "/DC Motor/Motor 1";
String childPathForM1[3] = {"/Rotation", "/Speed", "/State"};


// Define Firebase Data For 1st Dc motor
FirebaseData dcMotor2;

//Define pinout for 2nd Dc motor. Pin Numbers For Arduino Framework
#define motor2PWM 33
#define channel2 5
#define m2CwDirection 2
#define m2CcwDirection 15

String parentPathForM2 = "/DC Motor/Motor 2";
String childPathForM2[3] = {"/Rotation", "/Speed", "/State"};

FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

// DC Motor 1 Attributes
volatile bool m1DataChanged = false;
volatile bool m1Rotation, m1State;
int m1Speed;

// DC Motor 2 Attributes
volatile bool m2DataChanged = false;
volatile bool m2Rotation, m2State;
int m2Speed;

// CallBack function for 1st dc motor
void streamCallbackForM1(MultiPathStream dcMotor1)
{
  size_t numChild = sizeof(childPathForM1) / sizeof(childPathForM1[0]);

  for (size_t i = 0; i < numChild; i++)
  {
    if (dcMotor1.get(childPathForM1[i]))
    {
      if (i == 0)
      {
        dcMotor1.value.toInt() == 1 ? m1Rotation = true : m1Rotation = false;
      }

      if (i == 1)
      {
        m1Speed = dcMotor1.value.toInt();
      }

      if (i == 2)
      {
        dcMotor1.value.toInt() == 1 ? m1State = true : m1State = false;
      }
    }
  }

  Serial.println();

  Serial.printf("Received stream payload size: %d (Max. %d)\n\n", dcMotor1.payloadLength(), dcMotor1.maxPayloadLength());

  m1DataChanged = true;
}

void streamTimeoutCallbackForM1(bool timeout)
{
  if (timeout)
    Serial.println("stream timed out, resuming...\n");

  if (!dcMotor1.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", dcMotor1.httpCode(), dcMotor1.errorReason().c_str());
}


// CallBack function for 2nd dc motor
void streamCallbackForM2(MultiPathStream dcMotor2)
{
  size_t numChild = sizeof(childPathForM2) / sizeof(childPathForM2[0]);

  for (size_t i = 0; i < numChild; i++)
  {
    if (dcMotor2.get(childPathForM2[i]))
    {
      if (i == 0)
      {
        dcMotor2.value.toInt() == 1 ? m2Rotation = true : m2Rotation = false;
      }

      if (i == 1)
      {
        m2Speed = dcMotor2.value.toInt();
      }

      if (i == 2)
      {
        dcMotor2.value.toInt() == 1 ? m2State = true : m2State = false;
      }
    }
  }

  Serial.println();

  Serial.printf("Received stream payload size: %d (Max. %d)\n\n", dcMotor2.payloadLength(), dcMotor2.maxPayloadLength());

  m1DataChanged = true;
}

void streamTimeoutCallbackForM2(bool timeout)
{
  if (timeout)
    Serial.println("stream timed out, resuming...\n");

  if (!dcMotor2.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", dcMotor2.httpCode(), dcMotor2.errorReason().c_str());
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
  

// For 1st DC Motor connection between firebase object and stream
  if (!Firebase.RTDB.beginMultiPathStream(&dcMotor1, parentPathForM1))
    Serial.printf("stream begin error, %s\n\n", dcMotor1.errorReason().c_str());

  Firebase.RTDB.setMultiPathStreamCallback(&dcMotor1, streamCallbackForM1, streamTimeoutCallbackForM1);


// For 2nd DC Motor connection between firebase object and stream
  if (!Firebase.RTDB.beginMultiPathStream(&dcMotor2, parentPathForM2))
    Serial.printf("stream begin error, %s\n\n", dcMotor2.errorReason().c_str());

  Firebase.RTDB.setMultiPathStreamCallback(&dcMotor2, streamCallbackForM2, streamTimeoutCallbackForM2);


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
  }

  if (m1DataChanged)
  {
    m1DataChanged = false;
    Serial.printf("m1Rotation: %d \nm1Speed: %d \nm1State: %d \n\n", m1Rotation, m1Speed, m1State);
    
    if (m1State)
    {
      if (m1Rotation)
      {
        ledcWrite(channel1, m1Speed * 255 / 100);
        digitalWrite(m1CcwDirection, LOW);
        digitalWrite(m1CwDirection, HIGH);
      }
      else if (!m1Rotation)
      {
        ledcWrite(channel1, m1Speed * 255 / 100);
        digitalWrite(m1CwDirection, LOW);
        digitalWrite(m1CcwDirection, HIGH);
      }
    }
    else if(!m1State)
    {
      digitalWrite(m1CwDirection, LOW);
      digitalWrite(m1CcwDirection, LOW);
    }
  }

  if (m2DataChanged)
  {
    m2DataChanged = false;
    Serial.printf("m2Rotation: %d \nm2Speed: %d \nm2State: %d \n\n", m2Rotation, m2Speed, m2State);
    
    if (m2State)
    {
      if (m2Rotation)
      {
        ledcWrite(channel2, m2Speed * 255 / 100);
        digitalWrite(m2CcwDirection, LOW);
        digitalWrite(m2CwDirection, HIGH);
      }
      else if (!m2Rotation)
      {
        ledcWrite(channel2, m2Speed * 255 / 100);
        digitalWrite(m2CwDirection, LOW);
        digitalWrite(m2CcwDirection, HIGH);
      }
    }
    else if(!m2State)
    {
      digitalWrite(m2CwDirection, LOW);
      digitalWrite(m2CcwDirection, LOW);
    }
  }
}