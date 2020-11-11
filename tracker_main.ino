// Adapted September 2020 for use with ESP8266 Hardware by http://cactusprojects.com
// Current project located at https://github.com/CactusProjects/SatelliteTracker
// Forked from https://github.com/alexchang0229/SatelliteTracker
// For debugging purposes the code will run on ESP8266 without any need for peripherals (compass, stepper etc.)

#include <Sgp4.h>                   // For Satellite Calculations
#include <ESP8266WiFi.h>            // Wifi Library
#include <WiFiUdp.h>                // UDP Library  
#include <NTPClient.h>              // Network Time Library
#include <AccelStepper.h>           // Stepper Motor Library 
#include <ESP8266HTTPClient.h>      // Library used to retrieve Satellite info from website
#include <Adafruit_LSM303DLH_Mag.h> // For Magnometer
#include <Adafruit_Sensor.h>        // For Magnometer & Accelerometer
#include <Wire.h>                   // I2C Library for Magnometer & IO Expander

// ########################### //
// ###### USER VARIABLES ##### //
#define WIFI_SSID "XXXXXXXXX"  // Your Network name.
#define WIFI_PASS "XXXXXXXXX"  // Your Network password.
float myLat = 48.85341; float myLong = 2.3488; float myAlt = 42;   // Your latitude, longitude and altitude (Default is Paris). https://latitude.to
const int timeZone = 0;        // Your Time zone relative to GMT.
int numSats = 5;               // Number of satellites to track. Just enter '1' if you only want to track ISS and forget about the ADVANCED USER VARIABLES
// ########################### //
// # ADVANCED USER VARIABLES # //
// ########################### //
char satnames[5][50] = {"ISS","RADARSAT-2","NEOSSAT","M3MSAT ","SCISAT"}; // Names of satellites // Matrix Row Size identifier needs to match variable 'numSats'
char satURL[5][50] = {"/satcat/tle.php?CATNR=25544","/satcat/tle.php?CATNR=32382","/satcat/tle.php?CATNR=39089","/satcat/tle.php?CATNR=41605","/satcat/tle.php?CATNR=27858"}; // URL of Celestrak TLEs for satellites (In same order as names) // Matrix Row Size identifier needs to match variable 'numSats'
// #### END USER VARIABLES ### //
// ########################### //

//  ### I/O Pins ###      //
//  Azimuth stepper pins  //
#define AZmotorPin1  16   // IN1 on the ULN2003 driver - D0 ESP8266 - GPIO 16
#define AZmotorPin2  0    // IN2 on the ULN2003 driver - D3 ESP8266 - GPIO 0
#define AZmotorPin3  2    // IN3 on the ULN2003 driver - D4 ESP8266 - GPIO 2, also onboard LED.
#define AZmotorPin4  14   // IN4 on the ULN2003 driver - D5 ESP8266 - GPIO 14
// Elevation stepper pins //
#define ELmotorPin1  12   // IN1 on the ULN2003 driver - D6 ESP8266 - GPIO 12
#define ELmotorPin2  13   // IN2 on the ULN2003 driver - D7 ESP8266 - GPIO 13  
#define ELmotorPin3  15   // IN3 on the ULN2003 driver - D8 ESP8266 - GPIO 15
#define ELmotorPin4  3    // IN4 on the ULN2003 driver - RX ESP8266 - GPIO 3

// ### HARDWARE VARIABLES ### //
float oneTurn = 4076;         // Number of steps per one rotation of stepper motor.
#define MotorInterfaceType 8  // Define the AccelStepper interface type; 4 wire motor in half step mode:

