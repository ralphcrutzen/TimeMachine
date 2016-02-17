/*  RGBDigit Countdown Clock
    Copyright (C) 2016 Ralph Crützen

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h> //https://github.com/adafruit/Adafruit_NeoPixel
#include <DS3232RTC.h> //http://github.com/JChristensen/DS3232RTC
#include <IRremote.h>
#include <Time.h> //https://github.com/PaulStoffregen/Time

#include <RGBDigit.h>

// If necessary, change the number of digits in the next line
#define nDigits 4
RGBDigit rgbDigit(nDigits);       // uses default pin 12

#define RED 0
#define GREEN 1
#define BLUE 2

const int switchPin = 2;
const int ledPin = 3;
const int buttonPin = 8;
const int potPin = A0;

const int nModesA = 4;
const int nModesB = 8;
bool hold = false;
int mode = 0;
bool autoColor = false;
int autoColorPos = 0;
int ledBrightness = 0;
int switchState = 0;
int buttonState = 0;
int lastButtonState = 0;
int buttonPressCounter = 0;

int potValue = 0;
int temp;
int alarmDay = 19;
int alarmMonth = 2;
int alarmYear;
int daysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
String months[12] = {"jan", "feb", "maa", "apr", "mei", "jun", "jul", "aug", "sep", "okt", "nov", "dec"};

void setup() {
  pinMode(switchPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  Serial.begin(9600);
  rgbDigit.begin();
  rgbDigit.clearAll();
  // set the time of the RTC (h, m, s, d, m, y)
  //rgbDigit.setTimeDate(18, 57, 0, 31, 1, 16);
  temp = (int)rgbDigit.readTemp();
}

void loop() {
  analogWrite(ledPin, ledBrightness);
  switchState = digitalRead(switchPin);
  if (switchState == HIGH) {
    ledBrightness = 255;
    potValue = analogRead(potPin); 
    buttonState = digitalRead(buttonPin);
    
    if (buttonState != lastButtonState) {
      if (buttonState == LOW) { // from on to off
        rgbDigit.clearDot(1);
        if (!hold)
          if (mode < 10)
            mode = (mode + 1) % nModesA;
          else
            mode = ((mode - 9) % nModesB) + 10;
        else
          hold = false;
      }
      else { // from off to on
        buttonPressCounter = 0;
      }
      delay(50);
    }
    else {
      if (buttonState == LOW) {
        buttonPressCounter = 0;
      }
      else {
        buttonPressCounter++;
      }
    }
    lastButtonState = buttonState;
  
    if (buttonPressCounter > 300) {
      hold = true;
      buttonPressCounter = 0;
      if (mode < 10)
        mode = 10;
      else
        mode = 0;
    }
    
    switch (mode) {
      case 0:
        showTime();
        break;
      case 1:
        showDate();
        break;
      case 2:
        showTemp();
        break;
      case 3:
        showTimer();
        break;
      case 10:
        setAutoColor();
        break;
      case 11:
        setCurrentYear();
        break;
      case 12:
        setCurrentMonth();
        break;
      case 13:
        setCurrentDay();
        break;
      case 14:
        setCurrentHour();
        break;
      case 15:
        setCurrentMinute();
        break;
      case 16:
        setAlarmMonth();
        break;
      case 17:
        setAlarmDay();
        break;      
      default:
        for (int i=0; i<4; i++)
          rgbDigit.setDigit('-', i, 255, 255, 16);
        break;   
    }
    autoColorPos = (autoColorPos + 1) % 1024;
    //ledBrightness = (ledBrightness + 1) % 256;
  }
  else { // switchState == LOW
    rgbDigit.clearAll();
    ledBrightness = 0;
  }
}

void setAlarmDay() {
  int m = rgbDigit.getMonth();
  int y = rgbDigit.getYear();
  // Allow for Feb 29 if:
  // AlarmMonth == 2
  // and
  // (
  //    current month <= 2 and current year is leap
  //    or
  //    current month > 2 and next year (current + 1) is leap
  // )
  if (alarmMonth == 2 && ((m <= 2 && y%4 == 0) || (m > 2 && (y+1)%4 == 0)))
    alarmDay = potValue * 29 / 1024 + 1; // allow Feb 29;
  else
    alarmDay = potValue * daysPerMonth[alarmMonth-1] / 1024 + 1;

  int d1 = alarmDay/10;
  int d2 = alarmDay - d1*10;
  int m1 = alarmMonth/10;
  int m2 = alarmMonth - m1*10;
  rgbDigit.setDigit(d1, 0, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit(d2, 1, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit(months[alarmMonth-1][0], 2, color(RED), color(GREEN), color(BLUE));
  if (alarmMonth == 6 || alarmMonth == 7) // jun -> jn and jul -> jl
    rgbDigit.setDigit(months[alarmMonth-1][2], 3, color(RED), color(GREEN), color(BLUE));
  else
    rgbDigit.setDigit(months[alarmMonth-1][1], 3, color(RED), color(GREEN), color(BLUE));
}

void setAlarmMonth() {
  alarmMonth = potValue * 12 / 1024 + 1;
  int d1 = alarmDay/10;
  int d2 = alarmDay - d1*10;
  int m1 = alarmMonth/10;
  int m2 = alarmMonth - m1*10;
  rgbDigit.clearDigit(0);
  for (int i=0; i<3; i++)
    rgbDigit.setDigit(months[alarmMonth-1][i], i+1, color(RED), color(GREEN), color(BLUE));
}

void setCurrentYear() {
  int h  = rgbDigit.getHour();
  int mi = rgbDigit.getMinute();
  int s  = rgbDigit.getSecond();
  int d  = rgbDigit.getDay();
  int mo = rgbDigit.getMonth();
  int y = 16 + potValue / 1024.0 * 50;
  rgbDigit.setDigit(2, 0, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit(0, 1, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit(y/10, 2, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit(y - (y/10) * 10, 3, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setTimeDate(h, mi, s, d, mo, y);
}

void setCurrentMonth() {
  int h  = rgbDigit.getHour();
  int mi = rgbDigit.getMinute();
  int s  = rgbDigit.getSecond();
  int d  = rgbDigit.getDay();
  int mo = potValue * 12 / 1024 + 1;
  int y = rgbDigit.getYear();
  rgbDigit.clearDigit(0);
  for (int i=0; i<3; i++)
    rgbDigit.setDigit(months[mo-1][i], i+1, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setTimeDate(h, mi, s, d, mo, y);
}

void setCurrentDay() {
  int h  = rgbDigit.getHour();
  int mi = rgbDigit.getMinute();
  int s  = rgbDigit.getSecond();
  int mo = rgbDigit.getMonth();
  int y = rgbDigit.getYear();
  int d;  
  if (mo == 2 && (y%4 == 0)) // leap year
    d = potValue * 29 / 1024 + 1; // allow Feb 29;
  else
    d = potValue * daysPerMonth[mo-1] / 1024 + 1;  

  int d1 = d/10;
  int d2 = d - d1*10;
  int m1 = mo/10;
  int m2 = mo - m1*10;
  rgbDigit.setDigit(d1, 0, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit(d2, 1, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit(months[mo-1][0], 2, color(RED), color(GREEN), color(BLUE));
  if (mo == 6 || mo == 7) // jun -> jn and jul -> jl
    rgbDigit.setDigit(months[mo-1][2], 3, color(RED), color(GREEN), color(BLUE));
  else
    rgbDigit.setDigit(months[mo-1][1], 3, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setTimeDate(h, mi, s, d, mo, y);
}

void setCurrentHour() {
  int h  = potValue * 24 / 1024;
  int mi = rgbDigit.getMinute();
  int s  = rgbDigit.getSecond();
  int d  = rgbDigit.getDay();
  int mo = rgbDigit.getMonth();
  int y = rgbDigit.getYear();
  rgbDigit.setDigit(h/10, 0, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit(h - (h/10) * 10, 1, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit(mi/10, 2, 64, 64, 64);
  rgbDigit.setDigit(mi - (mi/10) * 10, 3, 64, 64, 64);
  bool dot = ((millis() / 1000) % 2) == 1;
  if (dot)
    //Serial.println("On");
    rgbDigit.segmentOn(1, 7, 64, 64, 64);
  else
    //Serial.println("Off");
    rgbDigit.segmentOff(1, 7);
  rgbDigit.setTimeDate(h, mi, s, d, mo, y);
}

void setCurrentMinute() {
  int h  = rgbDigit.getHour();
  int mi = potValue / 1024.0 * 60;
  int s  = rgbDigit.getSecond();
  int d  = rgbDigit.getDay();
  int mo = rgbDigit.getMonth();
  int y = rgbDigit.getYear();
  rgbDigit.setDigit(h/10, 0, 64, 64, 64);
  rgbDigit.setDigit(h - (h/10) * 10, 1, 64, 64, 64);
  rgbDigit.setDigit(mi/10, 2, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit(mi - (mi/10) * 10, 3, color(RED), color(GREEN), color(BLUE));
  bool dot = ((millis() / 1000) % 2) == 1;
  if (dot)
    //Serial.println("On");
    rgbDigit.segmentOn(1, 7, 64, 64, 64);
  else
    //Serial.println("Off");
    rgbDigit.segmentOff(1, 7);
  rgbDigit.setTimeDate(h, mi, s, d, mo, y);
}

void setAutoColor() {
  autoColor = potValue >= 512;
  if (autoColor) {
    rgbDigit.setDigit('a', 0, color(RED), color(GREEN), color(BLUE));
    rgbDigit.setDigit('u', 1, color(RED), color(GREEN), color(BLUE));
    rgbDigit.setDigit('t', 2, color(RED), color(GREEN), color(BLUE));
    rgbDigit.setDigit('o', 3, color(RED), color(GREEN), color(BLUE));
  }
  else {
    rgbDigit.setDigit('h', 0, color(RED), color(GREEN), color(BLUE));
    rgbDigit.setDigit('a', 1, color(RED), color(GREEN), color(BLUE));
    rgbDigit.setDigit('n', 2, color(RED), color(GREEN), color(BLUE));
    rgbDigit.setDigit('d', 3, color(RED), color(GREEN), color(BLUE));
  }
}

void showTimer() {
  int d = rgbDigit.getDay();
  int m = rgbDigit.getMonth();
  int y = rgbDigit.getYear();
  int nowDaysSinceNY = daysSinceNewyear(d, m, y);
  
  if (alarmMonth < m || (alarmMonth == m && alarmDay < d))
    alarmYear = y + 1;
  else
    alarmYear = y;

  int alarmDaysSinceNY = daysSinceNewyear(alarmDay, alarmMonth, alarmYear);

  int timer = alarmDaysSinceNY - nowDaysSinceNY;;
  if (alarmYear > y) {
    timer += 365;
    if (alarmYear%4 == 0 || y%4 == 0) timer++; // Is this correct?
  }

  int timer1 = timer/1000;
  int timer2 = (timer - timer1*1000) / 100;
  int timer3 = (timer - timer1*1000 - timer2*100) / 10;
  int timer4 = timer - timer1*1000 - timer2*100 - timer3*10;
  if (timer2 == 0)
    rgbDigit.clearDigit(0);
  else
    rgbDigit.setDigit(timer2, 0, color(RED), color(GREEN), color(BLUE));
  if (timer2 == 0 && timer3 == 0)
    rgbDigit.clearDigit(1);
  else
    rgbDigit.setDigit(timer3, 1, color(RED), color(GREEN), color(BLUE));
  if (timer2 == 0 && timer3 == 0 & timer4 == 0)
    party();
  else
    rgbDigit.setDigit(timer4, 2, color(RED), color(GREEN), color(BLUE));
  rgbDigit.clearDigit(3);
}

int daysSinceNewyear(int d, int m, int y) {
  int days = 0;
  for (int i = 0; i < m - 1; i++)
    days += daysPerMonth[i];
  days += d;
  if (m > 2 && (y%4 == 0)) { // Leap year
    days++;
  }
  return days;
}

void showTime() {
  int h = rgbDigit.getHour();
  int h1 = h/10;
  int h2 = h - h1*10;
  rgbDigit.setDigit(h1, 0, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit(h2, 1, color(RED), color(GREEN), color(BLUE));
  int m = rgbDigit.getMinute();
  int m1 = m/10;
  int m2 = m - m1*10;
  rgbDigit.setDigit(m1, 2, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit(m2, 3, color(RED), color(GREEN), color(BLUE));
  bool dot = ((millis() / 1000) % 2) == 1;
  if (dot)
    //Serial.println("On");
    rgbDigit.segmentOn(1, 7, color(RED), color(GREEN), color(BLUE));
  else
    //Serial.println("Off");
    rgbDigit.segmentOff(1, 7);
}

void showDate() {
  int d = rgbDigit.getDay();
  int d1 = d/10;
  int d2 = d - d1*10;
  if (d1 == 0) {
    rgbDigit.setDigit(d2, 0, color(RED), color(GREEN), color(BLUE));
    rgbDigit.clearDigit(1);
  }
  else {
    rgbDigit.setDigit(d1, 0, color(RED), color(GREEN), color(BLUE));
    rgbDigit.setDigit(d2, 1, color(RED), color(GREEN), color(BLUE));
  }
  int m = rgbDigit.getMonth();
  int m1 = m/10;
  int m2 = m - m1*10;
  rgbDigit.setDigit(months[m-1][0], 2, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit(months[m-1][1], 3, color(RED), color(GREEN), color(BLUE));
}

void party() {
  rgbDigit.clearAll();
  for (int i=0; i<potValue*32/1024+1; i++)
    rgbDigit.segmentOn(random(4), random(8), random(256), random(256), random(256));
  delay(100);
}

void showTemp() {
  if (autoColorPos == 0)
    temp = (int)rgbDigit.readTemp();
  int temp1 = temp/10;
  int temp2 = temp - temp1*10;
  if (temp1 != 0)
    rgbDigit.setDigit(temp1, 0, color(RED), color(GREEN), color(BLUE));
  else
    rgbDigit.clearDigit(0);
  rgbDigit.setDigit(temp2, 1, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit('*', 2, color(RED), color(GREEN), color(BLUE));
  rgbDigit.setDigit('c', 3, color(RED), color(GREEN), color(BLUE));
}

int color(int rgb) {
  if (autoColor)
    return wheel(autoColorPos, rgb);
  else
    return wheel(potValue, rgb);
}


// Input a value 0 to 1023 to get a color value.
// The colours are a transition g - r - b - back to g.
int wheel(int WheelPos, byte rgb) {
  //if(WheelPos > 960) return 255; // white
  WheelPos /= 4;
  if(WheelPos < 85) {
   if (rgb == RED) return WheelPos * 3;
   if (rgb == GREEN) return 255 - WheelPos * 3;
   if (rgb == BLUE) return 0;
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   if (rgb == RED) return 255 - WheelPos * 3;
   if (rgb == GREEN) return 0;
   if (rgb == BLUE) return WheelPos * 3;
  } else {
   WheelPos -= 170;
   if (rgb == RED) return 0;
   if (rgb == GREEN) return WheelPos * 3;
   if (rgb == BLUE) return 255 - WheelPos * 3;
  }
}
