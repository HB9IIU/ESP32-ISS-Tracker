// Arduino Sunrise Library
// https://github.com/JChristensen/Sunrise
// Copyright (C) 2021 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html
//
// Arduino Sunrise Library example sketch.
// Prints a listing of sunrise and sunset times for a given range of years.
// Jack Christensen 03Jan2021

#include <JC_Sunrise.h>     // https://github.com/JChristensen/JC_Sunrise
#include <Streaming.h>      // https://github.com/janelia-arduino/Streaming
#include <TimeLib.h>        // https://github.com/PaulStoffregen/Time
#include <Timezone.h>       // https://github.com/JChristensen/Timezone

constexpr int startYear {2015}, endYear {2030};

// latitude & longitude for sunrise & sunset calculations
constexpr float myLat {45.8171}, myLon {-84.7278};

// timezone objects for US Eastern Time
TimeChangeRule EDT {"EDT", Second, Sun, Mar, 2, -240};  // daylight time = UTC - 4 hours
TimeChangeRule EST {"EST", First, Sun, Nov, 2, -300};   // standard time = UTC - 5 hours
Timezone Eastern(EDT, EST);
TimeChangeRule *tcr;

JC_Sunrise sun {myLat, myLon, JC_Sunrise::officialZenith};

void setup()
{
    delay(500);
    Serial.begin(115200);
    Serial << F( "\n" __FILE__ "\n" __DATE__ " " __TIME__ "\n" );
    
    tmElements_t tm;
    tm.Year = CalendarYrToTm(startYear);
    tm.Month = 1;
    tm.Day = 1;
    tm.Hour = 12;
    tm.Minute = 0;
    tm.Second = 0;
    time_t utcNow = makeTime(tm);
    
    while (year(utcNow) < endYear + 1) {
        time_t locNow = Eastern.toLocal(utcNow, &tcr);  // tz adjustment
        time_t rise, set;
        sun.calculate(locNow, tcr->offset, rise, set);
        print8601(utcNow);
        print8601(locNow);
        printHHMM(rise);
        Serial << ' ';
        printHHMM(set);
        Serial << endl;
        utcNow += SECS_PER_DAY;
    }
}

void loop() {}

// ---- Formatting Functions ---- //

// print a timestamp as hh:mm
void printHHMM(time_t t)
{
    printI00(hour(t), ':');
    printI00(minute(t));
}

// print a timestamp to serial in ISO8601 format, followed by a space
void print8601(time_t t)
{
    printI00(year(t), '-');
    printI00(month(t), '-');
    printI00(day(t), ' ');
    printI00(hour(t), ':');
    printI00(minute(t), ' ');
}

// Print an integer in "00" format (with leading zero),
// followed by a delimiter character to Serial.
// Input value assumed to be between 0 and 99.
void printI00(int val, char delim)
{
    if (val < 10) Serial << '0';
    Serial << _DEC(val);
    if (delim > 0) Serial << delim;
    return;
}

// Print an integer in "00" format (with leading zero).
// Input value assumed to be between 0 and 99.
void printI00(int val)
{
    if (val < 10) Serial << '0';
    Serial << _DEC(val);
    return;
}
