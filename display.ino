
const uint8_t displayStateHome = 0x01;
const uint8_t displayStateMenu = 0x02;
const uint8_t displayStateEditor = 0x03;
const uint8_t displayStateTremors = 0x04;
const uint8_t displayStatePulse = 0x05;
const uint8_t displayStateSleep = 0x06;

uint8_t currentDisplayState = displayStateHome;
void (*menuHandler)(uint8_t) = NULL;
uint8_t (*editorHandler)(uint8_t, int*, char*, void (*)()) = NULL;

//bool ped = false;
const uint8_t upButton = TSButtonUpperRight;
const uint8_t downButton = TSButtonLowerRight;
const uint8_t selectButton = TSButtonLowerLeft;
const uint8_t backButton = TSButtonUpperLeft;
const uint8_t menuButton = TSButtonLowerLeft;
const uint8_t viewButton = TSButtonLowerRight;
const uint8_t clearButton = TSButtonLowerRight;

void buttonPress(uint8_t buttons) {
  if (currentDisplayState == displayStateHome) {
    if (buttons == viewButton && !medicineTaken && !cycleDisrupted) {
      menuHandler = viewNotifications;
      menuHandler(0);
    } else if (buttons == menuButton) {
      menuHandler = viewMenu;
      menuHandler(0);
    }/* else if(buttons == upButton){
      menuHandler = viewTremors;
      menuHandler(0);
    } else if(buttons == xButton){
      menuHandler = viewPulse;
      menuHandler(0);
    } else if(buttons == backButton){
      menuHandler = viewSleep;
      menuHandler(0);
    }*/
  } else if (currentDisplayState == displayStateMenu) {
    if (menuHandler) {
      menuHandler(buttons);
    }
  } else if (currentDisplayState == displayStateEditor) {
    if (editorHandler) {
      editorHandler(buttons, 0, 0, NULL);
    }
  } else if (currentDisplayState == displayStateTremors) {
    if(menuHandler) {
      menuHandler(buttons);
    }
  } else if (currentDisplayState == displayStatePulse) {
    if(menuHandler) {
      menuHandler(buttons);
    }
  } else if (currentDisplayState == displayStateSleep) {
    if(menuHandler) {
      menuHandler(buttons);
    }
  }
  if (medicine == true && buttons == viewButton){
    medicineTaken = true;
    menuHandler = viewMenu;
    menuHandler(0);
  }
  if(sleepState && buttons == menuButton && !cycleDisrupted){
    sleepState = false;
    cycleDisrupted = false;
    menuHandler = viewMenu;
    menuHandler(0);
  }else if(cycleDisrupted && buttons == viewButton){
    sleepState = false;
    cycleDisrupted = false;
    menuHandler = viewMenu;
    menuHandler(0);
  }
  /*
  if (buttons == upButton) {
    if (ped) {
      display.clearWindow(0, 0, 96, 80);
      displayPedometer();
    } else
      initHomeScreen();
    ped = !ped;
  }
  */
}

void viewNotifications(uint8_t button) {
  if (!button) {
    if (menu_debug_print)SerialMonitorInterface.println("viewNotificationsInit");
    currentDisplayState = displayStateMenu;
    display.clearWindow(0, 12, 96, 64);
    display.setFont(font10pt);
    display.fontColor(defaultFontColor, defaultFontBG);
    if (amtNotifications) {
      if (menu_debug_print)SerialMonitorInterface.println("amtNotifications=true");
      //display.setCursor(0, menuTextY[1]);
      //display.setCursor(0, 0);
      //display.print(ANCSNotificationTitle());

      int line = 0;
      int totalMessageChars = strlen(ANCSNotificationMessage());
      int printedChars = 0;
      while (printedChars < totalMessageChars && line < 3) {
        char tempPrintBuff[40] = "";
        int tempPrintBuffPos = 0;
        while (display.getPrintWidth(tempPrintBuff) < 90 && printedChars < totalMessageChars) {
          if (!(tempPrintBuffPos == 0 && ANCSNotificationMessage()[printedChars] == ' ')) {
            tempPrintBuff[tempPrintBuffPos] = ANCSNotificationMessage()[printedChars];
            tempPrintBuffPos++;
          }
          printedChars++;
          tempPrintBuff[tempPrintBuffPos] = '\0';
        }
        display.setCursor(0, menuTextY[line]);
        display.print((char*)tempPrintBuff);
        line++;
      }



      display.setCursor(0, menuTextY[3]);
      display.print(F("< "));
      display.print(ANCSNotificationNegativeAction());

      char backStr[] = "Back >";
      int Xpos = 95 - display.getPrintWidth(backStr);
      display.setCursor(Xpos, menuTextY[3]);
      display.print(backStr);
    } else {
      if (menu_debug_print)SerialMonitorInterface.println("amtNotifications=false");
      display.setCursor(0, menuTextY[0]);
      display.print(F("  No notifications."));
      char backStr[] = "Back >";
      int Xpos = 95 - display.getPrintWidth(backStr);
      display.setCursor(Xpos, menuTextY[3]);
      display.print(backStr);
    }
  } else {
    if (button == clearButton) {//actually back?
      currentDisplayState = displayStateHome;
      initHomeScreen();
    } else if (button == selectButton) { //do action
      amtNotifications = 0;
      ANCSPerformNotificationNegativeAction();
      currentDisplayState = displayStateHome;
      initHomeScreen();
    }
  }
}

