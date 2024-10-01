#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

struct device {
  String id;
  String location;
  char type;
  bool state;
  String data;
  };

device deviceList[10] = {};
device sortedList[10] = {};

int deviceCount = 0;
int currentDevice = -1;
int state = 0;

bool isHoldingSelect = false;
int selectStartTime = 0;
extern int __heap_start, *__brkval;

byte scrollDown[8] = {
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01010,
  0b00100
};

byte scrollUp[8] = {
  0b00100,
  0b01010,
  0b10101,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00000
};

byte degrees[8] = {
  B00100,
  B01010,
  B01010,
  B00100,
  B00000,
  B00000,
  B00000,
  B00000
};

byte percent[8] = {
  B00111,
  B01001,
  B01001,
  B00111,
  B00010,
  B00100,
  B00000,
  B00000
};

void addDevice(String input) {
  String id = input.substring(2, 5);
  char type = input.charAt(6);
  String location = input.substring(8,18);
  location.trim();
  device newDevice;
  newDevice.id = id;
  newDevice.type = type;
  newDevice.location = location;
  newDevice.state = false;
  newDevice.data = "WXYZ";
  
  deviceList[deviceCount] = newDevice;
  deviceCount++;
}

bool validateID(String input) {
  bool valid = false;
  String input_id = input.substring(2,5);
  for (int i = 0; i < deviceCount; i ++) {
    device temp = deviceList[i];
    if (temp.id == input_id) {
      valid = true;
    }
  }
  return valid;
}

void removeDeviceById(String input) {
  String id = input.substring(2,5);
  int foundIndex = -1;
  for (int i = 0; i < deviceCount; i++) {
    if (deviceList[i].id == id) {
      foundIndex = i;
      break;
    }
  }

  if (foundIndex >= 0) {
    for (int i = foundIndex; i < deviceCount - 1; i++) {
      deviceList[i] = deviceList[i + 1];
    }
    deviceCount--;
  }
}

void setDevice (String input) {
  int foundIndex = -1;
  bool newState = false;
  String id = input.substring(2,5);
  String command = input.substring(6);
  if (command == "OFF") {
    newState = false;
  } else {
    newState = true;
  }
  for (int i = 0; i < deviceCount; i++) {
    if (deviceList[i].id == id) {
      foundIndex = i;
    }
  }
  deviceList[foundIndex].state = newState;
}

void dataDevice(String input) {
  int foundIndex = -1;
  String id = input.substring(2,5);
  String newdata = input.substring(6);
  int newdataInt = int(newdata.toInt());
  for (int i = 0; i < deviceCount; i++) {
    if (deviceList[i].id == id) {
      foundIndex = i;
    }
  }
  char deviceType = deviceList[foundIndex].type;
  if (deviceType == 'O' || deviceType == 'C') {
    Serial.print("Error: these devices don't support power");
  }
  else if (deviceType == 'T') {
    if (newdataInt > 8 && newdataInt < 33) {
      newdata += "°C";
      deviceList[foundIndex].data = newdata;
    }
    else {
      Serial.print("Error: Invalid value for temperature");
    }
  }
  else if (deviceType == 'S' or deviceType == 'L') {
    if (newdataInt < 101 && newdataInt > -1) {
      char formattedData[5];
      sprintf(formattedData, "%03d", newdataInt);
      deviceList[foundIndex].data = String(formattedData);
    }
  }
}

/* void testDeviceList() {
  for (int i = 0; i < deviceCount; i++) {
    Serial.print("Device ");
    Serial.print(i);
    Serial.print(": ID=");
    Serial.print(deviceList[i].id);
    Serial.print(", Type=");
    Serial.print(deviceList[i].type);
    Serial.print(", Location=");
    Serial.print(deviceList[i].location);
    Serial.print(", State=");
    Serial.print(deviceList[i].state);
    Serial.print(", Data=");
    Serial.println(deviceList[i].data);
  }
} */

