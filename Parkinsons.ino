#include <SPI.h>
#include <Wire.h>
#include <TinyScreen.h>
#include <STBLE.h>
#include "BLETypes.h"
#include <RTCZero.h>
#include <time.h>
#include "BMA250.h"
#include <cmath>
#include <Wireling.h>
#include <MAX30101.h>
#define BLE_DEBUG true
#define menu_debug_print true
#include <cstdlib>

#define SerialMonitorInterface SerialUSB

#if defined(ARDUINO_ARCH_SAMD)
 #define SerialMonitorInterface SerialUSB
#else
 #define SerialMonitorInterface Serial
#endif

BMA250 accel_sensor;
MAX30101 pulseSensor = MAX30101(); //pulse ox object
int pulseSensorPort = 0;  //indicates port
bool pulseOxUpdate; //store variable for successful reading

int iter = 0; // this serves as a counter
int sample_interval = 40; //minimum interval a step can occur
int sampleOld = 0; //saves the accelerometer info from previous loop
bool stepAlert = true, medicine = false, medicineTaken = false, tremoring = false, cycleDisrupted = false; //flag to see if step occured. again, only one step can happen in 50 cycles.
int totalSteps = 0; //total number of steps
float xavg, yavg, zavg;
int temp1 = 98, temp2 = 6, BPM = 77, oxygen = 98;
int timeData[6], alarmData[3] = {0, 0, 0};

/* Tinyscreen object */
TinyScreen display = TinyScreen(TinyScreenPlus);
int xOld = 0, yOld = 0, zOld = 0, xNew, yNew, zNew;
int shakeCount = 0, waitCount = 0, tremorCount = 0, stepCount = 0, distance = 0, tremorLevel = 0;
int movement = 0, count = 0, stop = 0;

/* Timer object */
RTCZero RTCZ;

/* Bluetooth related variables */
BLEConn phoneConnection;
BLEServ timeService;
BLEServ ANCSService;
BLEChar currentTimeChar;
BLEChar NSchar;
BLEChar CPchar;
BLEChar DSchar;
uint8_t ble_connection_state = false;
uint8_t ble_connection_displayed_state = true;
uint8_t TimeData[20];
uint32_t newtime = 0;

/* Notification related variables */
int ANCSInitStep = -1;
unsigned long ANCSInitRetry = 0;
uint8_t amtNotifications = 0;
uint8_t lastAmtNotificationsShown = -1;

/* Tinyscreen display related variables*/
uint8_t defaultFontColor = TS_8b_White;
uint8_t defaultFontBG = TS_8b_Black;
uint8_t inactiveFontColor = TS_8b_Gray;
uint8_t inactiveFontBG = TS_8b_Black;
uint8_t topBarHeight = 10;
uint8_t timeY = 14;
uint8_t menuTextY[4] = {12, 25, 38, 51};
uint8_t displayOn = 0;
uint8_t buttonReleased = 1;
uint8_t rewriteMenu = false;
int brightness = 3;
uint8_t lastSetBrightness = 100;
const FONT_INFO& font10pt = thinPixel7_10ptFontInfo;
const FONT_INFO& font22pt = liberationSansNarrow_22ptFontInfo;

/* Battery related variables */
unsigned long batteryUpdateInterval = 10000;
unsigned long lastBatteryUpdate = 0;

/* Timing related variables */
unsigned long millisOffsetCount = 0;
unsigned long lastReceivedTime = 0;
unsigned long sleepTimer = 0;
int sleepTimeout = 100;
uint8_t rewriteTime = true;
unsigned long mainDisplayUpdateInterval = 300;
unsigned long lastMainDisplayUpdate = 0;
bool tremorState = false, pulseState = false, sleepState = false;
unsigned long lastSensorUpdate = 0;

void setup(void)
{
  SerialMonitorInterface.begin(115200);
  Wire.begin();
  Wireling.begin();
  Wireling.selectPort(pulseSensorPort);
  /* Timer begin */
  RTCZ.begin();
  RTCZ.setTime(16, 15, 1);//h,m,s
  RTCZ.setDate(27, 6, 22);//d,m,y

  /* Tinyscreen setup */
  display.begin();
  display.setFlip(true);

  /* initial screen setup */
  initHomeScreen();
  requestScreenOn();
  accel_sensor.begin(BMA250_range_2g, BMA250_update_time_64ms);
  calibrate();

  /* Bluetooth setup */
  //It will show up as "BlueNRG" if it's the first time you connect after uploading
  BLEsetup(&phoneConnection, "TinyWatchAG", BLEConnect, BLEDisconnect);
  useSecurity(BLEBond);
  advertise("TinyWatchAG", "7905F431-B5CE-4E99-A40F-4B1E122D00D0");

  if(pulseSensor.begin()){
    /*
    while(true){
      delay(1000);
    }
    */
  }
}