// ### PROGRAM VARIABLES ### //
#define DEBUG                       // Enable serial output
char TLE[200];                      // Variable to store satellite TLEs
char TLE1[5][70]; char TLE2[5][70]; // Variable to store satellite TLEs // Matrix Row Size identifier needs to match variable 'numSats'
String payload = "";                // Holds HTTP TLE text when retrieved
int wifiStatus;                     // Holds variable if successfully connected to Wifi or not
int satAZsteps; int satELsteps;                                               // Step positions of where satellite currently is
int turns = 0;                                                                // Tracks if we have done a full rotation of dish (to stop tangling cables)
int i; int j; int k;                                                          // Loop counters
int SAT; int nextSat; int AZstart; long passEnd; int satVIS;                  // 
char satname[] = " ";                                                         // 
int passStatus = 0;                                                           // Hold if currently satellite passing overhead or not
char server[] = "104.168.149.178";                                            // Web address to get TLE (CELESTRAK)
int  year; int mon; int day; int hr; int minute; double sec; int today;       // 
long nextpassEpoch; long upcomingPasses[4];                                   // 
unsigned long timeNow = 0;                                                    // Holds current time in seconds
float Pi = 3.14159;                                                           // Hold value of Pi
float compass_heading = 0;                                                    // Holds compass heading 0-360
String compass_cardinal = "";                                                 // Holds compass cardinal string, N, S, E, W etc.
bool compass_present = 0;

// ### INITIALISATIONS ### //
Adafruit_LSM303DLH_Mag_Unified magnetometer = Adafruit_LSM303DLH_Mag_Unified(12345);                            // Initialises Magnometer / Compass
AccelStepper stepperAZ = AccelStepper(MotorInterfaceType, AZmotorPin1, AZmotorPin3, AZmotorPin2, AZmotorPin4);  // Initalises Azumith Stepper (Rotation)
AccelStepper stepperEL = AccelStepper(MotorInterfaceType, ELmotorPin1, ELmotorPin3, ELmotorPin2, ELmotorPin4);  // Initalises Elevation Stepper
Sgp4 sat;           
HTTPClient http;
WiFiUDP udp;
NTPClient ntp(udp, "europe.pool.ntp.org", 0); // [3] variable is utcOffsetInSeconds
WiFiClient client;                            // Initialize the Ethernet client library

void setup() {

  #ifdef DEBUG
  Serial.begin(9600,SERIAL_8N1,SERIAL_TX_ONLY); while (!Serial) {delay(100);} //Initialize serial and wait for port to open
  delay(100);
  Serial.println("Entering Setup");Serial.println("Entering Setup");Serial.println("Entering Setup");
  
  #endif
    
  if (magnetometer.begin()) {   // Initialises compass, does not matter if not present just will not automatically find North 
    compass_present = 1;        // Confirmed Compass is present, we can position based on known position now
    get_heading();              // Get current compass heading
    get_cardinal();             // Get current compass cardinal direction
    #ifdef DEBUG
    Serial.println("LSM303 (compass) detected.");
    Serial.print("Compass Heading (X Arrow): ");Serial.print(compass_heading);Serial.print("° - ");Serial.println(compass_cardinal); // Print out compass info
    #endif
  }
  else {
    #ifdef DEBUG
    Serial.println("No LSM303 (compass) detected.");
    #endif
  }

  // Setup stepper movements //
  stepperEL.setMaxSpeed(400);         // Sets Elevation MaxSpeed (Steps per second)
  #ifdef DEBUG
  Serial.println("Assuming Elevation is set to 20°"); // Assumes Dish Elevation set to 20 Degree's, Accelerometer in future to automate this.
  #endif
  stepperEL.setCurrentPosition(-228); // Assumes Elevation stepper starts at 20 degrees above horizon. 228 = 4096*(20/360) // Negative as stepper rotates CCW to raise dish
  stepperEL.setAcceleration(100);     // Acceleration (steps per second per second)
  
  stepperAZ.setMaxSpeed(400);         // Sets Azimuth MaxSpeed (Steps per second)
  stepperAZ.setCurrentPosition(0);    // Assumes Azimuth stepper starts at 0 (North) (will be corrected later with compass if present)
  stepperAZ.setAcceleration(100);     // Acceleration (steps per second per second)

  stepperAZ.disableOutputs();         // Disable Steppers, save power
  stepperEL.disableOutputs();         // Disable Steppers, save power

  stepperEL.runToNewPosition(-500); // Move Up for fun during setup
  stepperEL.runToNewPosition(-228); // Standby at 20 degrees above horizon

  standby();                          // Puts dish in standby position facing North and 20 Degree Elevation
 
  wifiStatus = WiFi.status();         // Gets current Wifi status
  if (wifiStatus != WL_CONNECTED) {   
    new_connection();                 // If not connected opens connection
  }
  else {
      #ifdef DEBUG
      Serial.println("WiFi Failed");
      #endif
  }
  
  sat.site(myLat,myLong,myAlt);   //set location latitude[°], longitude[°] and altitude[m] of where dish located

    while(timeNow < 100){         // While timeNow still at default variable
    ntp.update();                 // Get NTP Time
    today = ntp.getDay();         // 
    timeNow = ntp.getEpochTime(); // Epoch time is s
  }
  
  #ifdef DEBUG
    Serial.println("Today: " + String(today));                      // 0 = Sunday, 1 = Monday etc.
    Serial.println("timeNow (unix timestamp): " + String(timeNow));
  #endif
  
  for(SAT = 0; SAT < numSats; SAT++){ // Get TLEs for all the satellites we have listed
    getTLE(SAT);                      // Get two-line element set

    #ifdef DEBUG
    Serial.print("Back from getting TLE for: ");
    Serial.println(satnames[SAT]);
    Serial.println(TLE1[SAT]);
    Serial.println(TLE2[SAT]);
    #endif

    sat.init(satname,TLE1[SAT],TLE2[SAT]);     // Initialize satellite parameters
    sat.findsat(timeNow);                      // Finds where satellite is currently
    upcomingPasses[SAT] = Predict(SAT);        // Return time of next overpass and stores in array
  }

  #ifdef DEBUG
    Serial.println("");
    Serial.println("All TLE's Requests Recieved");
    Serial.println("");
  #endif
  
  nextSat = nextSatPass(upcomingPasses);          // Returns which satellite will be next to overpass
  sat.init(satname,TLE1[nextSat],TLE2[nextSat]);  // Initialize satellite parameters
  Predict(nextSat);                               // Remove??

    #ifdef DEBUG
    Serial.println("");
    Serial.println("##############");
    Serial.println("##############");
    Serial.println("Finished Setup");
    Serial.println("##############");
    Serial.println("##############");
    Serial.println("");
    #endif
}

