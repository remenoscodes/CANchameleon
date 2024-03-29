//
// OLED SSD1306 128x64
//
#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>
#include <ACAN2515.h>
#include <SPI.h>

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

SSD1306AsciiWire ascii;
enum DisplayArea
{
  HEADER_AREA,
  MAIN_AREA,
  MAIN_AREA1,
  MAIN_AREA2,
  MAIN_AREA3,
  FOOTER_AREA
};

// ——————————————————————————————————————————————————————————————————————————————
//   MCP2515 connections:
//     - standard SPI pins for SCK, MOSI and MISO
//     - a digital output for CS
//     - interrupt input pin for INT
// ——————————————————————————————————————————————————————————————————————————————
//  If you use CAN-BUS shield (http://wiki.seeedstudio.com/CAN-BUS_Shield_V2.0/) with Arduino Uno,
//  use B connections for MISO, MOSI, SCK, #9 or #10 for CS (as you want),
//  #2 or #3 for INT (as you want).
// ——————————————————————————————————————————————————————————————————————————————
//  Error codes and possible causes:
//     In case you see "Configuration error 0x1", the Arduino doesn't communicate
//        with the 2515. You will get this error if there is no CAN shield or if
//        the CS pin is incorrect.
//     In case you see success up to "Sent: 17" and from then on "Send failure":
//        There is a problem with the interrupt. Check if correct pin is configured
// ——————————————————————————————————————————————————————————————————————————————
constexpr uint32_t CAN_BIT_RATE = 125; // 125 kbps
constexpr byte MCP2515_CS = 10;        // CS input of MCP2515 (adapt to your design)
constexpr byte MCP2515_INT = 2;        // INT output of MCP2515 (adapt to your design)
ACAN2515 can(MCP2515_CS, SPI, MCP2515_INT);
constexpr uint32_t QUARTZ_FREQUENCY = 8UL * 1000UL * 1000UL; // 8 MHz

constexpr int LED_CAN_READY = A0;     // Yellow LED pin
constexpr int LED_CAN_RECEIVING = A1; // Blue LED pin
constexpr int LED_CAN_SENDING = A2;   // Builtin LED pin

constexpr int LED_INJECTOR_SIM_MODE = 5; // Red LED
constexpr int LED_SNIFFER_MODE = 6;      // Blue LED

#define SNIFFER_MODE 0
#define INJECTOR_SIM_MODE 1
uint8_t OPERATION_MODE = SNIFFER_MODE; // 0 sniffer 1 injector
#define TOGGLE_OPERATION_MODE_BUTTON_PIN 3

// ——————————————————————————————————————————————————————————————————————————————

#define MAX_CAN_IDS (0 / sizeof(uint32_t))
uint32_t uniqueIDs[MAX_CAN_IDS];
uint8_t numUniqueIDs = 0;

#define MEMORY_LIMIT 150                 // Total available memory for bitmap
#define CAN_ID_LIMIT (MEMORY_LIMIT * 8)  // Maximum number of CAN IDs that can be tracked
uint8_t canIdBitmap[MEMORY_LIMIT] = {0}; // Bitmap for the active group
uint8_t selectedGroup = -1;              // false for group 1, true for group 2

#define CAN_IDS_NO_RANGE -1
#define CAN_IDS_RANGE_0 0
#define CAN_IDS_RANGE_1 1

#define CAN_IDS_RANGE_0_BUTTON_PIN 8 // Button to select group 1
#define CAN_IDS_RANGE_1_BUTTON_PIN 9 // Button to select group 2

void setup()
{
  startSerial();
  SPI.begin();
  printAvailableMem();
  initializeGPIOs();
  printAvailableMem();
  setupASCII();
  printAvailableMem();
  updateOperationModeDisplay();
  initializeCAN();
  printAvailableMem();
  updateFooterWithFreeMem();
  displayCurrentCANIDRange();
  updateCANIDsCountDisplay();
}

