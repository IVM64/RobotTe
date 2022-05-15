// RobTeJoystick.ino
//4/20/2020
//Vistelilabs (www.vistelilabs.com)
//This program connects to the BLE UART service and process the data received to control a robot arm.
// Copyright <2020> <VisteliLabs>
//Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <Dynamixel2Arduino.h>
#include <CurieBLE.h>

#define DXL_SERIAL   Serial1
#define DEBUG_SERIAL Serial
const uint8_t DXL_DIR_PIN = 2; // DYNAMIXEL Shield DIR PIN
 

const uint8_t HAND_ID = 9; 
const uint8_t WRIST_ID = 11;
const uint8_t ELBOW_ID = 12;
const uint8_t SHOULDER_ID = 3;
const float DXL_PROTOCOL_VERSION = 1.0; 

Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN);

#define CMD_IDDLE       0
#define CMD_OPEN_HAND   1
#define CMD_CLOSE_HAND  2
#define CMD_WRIST_HAND  4
#define CMD_ELBOW_SHOULDER 8


bool StopFlag = false;
int cmdData = CMD_IDDLE;
int lastCmdData = CMD_IDDLE;
char *cmdStrCoordX;
char *cmdStrCoordY;
int cmdCoordX;
int cmdCoordY;
bool newData=false;
BLEDevice joystickPeripheral;

void UpdateValue(BLEDevice bledev, BLECharacteristic characteristic)
{
  char *data;
  data = (char *)characteristic.value();
  Serial.println(data);
  if (strstr(data,"START"))
  {
    StopFlag = true;
    characteristic.unsubscribe();
  }
  if (strstr(data,"cmd:B"))
  {
    if (lastCmdData != CMD_OPEN_HAND || lastCmdData != CMD_CLOSE_HAND)
    {
      lastCmdData = cmdData;
    }
    cmdData = CMD_CLOSE_HAND;
  }
  if (strstr(data,"cmd:X"))
  {
    if (lastCmdData != CMD_OPEN_HAND || lastCmdData != CMD_CLOSE_HAND)
    {
      lastCmdData = cmdData;
    }
    cmdData = CMD_OPEN_HAND;
  }
  if (strstr(data,"cmd:Y"))
  {
    cmdData = CMD_WRIST_HAND;
  }
  if (strstr(data,"cmd:A"))
  {
    cmdData = CMD_ELBOW_SHOULDER;
  }
  if (strstr(data,","))
  {
    cmdStrCoordX = strtok(data,",");
    cmdCoordX = atoi(cmdStrCoordX+1);
    cmdStrCoordY = strtok(NULL,")");
    cmdCoordY = atoi(cmdStrCoordY);
    newData = true;
  }
}

bool connect2Joystick()
{
  bool notFound = true;
  // initialize the BLE hardware
  BLE.begin();

  Serial.println("BLE Central - Peripheral Explorer");

  // start scanning for peripherals
  BLE.scan();
  
  while (notFound)
  {
    BLEDevice peripheral = BLE.available();
  
    if (peripheral) {
      // discovered a peripheral, print out address, local name, and advertised service
      Serial.print("Found ");
      Serial.print(peripheral.address());
      Serial.print(" '");
      Serial.print(peripheral.localName());
      Serial.print("' ");
      Serial.print(peripheral.advertisedServiceUuid());
      Serial.println();
  
      // see if peripheral is our UART service
      if (peripheral.localName() == "ROBOTTE Service") {
        // stop scanning
        BLE.stopScan();
  
        // connect to the peripheral
        if (!peripheral.connect()) {
          Serial.println("Failed to connect!");
          return false;
        }
      
        if (!peripheral.discoverAttributes()) {
          Serial.println("Attribute discovery failed!");
          peripheral.disconnect();
          return false;
        }
      
        // retrieve the LED characteristic
        BLECharacteristic notifyCharacteristic = peripheral.characteristic("581108e2-6a9a-40d3-9d5f-db6f7529aa6a");
      
        if (!notifyCharacteristic) {
          Serial.println("Peripheral does not have NOTIFY characteristic!");
          peripheral.disconnect();
          return false;
        } else 
        {
          if (notifyCharacteristic.canNotify())
          {
            notifyCharacteristic.subscribe();
            notifyCharacteristic.setEventHandler(BLEValueUpdated,UpdateValue);
            joystickPeripheral = peripheral;
          }
        }  
        
        notFound = false;
      }
    }
  }

  return(notFound);
}

void setup() {
  // Use UART port of DYNAMIXEL Shield to debug.
  DEBUG_SERIAL.begin(115200);
  //while (!DEBUG_SERIAL)
    //delay(10);

  connect2Joystick();
   
  // This has to match with DYNAMIXEL baudrate.
  dxl.begin(1000000);
  // Set Port Protocol Version. This has to match with DYNAMIXEL protocol version.
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);
  // Get DYNAMIXEL information
  dxl.ping(HAND_ID);
  dxl.ping(WRIST_ID);
  dxl.ping(ELBOW_ID);
  dxl.ping(SHOULDER_ID);

  dxl.torqueOff(HAND_ID);
  dxl.setOperatingMode(HAND_ID, OP_POSITION);
  dxl.torqueOn(HAND_ID);

  dxl.torqueOff(WRIST_ID);
  dxl.setOperatingMode(WRIST_ID, OP_POSITION);
  dxl.torqueOn(WRIST_ID);

  dxl.torqueOff(ELBOW_ID);
  dxl.setOperatingMode(ELBOW_ID, OP_POSITION);
  dxl.torqueOn(ELBOW_ID);

  dxl.torqueOff(SHOULDER_ID);
  dxl.setOperatingMode(SHOULDER_ID, OP_POSITION);
  dxl.torqueOn(SHOULDER_ID);

}

void loop() {
  bool contLoop = true;
  
  while (joystickPeripheral.connected() && contLoop) {
    if (StopFlag)
    {
      contLoop = false;
      continue;
    }

    switch(cmdData)
    {
      case CMD_OPEN_HAND:
      {
          Serial.println("OPEN HAND");
          // Set Goal Position in RAW value
          dxl.setGoalPosition(HAND_ID, 512);
          cmdData = lastCmdData;
      }
      break;
      case CMD_CLOSE_HAND:
      {
          Serial.print("CLOSE HAND");
          // Set Goal Position in DEGREE value
          dxl.setGoalPosition(HAND_ID, 5.7, UNIT_DEGREE);
          cmdData = lastCmdData;
      }
      break;
      case CMD_WRIST_HAND:
      {
          // Check if new data is available
          if (newData)
          {
            dxl.setGoalPosition(WRIST_ID, cmdCoordX);
            newData = false;
          }        
      }
      break;
      case CMD_ELBOW_SHOULDER:
      {
          // Check if new data is available
          if (newData)
          {
            dxl.setGoalPosition(SHOULDER_ID, cmdCoordX);
            newData = false;
            if (cmdCoordY > 128 && cmdCoordY < 512)
            {
              dxl.setGoalPosition(ELBOW_ID, cmdCoordY);
              delay(250);
              newData = false;
            }
          }        
      }
      break;
    }
  }

  joystickPeripheral.disconnect();
}

