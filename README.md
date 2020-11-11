# ESP8266 Satellite Tracker
This repository contains the code for the 3D printed satellite tracker ported to ESP8266 Hardware. (forked from original Arduino MKR 1000 hardware created by Alex Chang)<br />
<img src="https://hackster.imgix.net/uploads/attachments/1156979/_ijWqYco4SG.blob?auto=compress%2Cformat&w=900&h=675&fit=min" width="600"> <br />

# Getting started
Hardware:
* ESP8266
* 28BYJ-48 Stepper Motors & ULN2003 Driver board (quantity 2) (Note: Stepper should not be powered from ESP regulator!)
* 3D Printed Satellite Model (https://www.thingiverse.com/thing:4541354)
* (Optional) LSM303DLH Compass & Accelerometer (Note: This allows dish to point in the correct cardinal direction, otherwise it assumes it's facing North when setup. Elevation is statically controlled but future update will use accelerometer to move elevation to known position.)

### Software:
* Arduino IDE
* You will need the following arduino libraries:
* https://github.com/Hopperpop/Sgp4-Library 
* https://github.com/arduino-libraries/RTCZero 
* https://www.airspayce.com/mikem/arduino/AccelStepper/ 
* https://github.com/adafruit/Adafruit_LSM303DLH_Mag (For Optional Compass/Accelerometer)
* https://github.com/adafruit/Adafruit_BusIO (For Optional Compass/Accelerometer)
* https://github.com/adafruit/Adafruit_Sensor (For Optional Compass/Accelerometer)

### Set-up & Use:
* Open tracker_main.ino in Arduino IDE.
* Update USER VARIABLES in the code, no other changes are necessary by default. (if you just want to track ISS then set 'numSats' variable to '1')
* Ensure code compiles
* Program ESP8266
* After uploading the code, you should see something like the below after opening the serial monitor and the dish should move up and down a little when reset.

### Notes
* The azimuth stepper can only determin its position if compass present, if not it assumes it was powered up facing north.
* The elevation stepper has no way of determining its position at the moment so it assumes it was powered up facing up 20 Degrees

### Advanced Notes:
* TLEs of satellites are obtained from Celestrak.com, if you want to change the satellites being tracked then copy the URL for the satellite you want (without the 'celestrak.com') into the array in the ADVANCED USER VARIABLE section of the code, for example the international space station is "/satcat/tle.php?CATNR=25544"
* If you are adding more than the default 5 satellies then you will need to update the constructors to match the number of satellites (satnames[5][50], satURL[5][50], char TLE1[5][70] & char TLE2[5][70])

### Serial Output Example
```
Entering Setup
LSM303 (compass) detected.
Compass Heading (X Arrow): 356.43° - N
Assuming Elevation is set to 20°
Compass Heading: 357.40°
WiFi Failed
Today: 3
timeNow (unix timestamp): 1605122660

HTTP Request #0 is for ISS
Requesting URL: http://www.celestrak.com/satcat/tle.php?CATNR=25544
[HTTP] GET... code: 200
Recieved HTTP Payload for: ISS (ZARYA)             
1 25544U 98067A   20315.57668483  .00001637  00000+0  37361-4 0  9998
2 25544  51.6443 344.1131 0001890  91.8913 268.2292 15.49406339254715

Back from getting TLE for: ISS
1 25544U 98067A   20315.57668483  .00001637  00000+0  37361-4 0  9998
2 25544  51.6443 344.1131 0001890  91.8913 268.2292 15.49406339254715
Next pass for ISS in 5604
Start: az=192.15° 20:57:44.37

HTTP Request #1 is for RADARSAT-2
Requesting URL: http://www.celestrak.com/satcat/tle.php?CATNR=32382
[HTTP] GET... code: 200
Recieved HTTP Payload for: RADARSAT-2              
1 32382U 07061A   20316.11992994  .00000015  00000+0  22611-4 0  9999
2 32382  98.5779 320.5641 0001098  89.9392 270.1916 14.29984533673824

Back from getting TLE for: RADARSAT-2
1 32382U 07061A   20316.11992994  .00000015  00000+0  22611-4 0  9999
2 32382  98.5779 320.5641 0001098  89.9392 270.1916 14.29984533673824
Next pass for RADARSAT-2 in 1681
Start: az=271.86° 19:52:21.58

HTTP Request #2 is for NEOSSAT
Requesting URL: http://www.celestrak.com/satcat/tle.php?CATNR=39089
[HTTP] GET... code: 200
Recieved HTTP Payload for: NEOSSAT                 
1 39089U 13009D   20316.09720064  .00000031  00000+0  26273-4 0  9999
2 39089  98.4545 154.9392 0010678 207.0632 152.9998 14.34531304403571

Back from getting TLE for: NEOSSAT
1 39089U 13009D   20316.09720064  .00000031  00000+0  26273-4 0  9999
2 39089  98.4545 154.9392 0010678 207.0632 152.9998 14.34531304403571
Next pass for NEOSSAT in 528
Start: az=12.05° 19:33:8.35

HTTP Request #3 is for M3MSAT 
Requesting URL: http://www.celestrak.com/satcat/tle.php?CATNR=41605
[HTTP] GET... code: 200
Recieved HTTP Payload for: M3MSAT                  
1 41605U 16040G   20316.11390589  .00000732  00000+0  35403-4 0  9995
2 41605  97.2954   8.0693 0013289  48.9351 311.3031 15.21549383243599

Back from getting TLE for: M3MSAT 
1 41605U 16040G   20316.11390589  .00000732  00000+0  35403-4 0  9995
2 41605  97.2954   8.0693 0013289  48.9351 311.3031 15.21549383243599
Next pass for M3MSAT  in 2922
Start: az=160.97° 20:13:2.84

HTTP Request #4 is for SCISAT
Requesting URL: http://www.celestrak.com/satcat/tle.php?CATNR=27858
[HTTP] GET... code: 200
Recieved HTTP Payload for: SCISAT 1                
1 27858U 03036A   20316.14566937  .00000047  00000+0  11659-4 0  9991
2 27858  73.9331 255.4531 0006538  64.5927 295.5923 14.77309447929313

Back from getting TLE for: SCISAT
1 27858U 03036A   20316.14566937  .00000047  00000+0  11659-4 0  9991
2 27858  73.9331 255.4531 0006538  64.5927 295.5923 14.77309447929313
Next pass for SCISAT in 2754
Start: az=0.67° 20:10:14.63

All TLE's Requests Recieved

Next pass for NEOSSAT in 528
Start: az=12.05° 19:33:8.35

##############
##############
Finished Setup
##############
##############


Local time: 11/11/2020 19:24:24.00
Sat azimuth = 12.66, Sat elevation = -23.39, Distance = 6684.20
Sat latitude = 70.08, Sat longitude = 149.07, Sat altitude = 793.95km
AZ Step pos: 0, EL Step pos: -228
Status: Standby, Next satellite is NEOSSAT in 524 seconds.
Compass Heading: 356.96°
Need to Adjust Heading CW to be facing North
Reset to 0° (North) Complete

```

* Here you can verify the predictions are correct and that the local time is correct (your results will differ depending on your time/location/satellite). 
* The last few lines will continue to update over time and you should see the satellite approach. 

# Ported to ESP by CactusProjects, Original Author: Alex Chang
Contact for Alex: yuc888@mail.usask.ca <br />
Link to the original project on Hackster.io: https://www.hackster.io/alex_chang/satellite-tracker-13a9aa <br />


