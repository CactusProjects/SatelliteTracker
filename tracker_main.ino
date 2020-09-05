// Adapted September 2020 for use with ESP8266 Hardware by http://cactusprojects.com
// Current project located at https://github.com/CactusProjects/SatelliteTracker
// Forked from https://github.com/alexchang0229/SatelliteTracker

#include <Sgp4.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <AccelStepper.h>
#include <ESP8266HTTPClient.h>

// ### USER VARIABLES ### //
#define WIFI_SSID ""  // Your Network name.
#define WIFI_PASS ""     // Your Network password.
float myLat = 48.85341; float myLong = 2.3488; float myAlt = 42;   // Your latitude, longitude and altitude. https://latitude.to

const int timeZone = 0;            // Your Time zone relative to GMT.

int numSats = 5;                                                    // Number of satellites to track.
char satnames[5][50] = {"COSMOS-1271","RADARSAT-2","NEOSSAT","M3MSAT ","SCISAT"}; // Names of satellites.
char satURL[5][50] = {"/satcat/tle.php?INTDES=1981-046","/satcat/tle.php?CATNR=32382","/satcat/tle.php?CATNR=39089","/satcat/tle.php?CATNR=41605","/satcat/tle.php?CATNR=27858"}; // URL of Celestrak TLEs for satellites (In same order as names).

//  ### I/O Pins ###      //
//  Azimuth stepper pins  //
#define AZmotorPin1  16   // IN1 on the ULN2003 driver - D0 ESP8266
#define AZmotorPin2  5    // IN2 on the ULN2003 driver - D1 ESP8266
#define AZmotorPin3  4    // IN3 on the ULN2003 driver - D2 ESP8266
#define AZmotorPin4  0    // IN4 on the ULN2003 driver - D3 ESP8266
// Elevation stepper pins //
#define ELmotorPin1  2    // IN1 on the ULN2003 driver - D4 ESP8266  
#define ELmotorPin2  14   // IN2 on the ULN2003 driver - D5 ESP8266     
#define ELmotorPin3  12   // IN3 on the ULN2003 driver - D6 ESP8266   
#define ELmotorPin4  13   // IN4 on the ULN2003 driver - D7 ESP8266  

// ### HARDWARE VARIABLES ### //
float oneTurn = 4076;         // Number of steps per one rotation for stepper motor.
#define MotorInterfaceType 8  // Define the AccelStepper interface type; 4 wire motor in half step mode:

// ### PROGRAM VARIABLES ### //
#define DEBUG                       // Enable serial output.
char TLE[500];                      // Variable to store satellite TLEs.
char TLE1[5][70]; char TLE2[5][70]; // Variable to store satellite TLEs.
int wifiStatus;
int debug = 1;
int satAZsteps; int satELsteps; int turns = 0;
int i; int k; int SAT; int nextSat; int AZstart; long passEnd; int satVIS;
char satname[] = " ";
int passStatus = 0;
char server[] = "104.168.149.178";    //Web address to get TLE (CELESTRAK)
int  year; int mon; int day; int hr; int minute; double sec; int today;
long nextpassEpoch; long upcomingPasses[4];
unsigned long timeNow = 0;

// ### INITIALISATIONS ### //
AccelStepper stepperAZ = AccelStepper(MotorInterfaceType, AZmotorPin1, AZmotorPin3, AZmotorPin2, AZmotorPin4);
AccelStepper stepperEL = AccelStepper(MotorInterfaceType, ELmotorPin1, ELmotorPin3, ELmotorPin2, ELmotorPin4);
Sgp4 sat;
HTTPClient http;
WiFiUDP udp;
NTPClient ntp(udp, "europe.pool.ntp.org", 0); //[3]=utcOffsetInSeconds
WiFiClient client; // Initialize the Ethernet client library