void loop()
{
  static unsigned long lastDebounceTime1 = 0;
  static unsigned long lastDebounceTime2 = 0;
  unsigned long debounceDelay = 50; // Debounce delay in milliseconds

  if (digitalRead(TOGGLE_OPERATION_MODE_BUTTON_PIN) == LOW && (millis() - lastDebounceTime1) > debounceDelay)
  {
    lastDebounceTime1 = millis();
    toggleOperationMode();
  }

  // Group selection
  if (digitalRead(CAN_IDS_RANGE_0_BUTTON_PIN) == LOW && (millis() - lastDebounceTime1) > debounceDelay)
  {
    selectedGroup = selectedGroup == CAN_IDS_RANGE_0 ? CAN_IDS_NO_RANGE : 0;
    lastDebounceTime1 = millis();
    printSerial("Group selected", selectedGroup);
    displayCurrentCANIDRange();
    updateCANIDsCountDisplay();
  }

  if (digitalRead(CAN_IDS_RANGE_1_BUTTON_PIN) == LOW && (millis() - lastDebounceTime2) > debounceDelay)
  {
    selectedGroup = selectedGroup == CAN_IDS_RANGE_1 ? CAN_IDS_NO_RANGE : 1; // Select group 1
    lastDebounceTime2 = millis();
    printSerial("Group selected", selectedGroup);
    displayCurrentCANIDRange();
    updateCANIDsCountDisplay();
  }

  if (OPERATION_MODE == INJECTOR_SIM_MODE)
  {
    sendCANId();
  }

  CANMessage message;
  if (can.available())
  {
    if (can.receive(message))
    {
      setCanId(message.id);
      //   storeUniqueID(message.id);
      handleReceivedMessage(message);
    }
  }

  if (somethingChangedThatAffectsMemory())
  {
    updateFooterWithFreeMem();
  }
}

bool somethingChangedThatAffectsMemory()
{
  return true;
}

void toggleOperationMode()
{
  OPERATION_MODE = OPERATION_MODE == 0 ? 1 : 0;
  updateOperationModeDisplay();
}

void updateOperationModeDisplay()
{
  if (OPERATION_MODE == SNIFFER_MODE)
  {
    displayASCII("CAN: SNIFFER", HEADER_AREA);
    digitalWrite(LED_INJECTOR_SIM_MODE, LOW);
    digitalWrite(LED_SNIFFER_MODE, HIGH);
  }
  else if (OPERATION_MODE == INJECTOR_SIM_MODE)
  {
    displayASCII("CAN: INJECTOR_SIM", HEADER_AREA);
    digitalWrite(LED_INJECTOR_SIM_MODE, HIGH);
    digitalWrite(LED_SNIFFER_MODE, LOW);
  }
}

void displayCurrentCANIDRange()
{
  if (selectedGroup == CAN_IDS_RANGE_0)
  {
    displayASCII("RANGE 0: 0..1023", MAIN_AREA);
  }
  else if (selectedGroup == CAN_IDS_RANGE_1)
  {
    displayASCII("RANGE 1: 1024..2047", MAIN_AREA);
  }
  else
  {
    displayASCII("RANGE: NONE", MAIN_AREA);
  }
}

void updateCANIDsCountDisplay()
{
  // uint16_t count = countSetBits();
  // String message =
  // displayASCII("GROUP: " + String(selectedGroup) + " CAN IDs: " + String(countSetBits()), MAIN_AREA);
}

void setCanId(uint16_t canId)
{
  if (selectedGroup == CAN_IDS_NO_RANGE)
  {
    return;
  }

  uint16_t rangeStart = selectedGroup == CAN_IDS_RANGE_0 ? 0 : 1024;
  uint16_t rangeEnd = selectedGroup == CAN_IDS_RANGE_0 ? 1023 : 2047;
  if (canId >= rangeStart && canId <= rangeEnd)
  {
    uint16_t adjustedId = canId - rangeStart; // Adjust CAN ID to start from 0 within each group
    canIdBitmap[adjustedId / 8] |= (1 << (adjustedId % 8));
    updateCANIDsCountDisplay();
  }
}

bool isCanIdSet(uint16_t canId)
{
  if (selectedGroup == CAN_IDS_NO_RANGE)
  {
    return true;
  }

  uint16_t rangeStart = selectedGroup == CAN_IDS_RANGE_0 ? 0 : 1024;
  uint16_t rangeEnd = selectedGroup == CAN_IDS_RANGE_0 ? 1023 : 2047;
  if (canId >= rangeStart && canId <= rangeEnd)
  {
    uint16_t adjustedId = canId - rangeStart; // Adjust CAN ID to start from 0 within each group
    return canIdBitmap[adjustedId / 8] & (1 << (adjustedId % 8));
  }
  return false;
}

uint16_t countSetBits()
{
  uint16_t count = 0;
  for (uint16_t i = 0; i < MEMORY_LIMIT * 8; ++i)
  {
    if (canIdBitmap[i / 8] & (1 << (i % 8)))
    {
      ++count;
    }
  }
  return count;
}

