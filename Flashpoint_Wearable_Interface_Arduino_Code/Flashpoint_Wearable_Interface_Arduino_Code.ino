/*
 SD card datalogger

This example shows how to log data from three analog sensors
to an SD card using the SD library.

The circuit:
* analog sensors on analog ins 0, 1, and 2
* SD card attached to SPI bus as follows:
** MOSI - pin 11 - Arduino Mega Pin 51
** MISO - pin 12 - Arduino Mega Pin 50 
** CLK - pin 13 - Arduino Mega Pin 52
** CS - pin 4 

created  24 Nov 2010
modified 9 Apr 2012
by Tom Igoe

This example code is in the public domain.

*/
#include <SoftwareSerial.h>
#include <TinyGPS.h>

#include <SPI.h>
#include <SD.h>

/* This sample code demonstrates the normal use of a TinyGPS object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 11(rx) and 10(tx).
*/

TinyGPS gps;
//SoftwareSerial ss(9, 8);
int pulsePin = A0;                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 13;                // pin to blink led at each beat
int fadePin = A5;                  // pin to do fancy classy fading blink at each beat
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin

static void smartdelay(unsigned long ms);
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);
static void print_int(unsigned long val, unsigned long invalid, int len);
static int fixhour(byte bhour);

// these variables are volatile because they are used during the interrupt service routine!
volatile int BPM;                   // used to hold the pulse rate
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // holds the time between beats, must be seeded! 
volatile boolean Pulse = false;     // true when pulse wave is high, false when it's low
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

int new_hour;


const int chipSelect = 22;


void setup() {

  pinMode(blinkPin,OUTPUT);         // pin that will blink to your heartbeat!
  pinMode(fadePin,OUTPUT);          // pin that will fade to your heartbeat!
 Serial.begin(9600);
 Serial1.begin(9600);
 interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 
   // UN-COMMENT THE NEXT LINE IF YOU ARE POWERING The Pulse Sensor AT LOW VOLTAGE, 
   // AND APPLY THAT VOLTAGE TO THE A-REF PIN
   //analogReference(EXTERNAL);   
  

// Open serial communications and wait for port to open:
 
 //Serial.begin(9600);
 while (!Serial) {
   ; // wait for serial port to connect. Needed for native USB port only
 }

 Serial.print("Initializing SD card...");

 // see if the card is present and can be initialized:
 if (!SD.begin(chipSelect)) {
   Serial.println("Card failed, or not present");
   // don't do anything more:
   return;
 }
 Serial.println("card initialized.");
 
 File dataFile = SD.open("datalog.txt", FILE_WRITE);
 if (dataFile) {
  dataFile.print("Testing TinyGPS library v. "); Serial.println(TinyGPS::library_version());
  dataFile.println("by Mikal Hart");
  dataFile.println();
  dataFile.println("Latit  Long   Alt   Fix  Date   Time     Date     ");
  dataFile.println("(deg)  (deg)  (m)   Age                  Age     ");
  dataFile.println("--------------------------------------------------------------------");
  dataFile.println();
   dataFile.close();
 }
 else {
   Serial.println("error opening datalog.txt in setup");
 }
  Serial.print("Testing TinyGPS library v. "); Serial.println(TinyGPS::library_version());
  Serial.println("by Mikal Hart");
  Serial.println();
  Serial.println("Latit  Long   Alt   Fix  Date   Time     Date     ");
  Serial.println("(deg)  (deg)  (m)   Age                  Age     ");
  Serial.println("----------------------------------------------------------------------");
  
  Serial2.begin(9600);
 
}

