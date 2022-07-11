#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Keyboard.h"
#include "Splash.h"

//constants and variables declaration
const String Version = "1.2";
bool Active = false;
int DelayTime = 0;
int DelayFinished = 0;
byte MenuID = 0;
byte Interval = 3;
byte Offset = 0;

//display text buffer
struct DisplayText
{
  String Line0;
  String Line1;
  String Line2;
  String Line3;
  String Line4;
  String Line5;
  String Line6;
  String Line7; 
};
DisplayText DT = {"", "", "", "", "", "", "", ""};

//BlastKey settings
byte BlastKey = 0; // Default 0
//allowed ASCII keys: 0-9, a-z, A-Z
int BlastKeys[62]={48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
                   97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
                   65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90};

//last Millis() value 
unsigned long previousMillis = 0;
//blink pulse interval
const long Pulse = 1000;  

//configure display type
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//update screen
void DisplayPrint(String Text, int Line){ 
  //set display text
  if (Line<1) {
    DT.Line0 = Text;
  }
  else if (Line==1) {
    DT.Line1 = Text;
  }
  else if (Line==2) {
    DT.Line2 = Text;
  }
  else if (Line==3) {
    DT.Line3 = Text;
  }
  else if (Line==4) {
    DT.Line4 = Text;
  }
  else if (Line==5) {
    DT.Line5 = Text;
  }
  else if (Line==6) {
    DT.Line6 = Text;
  }
  else if (Line==7) {
    DT.Line7 = Text;
  }

  //write display text
  display.clearDisplay();
  display.setCursor(0,0);
  display.print(DT.Line0);  
  display.setCursor(0,8);
  display.print(DT.Line1);  
  display.setCursor(0,16);
  display.print(DT.Line2);  
  display.setCursor(0,24);
  display.print(DT.Line3);  
  display.setCursor(0,32);
  display.print(DT.Line4);  
  display.setCursor(0,40);
  display.print(DT.Line5);  
  display.setCursor(0,48);
  display.print(DT.Line6);  
  display.setCursor(0,56);
  display.print(DT.Line7); 

  //pulse blink
  if (Active == true)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= Pulse) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      display.fillCircle(display.width()-5, 1, 1, SSD1306_INVERSE);
    }
  }

  //refresh display
  display.display();
  delay(1);
}

void setup(void) {
  //hardcoded build date
  const char BuildDate[] = __DATE__ " " __TIME__ "\0";
  String BuildDateStr = BuildDate; 
  BuildDateStr.replace(":","\0");
  BuildDateStr.replace(" ","\0");
  BuildDateStr.trim();

  //button inputs 4=on/off  5=down 6=up
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  
  //internal Reset pin
  digitalWrite(8, HIGH);  
  pinMode(8, OUTPUT);

  //randomize random generator
  randomSeed(analogRead(0));
   
  //initialize display
  //SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    delay(10);
    for(;;); // Don't proceed, loop forever
  }
  
  //set screen contrast
  //display.dim(true);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(127);
  
  //Splash intro
  int16_t i;
  display.clearDisplay(); // Clear display buffer
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for(i=0; i<display.width()/2; i+=4) {
    display.drawLine(display.width()/2, 23, display.width()/2+(i/2), 23, SSD1306_WHITE);
    display.drawLine(display.width()/2-(i/2), 23, display.width()/2, 23, SSD1306_WHITE);
    display.drawLine(display.width()/2, 31, display.width()/2+i, 31, SSD1306_WHITE);
    display.drawLine(display.width()/2-i, 31, display.width()/2, 31, SSD1306_WHITE);
    display.drawLine(display.width()/2, 39, display.width()/2+(i/2), 39, SSD1306_WHITE);
    display.drawLine(display.width()/2-(i/2), 39, display.width()/2, 39, SSD1306_WHITE);
    display.display();
  }
  delay(250);
  display.clearDisplay();
  display.drawBitmap( (display.width()  - Splash_width ) / 2, (display.height() - Splash_height) / 2, Splash_data, Splash_width, Splash_height, SSD1306_WHITE);
  display.setCursor(90,24);
  display.print("KEY");   
  display.setCursor(80,32);
  display.print("BLASTER");   
  display.display(); 
  delay(1500);

  //read last used BlastKey from EEprom
  BlastKey = EEPROM.read(0);
  if (BlastKey > (sizeof(BlastKeys) / sizeof(BlastKeys[0])-1))
  {
    BlastKey = 0;
  }

  //read last used interval from EEprom
  Interval = EEPROM.read(1);
  if ((Interval > 60) or (Interval < 1))
  {
    Interval = 3;
  }

  //read last used offset from EEprom and check if offset is smaller than the interval
  Offset = EEPROM.read(2);
  if (Offset >= (Interval * 4)){
    Offset = (Interval * 4) - 1;
  }

  //show default startscreen
  display.clearDisplay();
  DisplayPrint("KeyBlaster V"+Version,0);
  DisplayPrint("Build "+BuildDateStr,1);
  DisplayPrint("(c) Alexander Feuster",2);  
  DisplayPrint("Selected Key: "+String(char(BlastKeys[BlastKey])),3);
}

