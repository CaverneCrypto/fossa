#include <Wire.h>
#include <FS.h>
#include <SPIFFS.h>
#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include <RAK14014_FT6336U.h>
#include <Hash.h>
#include <ArduinoJson.h>
#include "qrcoded.h"
#include "Bitcoin.h"
#include <Adafruit_Thermal.h>
#include "mbedtls/aes.h"
#include "mbedtls/md5.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "lgfx_config.h"
#define FORMAT_ON_FAIL true
#define PARAM_FILE "/elements.json"

///////////////////////////////////////////////////
//////CHANGE MANUALLY OR USE FOSSA.lnbits.com//////
///////////////////////////////////////////////////

// WT32-SC01 PLUS GPIO mapping using EXT connector pins
#define TOUCH_SDA 6    // I2C SDA for FT6336
#define TOUCH_SCL 5    // I2C SCL for FT6336
#define TOUCH_INT 7    // Touch interrupt pin
#define TOUCH_RST -1   // Touch reset pin (not used)
#define BILL_RX 10     // RX Bill acceptor (EXT_IO1)
#define BILL_TX 11     // TX Bill acceptor (EXT_IO2)
#define COIN_TX 12     // TX Coinmech (EXT_IO3)
#define COIN_INHIBIT 13 // Coinmech (EXT_IO4)
#define PRINTER_RX 14  // RX of the thermal printer (EXT_IO5)
#define PRINTER_TX 21  // TX of the thermal printer (EXT_IO6)

// default settings
#include "hardcoded_user_config.h"

bool hardcoded = HARDCODED; // set to true if you want to use the above hardcoded settings
bool printerBool = false;
float billAmountFloat[10];
float coinAmountFloat[10];

///////////////////////////////////////////////////
///////////////////////////////////////////////////
///////////////////////////////////////////////////

String deviceString = DEVICE_STRING;
String language = LANGUAGE;
String coinAmounts = COIN_AMOUNTS;
String billAmounts = BILL_AMOUNTS;
int charge = CHARGE;
int maxAmount = MAX_AMOUNT;
int maxBeforeReset = MAX_BEFORE_RESET;

String baseURLATM;
String baseUrlAtmPage;
String secretATM;
String currencyATM;

fs::SPIFFSFS &FlashFS = SPIFFS;

String qrData;

int maxBeforeResetTally;
int bills;
float coins;
float total;
float billAmountSize = sizeof(billAmountFloat) / sizeof(float);
float coinAmountSize = sizeof(coinAmountFloat) / sizeof(float);
int moneyTimer = 0;
bool waitForTap = true;
struct KeyValue {
  String key;
  String value;
};
String translate(String key);
uint16_t homeScreenColors[] = { TFT_GREEN, TFT_BLUE, TFT_ORANGE };
int homeScreenNumColors = sizeof(homeScreenColors) / sizeof(homeScreenColors[0]);
int homeScreenNumColorCount = 0;

String usbT, tapScreenT, scanMeT, totalT, fossaT, satsT, forT, fiatT, feedT, chargeT, printingT, waitT, workingT, thisVoucherT, ofBitcoinT, thankYouT, scanMeClaimT, tooMuchFiatT, contactOwnerT;

HardwareSerial SerialPort1(1);
HardwareSerial SerialPort2(2);
SoftwareSerial printerSerial(PRINTER_RX, PRINTER_TX);
Adafruit_Thermal printer(&printerSerial);
LGFX tft;
FT6336U touchScreen;
bool lastTouchState = false;