void loop() {
  int year;
  byte month, day, hour, minutes, second, hundredths;
  long lat, lon;
  float flat, flon, falt;
  double lat1, lon1;
  unsigned long fix_age, gps_age, time_age, date, time, chars = 0;
  unsigned short sentences = 0, failed = 0;
  static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
  String SerialData, location_data, datalog_name, date_time;

  sendDataToProcessing('S', Signal);     // send Processing the raw Pulse Sensor data
  if (QS == true){                       // Quantified Self flag is true when arduino finds a heartbeat
        fadeRate = 255;                  // Set 'fadeRate' Variable to 255 to fade LED with pulse
        sendDataToProcessing('B',BPM);   // send heart rate with a 'B' prefix
        sendDataToProcessing('Q',IBI);   // send time between beats with a 'Q' prefix
        QS = false;                      // reset the Quantified Self flag for next time    
     }
  
  ledFadeToBeat();
  
  smartdelay(0);                             //  take a break
  
 // open the file. note that only one file can be open at a time,
 // so you have to close this one before opening another.

    File dataFile = SD.open("datalog.txt", FILE_WRITE);
 // if the file is available, write to it:
 if (dataFile) {
   gps.f_get_position(&flat, &flon, &gps_age);
//   falt = gps.f_altitude(); // +/- altitude in meters
   gps.get_position(&lat,&lon, &fix_age);
   gps.crack_datetime(&year, &month, &day,
       &hour, &minutes, &second, &hundredths, &time_age);
   smartdelay(0);
   new_hour=  fixhour(hour);
   //datalog_name = (String)month + "_" + (String)day + "_" + (String)year + "_" + 
   //        (String)hour + "_" + (String)minutes + "_" + (String)second + "_datalog.txt";
   date_time = (String)month+ "/" + (String)day + "/" + (String)year + " " + 
          (String)new_hour + ":" + (String)minutes + ":" + (String)second +"]";
//   lat1 = lat/1000000.0;
//   lon1 = lon/1000000.0;
   location_data = (String)lat + "," + (String)(lon) + ",";//+ (String)falt + " " + (String)gps_age +" ";
   SerialData = location_data + (String)BPM + ","+ date_time;
   dataFile.println(SerialData);
   dataFile.close();
   // print to the serial port too:
   Serial.println(SerialData);
//   Serial.print("The lat long are: ");
//   Serial.print(lat);
//   Serial.print(" ");
//   Serial.println(lon);
//   Serial.print("The lat long are1000: ");
//   Serial.print(lat/1000000.0,6);
//   Serial.print(" ");
//   Serial.println(lon/1000000.0,6);
//   Serial.print("The lat long are doub: ");
//   Serial.print(lat1);
//   Serial.print(" ");
//   Serial.println(lon1);
   if(Serial1.available()){
    Serial1.println(SerialData);
    //Serial1.println("   This is the real stuff");
 //   Serial.println("I am sending stuff see?");
//    Serial.println(SerialData);
//    Serial.println("endofstuff");
   }
 }
 // if the file isn't open, pop up an error:
 else {
   Serial.println("error opening datalog.txt");
 }
}


static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (Serial2.available())
      gps.encode(Serial2.read());
  } while (millis() - start < ms);
}

static void print_float(float val, float invalid, int len, int prec)
{
  if (val == invalid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(' ');
  }
  smartdelay(0);
}
static void print_str(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  smartdelay(0);
}

static void print_int(unsigned long val, unsigned long invalid, int len)
{
  char sz[32];
  if (val == invalid)
    strcpy(sz, "*******");
  else
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  Serial.print(sz);
  smartdelay(0);
}

static int fixhour(byte bhour)
{
  int h = (int)bhour;

  if(h< 20)
  {
    h+=4;
  }
  else
  {
    h = 20-h;
  }

  return h;
}

void ledFadeToBeat(){
    fadeRate -= 15;                         //  set LED fade value
    fadeRate = constrain(fadeRate,0,255);   //  keep LED fade value from going into negative numbers!
    analogWrite(fadePin,fadeRate);          //  fade LED
  }


void sendDataToProcessing(char symbol, int data ){
    if (symbol != 'S'){
    Serial.print(symbol);                // symbol prefix tells Processing what type of data is coming
    Serial.println(data);                // the data to send culminating in a carriage return
  }
}

  