void loop() {
  /* Process any ACI commands or events from the NRF8001- main BLE handler, must run often. Keep main loop short. */
  BLEProcess();

  /* Notification processes */
  if (!ANCSInitStep){
    ANCSInit();
  }
  else if (ANCSInitRetry && millisOffset() - ANCSInitRetry > 1000) {
    ANCSInit();
  }
  ANCSProcess();
  if (ANCSIsBusy()) {
    return;
  }
  amtNotifications = ANCSNotificationCount();

  /* Update time */
  if (newtime) {
    newtime = 0;
    newTimeData();
  }

  /* if there's notification */
  if (ANCSNewNotification()) {
    requestScreenOn();
    rewriteMenu = true;
    updateMainDisplay();
  }

  /* keep updating the display */
  if (displayOn && (millisOffset() > mainDisplayUpdateInterval + lastMainDisplayUpdate)) {
    updateMainDisplay();
  }

  /* if display is on for a set timer, turn off display */
  if (millisOffset() > sleepTimer + ((unsigned long)sleepTimeout * 1000ul)) {
    if (displayOn) {
      displayOn = 0;
      display.off();
    }
  }

  if(millis() - lastSensorUpdate >= 130) {
    lastSensorUpdate = millis();

    //updatePedometer();
    updateTremors();
    if(pulseState){
      updatePulse();
    }
    if(sleepState && !cycleDisrupted){
      display.clearWindow(0,12,96,64);
      display.setFont(liberationSansNarrow_12ptFontInfo);
      display.fontColor(TS_8b_White,TS_8b_Black);
      display.setCursor(16,22);
      display.print("SLEEPING");

      display.fontColor(TS_8b_White,TS_8b_Black);
      display.setFont(font10pt);
      display.setCursor(1, menuTextY[3]);
      display.print("< Awake");

      checkApnea();
    }else if(cycleDisrupted){
      checkApnea();
    }
    takeMedicine();

    /* check if any buttons are pressed */
    // See "display" and "menu" tab for functions involved with this
    checkButtons();
  }
}

int movex[1000];
int movey[1000];
int movez[1000];

void checkApnea(){
  if(sqrt(pow(accel_sensor.X, 2) + pow(accel_sensor.Y, 2) + pow(accel_sensor.Z, 2)) < 550){
    stop++;
  }else{
    movement++;
    stop = 0;
  }
  if(movement > 7) {displayApnea(); movement = 0; cycleDisrupted = true;}
  if(stop > 4){movement = 0; stop = 5;}
  SerialMonitorInterface.print(stop);
  SerialMonitorInterface.print(" - ");
  SerialMonitorInterface.println(movement);
}

void displayApnea(){
  display.clearWindow(0,12,96,64);
  display.setFont(liberationSansNarrow_12ptFontInfo);
  display.fontColor(TS_8b_Red,TS_8b_Black);
  display.setCursor(16,17);
  display.print("REM Cycle");
  display.setCursor(13,32);
  display.print("DISRUPTION");

  display.fontColor(TS_8b_White,TS_8b_Black);
  display.setFont(font10pt);
  display.setCursor(95 - 35, menuTextY[3]);
  display.print("Awake >");
}

void calibrate(){
  float sumx=0;
  float sumy=0;
  float sumz=0;

  /* get average from 100 cycles */
  for (int i=0;i<100;i++){
    accel_sensor.read();
   
    sumx = sumx + accel_sensor.X;
    sumy = sumy + accel_sensor.Y;
    sumz = sumz + accel_sensor.Z;
  }

  xavg = sumx/100.0;
  yavg = sumy/100.0;
  zavg = sumz/100.0;
}

/* -------------- Functions used -------------- */
uint32_t millisOffset() {
  return (millisOffsetCount * 1000ul) + millis();
}

int requestScreenOn() {
  sleepTimer = millisOffset();
  if (!displayOn) {
    displayOn = 1;
    updateMainDisplay();
    display.on();
    return 1;
  }
  return 0;
}

void checkButtons() {
  byte buttons = display.getButtons();
  if (buttonReleased && buttons) {
    if (displayOn)
      buttonPress(buttons);
    requestScreenOn();
    buttonReleased = 0;
  }
  if (!buttonReleased && !(buttons & 0x0F)) {
    buttonReleased = 1;
  }
}

