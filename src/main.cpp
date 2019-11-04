/*
 * ESP8266 MQTT Homie based gateway for RFLink
 * for WITTY module
 */
// Constants

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
void HomieEventHandler(const HomieEvent& event); // Homie event handler
bool serialMessageHandler(const HomieRange& range, const String& message); // Serial message hander
bool publishModeHandler(const HomieRange& range, const String& message); // Method of publishing

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
  Homie_setFirmware(FIRMWARE_NAME, FIRMWARE_VER);

  Homie.setLedPin(PIN_LED_RED, HIGH);
  Homie.setResetTrigger(PIN_BUTTON, LOW, 10000);

  serialNode.advertise("to-send").settable(serialMessageHandler);
  serialNode.advertise("publish-mode").settable(publishModeHandler);

  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);

#ifndef DEBUG
  Homie.disableLogging();
#endif

  Homie.onEvent(HomieEventHandler);
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
void HomieEventHandler(const HomieEvent& event) 
{
  switch(event.type) {
    case HomieEventType::CONFIGURATION_MODE: // Default eeprom data in configuration mode
      // Configuration mode is started
      EEpromData.publishMode = MODE_STANDARD;
      EEPROM.put(0, EEpromData);
      EEPROM.commit();
      break;
    case HomieEventType::STANDALONE_MODE:
      // Do whatever you want when standalone mode is started
      break;
   case HomieEventType::NORMAL_MODE:
      // Do whatever you want when normal mode is started
      break;
    case HomieEventType::OTA_STARTED:
      // Do whatever you want when OTA is started
      break;
    case HomieEventType::OTA_PROGRESS:
      // Do whatever you want when OTA is in progress

      // You can use event.sizeDone and event.sizeTotal
      break;
    case HomieEventType::OTA_FAILED:
      // Do whatever you want when OTA is failed
      break;
    case HomieEventType::OTA_SUCCESSFUL:
      // Do whatever you want when OTA is successful
      break;
    case HomieEventType::ABOUT_TO_RESET:
      // Do whatever you want when the device is about to reset
      break;
    case HomieEventType::WIFI_CONNECTED:
      // Do whatever you want when Wi-Fi is connected in normal mode

      // You can use event.ip, event.gateway, event.mask
      break;
    case HomieEventType::WIFI_DISCONNECTED:
      // Do whatever you want when Wi-Fi is disconnected in normal mode

      // You can use event.wifiReason
      /*
        Wi-Fi Reason (souce: https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiClientEvents/WiFiClientEvents.ino)
        0  SYSTEM_EVENT_WIFI_READY               < ESP32 WiFi ready
        1  SYSTEM_EVENT_SCAN_DONE                < ESP32 finish scanning AP
        2  SYSTEM_EVENT_STA_START                < ESP32 station start
        3  SYSTEM_EVENT_STA_STOP                 < ESP32 station stop
        4  SYSTEM_EVENT_STA_CONNECTED            < ESP32 station connected to AP
        5  SYSTEM_EVENT_STA_DISCONNECTED         < ESP32 station disconnected from AP
        6  SYSTEM_EVENT_STA_AUTHMODE_CHANGE      < the auth mode of AP connected by ESP32 station changed
        7  SYSTEM_EVENT_STA_GOT_IP               < ESP32 station got IP from connected AP
        8  SYSTEM_EVENT_STA_LOST_IP              < ESP32 station lost IP and the IP is reset to 0
        9  SYSTEM_EVENT_STA_WPS_ER_SUCCESS       < ESP32 station wps succeeds in enrollee mode
        10 SYSTEM_EVENT_STA_WPS_ER_FAILED        < ESP32 station wps fails in enrollee mode
        11 SYSTEM_EVENT_STA_WPS_ER_TIMEOUT       < ESP32 station wps timeout in enrollee mode
        12 SYSTEM_EVENT_STA_WPS_ER_PIN           < ESP32 station wps pin code in enrollee mode
        13 SYSTEM_EVENT_AP_START                 < ESP32 soft-AP start
        14 SYSTEM_EVENT_AP_STOP                  < ESP32 soft-AP stop
        15 SYSTEM_EVENT_AP_STACONNECTED          < a station connected to ESP32 soft-AP
        16 SYSTEM_EVENT_AP_STADISCONNECTED       < a station disconnected from ESP32 soft-AP
        17 SYSTEM_EVENT_AP_STAIPASSIGNED         < ESP32 soft-AP assign an IP to a connected station
        18 SYSTEM_EVENT_AP_PROBEREQRECVED        < Receive probe request packet in soft-AP interface
        19 SYSTEM_EVENT_GOT_IP6                  < ESP32 station or ap or ethernet interface v6IP addr is preferred
        20 SYSTEM_EVENT_ETH_START                < ESP32 ethernet start
        21 SYSTEM_EVENT_ETH_STOP                 < ESP32 ethernet stop
        22 SYSTEM_EVENT_ETH_CONNECTED            < ESP32 ethernet phy link up
        23 SYSTEM_EVENT_ETH_DISCONNECTED         < ESP32 ethernet phy link down
        24 SYSTEM_EVENT_ETH_GOT_IP               < ESP32 ethernet got IP from connected AP
        25 SYSTEM_EVENT_MAX
      */
      break;
    case HomieEventType::MQTT_READY:
      // Do whatever you want when MQTT is connected in normal mode
      break;
    case HomieEventType::MQTT_DISCONNECTED:
      // Do whatever you want when MQTT is disconnected in normal mode

      // You can use event.mqttReason
      /*
        MQTT Reason (source: https://github.com/marvinroger/async-mqtt-client/blob/master/src/AsyncMqttClient/DisconnectReasons.hpp)
        0 TCP_DISCONNECTED
        1 MQTT_UNACCEPTABLE_PROTOCOL_VERSION
        2 MQTT_IDENTIFIER_REJECTED
        3 MQTT_SERVER_UNAVAILABLE
        4 MQTT_MALFORMED_CREDENTIALS
        5 MQTT_NOT_AUTHORIZED
        6 ESP8266_NOT_ENOUGH_SPACE
        7 TLS_BAD_FINGERPRINT
      */
      break;
    case HomieEventType::MQTT_PACKET_ACKNOWLEDGED:
      // Do whatever you want when an MQTT packet with QoS > 0 is acknowledged by the broker

      // You can use event.packetId
      break;
    case HomieEventType::READY_TO_SLEEP:
      // After you've called `prepareToSleep()`, the event is triggered when MQTT is disconnected
      break;
    case HomieEventType::SENDING_STATISTICS:
      // Do whatever you want when statistics are sent in normal mode
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
      serialNode.setProperty( "button").send("pressed");
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
    while (stopIdxEOL!=(int)msgFromRF.length())
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
          deviceName = "RFDEBUG";
          message01 = messageSingleRow.substring(beginOfMessgeIdx+1)+";";
        } else if (messageSingleRow.substring(beginOfMessgeIdx+1,beginOfMessgeIdx+9)=="RFUDEBUG") {
          deviceName = "RFUDEBUG";
          message01 = messageSingleRow.substring(beginOfMessgeIdx+1)+";";
        } else if (messageSingleRow.substring(beginOfMessgeIdx+1,beginOfMessgeIdx+9)=="QRFDEBUG") {
          deviceName = "QRFDEBUG";
          message01 = messageSingleRow.substring(beginOfMessgeIdx+1)+";";
        } else if (messageSingleRow.substring(beginOfMessgeIdx+1,beginOfMessgeIdx+6)=="DEBUG") {
          tmpPublishMode = MODE_RAW;
          deviceName = "DEBUG";
        } else {
          deviceName = messageSingleRow.substring(beginOfMessgeIdx+1,beginOfDataIdx);
          message01 = messageSingleRow.substring(beginOfDataIdx+1);
        }
        bool publishStatus;
        if (tmpPublishMode == MODE_JSON)
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

          publishStatus = serialNode.setProperty("JSONmsg").send(outMessage);
        } else if (tmpPublishMode == MODE_RAW) {
#ifdef DEBUG
          Serial.println(messageSingleRow);
#endif
          publishStatus = serialNode.setProperty("rawmsg").send(messageSingleRow);
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


          if ( deviceName=="VERSION" || deviceName == "PONG" || deviceName =="QRFDEBUG"
            || deviceName=="RFDEBUG" || deviceName == "RFUDEBUG")
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

          publishStatus = serialNode.setProperty(newTopic).send(outMessage);
        }
        if (!publishStatus)
          serialNode.setProperty("error").send("failed to pubish " + deviceName + ", message too long");

      } else {
        serialNode.setProperty("unsupported").send(msgFromRF);
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
  serialNode.setProperty("publish-mode").send(String(EEpromData.publishMode));
  lastButtonValue = digitalRead(PIN_BUTTON);
}
/****************************************************************
 * publish Method Handler
 */
bool publishModeHandler(const HomieRange& range, const String& message)
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
    serialNode.setProperty("publish-mode").send(String(EEpromData.publishMode));
  }
  return true;
}
/****************************************************************
 * Serial message handler
 */
bool serialMessageHandler(const HomieRange& range, const String& value)
{
#ifdef DEBUG
   Serial.print("Send to RF:");
   Serial.println(value);
#endif
   mySerial.println(value);
   return true;
}
