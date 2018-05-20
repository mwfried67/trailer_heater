
//***V11 Notes 180520*********************
//Tried for first time a couple weeks back. Worked good,except that when the Tin reached Tset the fan stayed on until the water cooled down. Do not want
//fan and pump to stay on that long. WOuld rather it shout down sooner.
//Changed this line:
// else if (((Tset <= Tin) and (stat_h == 0)) or (Twat < (Tin + min_wat_delta))) { //Turn off fan and circulation. Want to leave heat on if tank is not hot.
//To this
// else if (((Tset <= Tin) and (stat_h == 0)) or (Twat < (Tin + min_wat_delta+20))) { //Turn off fan and circulation. Want to leave heat on if tank is not hot.
//***V2.9 Notes 170326***********************
//Fixed issue of 2.8 where would not shut down for no heat. Tested. Saved. DOne.
//***V2.8 Notes 170326***********************
//Flashed today. Seems to work best yet. Developed to work with 95 deg C thermal switch. Thot goes up to 214 before it cuts out. 
//Will probably only work with anti-freeze. 
//Twat_max = 180;// Tank water temperature above which to turn off heater. Set really high to run off overtemp in L5.(V2.x)
//Twat_min;// Final arbritrated tank out temp below which to turn heater on.2
//Twat_min_d = 165;//Default tank water temperature above which to turn on heat. (when not running up against overtemp)
//min_wat_delta = 50;//Minimum difference between room temp and water temp to run fan.
//Remaining issue is noise in the Tset signal. Done dinking around with this for now though. Works pretty good.
//Discovered another issues. Does not shut down for no heat )+R)(+ER)(+ER+()EWR09
//***V2.4 Notes 170319***********************
//Issues with 2.3
// 1) Fan stays on when neither pump is on when room gets to room temp setpoint.
// 2) When hit the room temp setpoint the heater pump goes of and does not run until hit overtemp SP.
//Flashed 170319. Debugged. WOrks. This is now the hothot ticket.
//***V2.3 Notes 170319***********************
//Flashed. Works. This is a keeper. ....
//Removed tracers. Cleaned up format.
//This version runs over the overtemp protection switch in the ecotemp L5
//Worked OK with production 85 deg C unit. Waiting for 95 and 100 deg C units.
//***V2.2 Notes 170312***********************
//This version has not been flashed or tested.
//This version is inteded to turn off when overtemp ss**170312***********************
//Has not yet been flashed.
//Shows HeaterRes error on LCD. REmoved tracers.
//***************
//References for multi one wire buses
//https://arduino-info.wikispaces.com/Brick-Temperature-DS18B20#multibus
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

// Set up which digital IO pins will be used for DS18B20 digital temperature sensors.
//I2C or DallasTemp does not play nice with sensors on 2,3.
#define ONE_WIRE_BUS4 4
#define ONE_WIRE_BUS5 5
#define ONE_WIRE_BUS6 6
#define ONE_WIRE_BUS7 7
#define TEMPERATURE_PRECISION 9

// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
//Running 2 buses becuase I do not want to have to find addresses when replacing sensors.

OneWire oneWire4(ONE_WIRE_BUS4);
OneWire oneWire5(ONE_WIRE_BUS5);
OneWire oneWire6(ONE_WIRE_BUS6);
OneWire oneWire7(ONE_WIRE_BUS7);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors4(&oneWire4);
DallasTemperature sensors5(&oneWire5);
DallasTemperature sensors6(&oneWire6);
DallasTemperature sensors7(&oneWire7);
//unsigned long tm;
int su = 1;// indicates first setup loop
float tmp;
float Tin;// Temperature inside living space
float Tout;//Outdoor temperature
int Twat;//Water tank temperature
int Thot;//Water temperature from heater
float Tset;//Desired inside temperature
String error = "OK       ";//9 Char error string
int stat_c;//Circulation pump status
int stat_h; //Heater pump status
int stat_f;// Fan status
int hpmp = 8;//Digital OI for pump to heater
int fan = 9;//Digital IO for fan.
int cpmp = 10;//Digital IO to control circlation pump.
float Tset_raw;
int ctr;
int Twat_max = 180;// Tank water temperature above which to turn off heater. Set really high to run off overtemp in L5.(V2.x)
int Twat_min;// Final arbritrated tank out temp below which to turn heater on.
int Twat_min_d = 165;//Default tank water temperature above which to turn on heat. (when not running up against overtemp)
int Twat_peak; //Max water temperature when heater shuts off for overtemp
int min_wat_delta = 50;//Minimum difference between room temp and water temp to run fan.
int Thot_start;
const int Tset_pin = A0;
int ctr1;
int no_heat_ctr = 0;//Counter to determine how many times to attempt to start heater before NoHeat fault
//int heater_reset = 0;
int hyst = 2; //Room temperature control hysterisis