void setup() {
  Serial.begin(9600); while (!Serial) {delay(10);} //Initialize serial and wait for port to open:

  // Setup stepper movements //
  stepperEL.setMaxSpeed(1000);
  stepperEL.setCurrentPosition(-227); // Elevation stepper starts at -227 steps (20 degrees above horizon).
  stepperEL.setAcceleration(100);
  stepperAZ.setMaxSpeed(1000);
  stepperAZ.setCurrentPosition(0);    // Azimuth stepper starts at 0.
  stepperAZ.setAcceleration(100);
  
  new_connection(); // Connects to Wifi
  
  sat.site(myLat,myLong,myAlt); //set location latitude[°], longitude[°] and altitude[m]

  // Get Unix time //
  while(timeNow == 0){
    ntp.update();
    today = ntp.getDay();
    timeNow = ntp.getEpochTime();
  } 
  #ifdef DEBUG
    Serial.println("today: " + String(today));
    Serial.println("timeNow (unix timestamp): " + String(timeNow));
  #endif
  
  // Get TLEs //
  for(SAT = 0; SAT < numSats; SAT++){
    getTLE(SAT);
    Serial.println("Back from TLE: ");
    Serial.println(satname);
    Serial.println(TLE1[SAT]);
    Serial.println(TLE2[SAT]);
    
    sat.init(satname,TLE1[SAT],TLE2[SAT]);     //initialize satellite parameters
    sat.findsat(timeNow);
    upcomingPasses[SAT] = Predict(1);
  }
  nextSat = nextSatPass(upcomingPasses);
  sat.init(satname,TLE1[nextSat],TLE2[nextSat]);
  Predict(1);
  
  // Print obtained TLE in serial. //
  #ifdef DEBUG
    for(SAT=0; SAT < numSats; SAT++){
      Serial.println("TLE set #:" + String(SAT));
      for(i=0;i<70;i++){     
        Serial.print(TLE1[SAT][i]); 
      }
      Serial.println();
      for(i=0;i<70;i++){
        Serial.print(TLE2[SAT][i]);
      }
      Serial.println();
    }  
    Serial.println("Next satellite: " + String(nextSat));
  #endif

  standby();
}

void loop() {
  ntp.update();
  timeNow = ntp.getEpochTime();   // Update time.
  Serial.print("Time Now: ");Serial.println(timeNow);  
  sat.findsat(timeNow);
  satAZsteps = round(sat.satAz*oneTurn/360);   //Convert degrees to stepper steps
  satELsteps = -round(sat.satEl*oneTurn/360);  
  #ifdef DEBUG
    invjday(sat.satJd , timeZone, true, year, mon, day, hr, minute, sec);
    Serial.println("\nLocal time: " + String(day) + '/' + String(mon) + '/' + String(year) + ' ' + String(hr) + ':' + String(minute) + ':' + String(sec));
    Serial.println("azimuth = " + String( sat.satAz) + " elevation = " + String(sat.satEl) + " distance = " + String(sat.satDist));
    Serial.println("latitude = " + String( sat.satLat) + " longitude = " + String( sat.satLon) + " altitude = " + String( sat.satAlt));
    Serial.println("AZStep pos: " + String(stepperAZ.currentPosition()));
  #endif
  
  
  while(true){
    if(nextpassEpoch-timeNow < 300 && nextpassEpoch+5-timeNow > 0){
      #ifdef DEBUG
        Serial.println("Status: Pre-pass");
        Serial.println("Next satellite is #: " + String(satnames[nextSat]) + " in: " + String(nextpassEpoch-timeNow));
      #endif
      prepass();
      break;
    }
    if(sat.satVis != -2){
      #ifdef DEBUG
        Serial.println("Status: In pass");
      #endif
      inPass();
      break;
    }
    if(timeNow - passEnd < 120){
      #ifdef DEBUG
        Serial.println("Status: Post-pass");
      #endif
      postpass();
      break;
    }
    if(sat.satVis == -2){
      #ifdef DEBUG
        Serial.println("Status: Standby");
        Serial.println("Next satellite is: " + String(satnames[nextSat]) + " in: " + String(nextpassEpoch-timeNow));
      #endif
      standby();      
      break;
    }
    
  }
  delay(20);
  
  
  // Update TLE & Unix time everyday.//
  if(passStatus == 0 && today != ntp.getDay()){
    for(SAT = 0; SAT < numSats; SAT++){
      getTLE(SAT);
    }
    ntp.update();
    timeNow = ntp.getEpochTime();
    today = ntp.getDay();
    #ifdef DEBUG
      Serial.println("Updating TLEs and time");
    #endif
  }

  
}

int nextSatPass(long _nextpassEpoch[4]){ // Replace with number of satellites
  for(i = 0;i < numSats; ++i){
    if( _nextpassEpoch[0]-timeNow >= _nextpassEpoch[i]-timeNow){
      _nextpassEpoch[0] = _nextpassEpoch[i];
      nextSat = i;
    }
  }
  return nextSat;
}

void standby (){

  // Azimuth //
  stepperAZ.runToNewPosition(0); 
  // ELEVATION // 
  stepperEL.runToNewPosition(-227); //Standby at 20 degrees above horizon
  
  digitalWrite(AZmotorPin1, LOW);
  digitalWrite(AZmotorPin2, LOW);
  digitalWrite(AZmotorPin3, LOW);
  digitalWrite(AZmotorPin4, LOW);
  
  digitalWrite(ELmotorPin1, LOW);
  digitalWrite(ELmotorPin2, LOW);
  digitalWrite(ELmotorPin3, LOW);
  digitalWrite(ELmotorPin4, LOW);
}