void loop() {
  ntp.update();
  timeNow = ntp.getEpochTime();   // Update time.
  sat.findsat(timeNow);
  satAZsteps = round(sat.satAz*oneTurn/360);   // Convert degrees to stepper steps
  satELsteps = -round(sat.satEl*oneTurn/360);  // Convert degrees to stepper steps
    
  #ifdef DEBUG
    invjday(sat.satJd , timeZone, true, year, mon, day, hr, minute, sec);
    Serial.println("\nLocal time: " + String(day) + '/' + String(mon) + '/' + String(year) + ' ' + String(hr) + ':' + String(minute) + ':' + String(sec));
    Serial.println("Sat azimuth = " + String( sat.satAz) + ", Sat elevation = " + String(sat.satEl) + ", Distance = " + String(sat.satDist));
    Serial.println("Sat latitude = " + String( sat.satLat) + ", Sat longitude = " + String( sat.satLon) + ", Sat altitude = " + String( sat.satAlt) + "km");
    Serial.print("AZ Step pos: " + String(stepperAZ.currentPosition()));Serial.println(", EL Step pos: " + String(stepperEL.currentPosition()));
  #endif
  
  while(true){
    if(nextpassEpoch-timeNow < 60 && nextpassEpoch+5-timeNow > 0){
      #ifdef DEBUG
        Serial.println("Status: Pre-pass of " + String(satnames[nextSat]) + " in: " + String(nextpassEpoch-timeNow) + " seconds.");
      #endif
      prepass();
      break;
    }
    if(sat.satVis != -2){
      #ifdef DEBUG
        Serial.println("Status: In pass of " + String(satnames[nextSat]));
      #endif
      inPass();
      break;
    }
    if(timeNow - passEnd < 120){
      #ifdef DEBUG
        Serial.println("Status: Post-pass of " + String(satnames[nextSat]));
      #endif
      postpass();
      break;
    }
    if(sat.satVis == -2){
      #ifdef DEBUG
        Serial.print("Status: Standby, ");
        Serial.println("Next satellite is " + String(satnames[nextSat]) + " in " + String(nextpassEpoch-timeNow ) + " seconds.");
      #endif
      standby();      
      break;
    }   
  }
  delay(20);
  
  if(passStatus == 0 && today != ntp.getDay()){  // Update TLE & Unix time everyday.//
    for(SAT = 0; SAT < numSats; SAT++){
      getTLE(SAT);
    }
    ntp.update();
    timeNow = ntp.getEpochTime();
    today = ntp.getDay();
    #ifdef DEBUG
      Serial.println("Daily update of TLE's and Time Complete");
    #endif
  }
}

