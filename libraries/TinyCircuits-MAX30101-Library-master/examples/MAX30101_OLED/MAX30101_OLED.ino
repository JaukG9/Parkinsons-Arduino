/*************************************************************************
  MAX30101 Wireling OLED Example

  This example graphs the MAX30101 output on an OLED Wireling, as well as
  temperature of the sensor, BPM, and pulse oxygen.

  BPM and pulse oxygen output is not for medical use!

  Original MAX30100 code by Raivis Strogonovs:
  https://morf.lv/implementing-pulse-oximeter-using-max30100

  Updated: 4/19/2019 by Ben Rose for TinyCircuits
 *************************************************************************/

#include <Wire.h>
#include <Wireling.h>
#include <SPI.h>
#include <TinierScreen.h>
#include <GraphicsBuffer.h>

TinierScreen display = TinierScreen(TinierScreen042);
//TinierScreen display = TinierScreen(TinierScreen069);
//TinierScreen display = TinierScreen(TinierScreen096);

GraphicsBuffer screenBuffer = GraphicsBuffer(72, 40, colorDepth1BPP);
//GraphicsBuffer screenBuffer = GraphicsBuffer(96, 16, colorDepth1BPP);
//GraphicsBuffer screenBuffer = GraphicsBuffer(128, 64, colorDepth1BPP);

int displayPort = 1;
int resetPin = A0 + displayPort;

#if defined (ARDUINO_ARCH_AVR)
#define SerialMonitorInterface Serial
#elif defined(ARDUINO_ARCH_SAMD)
#define SerialMonitorInterface SerialUSB
#endif

bool slowUpdateRate = false;

#include <MAX30101.h>

MAX30101 pulseSensor = MAX30101();
int pulseSensorPort = 0;

const int graphWidth = 72;

float samples[graphWidth];

void setup() {
  SerialMonitorInterface.begin(9600);
  Wire.begin();
  Wireling.begin();
  delay(100);

  while (!SerialMonitorInterface && millis() < 5000); //This will block until the Serial Monitor is opened on TinyScreen+/TinyZero platform!

  Wireling.selectPort(pulseSensorPort);
  if (pulseSensor.begin()) {
    while (true) {
      SerialMonitorInterface.println("MAX30101 Wireling not detected!");
      delay(1000);
    }
  }

  Wireling.selectPort(displayPort);
  display.begin(resetPin);

  if (screenBuffer.begin()) {
    //memory allocation error- buffer too big!
  }

  screenBuffer.setFont(thinPixel7_10ptFontInfo);

  for (int i = 0; i < graphWidth; i++) {
    samples[i] = 0.0;
  }

#if defined (ARDUINO_ARCH_AVR)
//Unfortunately TinyDuino has a pretty slow maximum I2C speed!
slowUpdateRate = true;
#endif
}

void loop() {
  Wireling.selectPort(pulseSensorPort);
  if (pulseSensor.update()) {
    if (slowUpdateRate) { // double each result to increase perceived speed
      for (int i = 2; i < graphWidth / 2; i++) {
        samples[i * 2 - 1] = samples[i * 2];
        samples[i * 2 - 2] = samples[i * 2];
      }
      samples[graphWidth - 1] = pulseSensor.cardiogram();
      samples[graphWidth - 2] = pulseSensor.cardiogram();
    } else {

      for (int i = 1; i < graphWidth; i++) {
        samples[i - 1] = samples[i];
      }
      samples[graphWidth - 1] = pulseSensor.cardiogram();
    }
    Wireling.selectPort(displayPort);
    screenBuffer.clear();
    drawGraph(&screenBuffer, screenBuffer.width - graphWidth, 0, graphWidth, screenBuffer.height, samples) ;

    if (pulseSensor.pulseValid()) {
      screenBuffer.setCursor(0, 0);
      screenBuffer.print("BPM: ");
      screenBuffer.print(round(pulseSensor.BPM()));
      screenBuffer.setCursor(50, 0);
      screenBuffer.print("Temp: ");
      screenBuffer.print(round(pulseSensor.temperature()));
      screenBuffer.setCursor(0, 28);
      screenBuffer.print("Oxygen: ");
      screenBuffer.print(pulseSensor.oxygen());
    } else {
      screenBuffer.setCursor(0, 0);
      screenBuffer.print("BPM: ");
      screenBuffer.print("-");
      screenBuffer.setCursor(50, 0);
      screenBuffer.print("Temp: ");
      screenBuffer.print("-");
      screenBuffer.setCursor(0, 28);
      screenBuffer.print("Oxygen: ");
      screenBuffer.print("-");
    }
    Wire.setClock(500000);
    display.writeBuffer(screenBuffer.getBuffer(), screenBuffer.getBufferSize());
    Wire.setClock(100000);
  }
  //The delay below is commented out because writing to the screen takes plenty of time.
  //delay(20);//Polling the sensor too often can cause extra noise. The sensor can buffer about 300ms of data with the default settings.
}


void drawGraph(GraphicsBuffer * sbuff, int xDispPos, int yDispPos, int gWidth, int gHeight, float samples[]) {
  for (uint8_t x = xDispPos; x < xDispPos + gWidth; x++) {
    sbuff->drawLine( x, yDispPos, x, yDispPos + gHeight, 0);
  }
  int midPoint = yDispPos + gHeight / 2;
  for (int x = 1; x < gWidth; x++) {
    int sample0 = (float)(samples[x - 1] * (float)gHeight / 2.0);
    int sample1 = (float)(samples[x    ] * (float)gHeight / 2.0);
    sbuff->drawLine(xDispPos + x - 1, midPoint - sample0, xDispPos + x, midPoint - sample1, 1);
  }
}