void storeUniqueID(uint32_t id)
{
  if (numUniqueIDs >= MAX_CAN_IDS)
  {
    printSerial("Max unique IDs reached: ", numUniqueIDs);
    return;
  }

  for (uint8_t i = 0; i < numUniqueIDs; i++)
  {
    if (uniqueIDs[i] == id)
    {
      // ID already exists.
      return;
    }
  }

  // If we're here, it means id doesn't exist and we haven't reached max capacity.
  uniqueIDs[numUniqueIDs] = id;
  numUniqueIDs++;

  printSerial("CAN ID Stored: ", id);
  printSerial("CAN IDs Count: ", numUniqueIDs);
}

void handleReceivedMessage(const CANMessage &message)
{
  digitalWrite(LED_CAN_RECEIVING, HIGH);

  char mIdBuff[12];
  sprintf(mIdBuff, "CAN ID Ox%x", message.id);

  displayASCII(mIdBuff, MAIN_AREA1);

  Serial.print(mIdBuff);
  // Serial.print(", Length: ");
  // Serial.print(message.len);
  // Serial.print(", Data: ");
  // for (int i = 0; i < message.len; i++)
  // {
  //   Serial.print(message.data[i], HEX);
  //   Serial.print(" ");
  // }

  digitalWrite(LED_CAN_RECEIVING, LOW);
}

void initializeGPIOs()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(LED_CAN_RECEIVING, OUTPUT);
  pinMode(LED_CAN_READY, OUTPUT);
  pinMode(LED_CAN_SENDING, OUTPUT);
  pinMode(LED_SNIFFER_MODE, OUTPUT);
  pinMode(LED_INJECTOR_SIM_MODE, OUTPUT);

  pinMode(CAN_IDS_RANGE_0_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CAN_IDS_RANGE_1_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TOGGLE_OPERATION_MODE_BUTTON_PIN, INPUT_PULLUP);
}

void startSerial()
{
  Serial.begin(38400);
  while (!Serial)
  {
    delay(50);
    toggleBuiltinLED();
  }
}

void toggleBuiltinLED()
{
  static unsigned long lastToggleTimestamp = 0;
  const long interval = 100; // Interval at which to blink (milliseconds)
  unsigned long currentMillis = millis();
  if (currentMillis - lastToggleTimestamp >= interval)
  {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    lastToggleTimestamp = currentMillis;
  }
}

void initializeCAN()
{
  displayASCII(F("CAN READY!"), MAIN_AREA);

  // CAN bitrate 500 kb/s worked -> ELANTRA, TUCSON
  // ACAN2515Settings settings(QUARTZ_FREQUENCY, 500UL * 1000UL);
  // Rede Peer 2 Peer Arduino
  ACAN2515Settings settings(QUARTZ_FREQUENCY, CAN_BIT_RATE * 1000UL); // CAN bit rate 125 kb/s //Worked on TUCSON
  settings.mRequestedMode = ACAN2515Settings::NormalMode;             // Select mode
  const uint16_t errorCode = can.begin(settings, []
                                       { can.isr(); });

  if (errorCode == 0)
  {
    digitalWrite(LED_CAN_READY, HIGH);
    printCANConfiguration(settings);
  }
  else
  {
    digitalWrite(LED_CAN_READY, LOW);
    printCANError(errorCode);
  }
}

