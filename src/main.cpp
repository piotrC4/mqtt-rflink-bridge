/*
 * ESP8266 MQTT Homie based gateway for RFLink
 * for WITTY module
 */
// Constants
#define FIRMWARE_NAME "rflink-bridge"
#define FIRMWARE_VERSION "0.001"

#define DEBUG 1

#define PIN_LED_RED 15
#define PIN_LED_GREEN 12
#define PIN_LED_BLUE 13
#define PIN_BUTTON 4           // D2
#define PIN_SOFT_SERIAL_RX 14  // D5
#define PIN_SOFT_SERIAL_TX  5  // D1

#define MODE_STANDARD 1
#define MODE_JSON 2
#define MODE_RAW 3

// Includes
#include <EEPROM.h>
#include <Homie.h>
#include <SoftwareSerial.h>

// Global Objects

// EEPROM data structure
struct EEpromDataStruct {
  int publishMode;
};

EEpromDataStruct EEpromData; // EEprom data instance

HomieNode serialNode("serial01", "Serial gateway");
Bounce debouncerButton = Bounce(); // Button debuncer
// Software serial - connection to RF arduino
SoftwareSerial mySerial(PIN_SOFT_SERIAL_RX,PIN_SOFT_SERIAL_TX);

// Global variables
int lastButtonValue = 1; // previous state of build-in button

// **************************************************************
// Functions delcaration
void loopHandler(); // Homie loop handler
void setupHandler(); // Homie setup handler
void onHomieEvent(HomieEvent event); // Homie event handler
bool serialMessageHandler(String message); // Serial message hander
bool publishModeHandler(String message); // Method of publishing
/***************************************************************
 * Main setup - Homie initialization
 */
void setup() {

  // Init EEPROM
  EEPROM.begin(sizeof(EEpromData));
  EEPROM.get(0,EEpromData);

  // set PIN modes
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);

  // Turn OFF RGB LED
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_BLUE, LOW);

  // Initialize button
  debouncerButton.attach(PIN_BUTTON);
	debouncerButton.interval(50);

  mySerial.begin(57600);

  /* Initiate homie object */
  Homie.setFirmware(FIRMWARE_NAME, FIRMWARE_VERSION);

  Homie.setLedPin(PIN_LED_RED, HIGH);
  Homie.setResetTrigger(PIN_BUTTON, LOW, 10000);

  serialNode.subscribe("to-send", serialMessageHandler);
  serialNode.subscribe("publish-mode", publishModeHandler);

  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);

#ifdef DEBUG
  Homie.enableLogging(true);
#else
  Homie.enableLogging(false);
#endif

  Homie.registerNode(serialNode);
  Homie.onEvent(onHomieEvent);
  Homie.setup();

}

/*


***************************************************************
 * Main loop - only homie support
 */
void loop()
{
  Homie.loop();
}

/***************************************************************
 * Homie event
 */
void onHomieEvent(HomieEvent event) {
  switch(event) {
    case HOMIE_CONFIGURATION_MODE:
      // Configuration mode is started
      EEpromData.publishMode = MODE_STANDARD;
      EEPROM.put(0, EEpromData);
      EEPROM.commit();
      break;
    case HOMIE_NORMAL_MODE:
      // Normal mode is started
      break;
    case HOMIE_OTA_MODE:
      // OTA mode is started
      break;
    case HOMIE_ABOUT_TO_RESET:
      // The device is about to reset
      break;
    case HOMIE_WIFI_CONNECTED:
      // Wi-Fi is connected in normal mode - force reboot
      break;
    case HOMIE_WIFI_DISCONNECTED:
      // Wi-Fi is disconnected in normal mode - reboot to defaults
      // ESP.restart();
      break;
    case HOMIE_MQTT_CONNECTED:
      // MQTT is connected in normal mode
      break;
    case HOMIE_MQTT_DISCONNECTED:
      // MQTT is disconnected in normal mode - force reboot
      // ESP.restart();
      break;
  }
}


/****************************************************************
 * Loop (inside Homie)
 */