void lcdDisplay() {
  lcd.setCursor(0,0);
  if (currentDevice == 0) {
    lcd.print(" ");
  } else {
    lcd.write(byte(1));
  }
  lcd.print(sortedList[currentDevice].id);
  lcd.setCursor(6,0);
  lcd.print(sortedList[currentDevice].location);
  lcd.setCursor(0,1);
  if ((currentDevice + 1) == deviceCount) {
    lcd.print(" ");
  } else {
      lcd.write(byte(0));
  }
  lcd.print(sortedList[currentDevice].type);
  lcd.setCursor(4,1);
  bool currentState = sortedList[currentDevice].state;
  if (currentState == false) {
    lcd.print("OFF");
    lcd.setBacklight(3);
  } else {
    lcd.print(" ON");
    lcd.setBacklight(2);
  }
  lcd.setCursor(8,1);
  if (sortedList[currentDevice].data != "WXYZ") {
    if (sortedList[currentDevice].type == 'T') {
      lcd.print(sortedList[currentDevice].data.substring(0,2));
      lcd.write(byte(2));
      lcd.print("C");
    } else if (sortedList[currentDevice].type == 'L' || sortedList[currentDevice].type == 'S') {
      lcd.print(sortedList[currentDevice].data);
      lcd.print("%");
    }

  } else {
    lcd.print("      ");
  }
}

void alphabetSort() {
    for (int i = 0; i < 10; i++) {
        sortedList[i] = deviceList[i];
    }

    for (int i = 0; i < 9; i++) {
        int minIndex = i;
        for (int j = i+1; j < 10; j++) {
            if (sortedList[minIndex].id == "" || (sortedList[j].id != "" && sortedList[j].id < sortedList[minIndex].id)) {
                minIndex = j;
            }
        }
        if (minIndex != i) {
            device temp = sortedList[i];
            sortedList[i] = sortedList[minIndex];
            sortedList[minIndex] = temp;
        }
    }
}


int freeSRAM() {
  int free;
  if((int)__brkval == 0) {
    free = ((int)&free) - ((int)&__heap_start);
  }
  else {
    free = ((int)&free) - ((int)__brkval);
    free += sizeof(int);
  }
  return free;
}



void listenButtons() {
  uint8_t buttons = lcd.readButtons();
  if (buttons & BUTTON_DOWN && currentDevice < (deviceCount-1)) {
    currentDevice ++;
    delay(500);
    lcd.clear();
  }
  if (buttons & BUTTON_UP && currentDevice != 0) {
    currentDevice --;
    delay(500);
    lcd.clear();
  }
}

void selectPressed() {
    uint8_t button = lcd.readButtons();
    if (button & BUTTON_SELECT) {
      if (!isHoldingSelect) {
        isHoldingSelect = true;
        selectStartTime = millis();
      } else if (millis() - selectStartTime >= 1000) {
        isHoldingSelect = false;
        selectStartTime = 0;
        lcd.clear();
        lcd.setBacklight(5);
        lcd.print("F221887");
        lcd.setCursor(0, 1);
        lcd.print("Free SRAM: " + String(freeSRAM()));
        while (lcd.readButtons() & BUTTON_SELECT) {
          delay(10);
        }
        lcd.clear();
      }
    } else {
      isHoldingSelect = false;
      selectStartTime = 0;
    }
}

bool isValidType(char type) {
  if (type == 'O' || type == 'L' || type == 'S' || type == 'T' || type == 'C') {
    return true;
  } else {
    return false;
  }
}

bool validateProtocol(String input) {
  if (input.charAt(1) != '-') {
    return false;
  }
  
  char type1;
  String val;
  
  if (input.startsWith("A")) {
    if (input.length() < 9 || input.charAt(5) != '-') {
      return false;
    }
    type1 = input.charAt(6);
  }
  else if (input.startsWith("R")) {
    if (input.length() < 3 || input.charAt(1) != '-') {
      return false;
    }
  }
  else if (input.startsWith("S")) {
    if (input.charAt(5) != '-') {
      return false;
    }
    val = input.substring(6);
  }
  else if (input.startsWith("P")) {
    if (input.charAt(5) != '-') {
      return false;
    }
    val = input.substring(6);
    int checkInt = val.toInt();
    if (checkInt == 0 and val != "0") {
      return false;
    }
  } else {
    return false;
  }
  
  if (input.startsWith("A") && !isValidType(type1)) {
    return false;
  }
  if (input.startsWith("S") && (val != "ON" && val != "OFF")) {
    return false;
  }
  
  return true;
}