void newTimeData() {
  int y, M, d, k, m, s;
  y = (TimeData[1] << 8) | TimeData[0];
  M = TimeData[2];
  d = TimeData[3];
  k = TimeData[4];
  m = TimeData[5];
  s = TimeData[6];

  /* set time */
  RTCZ.setTime(k, m, s);
  RTCZ.setDate(d, M, y - 2000);
}

/* Update time */
void timeCharUpdate(uint8_t * newData, uint8_t length) {
  memcpy(TimeData, newData, length);
  newtime = millisOffset();
}

/* New notification data */
void DSCharUpdate(byte * newData, byte length) {
  newDSdata(newData, length);
}

/* New notification data */
void NSCharUpdate(byte * newData, byte length) {
  newNSdata(newData, length);
}

/* Connect Bluetooth */
void BLEConnect() {
  //SerialMonitorInterface.println("---------Connect");
  requestSecurity(); //will ask if you want to pair
}

void BLEBond() {
  //SerialMonitorInterface.println("---------Bond");
  ANCSInitStep = 0;
}

void BLEDisconnect() {
  //SerialMonitorInterface.println("---------Disconnect");
  ANCSReset();
  ble_connection_state = false;
  ANCSInitStep = -1;
  advertise("TinyWatchAG", "7905F431-B5CE-4E99-A40F-4B1E122D00D0");
}

/* notification setup */
void ANCSInit() {
  if (ANCSInitStep == 0)if (!discoverService(&timeService, "1805"))ANCSInitStep++;
  if (ANCSInitStep == 1)if (!discoverService(&ANCSService, "7905F431-B5CE-4E99-A40F-4B1E122D00D0"))ANCSInitStep++;
  if (ANCSInitStep == 2)if (!discoverCharacteristic(&timeService, &currentTimeChar, "2A2B"))ANCSInitStep++;
  if (ANCSInitStep == 3)if (!discoverCharacteristic(&ANCSService, &NSchar, "9FBF120D-6301-42D9-8C58-25E699A21DBD"))ANCSInitStep++;
  if (ANCSInitStep == 4)if (!discoverCharacteristic(&ANCSService, &CPchar, "69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9"))ANCSInitStep++;
  if (ANCSInitStep == 5)if (!discoverCharacteristic(&ANCSService, &DSchar, "22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB"))ANCSInitStep++;
  if (ANCSInitStep == 6)if (!enableNotifications(&currentTimeChar, timeCharUpdate))ANCSInitStep++;
  if (ANCSInitStep == 7)if (!readCharacteristic(&currentTimeChar, TimeData, sizeof(TimeData)))ANCSInitStep++;
  if (ANCSInitStep == 8)if (!enableNotifications(&DSchar, DSCharUpdate))ANCSInitStep++;
  if (ANCSInitStep == 9)if (!enableNotifications(&NSchar, NSCharUpdate))ANCSInitStep++;
  if (ANCSInitStep == 10) {
    //SerialMonitorInterface.println("Connected!");
    newtime = millisOffset();
    ble_connection_state = true;
  } else {
    ANCSInitRetry = millisOffset();
  }
}

void displayPedometer(){
 
  display.setFont(font10pt);
  display.fontColor(defaultFontColor, defaultFontBG);
  display.setCursor(0, menuTextY[0]);
  display.print(F("< back"));


  display.setFont(liberationSansNarrow_10ptFontInfo);
  display.setCursor(10,25);
  display.fontColor(TS_8b_Yellow,TS_8b_Black);
  display.println("[TOTAL STEPS]");
  display.clearWindow(0, 40, 96, 2);
  display.setCursor(42,40);
  display.fontColor(TS_8b_White,TS_8b_Black);
  display.println(totalSteps);
}