// Helper function to check if touch was released
bool touchWasReleased() {
  uint8_t touches = touchScreen.read_touch_number();
  bool currentTouchState = (touches > 0);
  bool wasReleased = false;

  if (lastTouchState && !currentTouchState) {
    wasReleased = true;
  }

  lastTouchState = currentTouchState;
  return wasReleased;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize display FIRST (critical for WT32-SC01 PLUS)
  Serial.println("=== WT32-SC01 PLUS Starting ===");
  Serial.println("Initializing display...");
  tft.init();
  tft.setRotation(1);
  tft.setBrightness(255);
  tft.fillScreen(TFT_BLACK);
  Serial.println("Display initialized");

  // Now initialize other peripherals
  translateAll(language);
  Serial.println(workingT);

  // Initialize I2C and touch controller
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  touchScreen.begin(Wire);
  Serial.println("Touch initialized");

  FlashFS.begin(FORMAT_ON_FAIL);
  Serial.println("SPIFFS initialized");

  printMessage("", "Loading..", "", TFT_WHITE, TFT_BLACK);
  printMessage("", "Loading..", "", TFT_BLACK, TFT_WHITE);

  // wait few secods for tap to start config mode
  while (waitForTap && total < 100) {
    if (touchWasReleased()) {
      printMessage(usbT, "", tapScreenT, TFT_WHITE, TFT_BLACK);
      executeConfig();
      waitForTap = false;
    }
    delay(20);
    total++;
  }
  
  if(hardcoded == false){
    readFiles();
  }
  else{
    printDefaultValues();
  }
  splitSettings(deviceString);

  // initialize bill and coin acceptor
  SerialPort1.begin(300, SERIAL_8N2, BILL_TX, BILL_RX);
  SerialPort2.begin(4800, SERIAL_8N1, COIN_TX);
  printerSerial.begin(9600);
  pinMode(COIN_INHIBIT, OUTPUT);
}

void loop() {
  if (maxBeforeResetTally >= maxBeforeReset) {
    printMessage("", tooMuchFiatT, contactOwnerT, TFT_WHITE, TFT_BLACK);
    delay(100000000);
  } else {
    // initialize printer

    SerialPort1.write(184);
    digitalWrite(COIN_INHIBIT, HIGH);
    tft.fillScreen(TFT_BLACK);
    if (SerialPort1.available()) {
      Serial.println("Bill acceptor connected");
    }
    if (SerialPort1.available()) {
      Serial.println("Coin acceptor connected");
    }
    moneyTimerFun();
    Serial.println("total" + String(total));
    Serial.println("maxBeforeResetTally" + String(maxBeforeResetTally));
    maxBeforeResetTally = maxBeforeResetTally + (total / 100);
    Serial.println("maxBeforeResetTally" + String(maxBeforeResetTally));
    makeLNURL();
    qrShowCodeLNURL(scanMeT);
  }
}

void moneyTimerFun() {
  waitForTap = true;
  coins = 0;
  bills = 0;
  total = 0;
  while (waitForTap || total == 0) {
    if (homeScreenNumColorCount == homeScreenNumColors) {
      homeScreenNumColorCount = 0;
    }
    if (total == 0) {
      waitForTap = true;
      feedmefiat();
      feedmefiatloop();
    }
    if (SerialPort1.available()) {
      int x = SerialPort1.read();
      for (int i = 0; i < billAmountSize; i++) {
        if ((i + 1) == x) {
          bills = bills + billAmountFloat[i];
          total = (coins + bills);
          printMessage(billAmountFloat[i] + currencyATM, totalT + String(total) + currencyATM, tapScreenT, TFT_WHITE, TFT_BLACK);
        }
      }
    }
    if (SerialPort2.available()) {
      int x = SerialPort2.read();
        for (int i = 0; i < coinAmountSize; i++) {
          if ((i + 1) == x) {
            coins = coins + coinAmountFloat[i];
            total = (coins + bills);
            printMessage(coinAmountFloat[i] + currencyATM, totalT + String(total) + currencyATM, tapScreenT, TFT_WHITE, TFT_BLACK);
          }
        }
    }
    if (touchWasReleased() || total > maxAmount) {
      waitForTap = false;
    }
    homeScreenNumColorCount++;
  }
  total = (coins + bills) * 100;

  // Turn off machines
  SerialPort1.write(185);
  digitalWrite(COIN_INHIBIT, LOW);
}
