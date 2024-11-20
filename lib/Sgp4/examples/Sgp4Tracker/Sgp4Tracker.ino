#include <Sgp4.h>
#include <Ticker.h>

Sgp4 sat;
Ticker tkSecond;
unsigned long unixtime = 1458950400;
int timezone = 12 ;  //utc + 12
int framerate;

int  year; int mon; int day; int hr; int minute; double sec;

void Second_Tick()
{
  unixtime += 1;
       
  invjday(sat.satJd , timezone,true, year, mon, day, hr, minute, sec);
  Serial.println(String(day) + '/' + String(mon) + '/' + String(year) + ' ' + String(hr) + ':' + String(minute) + ':' + String(sec));
  Serial.println("azimuth = " + String( sat.satAz) + " elevation = " + String(sat.satEl) + " distance = " + String(sat.satDist));
  Serial.println("latitude = " + String( sat.satLat) + " longitude = " + String( sat.satLon) + " altitude = " + String( sat.satAlt));

  switch(sat.satVis){
              case -2:
                  Serial.println("Visible : Under horizon");
                  break;
              case -1:
                  Serial.println("Visible : Daylight");
                  break;
              default:
                  Serial.println("Visible : " + String(sat.satVis));   //0:eclipsed - 1000:visible
                  break;
          }

  Serial.println("Framerate: " + String(framerate) + " calc/sec");
  Serial.println();
     
  framerate=0;
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

  tkSecond.attach(1,Second_Tick);
}


void loop() {

  sat.findsat(unixtime);
  framerate += 1;

}