void loop() { 
  delay(1);
  
  //start/stop blasting
  if ((digitalRead(4)==LOW) && (digitalRead(5)==HIGH) && (digitalRead(6)==HIGH)){
    delay(150);
    if ((digitalRead(4)==LOW) && (digitalRead(5)==HIGH) && (digitalRead(6)==HIGH)){
      Active = !Active;
      delay(150);
    }
  }

  //reset the device
  if ((digitalRead(4)==HIGH) && (digitalRead(5)==LOW) && (digitalRead(6)==LOW)){
    delay(50);
    if ((digitalRead(4)==HIGH) && (digitalRead(5)==LOW) && (digitalRead(6)==LOW)){
      display.clearDisplay();
      display.drawBitmap( (display.width()  - Splash_width ) / 2, (display.height() - Splash_height) / 2, Splash_data, Splash_width, Splash_height, SSD1306_WHITE);
      display.setCursor(80,28);
      display.print("RESET"); 
      display.display();
      EEPROM.update(0,0);
      EEPROM.update(1,3);
      EEPROM.update(2,0);
      delay(1000);    
      digitalWrite(8, LOW); 
    }
  }

  //switch menu
  if ((digitalRead(4)==LOW) && (digitalRead(5)==LOW) && (digitalRead(6)==HIGH)){
    delay(150);
    if ((digitalRead(4)==LOW) && (digitalRead(5)==LOW) && (digitalRead(6)==HIGH)){
      if (MenuID == 2){
        MenuID = 1;
        DisplayPrint("Interval: "+String(Interval)+" seconds",3);
      }
      else if (MenuID == 1){
        MenuID = 0;
        DisplayPrint("Selected Key: "+String(char(BlastKeys[BlastKey])),3);
     }
     else if (MenuID == 0){
       MenuID = 2;
       DisplayPrint("Offset: +/- "+String(Offset*250)+" ms",3);
     }
     else{
        MenuID = 0;
        DisplayPrint("Selected Key: "+String(char(BlastKeys[BlastKey])),3);
     }
     delay(50);
    }    
  }
  if ((digitalRead(4)==LOW) && (digitalRead(5)==HIGH) && (digitalRead(6)==LOW)){
    delay(150);
    if ((digitalRead(4)==LOW) && (digitalRead(5)==HIGH) && (digitalRead(6)==LOW)){
      if (MenuID == 0){
        MenuID = 1;
        DisplayPrint("Interval: "+String(Interval)+" seconds",3);
      }
      else if (MenuID == 1){
        MenuID = 2;
        DisplayPrint("Offset: +/- "+String(Offset*250)+" ms",3);
      }
      else if (MenuID == 2){
        MenuID = 0;
        DisplayPrint("Selected Key: "+String(char(BlastKeys[BlastKey])),3);
      }
     else{
        MenuID = 0;
        DisplayPrint("Selected Key: "+String(char(BlastKeys[BlastKey])),3);
      }
     delay(50);
    }    
  }

  if (MenuID == 0){
    //select lower BlastKey
    if ((digitalRead(4)==HIGH) && (digitalRead(5)==LOW) && (digitalRead(6)==HIGH)){
      Active = false;
      if (BlastKey > 0)
      {
          BlastKey--;
      }    
      else
      {
        BlastKey = (sizeof(BlastKeys) / sizeof(BlastKeys[0])-1);
      }
      EEPROM.update(0,BlastKey);
      DisplayPrint("Selected Key: "+String(char(BlastKeys[BlastKey])),3);
    }
  
    //select higher BlastKey
    if ((digitalRead(4)==HIGH) && (digitalRead(5)==HIGH) && (digitalRead(6)==LOW)){
      Active = false;
      BlastKey++;
      if (BlastKey > (sizeof(BlastKeys) / sizeof(BlastKeys[0])-1))
      {
        BlastKey = 0;
      }
      EEPROM.update(0,BlastKey);
      DisplayPrint("Selected Key: "+String(char(BlastKeys[BlastKey])),3);
    }
  }

  if (MenuID == 1){  
    //select lower interval
    if ((digitalRead(4)==HIGH) && (digitalRead(5)==LOW) && (digitalRead(6)==HIGH)){
      Active = false;
      if (Interval > 2)
      {
        Interval--;
      }    
      else
      {
        Interval = 60;
      }
      if (Offset >= (Interval * 4)){
        Offset = (Interval * 4) - 1;
      }
      EEPROM.update(1,Interval);
      EEPROM.update(2,Offset);
      DisplayPrint("Interval: "+String(Interval)+" seconds",3);
    }  
    //select higher interval
    if ((digitalRead(4)==HIGH) && (digitalRead(5)==HIGH) && (digitalRead(6)==LOW)){
      Active = false;
      if (Interval > 59)
      {
        Interval = 1;
      }    
      else
      {
        Interval++;
      }
      if (Offset >= (Interval * 4)){
        Offset = (Interval * 4) - 1;
      }
      EEPROM.update(1,Interval);
      EEPROM.update(2,Offset);
      DisplayPrint("Interval: "+String(Interval)+" seconds",3);
    }  
  }

  if (MenuID == 2){  
    //select lower offset
    if ((digitalRead(4)==HIGH) && (digitalRead(5)==LOW) && (digitalRead(6)==HIGH)){
      Active = false;
      if (Offset > 0)
      {
        Offset--;
      }    
      else
      {
        Offset = (Interval * 4) - 1;
      }
      EEPROM.update(2,Offset);
      DisplayPrint("Offset: +/- "+String(Offset*250)+" ms",3);
    }  
    //select higher offset
    if ((digitalRead(4)==HIGH) && (digitalRead(5)==HIGH) && (digitalRead(6)==LOW)){
      Active = false;
      if (Offset < (Interval * 4)-1){
        Offset++;
      }
      else
      {
        Offset = 0;
      }    
      EEPROM.update(2,Offset);
      DisplayPrint("Offset: +/- "+String(Offset*250)+" ms",3);
    }  
  }
  
  //key blasting
  if (Active == true) {   
    DelayTime++;
    if (DelayTime>=DelayFinished){
      Keyboard.begin();
      //send BlastKey but exchange yz for german keyboard layout
      if (BlastKeys[BlastKey] == 121){
        Keyboard.print("z");
      }
      else if(BlastKeys[BlastKey] == 122){
        Keyboard.print("y");  
      }
      else {
        Keyboard.print(char(BlastKeys[BlastKey]));
      }
      Keyboard.end();
      DisplayPrint("KeyBlaster active",5);
      DelayTime=0;    
      DelayFinished=random((Interval*1000)-(Offset*250),(Interval*1000)+(Offset*250));
      DisplayPrint("Next Key in "+String(DelayFinished)+"ms",7);
    }
  }
  else {   
    DisplayPrint("KeyBlaster inactive",5);    
    DisplayPrint("",7);
  }
  
}