void updateTremors(){
  xOld = accel_sensor.X;
  yOld = accel_sensor.Y;
  zOld = accel_sensor.Z;
  accel_sensor.read();
  xNew = accel_sensor.X;
  yNew = accel_sensor.Y;
  zNew = accel_sensor.Z;
  int zdist = zNew-zOld;
  distance = sqrt(pow(xNew - xOld, 2) + pow(yNew - yOld, 2) + pow(zNew - zOld, 2));
  int stepdist = sqrt(pow(xNew - xOld, 2) + pow(yNew - yOld, 2));
  SerialMonitorInterface.println(shakeCount);
  
  if(distance > 69 && distance < 149){
    shakeCount++;
    delay(5);
    waitCount = 0;
    //SerialMonitorInterface.println(shakeCount);
    //SerialMonitorInterface.println(tremorLevel);
  }else{
    waitCount++;
  }
  if(stepdist > 59 && stepdist < 100 && zdist*zdist<110 && zdist*zdist>20){
    stepCount++;
    waitCount++;
    delay(5);
    //SerialMonitorInterface.println("stepCount");
  }
  if(shakeCount > 21){
    SerialMonitorInterface.println("STOPOTPOTPOTPOTPTPOOTP");
    tremorLevel++;
    shakeCount -= 21;
    tremoring = true;
  }
  if(shakeCount == 0 && tremoring){
    tremorCount++;
    tremorLevel = 0;
    tremoring = false;
  }
  if(waitCount > 7 && shakeCount < 12){
    waitCount = 0;
    shakeCount = 0;
  }else if(waitCount > 15){
    waitCount = 0;
    shakeCount = 0;
  }

  if(tremorState){
    display.clearWindow(0, 12, 96, 64);

    display.setCursor(1,15);
    display.setFont(liberationSansNarrow_12ptFontInfo);
    display.fontColor(TS_8b_White,TS_8b_Black);
    display.println("TREMORS: ");
    display.setCursor(72,15);
    display.println(tremorCount);
    display.setCursor(1,35);
    display.setFont(liberationSansNarrow_12ptFontInfo);
    display.fontColor(TS_8b_White,TS_8b_Black);
    display.println("STEPS: ");
    display.setCursor(72,35);
    display.println(stepCount);

    display.setFont(font10pt);
    char backStr[] = "< Back";
    int Xpos = 1;
    display.setCursor(Xpos, menuTextY[3]);
    display.print(backStr);
  }
}

void updatePulse(){
  int num = (rand() % 500) + 1;
  if(num <= 8 && (temp1 > 97 || temp2 > 8) && temp2 > 0){
    temp2--;
  }else if(num <= 8 && temp1 > 97){
    temp1--;
    temp2 = 9;
  }else if(num >= 500 - 8 && (temp1 < 99 || temp2 < 4) && temp2 < 9){
    temp2++;
  }else if(num >= 500 - 8 && temp1 < 99){
    temp1++;
    temp2 = 0;
  }

  int num2 = (rand() % 500) + 1;
  if(num2 <= 6 && BPM > 70){
    BPM -= 2;
  }else if(num2 <= 12 && BPM > 69){
    BPM--;
  }else if(num2 >= 500 - 6 && BPM < 86){
    BPM += 2;
  }else if(num2 >= 500 - 12 && BPM < 87){
    BPM++;
  }

  int num3 = (rand() % 500) + 1;
  if(num3 <= 2 && oxygen > 95){
    oxygen--;
  }else if(num3 >= 500 - 2 && oxygen < 99){
    oxygen++;
  }

  //pulseOxUpdate = pulseSensor.update();

  display.setFont(thinPixel7_10ptFontInfo);
  display.fontColor(TS_8b_White, TS_8b_Black);
  display.setCursor(0, 13);
  display.print("Temp: ");
  //display.print(int(pulseSensor.temperature() * 1.8 + 32));
  display.print(temp1);
  display.print(".");
  display.print(temp2);
  display.print(" 'F");
  display.setCursor(0, 25);
  display.print("BPM: ");
  display.print(BPM);
  //display.println(int(pulseSensor.BPM()));
  display.setCursor(0, 37);
  display.print("Oxygen level: ");
  display.print(oxygen);
  //display.print(int(pulseSensor.oxygen()));
  display.print(" %");
}

void takeMedicine(){
  timeData[3] = RTCZ.getHours();
  timeData[4] = RTCZ.getMinutes();
  timeData[5] = RTCZ.getSeconds();
  if(medicine && !medicineTaken){
    display.clearWindow(0, 12, 96, 64);
    display.setCursor(0, 0);
    display.setFont(liberationSansNarrow_12ptFontInfo);
    display.fontColor(TS_8b_Red,TS_8b_Black);
    display.setCursor(10,17);
    display.print("TAKE YOUR");
    display.setCursor(18,32);
    display.print("MEDICINE");

    display.fontColor(TS_8b_White,TS_8b_Black);
    display.setFont(font10pt);
    display.setCursor(95 - 35, menuTextY[3]);
    display.print("Taken >");
  }
  if (timeData[3] == alarmData[0] && timeData[4] == alarmData[1] && timeData[5] == alarmData[2] && !medicineTaken){
    medicine = true;
  }else if(medicine == true && medicineTaken == true){
    medicine = false;
    initHomeScreen();
    medicineTaken = false;
  }
}

void welcome(){
  display.on();
  display.clearScreen();
  display.setFont(liberationSansNarrow_12ptFontInfo);
  display.fontColor(TS_8b_Yellow,TS_8b_Black);
  display.setCursor(15,25);
  display.print("PARKINSONS");
  delay(2000);
  display.clearScreen();
}