void lcd_print_neg(int in, int col, int row)// LC_I2C will not print negative integers without this.
{
  lcd.setCursor ( col, row);
  lcd.print("    ");
  lcd.setCursor ( col, row);
  if (in < 0) {
    in = Tout * -2 + Tout;
    lcd.print("-");
    lcd.print(in);
  }
  else
  {
    lcd.print(in, 1);
  }
}

void lcddrive()
{
  //Serial.print(Tin);
  //LCD Sense (COL, ROW)
  lcd_print_neg(Tin, 5, 0);
  lcd_print_neg(Tout, 15, 0);
  lcd_print_neg(Twat, 5, 1);
  lcd_print_neg(Thot, 15, 1);
  lcd_print_neg(Tset, 5, 2);

  lcd.setCursor ( 7, 3);
  lcd.print(error);//9 Char Error String

  lcd.setCursor ( 17, 3);
  lcd.print(stat_c);

  lcd.setCursor ( 18, 3);
  lcd.print(stat_h);

  lcd.setCursor ( 19, 3);
  lcd.print(stat_f);
}

void get_input()
{
  int x = 0;
  int sampnum = 30;//Number of times it will attempt to poll sensor.
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  // Serial.print(" Requesting temperatures...");
  do {
    sensors4.requestTemperatures(); // Send the command to get temperatures
    tmp = sensors4.getTempFByIndex(0);
    x++;
  } while ((tmp < -120) and (x < sampnum));
  if (su == 1) {
    Tin = Tin * .75 + tmp * 0.25;
  }
  else {
    Tin = tmp;
  }
  x = 0;

  do {
    sensors5.requestTemperatures(); // Send the command to get temperatures
    tmp = sensors5.getTempFByIndex(0);
    x++;
  } while ((tmp < -120) and (x < sampnum));
  if (su == 1) {
    Tout = Tout * .75 + tmp * 0.25;
  }
  else {
    Tout = tmp;
  }
  x = 0;

  do {
    sensors6.requestTemperatures(); // Send the command to get temperatures
    Twat = sensors6.getTempFByIndex(0);
    x++;
  } while ((Twat <  -120) and (x < sampnum));
  if ( x == sampnum) {
    error = "TwatError ";
  }
  x = 0;

  do {
    sensors7.requestTemperatures(); // Send the command to get temperatures
    Thot = sensors7.getTempFByIndex(0);
    x++;
  } while ((Thot < -120) and (x < sampnum));
  if ( x == sampnum) {
    error = "ThotError ";
  }
  x = 0;

  Tset_raw = analogRead(Tset_pin); //Get setpoint from analog in
  tmp = map(Tset_raw, 0, 1023, 8000, 2000);
  tmp = tmp/100;
  
  // Serial.print("tmp=");
  // Serial.print(tmp, 1);
  //Serial.print("\n");
  if (su == 1) {
    Tset = Tset * .75 + tmp * 0.25;
  }
  else {
    Tset = tmp;
  }
}

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");
  pinMode(hpmp, OUTPUT);
  pinMode(fan, OUTPUT);
  pinMode(cpmp, OUTPUT);
  digitalWrite(hpmp, LOW);
  digitalWrite(fan, LOW);
  digitalWrite(cpmp, LOW);

  // Start up the library
  sensors4.begin();
  sensors5.begin();
  sensors6.begin();
  sensors7.begin();
  // set the resolution to 9 bit - example I found would need to be revised for mfried refeences.
  //sensors2.setResolution(0, 9);// Works
  //sensors.setResolution(outsideThermometer, 9); Does not work.

  lcd.init();  //initialize the lcd - Will will disable the dallastemperature if it is on pin 2 and 3
  lcd.backlight();  //open the backlight
  //LCD Sense (COL, ROW)
  lcd.setCursor ( 0, 0 );
  lcd.print("Tin" ); //3 char then 5 for temp = 9
  lcd.setCursor ( 10, 0 );
  lcd.print("Tout"); //
  lcd.setCursor ( 0, 1 );
  lcd.print("Twat" ); //3 char then 5 for temp = 9
  lcd.setCursor ( 10, 1 );
  lcd.print("Thot"); //
  lcd.setCursor ( 0, 2 );
  lcd.print("Tset");
  lcd.setCursor ( 17, 2 );
  lcd.print("CHF");
  lcd.setCursor ( 0, 3 );
  lcd.print("Status "); //
  Twat_min = Twat_min_d;
  get_input();
  su = 0; // out of setup
}

