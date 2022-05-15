// RobTeJoystick.ino
//4/20/2020
//Vistelilabs (www.vistelilabs.com)
//This program creates an BLE UART service to transmit the joystick and button data to a BLE client.
// Copyright <2020> <VisteliLabs>
//Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include "Adafruit_seesaw.h"

Adafruit_seesaw ss;

#define BUTTON_RIGHT 6
#define BUTTON_DOWN  7
#define BUTTON_LEFT  9
#define BUTTON_UP    10
#define BUTTON_START   14
uint32_t button_mask = (1 << BUTTON_RIGHT) | (1 << BUTTON_DOWN) | 
                (1 << BUTTON_LEFT) | (1 << BUTTON_UP) | (1 << BUTTON_START);
#if defined(ESP8266)
  #define IRQ_PIN   2
#elif defined(ESP32)
  #define IRQ_PIN   14
#elif defined(ARDUINO_NRF52832_FEATHER)
  #define IRQ_PIN   27
#elif defined(TEENSYDUINO)
  #define IRQ_PIN   8
#elif defined(ARDUINO_ARCH_WICED)
  #define IRQ_PIN   PC5
#else
  #define IRQ_PIN   5
#endif

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

#define SERVICE_UUID           "8907022f-2a69-4dc4-9b98-85b1356ef422" // UART service UUID
#define CHARACTERISTIC_UUID_RX "42f1162a-4dea-4901-97ca-9cca75a1e8a3"
#define CHARACTERISTIC_UUID_TX "581108e2-6a9a-40d3-9d5f-db6f7529aa6a"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};


void BLESetup()
{
  // Create the BLE Device
  BLEDevice::init("ROBOTTE Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  
}

void setup()   {
  //while (!Serial);
  Serial.begin(115200);

  if(!ss.begin(0x49)){
    Serial.println("ERROR!");
    while(1);
  }
  else{
    Serial.println("seesaw started");
    Serial.print("version: ");
    Serial.println(ss.getVersion(), HEX);
  }
  ss.pinModeBulk(button_mask, INPUT_PULLUP);
  ss.setGPIOInterrupts(button_mask, 1);
  pinMode(IRQ_PIN, INPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32

  Serial.println("Initialized");
  delay(500);

  // text display tests
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);

  display.clearDisplay();
  display.display();

  BLESetup();
}

int last_x = 0, last_y = 0;
//msg contains the formatted commands.
char msg[20];

void loop() {
  int y = ss.analogRead(2);
  int x = ss.analogRead(3);
  
  display.setCursor(0,0);

  sprintf(msg,"");
  
  if ( (abs(x - last_x) > 3)  ||  (abs(y - last_y) > 3)) {
    sprintf(msg,"                    ");
    display.clearDisplay();
    display.display();
    display.setCursor(0,0);
    
    sprintf(msg,"(%03d,%03d)",x,y);
    display.println(msg);
  }
  else
  {
    display.println("");
  }

  if ( (abs(x - last_x) > 3)  ||  (abs(y - last_y) > 3)) {
    last_x = x;
    last_y = y;
  }
 
  if(!digitalRead(IRQ_PIN)){
    uint32_t buttons = ss.digitalReadBulk(button_mask);
    if (! (buttons & (1 << BUTTON_DOWN))) {
      display.println("B");
      sprintf(msg,"cmd:B          ");
    }
    
    if (! (buttons & (1 << BUTTON_RIGHT))) {
      display.println("A");
      sprintf(msg,"cmd:A          ");
    }
    
    if (! (buttons & (1 << BUTTON_LEFT))) {
      display.println("Y");
      sprintf(msg,"cmd:Y          ");
    }
    
    if (! (buttons & (1 << BUTTON_UP))) {
      display.println("X");
      sprintf(msg,"cmd:X          ");
    }
    
    if (! (buttons & (1 << BUTTON_START))) {
      display.println("START");
      sprintf(msg,"cmd:START          ");
    }
  }
  display.display();  
  delay(10);

    if (deviceConnected && strlen(msg) > 0) {
        pTxCharacteristic->setValue(msg); 
        pTxCharacteristic->notify();
        delay(10); // bluetooth stack will go into congestion, if too many packets are sent
  }  
}