void loopHandler()
{
  int buttonValue = debouncerButton.read();
  if (buttonValue != lastButtonValue)
  {
    lastButtonValue = buttonValue;
    if (buttonValue == 0)
    {
      // Button pressed
      Homie.setNodeProperty(serialNode, "button","pressed",false);
    }
  }
  debouncerButton.update();

  //check softUART for data
  if(mySerial.available())
  {
    String msgFromRF = mySerial.readString();
    msgFromRF.remove(msgFromRF.length()-1);
#ifdef DEBUG
    Serial.print("Received from RF:");
    Serial.println(msgFromRF);
#endif
    int startIdxEOL = 0;
    int stopIdxEOL=0;
    while (stopIdxEOL!=msgFromRF.length())
    {
      stopIdxEOL=msgFromRF.indexOf('\n',startIdxEOL);
      if (stopIdxEOL==-1)
        stopIdxEOL=msgFromRF.length();
      String messageSingleRow = msgFromRF.substring(startIdxEOL,stopIdxEOL); // Signle row from serial message
      startIdxEOL = stopIdxEOL+1; // next start index (start of next row)
// --- start of row processing
      if (messageSingleRow.substring(0,2) == "20") // Check for proper message header from RFLink to ESP8266
      {
        int beginOfMessgeIdx = messageSingleRow.indexOf(';',3);
        int beginOfDataIdx = messageSingleRow.indexOf(';',beginOfMessgeIdx+1);
        String msgIndex = messageSingleRow.substring(3,beginOfMessgeIdx);
        String deviceName;
        String message01;
        int tmpPublishMode = EEpromData.publishMode;
        if (messageSingleRow.substring(beginOfMessgeIdx+1,beginOfMessgeIdx+4)=="VER")
        {
          deviceName = "VERSION";
          message01 = messageSingleRow.substring(beginOfMessgeIdx+1);
        } else if (messageSingleRow.substring(beginOfMessgeIdx+1,beginOfMessgeIdx+5)=="PONG") {
          deviceName = "PONG";
          message01 = "PONG=1;;";
        } else if (messageSingleRow.substring(beginOfMessgeIdx+1,beginOfMessgeIdx+8)=="RFDEBUG") {
          tmpPublishMode = MODE_RAW;
          deviceName = "RFDEBUG";
        } else if (messageSingleRow.substring(beginOfMessgeIdx+1,beginOfMessgeIdx+9)=="RFUDEBUG") {
          tmpPublishMode = MODE_RAW;
          deviceName = "RFUDEBUG";
        } else if (messageSingleRow.substring(beginOfMessgeIdx+1,beginOfMessgeIdx+9)=="QRFDEBUG") {
          tmpPublishMode = MODE_RAW;
          deviceName = "QRFDEBUG";
        } else {
          deviceName = messageSingleRow.substring(beginOfMessgeIdx+1,beginOfDataIdx);
          message01 = messageSingleRow.substring(beginOfDataIdx+1);
        }
        bool publishStatus;
        if (EEpromData.publishMode == MODE_JSON)
        {
          int startIdx = 0;
          int stopIdx = 0;
#ifdef DEBUG
          Serial.print("Message to convert into JSON:");
          Serial.println(message01);
#endif
          DynamicJsonBuffer jsonBuffer;
          JsonObject& root = jsonBuffer.createObject();
          root["msgIdx"] = msgIndex;
          root["Name"] = deviceName;
          stopIdx=message01.indexOf(';',startIdx);
          while (stopIdx>-1)
          {
            stopIdx=message01.indexOf(';',startIdx);
            String subMessage = message01.substring(startIdx,stopIdx);
            int IdxOfEqual=subMessage.indexOf('=');
            if (IdxOfEqual>-1)
            {
              root[subMessage.substring(0,IdxOfEqual)]=subMessage.substring(IdxOfEqual+1);
            }
            startIdx = stopIdx+1;
          }
#ifdef DEBUG
          Serial.print("Publish:");
          root.printTo(Serial);
          Serial.println();
#endif
          String outMessage;
          root.printTo(outMessage);

          publishStatus = Homie.setNodeProperty(serialNode,"JSONmsg",outMessage,false);
        } else if (EEpromData.publishMode == MODE_RAW) {
#ifdef DEBUG
          Serial.println(messageSingleRow);
#endif
          publishStatus = Homie.setNodeProperty(serialNode,"rawmsg",messageSingleRow,false);
        } else {
          // Defeult publish method - topic path
          message01.remove(message01.length()-1);
          Serial.println("To parse:" + message01);
          int firstEQIdx = message01.indexOf('=');
          int preMsgIdx = message01.indexOf(';');

          String newTopic;
          int startIdx;
          int stopIdx = 0;
          DynamicJsonBuffer jsonBuffer;
          JsonObject& root = jsonBuffer.createObject();


          if (deviceName=="VERSION" || deviceName == "PONG")
          {
            newTopic = deviceName;
            startIdx = 0;
          } else {
            newTopic = deviceName + '/' + message01.substring(firstEQIdx+1,preMsgIdx);
            startIdx = preMsgIdx+1;
          }
          stopIdx=message01.indexOf(';',startIdx);
          while (stopIdx>-1)
          {
            stopIdx=message01.indexOf(';',startIdx);
            String subMessage = message01.substring(startIdx,stopIdx);
            int IdxOfEqual=subMessage.indexOf('=');
            if (IdxOfEqual>-1)
            {
              root[subMessage.substring(0,IdxOfEqual)]=subMessage.substring(IdxOfEqual+1);
            }
            startIdx = stopIdx+1;
          }

          String outMessage;
          root.printTo(outMessage);

#ifdef DEBUG
          Serial.println("Publish to: "+newTopic+" -> "+outMessage);
#endif

          publishStatus = Homie.setNodeProperty(serialNode,newTopic,outMessage,false);
        }
        if (!publishStatus)
          Homie.setNodeProperty(serialNode,"error", "failed to pubish " + deviceName + ", message too long", false);

      } else {
        Homie.setNodeProperty(serialNode,"unsupported",msgFromRF, false);
      }
// --- end of row processing
    }
  }
}

/****************************************************************
 * Setup (inside Homie)
 */
void setupHandler()
{
  Homie.setNodeProperty(serialNode,"publish-mode", String(EEpromData.publishMode));
  lastButtonValue = digitalRead(PIN_BUTTON);
}
/****************************************************************
 * publish Method Handler
 */
bool publishModeHandler(String message)
{
  int currentMode = EEpromData.publishMode;
  if (message == "JSON" || message == String(MODE_JSON))
  {
    EEpromData.publishMode = MODE_JSON;
  } else if (message == "RAW" || message == String(MODE_RAW)) {
    EEpromData.publishMode = MODE_RAW;
  } else {
    EEpromData.publishMode = MODE_STANDARD;
  }
  if (currentMode != EEpromData.publishMode)
  {
    EEPROM.put(0, EEpromData);
    EEPROM.commit();
    Homie.setNodeProperty(serialNode,"publish-mode", String(EEpromData.publishMode));
  }
}
/****************************************************************
 * Serial message handler
 */
bool serialMessageHandler(String value)
{
#ifdef DEBUG
   Serial.print("Send to RF:");
   Serial.println(value);
#endif
   mySerial.println(value);
   return true;
}