void loop(void)
{
  get_input();
  //*******Debug Tracing**************************
  Serial.print("Tin= ");
  Serial.print(Tin, 1);
  Serial.print(" Tout=");
  Serial.print(Tout, 1);
  Serial.print(" TWat=");
  Serial.print(Twat);
  Serial.print(" Thot= ");
  Serial.print(Thot);
  Serial.print(" Tset=");
  Serial.print(Tset, 1);
  Serial.print(" ctr1=");
  Serial.print(ctr1);
  Serial.print(" Thot_strt=");
  Serial.print(Thot_start);
  Serial.print(" NoHtCtr=");
  Serial.print(no_heat_ctr);
  Serial.print(" TwatMn=");
  Serial.print(Twat_min);
  Serial.print(" Error=");
  Serial.print(error);
  Serial.print(" CHF=");
  Serial.print(stat_c);
  Serial.print(stat_h);
  Serial.print(stat_f);
  Serial.print(" tm=");
  Serial.print(millis());
  Serial.print("\n");

  //Control heat addition to room.
  if ((Tset > (Tin + hyst) or (stat_h == 1)) and ((Twat > (Tin + min_wat_delta)) or (Thot > (Tin + min_wat_delta)))) { //Set point must be  above Tin and Twat must be above Tin
    ctr++;
    if (ctr >= 3);
    {
      if (!stat_h == 1) {
        stat_c = 1;
      }
      stat_f = 1;
    }
  }
  else if (((Tset <= Tin) and (stat_h == 0)) or (Twat < (Tin + min_wat_delta+20))) { //Turn off fan and circulation. Want to leave heat on if tank is not hot.
    stat_c = 0;
    stat_f = 0;
    ctr =  0;
    if (Tset <= Tin) { // Reset for this condition only.
      ctr1 = 0;
    }
  }

  else if ((stat_f == 1) and (stat_h ==  0)) { //case where Tin  is between Tset + hyst and Tset
    stat_c = 1;
  }

  if (stat_h == 1) {
    ctr1++;  // increment heater on counter
  }
  //******************Heater Failure or overtemp trip check************************************
  if (ctr1 >= 15) { //Check to see if heater is actually heating after some number of loops..
    if (Thot <= (Thot_start - 1) or (Thot >= 220)) {
      ctr1 = 0;
      Twat_peak = Twat;
      //Twat_min = Twat - 2;//Set temp upon which to turn heater back ON. If commented out uses Twat_min_d
      no_heat_ctr ++;
      //Serial.print("NoHeatCtr = ");
      //Serial.print(no_heat_ctr);
      //Serial.print("\n");
      if (( no_heat_ctr == 2) or (no_heat_ctr == 3)) { //Try to reset the heater
        lcd.setCursor ( 7, 3);
        lcd.print("HeaterRes");
        error = "HeaterRes";
        digitalWrite(hpmp, 1);//Must invert as when relay is energized pump (heat) is OFF
        delay(10000);
        digitalWrite(hpmp, 0);//Must invert as when relay is energized pump (heat) is OFF.
        ctr1 = 0;
      }
      if (no_heat_ctr > 3) { //Tried a couple times. Quit.
        error =  "NoHeat   ";//Shut down heater until reboot.
        stat_h = 0;
      }
    }
    else {//Reset counter for  next check.
      Thot_start = max(Thot, Thot_start);
      Twat_min = Twat_min_d;
      error = "OK       ";
      no_heat_ctr = 0;//V2.x add
    }
  }
  // ********************Control Heater*******************************
  if (((Twat < Twat_min) or (Twat < 130)) and (Tset >= (Tin + hyst)) and ((error == "OK       ") or (error == "HeaterRes"))) {
    stat_h = 1;
    stat_c = 0;
    if (ctr1 == 0) { //Heater fualt logic - record temperature when heat was turned on.
      Thot_start = Thot;
     // no_heat_ctr = 0; THis must stay commented out or will not reset with no heat. Got to find another way.
    }
  }
  else if ((Twat >= Twat_max) or (no_heat_ctr >= 1)) { //Shut off heater V2.x moded.
    ctr1 = 0;
    stat_h  =  0;
  }
  if (Tset < 25) { //Heat off want to turn on power to power point that powers heater but  nothing else
    stat_h = 0;
    stat_f = 0;
    stat_c = 0;
    digitalWrite(hpmp, !stat_h);//Must invert as when relay is energized pump (heat) is OFF.
    digitalWrite(fan, stat_f);
    digitalWrite(cpmp, stat_c);
    error = ("Idle     ");
  }
  else {
    digitalWrite(hpmp, !stat_h);//Must invert as when relay is energized pump (heat) is OFF.
    digitalWrite(fan, stat_f);
    digitalWrite(cpmp, stat_c);
    if (error == "Idle     ") {//Reset idle condition when temp gets turned up..
      error = ("OK       ");
    }
  }
  lcddrive();
  lcd.setCursor ( 0, 3 ); // Had to put this here otherwise 0 appeared in LCD lower left
  delay(200);
  //min()
}