int nextSatPass(long _nextpassEpoch[5]){ // Replace with number of satellites
  for(i = 0;i < numSats; ++i){
    if( _nextpassEpoch[0]-timeNow >= _nextpassEpoch[i]-timeNow){
      _nextpassEpoch[0] = _nextpassEpoch[i];
      nextSat = i;
    }
  }
  return nextSat;
}

void standby() {

  stepperEL.runToNewPosition(-228); // Standby at 20 degrees above horizon
  
  stepperAZ.runToNewPosition(0); // North 
  get_heading();
  
  #ifdef DEBUG
  Serial.print("Compass Heading: ");Serial.print(compass_heading);Serial.println("°");
  #endif
  
  if (compass_heading > 3 && compass_heading <= 180) {
      #ifdef DEBUG
      Serial.println("Need to Adjust Heading CCW to be facing North");
      stepperAZ.setCurrentPosition(compass_heading*(oneTurn/360));
      stepperAZ.runToNewPosition(0);
      Serial.println("Reset to 0° (North) Complete");
      #endif
    
  }
  else if (compass_heading < 357 && compass_heading > 180) {
      #ifdef DEBUG
      Serial.println("Need to Adjust Heading CW to be facing North");
      stepperAZ.setCurrentPosition(-(360-compass_heading)*(oneTurn/360));
      stepperAZ.runToNewPosition(0);
      Serial.println("Reset to 0° (North) Complete");
      #endif
  }

  stepperAZ.setCurrentPosition(0);

  stepperAZ.disableOutputs();         // Disable Steppers, save power
  stepperEL.disableOutputs();         // Disable Steppers, save power
}

void prepass() {                          // Pass is less than 300 seconds (5 mins) away, move antenna to start location and wait.
    if(AZstart < 360 && AZstart > 180){   // If start between 180* & 360*
      AZstart = AZstart - 360;            // Goes to start position counter-clockwise if closer. (AZstart will have value of -180 to 0)
      }
    stepperAZ.runToNewPosition(AZstart * oneTurn/360);// Move azimuth to start position (* oneTurn/360 converts degrees to steps)
    stepperEL.runToNewPosition(0);                    // Move elevation to Horizon (0)
      
    stepperAZ.disableOutputs();         // Disable Steppers, save power
    stepperEL.disableOutputs();         // Disable Steppers, save power
}

void inPass() {
  if(AZstart < 0){                        // Handle zero crossings
      satAZsteps = satAZsteps - oneTurn;
  }
  if((satAZsteps - stepperAZ.currentPosition()) > 100){                     // If the Satellite poistion is > 100 steps from our current position we must have crossed 0 Degree Mark
      stepperAZ.setCurrentPosition(stepperAZ.currentPosition() + oneTurn);  // So set out current position as a full revolution ahead. 
      turns--;                                                              // Decrements if we've crossed CCW
  }
  if((satAZsteps - stepperAZ.currentPosition()) < -100){                    // If the Satellite poistion is < 100 steps from our current position we must have crossed 0 Degree Mark
      stepperAZ.setCurrentPosition(stepperAZ.currentPosition() - oneTurn);  // So set out current position as a full revolution behind. 
      turns++;                                                              // Increments if we've crossed CW
  }

  stepperAZ.runToNewPosition(satAZsteps); // Update stepper position                 
  stepperEL.runToNewPosition(satELsteps); // Update stepper position
  passEnd = timeNow;
  passStatus = 1;
}

