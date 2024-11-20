#include <Sgp4.h>
#include <Ticker.h>

Sgp4 sat;
unsigned long unixtime = 1458950400;
int timezone = 12 ;  //utc + 12

int  year; int mon; int day; int hr; int minute; double sec;



void Predict(int many){
    
    passinfo overpass;                       //structure to store overpass info
    sat.initpredpoint( unixtime , 0.0 );     //finds the startpoint
    
    bool error;
    unsigned long start = millis();
    for (int i = 0; i<many ; i++){
        error = sat.nextpass(&overpass,20);     //search for the next overpass, if there are more than 20 maximums below the horizon it returns false
        delay(0);
        
        if ( error == 1){ //no error, prints overpass information
          
          invjday(overpass.jdstart ,timezone ,true , year, mon, day, hr, minute, sec);
          Serial.println("Overpass " + String(day) + ' ' + String(mon) + ' ' + String(year));
          Serial.println("  Start: az=" + String(overpass.azstart) + "° " + String(hr) + ':' + String(minute) + ':' + String(sec));
          
          invjday(overpass.jdmax ,timezone ,true , year, mon, day, hr, minute, sec);
          Serial.println("  Max: elev=" + String(overpass.maxelevation) + "° " + String(hr) + ':' + String(minute) + ':' + String(sec));
          
          invjday(overpass.jdstop ,timezone ,true , year, mon, day, hr, minute, sec);
          Serial.println("  Stop: az=" + String(overpass.azstop) + "° " + String(hr) + ':' + String(minute) + ':' + String(sec));
          
          switch(overpass.transit){
              case none:
                  break;
              case enter:
                  invjday(overpass.jdtransit ,timezone ,true , year, mon, day, hr, minute, sec);
                  Serial.println("  Enter earth shadow: " + String(hr) + ':' + String(minute) + ':' + String(sec)); 
                  break;
              case leave:
                  invjday(overpass.jdtransit ,timezone ,true , year, mon, day, hr, minute, sec);
                  Serial.println("  Leave earth shadow: " + String(hr) + ':' + String(minute) + ':' + String(sec)); 
                  break;
          }
          switch(overpass.sight){
              case lighted:
                  Serial.println("  Visible");
                  break;
              case eclipsed:
                  Serial.println("  Not visible");
                  break;
              case daylight:
                  Serial.println("  Daylight");
                  break;
          }
  
          
        }else{
            Serial.println("Prediction error");
        }
        delay(0);
    }
    unsigned long einde = millis();
    Serial.println("Time: " + String(einde-start) + " milliseconds");
    
}


void setup() {

  Serial.begin(115200);
  Serial.println();
  
  sat.site(-0.5276847,166.9359231,34); //set site latitude[°], longitude[°] and altitude[m]

  char satname[] = "ISS (ZARYA)";
  char tle_line1[] = "1 25544U 98067A   16065.25775256 -.00164574  00000-0 -25195-2 0  9990";  //Line one from the TLE data
  char tle_line2[] = "2 25544  51.6436 216.3171 0002750 185.0333 238.0864 15.54246933988812";  //Line two from the TLE data
  
  sat.init(satname,tle_line1,tle_line2);     //initialize satellite parameters 

  //Display TLE epoch time
  double jdC = sat.satrec.jdsatepoch;
  invjday(jdC , timezone, true, year, mon, day, hr, minute, sec);
  Serial.println("Epoch: " + String(day) + '/' + String(mon) + '/' + String(year) + ' ' + String(hr) + ':' + String(minute) + ':' + String(sec));
  Serial.println();

  Predict(50);   //Calculates the next 50 overpasses

}


void loop() {

}