void viewTremors(uint8_t button){
  if(!button){
    currentDisplayState = displayStateTremors;
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
    //EGG: SerialMonitorInterface.println("urmother");

    display.setFont(font10pt);
    char backStr[] = "< Back";
    int Xpos = 1;
    display.setCursor(Xpos, menuTextY[3]);
    display.print(backStr);
    tremorState = true;
  }else if(button == menuButton){
    tremorState = false;
    menuHandler = viewMenu;
    menuHandler(0);
  }
}

void viewPulse(uint8_t button){
  if(!button){
    currentDisplayState = displayStatePulse;

    display.clearWindow(0, 12, 96, 64);
    pulseState = true;

    display.setFont(font10pt);
    char backStr[] = "< Back";
    int Xpos = 1;
    display.setCursor(Xpos, menuTextY[3]);
    display.print(backStr);
  }else if(button == menuButton){
    pulseState = false;
    menuHandler = viewMenu;
    menuHandler(0);
  }
}

void viewSleep(uint8_t button){
  if(!button){
    currentDisplayState = displayStateSleep;
    display.clearWindow(0, 12, 96, 64);

    display.setFont(font10pt);
    display.fontColor(defaultFontColor, defaultFontBG);
    display.setCursor(1,15);
    display.println("Are you about to");
    display.setCursor(1,27);
    display.println("sleep?");

    display.setFont(font10pt);
    char backStr[] = "< Back";
    int Xpos = 1;
    display.setCursor(Xpos, menuTextY[3]);
    display.print(backStr);

    display.setFont(font10pt);
    display.setCursor(95 - 26, menuTextY[3]);
    display.print("Yes >");
  }else if(button == viewButton && !cycleDisrupted){
    SerialMonitorInterface.println("SLEEPTRUE");
    sleepState = true;
  }else if(button == menuButton){
    sleepState = false;
    menuHandler = viewMenu;
    menuHandler(0);
  }
}

void initHomeScreen() {
  display.clearWindow(0, 12, 96, 64);
  rewriteTime = true;
  rewriteMenu = true;
  if(medicineTaken){currentDisplayState == displayStateHome;}
  updateMainDisplay();
}

uint8_t lastDisplayedDay = -1;
uint8_t lastDisplayedMonth = -1;
uint8_t lastDisplayedYear = -1;

void updateDateDisplay() {

  int currentDay = RTCZ.getDay();
  int currentMonth = RTCZ.getMonth();
  int currentYear = RTCZ.getYear();

  if ((lastDisplayedDay == currentDay) &&
      (lastDisplayedMonth == currentMonth) &&
      (lastDisplayedYear == currentYear))
    return;
    
  lastDisplayedDay = currentDay;
  lastDisplayedMonth = currentMonth;
  lastDisplayedYear = currentYear;
  display.setFont(font10pt);
  display.fontColor(defaultFontColor, defaultFontBG);
  display.setCursor(2, 2);

  const char * wkday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  time_t currentTime = RTCZ.getEpoch();
  struct tm* wkdaycalc = gmtime(&currentTime);
  display.print(wkday[wkdaycalc->tm_wday]);
  display.print(' ');
  display.print(RTCZ.getMonth());
  display.print('/');
  display.print(RTCZ.getDay());
  display.print(F("  "));

  ble_connection_displayed_state = ~ble_connection_state;
  updateBLEstatusDisplay();
}

void updateMainDisplay() {
  if (lastSetBrightness != brightness) {
    display.setBrightness(brightness);
    lastSetBrightness = brightness;
  }
  updateDateDisplay();
  updateBLEstatusDisplay();
  displayBattery();
  if (currentDisplayState == displayStateHome) {
    updateTimeDisplay();
    if (rewriteMenu || lastAmtNotificationsShown != amtNotifications) {
      lastAmtNotificationsShown = amtNotifications;
      display.setFont(font10pt);
      display.clearWindow(0, menuTextY[2], 96, 13);
      if (amtNotifications) {
        int printPos = 48 - (display.getPrintWidth(ANCSNotificationTitle()) / 2);
        if (printPos < 0)printPos = 0;
        display.setCursor(printPos, menuTextY[2]);
        display.print(ANCSNotificationTitle());
      }
      display.setCursor(0, menuTextY[3]);
      display.print(F("< Menu          "));
      char viewStr[] = "View >";
      int Xpos = 95 - display.getPrintWidth(viewStr);
      display.setCursor(Xpos, menuTextY[3]);
      display.print(viewStr);
      rewriteMenu = false;
    }
  }
  lastMainDisplayUpdate = millisOffset();
}


uint8_t lastAMPMDisplayed = 0;
uint8_t lastHourDisplayed = -1;
uint8_t lastMinuteDisplayed = -1;
uint8_t lastSecondDisplayed = -1;