void setup() {
  lcd.begin(16, 2);
  lcd.setBacklight(5);
  Serial.begin(9600);
  lcd.createChar(0, scrollDown);
  lcd.createChar(1, scrollUp);
  lcd.createChar(2, degrees);
  lcd.createChar(3, percent);
  while (true) {
    Serial.write('Q');
    delay(1000);
    if (Serial.available()) {
      char input = Serial.read();
      if (input == 'X') {
        break;
      }
    }
  }
  Serial.write("BASIC\n");
  }



void loop() {
    alphabetSort();
    selectPressed();
    if (deviceCount == 0) {
      state = 0;
    }
    else if (deviceCount == 1) {
      currentDevice = 0;
      state = 1;
    }
    else if (deviceCount == 10) {
      state = 3;
    }
    else {
      state = 2;
    }
    switch (state) {
      case 0:
        lcd.setBacklight(7);
        lcd.clear();
        if (Serial.available() > 0) {
          String input = Serial.readStringUntil('\n');
          if (validateProtocol(input) == false) {
            Serial.print("ERROR: " + input + " doesn't conform to protocol \n");
            break;
          }
          if (input.startsWith("A")) {
            addDevice(input);
            }
          }  
        break;
      case 1:
        lcdDisplay();
        if (Serial.available() > 0) {
          String input = Serial.readStringUntil('\n');
          if (validateProtocol(input) == false) {
            Serial.print("ERROR: " + input + " doesn't conform to protocol \n");
            break;
          }
          if (input.startsWith("A")) {
            if (validateID(input) == false) {
              addDevice(input);
              //testDeviceList();
            } else {
              Serial.print("ERROR: ID already in use");
            }
          }
          if (input.startsWith("R")){
            if (validateID(input) == true) {
              removeDeviceById(input);
              //testDeviceList();
            } else {
              Serial.print("Invalid ID");
            }
          }
          if (input.startsWith("S")) {
            if (validateID(input) == true) {
              setDevice(input);
              //testDeviceList();
            } else {
              Serial.print("Invalid ID");
            }
          }
          if (input.startsWith("P")) {
            if (validateID(input) == true) {
              dataDevice(input);
            }
          }          
        }
          break;
      case 2:
        lcdDisplay();
        listenButtons();
        if (Serial.available() > 0) {
          String input = Serial.readStringUntil('\n');
          if (validateProtocol(input) == false) {
            Serial.print("ERROR: " + input + " doesn't conform to protocol \n");
            break;
          }
          if (input.startsWith("A")) {
            if (validateID(input) == false) {
              addDevice(input);
              //testDeviceList();
            } else {
              Serial.print("ERROR: ID already in use");
            }
          }
          if (input.startsWith("R")){
            if (validateID(input) == true) {
              removeDeviceById(input);
              //testDeviceList();
            } else {
              Serial.print("ERROR: Invalid ID");
            }
          }
          if (input.startsWith("S")) {
            if (validateID(input) == true) {
              setDevice(input);
              //testDeviceList();
            } else {
              Serial.print("ERROR: Invalid ID");
            }
          }
          if (input.startsWith("P")) {
            if (validateID(input) == true) {
              dataDevice(input);
            }
          }          
        }
          break;
      case 3:
        lcdDisplay();
        listenButtons();
        if (Serial.available() > 0) {
          String input = Serial.readStringUntil('\n');
          if (validateProtocol(input) == false) {
            Serial.print("ERROR: " + input + " doesn't conform to protocol \n");
            break;
          }
          if (input.startsWith("R")){
            if (validateID(input) == true) {
              removeDeviceById(input);
              //testDeviceList();
            } else {
              Serial.print("ERROR: Invalid ID");
            }
          }
          if (input.startsWith("S")) {
            if (validateID(input) == true) {
              setDevice(input);
              //testDeviceList();
            } else {
              Serial.print("ERROR: Invalid ID");
            }
          }
          if (input.startsWith("P")) {
            if (validateID(input) == true) {
              dataDevice(input);
            }
          }          
        }
          break;
    } 
}
















