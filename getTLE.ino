void getTLE(int SAT){
  
    Serial.println("Request #: " + String(SAT) + " For: " + String(satnames[SAT]));

    String url = String("GET ") + String(satURL[SAT]) + "\r\n" + "Accept: */*" + "\r\n" + "Host: " + "www.celestrak.com" + "\r\n" + "Connection: close\r\n\r\n";
    url = "http://www.celestrak.com/" + String(satURL[SAT]);
    
    Serial.print("[HTTP] begin...\n");Serial.print("Requesting URL: ");Serial.println(url); // This will send the request to the server
    
    if (http.begin(client, url)) {  // HTTP
      int httpCode = http.GET();   // start connection and send HTTP header

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          Serial.println(payload);
          payload.toCharArray(TLE, 500);
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }


Serial.print("About to parse TLE: ");Serial.println(TLE);

    int j = 0;
    k = 0;
    for (j=26; j<96; j++){  //TLE line 1 spans characters 26 - 96
      TLE1[SAT][k] = TLE[j];
      k++;
    }
    k = 0;
    for (j=97; j< 167; j++){  //TLE line 2 spans characters 97 - 167
      TLE2[SAT][k]= TLE[j];
      k++;
    } 
 }