int availableMemory()
{
  // Use 1024 with ATmega168
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

void printAvailableMem()
{
  Serial.print("M: ");
  Serial.println(availableMemory());
}

void printCANConfiguration(const ACAN2515Settings &settings)
{
  Serial.print("Bit Rate prescaler: ");
  Serial.println(settings.mBitRatePrescaler);
  Serial.print("Propagation Segment: ");
  Serial.println(settings.mPropagationSegment);
  Serial.print("Phase segment 1: ");
  Serial.println(settings.mPhaseSegment1);
  Serial.print("Phase segment 2: ");
  Serial.println(settings.mPhaseSegment2);
  Serial.print("SJW: ");
  Serial.println(settings.mSJW);
  Serial.print("Triple Sampling: ");
  Serial.println(settings.mTripleSampling ? "yes" : "no");
  Serial.print("Actual bit rate: ");
  Serial.print(settings.actualBitRate());
  Serial.println(" bit/s");
  Serial.print("Exact bit rate ? ");
  Serial.println(settings.exactBitRate() ? "yes" : "no");
  Serial.print("Sample point: ");
  Serial.print(settings.samplePointFromBitStart());
  Serial.println("%");

  displayASCII("ACAN CONFIGURED", MAIN_AREA);
}

void printCANError(uint16_t errorCode)
{
  Serial.print(F("Configuration error 0x"));
  Serial.println(errorCode, HEX);

  char errorText[20];
  snprintf(errorText, sizeof(errorText), "ERROR 0x%X", errorCode);
  displayASCII(errorText, MAIN_AREA);
}

void updateFooterWithFreeMem()
{
  char footerText[24]; // Buffer should be big enough to hold the footer text
  int freeMem = availableMemory();
  snprintf(footerText, sizeof(footerText), "Free Mem: %d ", freeMem);
  displayASCII(footerText, FOOTER_AREA); // Print to footer area
  printSerial("Free Mem: ", freeMem);
}

void setupASCII()
{
  Wire.begin();
  ascii.begin(&Adafruit128x64, SCREEN_ADDRESS); // Replace Adafruit128x64 with your display type and 0x3C with your display's I2C address
  ascii.clear();

  // Set the cursor position to start writing from the first column of the first row
  ascii.setCursor(0, 0);
  ascii.setFont(System5x7);
}

void displayASCII(const String &text, DisplayArea area)
{
  // Calculate the length of the previous text in pixels
  // Assuming each character is 6 pixels wide including space
  // int previousTextLengthPixels = 0;
  // int charWidthIncludingSpace = 6; // 5x7 font with 1 pixel horizontal spacing

  // Set cursor position based on area
  switch (area)
  {
  case HEADER_AREA:
    // Calculate previous text length in pixels
    // previousTextLengthPixels = headerAreaText.length() * charWidthIncludingSpace;
    // Set cursor to start position of header area
    ascii.setCursor(0, 1); // Assuming row 1 for header, adjust if needed
    ascii.clearToEOL();    // Clear the line
    // Optionally, clear more based on previous text length if necessary
    // This is a placeholder, actual clearing might need custom implementation
    // headerAreaText = text; // Update stored header text
    ascii.println(text); // Print new text
    break;
  case MAIN_AREA:
    // Calculate previous text length in pixels for main area
    // previousTextLengthPixels = mainAreaText.length() * charWidthIncludingSpace;
    // Set cursor to start position of main area
    ascii.setCursor(0, 2); // Assuming row 4 for main area, adjust if needed
    ascii.clearToEOL();    // Clear the line
    // Optionally, clear more based on previous text length if necessary
    // This is a placeholder, actual clearing might need custom implementation
    // mainAreaText = text; // Update stored main text
    ascii.println(text); // Print new text
    break;
  case MAIN_AREA1:
    // Calculate previous text length in pixels for main area
    // previousTextLengthPixels = mainAreaText.length() * charWidthIncludingSpace;
    // Set cursor to start position of main area
    ascii.setCursor(0, 3); // Assuming row 4 for main area, adjust if needed
    ascii.clearToEOL();    // Clear the line
    // Optionally, clear more based on previous text length if necessary
    // This is a placeholder, actual clearing might need custom implementation
    // mainAreaText = text; // Update stored main text
    ascii.println(text); // Print new text
    break;
  case MAIN_AREA2:
    ascii.setCursor(0, 4); // Assuming row 4 for main area, adjust if needed
    ascii.clearToEOL();    // Clear the line
    ascii.println(text);   // Print new text
    break;
  case MAIN_AREA3:
    ascii.setCursor(0, 5); // Assuming row 4 for main area, adjust if needed
    ascii.clearToEOL();    // Clear the line
    ascii.println(text);   // Print new text
    break;
  case FOOTER_AREA:
    ascii.setCursor(0, 7); // Assuming last row for footer
    ascii.clearToEOL();    // Clear the line
    ascii.println(text);
    break;
  }
}

void printSerial(const char *message, int value)
{
  Serial.print(message);
  Serial.println(value);
}

void sendCANId()
{
  static uint32_t lastSendTime = 0;
  if (millis() - lastSendTime > 100)
  {
    lastSendTime = millis();

    CANMessage frame;
    frame.id = random(0x7FF); // Random standard ID
    frame.len = 8;
    for (int i = 0; i < frame.len; ++i)
    {
      frame.data[i] = i; // Example data
    }

    if (can.tryToSend(frame))
    {
      digitalWrite(LED_CAN_SENDING, HIGH); // Indicate send success
      delay(10);                           // Short delay for LED indication
      digitalWrite(LED_CAN_SENDING, LOW);
    }
  }
}