void updateTimeDisplay() {
  int currentHour, currentMinute, currentSecond;

  currentHour = RTCZ.getHours();
  currentMinute = RTCZ.getMinutes();
  currentSecond = RTCZ.getSeconds();

  if (currentDisplayState != displayStateHome)
    return;
  char displayX;
  int hour12 = currentHour;
  int AMPM = 1;
  if (hour12 > 12) {
    AMPM = 2;
    hour12 -= 12;
  }
  display.fontColor(defaultFontColor, defaultFontBG);
  if (rewriteTime || lastHourDisplayed != hour12) {
    display.setFont(font22pt);
    lastHourDisplayed = hour12;
    displayX = 0;
    display.setCursor(displayX, timeY);
    if (lastHourDisplayed < 10)display.print('0');
    display.print(lastHourDisplayed);
    display.write(':');
    if (lastAMPMDisplayed != AMPM) {
      if (AMPM == 2)
        display.fontColor(inactiveFontColor, inactiveFontBG);
      display.setFont(font10pt);
      display.setCursor(displayX + 80, timeY - 0);
      display.print(F("AM"));
      if (AMPM == 2) {
        display.fontColor(defaultFontColor, defaultFontBG);
      } else {
        display.fontColor(inactiveFontColor, inactiveFontBG);
      }
      display.setCursor(displayX + 80, timeY + 11);
      display.print(F("PM"));
      display.fontColor(defaultFontColor, defaultFontBG);
    }
  }

  if (rewriteTime || lastMinuteDisplayed != currentMinute) {
    display.setFont(font22pt);
    lastMinuteDisplayed = currentMinute;
    displayX = 14 + 14 - 1;
    display.setCursor(displayX, timeY);
    if (lastMinuteDisplayed < 10)display.print('0');
    display.print(lastMinuteDisplayed);
    display.write(':');
  }

  if ((rewriteTime || lastSecondDisplayed != currentSecond) && !medicine) {
    display.setFont(font22pt);
    lastSecondDisplayed = currentSecond;
    displayX = 14 + 14 + 14 + 14 - 2;
    display.setCursor(displayX, timeY);
    if (lastSecondDisplayed < 10)display.print('0');
    display.print(lastSecondDisplayed);
  }
  rewriteTime = false;
}

void updateBLEstatusDisplay() {
  if (ble_connection_state == ble_connection_displayed_state)
    return;
  ble_connection_displayed_state = ble_connection_state;
  int x = 62;
  int y = 6;
  int s = 2;
  uint8_t color = 0x03;
  if (ble_connection_state)
    color = 0xE0;
  display.drawLine(x, y + s + s, x, y - s - s, color);
  display.drawLine(x - s, y + s, x + s, y - s, color);
  display.drawLine(x + s, y + s, x - s, y - s, color);
  display.drawLine(x, y + s + s, x + s, y + s, color);
  display.drawLine(x, y - s - s, x + s, y - s, color);
}

void displayBattery() {
  int result = 0;
 
  //http://atmel.force.com/support/articles/en_US/FAQ/ADC-example
  SYSCTRL->VREF.reg |= SYSCTRL_VREF_BGOUTEN;
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->SAMPCTRL.bit.SAMPLEN = 0x1;
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->INPUTCTRL.bit.MUXPOS = 0x19;         // Internal bandgap input
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->CTRLA.bit.ENABLE = 0x01;             // Enable ADC
  // Start conversion
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->SWTRIG.bit.START = 1;
  // Clear the Data Ready flag
  ADC->INTFLAG.bit.RESRDY = 1;
  // Start conversion again, since The first conversion after the reference is changed must not be used.
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->SWTRIG.bit.START = 1;
  // Store the value
  while ( ADC->INTFLAG.bit.RESRDY == 0 );   // Waiting for conversion to complete
  uint32_t valueRead = ADC->RESULT.reg;
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->CTRLA.bit.ENABLE = 0x00;             // Disable ADC
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  SYSCTRL->VREF.reg &= ~SYSCTRL_VREF_BGOUTEN;
  result = (((1100L * 1024L) / valueRead) + 5L) / 10L;
  uint8_t x = 70;
  uint8_t y = 3;
  uint8_t height = 5;
  uint8_t length = 20;
  uint8_t red, green;
  if (result > 325) {
    red = 0;
    green = 63;
  } else {
    red = 63;
    green = 0;
  }
  display.drawLine(x - 1, y, x - 1, y + height, 0xFF); //left boarder
  display.drawLine(x - 1, y - 1, x + length, y - 1, 0xFF); //top border
  display.drawLine(x - 1, y + height + 1, x + length, y + height + 1, 0xFF); //bottom border
  display.drawLine(x + length, y - 1, x + length, y + height + 1, 0xFF); //right border
  display.drawLine(x + length + 1, y + 2, x + length + 1, y + height - 2, 0xFF); //right border
  for (uint8_t i = 0; i < length; i++) {
    display.drawLine(x + i, y, x + i, y + height, red, green, 0);
  }
}