void prepass(){
  // Pass is less than 300 seconds (5 mins) away, move antenna to start location and wait.
    if(AZstart < 360 && AZstart > 180){
      AZstart = AZstart - 360; //Goes to start counter-clockwise if closer.
      }
    stepperAZ.runToNewPosition(AZstart * oneTurn/360); 
    stepperEL.runToNewPosition(0);
      
    digitalWrite(AZmotorPin1, LOW);
    digitalWrite(AZmotorPin2, LOW);
    digitalWrite(AZmotorPin3, LOW);
    digitalWrite(AZmotorPin4, LOW);
    
    digitalWrite(ELmotorPin1, LOW);
    digitalWrite(ELmotorPin2, LOW);
    digitalWrite(ELmotorPin3, LOW);
    digitalWrite(ELmotorPin4, LOW);

}

void inPass(){

  // Handle zero crossings
  if(AZstart < 0){
    satAZsteps = satAZsteps - oneTurn;
  }
  if(satAZsteps - stepperAZ.currentPosition() > 100){
  stepperAZ.setCurrentPosition(stepperAZ.currentPosition() + oneTurn);
  turns--;
  }
  if(satAZsteps - stepperAZ.currentPosition() < -100){
  stepperAZ.setCurrentPosition(stepperAZ.currentPosition() - oneTurn);
  turns++;
  }

  // Update stepper position
  stepperAZ.runToNewPosition(satAZsteps);                  
  stepperEL.runToNewPosition(satELsteps);
  passEnd = timeNow;
  passStatus = 1;
}

void postpass(){
  #ifdef DEBUG
  Serial.println("Post pass time left: " + String(passEnd + 120 - timeNow));
  #endif
    if(timeNow-passEnd > 90){  
  if(turns > 0){
    stepperAZ.setCurrentPosition(stepperAZ.currentPosition() + oneTurn);
    turns--;
  }
  if(turns < 0){
    stepperAZ.setCurrentPosition(stepperAZ.currentPosition() - oneTurn);
    turns++;
  }}
  if(passStatus == 1 && timeNow-passEnd > 100){
    for(SAT = 0; SAT < numSats; SAT++){
      sat.init(satname,TLE1[SAT],TLE2[SAT]); 
      sat.findsat(timeNow);
      upcomingPasses[SAT] = Predict(1);
      #ifdef DEBUG
      Serial.println("Next pass for Satellite #: " + String(SAT) + " in: " + String(upcomingPasses[SAT]-timeNow));
      #endif
    }
    nextSat = nextSatPass(upcomingPasses);
    sat.init(satname,TLE1[nextSat],TLE2[nextSat]);
    Predict(1);
    passStatus = 0;
  }

}

void new_connection() {
    WiFi.begin( WIFI_SSID, WIFI_PASS );
    int loops = 0;
    int retries = 0;
    wifiStatus = WiFi.status();
    while ( wifiStatus != WL_CONNECTED )
    {
      retries++;
      if( retries == 300 )
      {
          if (debug == 1) {Serial.println( "No connection after 300 steps, powercycling the WiFi radio. I have seen this work when the connection is unstable" );}
          WiFi.disconnect();
          delay( 10 );
          WiFi.forceSleepBegin();
          delay( 10 );
          WiFi.forceSleepWake();
          delay( 10 );
          WiFi.begin( WIFI_SSID, WIFI_PASS );
      }
      if ( retries == 600 )
      {
          WiFi.disconnect( true );
          delay( 1 );
          WiFi.mode( WIFI_OFF );
          WiFi.forceSleepBegin();
          delay( 10 );
          
          if( loops == 3 )
          {
              if (debug == 1) {Serial.println( "That was 3 loops, still no connection so let's go to deep sleep for 2 minutes" );}
              Serial.flush();
              ESP.deepSleep( 120000000, WAKE_RF_DISABLED );
          }
          else
          {
              if (debug == 1) {Serial.println( "No connection after 600 steps. WiFi connection failed, disabled WiFi and waiting for a minute" );}
          }
          
          delay( 60000 );
          return;
      }
      delay( 50 );
      wifiStatus = WiFi.status();
    }
    Serial.print(wifiStatus);Serial.print("My IP: ");Serial.println(WiFi.localIP()); 
}

void close_connection() {
  WiFi.disconnect( true );
  delay( 1 );
}
