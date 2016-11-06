/*
 * ESP8266 MQTT Homie based gateway for RFLink
 * for WITTY module
 */
// Constants
#define FIRMWARE_NAME "serial-gw"
#define FIRMWARE_VERSION "0.001"

#define DEBUG 1

#define PIN_LED_RED 15
#define PIN_LED_GREEN 12
#define PIN_LED_BLUE 13
#define PIN_BUTTON 4           // D2
#define PIN_SOFT_SERIAL_RX 14  // D5
#define PIN_SOFT_SERIAL_TX  5  // D1

// Includes
#include <Homie.h>
#include <SoftwareSerial.h>

// Global Objects
HomieNode serialNode("serial01", "Serial gateway");
Bounce debouncerButton = Bounce(); // Button debuncer
// Software serial - connection to RF arduino
SoftwareSerial mySerial(PIN_SOFT_SERIAL_RX,PIN_SOFT_SERIAL_TX);

// Global variables
int lastButtonValue = 1; // previous state of build-in button
bool debugMode = false;

// **************************************************************
// Functions delcaration
void loopHandler(); // Homie loop handler
void setupHandler(); // Homie setup handler
void onHomieEvent(HomieEvent event); // Homie event handler
bool serialMessageHandler(String message); // Serial message hander
bool debugHandler(String message); // Debug state change handler
/***************************************************************
 * Main setup - Homie initialization
 */
void setup() {


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
  serialNode.subscribe("debug", debugHandler);


  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);
#ifdef DEBUG
  Homie.enableLogging(true);
#endif
  Homie.registerNode(serialNode);

  Homie.onEvent(onHomieEvent);

  Homie.setup();

}

/****************************************************************
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
      ESP.restart();
      break;
    case HOMIE_MQTT_CONNECTED:
      // MQTT is connected in normal mode
      break;
    case HOMIE_MQTT_DISCONNECTED:
      // MQTT is disconnected in normal mode - force reboot
      ESP.restart();
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
      Homie.setNodeProperty(serialNode, "button","pressed");
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
    if (debugMode)
    {
      Homie.setNodeProperty(serialNode,"rawmsg",msgFromRF);
    }
    if (msgFromRF.substring(0,2) == "20") // Check for proper message header from RFLink to ESP8266
    {
      int beginOfMessgeIdx = msgFromRF.indexOf(';',3);
      int beginOfDataIdx = msgFromRF.indexOf(';',beginOfMessgeIdx+1);
      String msgIndex = msgFromRF.substring(3,beginOfMessgeIdx+1);
      String deviceName;
      String message01;

      if (msgFromRF.substring(beginOfMessgeIdx+1,beginOfMessgeIdx+4)=="VER")
      {
        deviceName = "VERSION";
        message01 = msgFromRF.substring(beginOfMessgeIdx+1);
      } else if (msgFromRF.substring(beginOfMessgeIdx+1,beginOfMessgeIdx+5)=="PONG") {
        deviceName = "PONG";
        message01 = msgFromRF.substring(beginOfMessgeIdx+1);
      } else if (msgFromRF.substring(beginOfMessgeIdx+1,beginOfMessgeIdx+8)=="RFDEBUG") {
        deviceName = "RFDEBUG";
        message01 = msgFromRF.substring(beginOfMessgeIdx+1);
      } else if (msgFromRF.substring(beginOfMessgeIdx+1,beginOfMessgeIdx+9)=="RFUDEBUG") {
        deviceName = "RFUDEBUG";
        message01 = msgFromRF.substring(beginOfMessgeIdx+1);
      } else if (msgFromRF.substring(beginOfMessgeIdx+1,beginOfMessgeIdx+9)=="QRFDEBUG") {
        deviceName = "QRFDEBUG";
        message01 = msgFromRF.substring(beginOfMessgeIdx+1);
      } else {
        deviceName = msgFromRF.substring(beginOfMessgeIdx+1,beginOfDataIdx);
        message01 = msgFromRF.substring(beginOfDataIdx+1);
      }
      int startIdx = 0;
      int stopIdx = 0;
      stopIdx=message01.indexOf(';',startIdx);
      DynamicJsonBuffer jsonBuffer;
      JsonArray& root = jsonBuffer.createArray();
      //JsonObject& rflinkJson = root.createNestedObject().createNestedObject(deviceName);
      JsonObject& rflinkJson = root.createNestedObject();
      rflinkJson["msgIdx"] = msgIndex;
      while (stopIdx>-1)
      {
        stopIdx=message01.indexOf(';',startIdx);
        String subMessage = message01.substring(startIdx,stopIdx);
        int IdxOfEqual=subMessage.indexOf('=');
        if (IdxOfEqual>-1)
        {
          rflinkJson[subMessage.substring(0,IdxOfEqual)]=subMessage.substring(IdxOfEqual+1);
        }
        startIdx = stopIdx+1;
      }
#ifdef DEBUG
      root.printTo(Serial);
      Serial.println();
#endif
      String outMessage;
      root.printTo(outMessage);
      Homie.setNodeProperty(serialNode,deviceName,outMessage);

    } else {
      Homie.setNodeProperty(serialNode,"unsupported",msgFromRF);
    }

  }
}

/****************************************************************
 * Setup (inside Homie)
 */
void setupHandler()
{
  Homie.setNodeProperty(serialNode,"debug","false");
  lastButtonValue = digitalRead(PIN_BUTTON);
}

/****************************************************************
 * Serial message handler
 */
bool serialMessageHandler(String value)
{
#ifdef DEBUG
   Serial.println(value);
#endif
   mySerial.println(value);
   return true;
}
/****************************************************************
 * Debug message handler
 */
bool debugHandler(String message)
{
  if (message=="true" || message=="1" || message=="ON")
  {
    debugMode =true;
    Homie.setNodeProperty(serialNode,"debug","true");
  } else {
    debugMode = false;
    Homie.setNodeProperty(serialNode,"debug","false");
  }
  return true;
}
