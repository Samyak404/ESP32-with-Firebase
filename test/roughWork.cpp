void loop()
{

  if (Firebase.ready() && (millis() - sendDataPrevMillis > idleTimeForStream || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    count++;
    //Due to single FirebaseData object used, stream connection will be interruped to send/receive data
    Serial.printf("Set string... %s\n\n", Firebase.setString(fbdo, "/test/stream/data", "Hello World! " + String(count)) ? "ok" : fbdo.errorReason().c_str());
  }

  if (Firebase.ready())
  {

    if (!Firebase.readStream(fbdo))
      Serial.printf("sream read error, %s\n\n", fbdo.errorReason().c_str());

    if (fbdo.streamTimeout())
    {
      Serial.println("stream timed out, resuming...\n");

      if (!fbdo.httpConnected())
        Serial.printf("error code: %d, reason: %s\n\n", fbdo.httpCode(), fbdo.errorReason().c_str());
    }

    if (fbdo.streamAvailable())
    {
      Serial.printf("sream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\nvalue, %s\n\n",
                    fbdo.streamPath().c_str(),
                    fbdo.dataPath().c_str(),
                    fbdo.dataType().c_str(),
                    fbdo.eventType().c_str(),
                    fbdo.stringData().c_str());
    }
  }
}