void postpass() {
  #ifdef DEBUG
  Serial.println("Post pass time left: " + String(passEnd + 120 - timeNow));
  #endif
  
  if(timeNow-passEnd > 90){  // Pass finsised > 90 Seconds ago
        if(turns > 0){
          stepperAZ.setCurrentPosition(stepperAZ.currentPosition() + oneTurn);
          turns--;
        }
        if(turns < 0){
          Serial.print("Code is setting current AZ position - one turn but not sure why, current step position: ");Serial.println(stepperAZ.currentPosition());
          stepperAZ.setCurrentPosition(stepperAZ.currentPosition() - oneTurn);
          turns++;
        }
  }
  
  if(passStatus == 1 && timeNow-passEnd > 100){
    for(SAT = 0; SAT < numSats; SAT++){
      sat.init(satname,TLE1[SAT],TLE2[SAT]); 
      sat.findsat(timeNow);
      upcomingPasses[SAT] = Predict(SAT);
      #ifdef DEBUG
      Serial.println("Next pass for " + String(satnames[nextSat]) + " in " + String(upcomingPasses[SAT]-timeNow) + " seconds.");
      #endif
    }
    nextSat = nextSatPass(upcomingPasses);
    sat.init(satname,TLE1[nextSat],TLE2[nextSat]);
    Predict(nextSat);
    passStatus = 0;
  }
}

void new_connection() {
    Serial.println("Attempting to connect to WiFi");
    wifiStatus = WiFi.status(); 
    if (wifiStatus != WL_CONNECTED) {   
       
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        int loops = 0;
        int connection_timer = 0;
       
        while (wifiStatus != WL_CONNECTED)
        {
          Serial.print("WiFi Connecting. . . ");Serial.println(connection_timer);
          wifiStatus = WiFi.status();
          delay(500);
          connection_timer++;
          
          if(connection_timer == 30) {
              #ifdef DEBUG
              Serial.println( "No connection after 30 loops, powercycling the WiFi radio. I have seen this work when the connection is unstable" );
              #endif
              WiFi.disconnect();
              delay( 10 );
              WiFi.forceSleepBegin();
              delay(10);
              WiFi.forceSleepWake();
              delay(10);
              WiFi.begin(WIFI_SSID, WIFI_PASS);
          }
          
          if (connection_timer >= 60) {
              #ifdef DEBUG
              Serial.println( "No connection after 60 loops. WiFi connection failed, disabled WiFi and waiting for a minute" );
              #endif
              WiFi.disconnect( true );
              delay(1);
              WiFi.mode( WIFI_OFF );
              WiFi.forceSleepBegin();
              delay(6000);
              WiFi.forceSleepWake();
              connection_timer = 0;  
         }
      }
    }   
    wifiStatus = WiFi.status();
    Serial.print("WiFi connected, IP address: ");Serial.println(WiFi.localIP());
}

void close_connection() {
  WiFi.disconnect( true );
  delay( 1 );
}

long Predict(int sat_num) {                    // Adapted from sgp4 library // Predicts time of next pass and start azimuth for satellites 
    passinfo overpass;                      // Structure to store overpass info
    sat.initpredpoint(timeNow , 0.0);     // Finds the startpoint
    bool error = sat.nextpass(&overpass,20);     //search for the next overpass, if there are more than 20 maximums below the horizon it returns false
    
    if (error == 1){ //no error, prints overpass information
      nextpassEpoch = (overpass.jdstart-2440587.5) * 86400;
      AZstart = overpass.azstart;
      invjday(overpass.jdstart ,timeZone ,true , year, mon, day, hr, minute, sec);   // Convert Julian date to print in serial.
      #ifdef DEBUG
      Serial.println("Next pass for " + String(satnames[sat_num]) + " in " + String(nextpassEpoch-timeNow));
      Serial.println("Start: az=" + String(overpass.azstart) + "° " + String(hr) + ':' + String(minute) + ':' + String(sec));
      #endif
    }
    else {
        #ifdef DEBUG
          Serial.println("Prediction error");
        #endif
        while(true);
    }  
    return nextpassEpoch;
}

