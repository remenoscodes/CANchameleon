//
// OLED SSD1306 128x64
//
#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

SSD1306AsciiWire ascii;

enum DisplayArea
{
  HEADER_AREA,
  MAIN_AREA,
  FOOTER_AREA
};

// ——————————————————————————————————————————————————————————————————————————————
//   ACAN2515 Demo in loopback mode
// ——————————————————————————————————————————————————————————————————————————————

#include <ACAN2515.h>
#include <SPI.h>

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

static const byte MCP2515_CS = 10; // CS input of MCP2515 (adapt to your design)
static const byte MCP2515_INT = 2; // INT output of MCP2515 (adapt to your design)

// ——————————————————————————————————————————————————————————————————————————————
//   MCP2515 Driver object
// ——————————————————————————————————————————————————————————————————————————————

ACAN2515 can(MCP2515_CS, SPI, MCP2515_INT);

// ——————————————————————————————————————————————————————————————————————————————
//   MCP2515 Quartz: adapt to your design
// ——————————————————————————————————————————————————————————————————————————————

static const uint32_t QUARTZ_FREQUENCY = 8UL * 1000UL * 1000UL; // 16 MHz

// ——————————————————————————————————————————————————————————————————————————————
//    SETUP
// ——————————————————————————————————————————————————————————————————————————————

//----------------------------------------------------------------------------------------------------------------------

static uint32_t gBlinkLedDate = 0;
static uint32_t gReceivedFrameCount = 0;
static uint32_t gSentFrameCount = 0;

// ——————————————————————————————————————————————————————————————————————————————

void setup()
{
  //--- Start serial
  startSerial();

  SPI.begin();

  Serial.println("BEFORE PINs");
  printAvailableMem();

  initializeGPIOs();

  Serial.println("BEFORE ASCII");
  printAvailableMem();
  setupASCII();

  Serial.println("BEFORE ACAN");
  printAvailableMem();
  initializeCAN();

  Serial.println("AFTER ACAN ASCII");
  printAvailableMem();
  updateFooterWithFreeMem();
}

void loop()
{
  if (somethingChangedThatAffectsMemory())
  {
    updateFooterWithFreeMem();
  }

  CANMessage message;
  if (can.available())
  {
    if (can.receive(message))
    {
      // Example: Light up the LED if any message is received
      digitalWrite(4, HIGH); // Assuming LED is connected to pin 4
      displayASCII("ID Ox" + String(message.id, HEX), MAIN_AREA);

      // Process the received message
      Serial.print("ID: ");
      Serial.print(message.id, HEX);
      Serial.print(", Length: ");
      Serial.print(message.len);
      Serial.print(", Data: ");
      for (int i = 0; i < message.len; i++)
      {
        Serial.print(message.data[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      delay(100); // Short visible delay
      digitalWrite(4, LOW);
    }
  }
  // updateFooterWithFreeMem();
}

bool somethingChangedThatAffectsMemory()
{
  return true;
}

void initializeGPIOs()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  pinMode(4, OUTPUT); // Blue LED
  pinMode(7, OUTPUT); // Red LED
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
  displayASCII(F("CAN Sniffer"), HEADER_AREA);
  displayASCII(F("ACAN2515 SETUP"), MAIN_AREA);
  Serial.println("SETUP ACAN2515");

  // CAN bitrate 500 kb/s worked -> ELANTRA, TUCSON
  // ACAN2515Settings settings(QUARTZ_FREQUENCY, 500UL * 1000UL);
  // Rede Peer 2 Peer Arduino
  ACAN2515Settings settings(QUARTZ_FREQUENCY, 125 * 1000UL); // CAN bit rate 125 kb/s //Worked on TUCSON
  settings.mRequestedMode = ACAN2515Settings::NormalMode;    // Select mode
  const uint16_t errorCode = can.begin(settings, []
                                       { can.isr(); });

  if (errorCode == 0)
  {
    digitalWrite(7, HIGH);
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

    displayASCII("DONE", MAIN_AREA);
    displayASCII("ACAN CONFIGURED", MAIN_AREA);
  }
  else
  {
    digitalWrite(7, LOW);
    Serial.print("Configuration error 0x");
    Serial.println(errorCode, HEX);

    displayASCII(F("ERROR"), HEADER_AREA);
    displayASCII("0X" + String(errorCode, HEX), MAIN_AREA);
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

void updateFooterWithFreeMem()
{
  char footerText[24]; // Buffer should be big enough to hold the footer text
  int freeMem = availableMemory();
  snprintf(footerText, sizeof(footerText), "Free Mem: %d ", freeMem);
  displayASCII(footerText, FOOTER_AREA); // Print to footer area
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
    ascii.setCursor(0, 4); // Assuming row 4 for main area, adjust if needed
    ascii.clearToEOL();    // Clear the line
    // Optionally, clear more based on previous text length if necessary
    // This is a placeholder, actual clearing might need custom implementation
    // mainAreaText = text; // Update stored main text
    ascii.println(text); // Print new text
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