long Predict_orig(int many) {                    // Adapted from sgp4 library // Predicts time of next pass and start azimuth for satellites 
    passinfo overpass;                      // Structure to store overpass info
    sat.initpredpoint( timeNow , 0.0 );     // Finds the startpoint
    
    bool error;
    for (int i = 0; i < many ; i++){
        error = sat.nextpass(&overpass,20);     //search for the next overpass, if there are more than 20 maximums below the horizon it returns false
        delay(0);
        
        if ( error == 1){ //no error, prints overpass information
          nextpassEpoch = (overpass.jdstart-2440587.5) * 86400;
          AZstart = overpass.azstart;
          invjday(overpass.jdstart ,timeZone ,true , year, mon, day, hr, minute, sec);   // Convert Julian date to print in serial.
          #ifdef DEBUG
          Serial.println("Next pass for " + String(satnames[nextSat]) + " in " + String(nextpassEpoch-timeNow));
          Serial.println("Start: az=" + String(overpass.azstart) + "° " + String(hr) + ':' + String(minute) + ':' + String(sec));
          #endif
        }
        else {
            #ifdef DEBUG
              Serial.println("Prediction error");
            #endif
            while(true);
        }
        delay(0);
    }  
    return nextpassEpoch;
}

void getTLE(int SAT) {
    #ifdef DEBUG
    Serial.println("");Serial.println("HTTP Request #" + String(SAT) + " is for " + String(satnames[SAT]));
    #endif
    
    String url = String("GET ") + String(satURL[SAT]) + "\r\n" + "Accept: */*" + "\r\n" + "Host: " + "www.celestrak.com" + "\r\n" + "Connection: close\r\n\r\n";
    url = "http://www.celestrak.com" + String(satURL[SAT]);
    
    Serial.print("Requesting URL: ");Serial.println(url);       // This will send the request to the server
    
    if (http.begin(client, url)) {                              // HTTP
      int httpCode = http.GET();                                // Start connection and send HTTP header
      if (httpCode > 0) {                                       // httpCode will be negative on error
          Serial.printf("[HTTP] GET... code: %d\n", httpCode);  // HTTP header has been send and Server response header has been handled
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {  // file found at server
            payload = http.getString();
            Serial.print("Recieved HTTP Payload for: ");Serial.println(payload);
            payload.toCharArray(TLE, 200);
          }
      }
      else {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    }
    else {
          Serial.printf("[HTTP} Unable to connect\n");
    }

    j = 0;
    k = 0;
    for (j=26; j<95; j++){    //TLE line 1 spans characters 26 - 96
      TLE1[SAT][k] = TLE[j];         
      k++;
    }

    k = 0;
    for (j=97; j< 166; j++){  //TLE line 2 spans characters 97 - 167
      TLE2[SAT][k] = TLE[j];      
      k++;
    } 
 }

void get_heading() {
  if (compass_present == 1) {
      sensors_event_t event;
      magnetometer.getEvent(&event);
      compass_heading = (atan2(event.magnetic.y, event.magnetic.x) * 180) / Pi; // Calculate the angle of the vector y,x
      if (compass_heading < 0) {
        compass_heading = 360 + compass_heading;   // Normalize to 0-360
      }
  }
  else {
      compass_heading = 0;
      #ifdef DEBUG
      Serial.println( "No Compass Present, Heading set to 0°");
      #endif
  }
}

void get_cardinal() {
  if (compass_heading < 22.5 && compass_heading >= 0){
    compass_cardinal = "N";
  }
  else if (compass_heading >= 22.5 && compass_heading < 67.5) {
    compass_cardinal = "NE";
  }
  else if (compass_heading >= 67.5 && compass_heading < 112.5) {
    compass_cardinal = "E";
  }
  else if (compass_heading >= 112.5 && compass_heading < 157.5) {
    compass_cardinal = "SE";
  }
  else if (compass_heading >= 157.5 && compass_heading < 202.5) {
    compass_cardinal = "S";
  }
  else if (compass_heading >= 202.5 && compass_heading < 247.5) {
    compass_cardinal = "SW";
  }
  else if (compass_heading >= 247.5 && compass_heading < 292.5) {
    compass_cardinal = "W";
  }
  else if (compass_heading >= 292.5 && compass_heading < 337.5) {
    compass_cardinal = "NW";
  }
  else if (compass_heading >= 337.5 && compass_heading <= 360) {
    compass_cardinal = "N";
  }
  else {
    compass_cardinal = "Code Bug";
  }
}
