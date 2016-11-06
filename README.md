[Homie](https://github.com/marvinroger/homie) based bridge MQTT to [RFLink module](http://www.nemcon.nl/blog2)

## Features:
* publish via MQTT in JSON format message received from RFlink gateway
* send to RFlink gateway message received via MQTT
* All Homie buildin features (OTA,configuration)

## Limitations
  * Raw debug messages are not supported for long codes because of MQTT message size limit in Homie implementation, JSON response may be broken

## Connections

* Do standard [RFlink wiring](http://www.nemcon.nl/blog2/wiring)
* Connect RFLink to ESP8266
  * Arduino MEGA TX (D1) to ESP8266 Soft Serial RX (GPIO14/D5) via logic level shifter 5V->3,3V or voltage divider (User 200Ohm and 470Ohm resistors)
  * Arduino MEGA RX (D0) to ESP8266 Soft Serial TX (GPIO5/D1)


## MQTT messages


<table>
<tr>
  <th>Property</th>
  <th>Message format</th>
  <th>Direction</th>
  <th>Description</th>
</tr>
<tr>
  <td>_HOMIE_PREFIX_/_node-id_/serial01/to-send/set</td>
  <td>See [RFlink protocol reference](http://www.nemcon.nl/blog2/protref) </td>
  <td>Controller → Device</td>
  <td></td>
</tr>
<tr>
  <td>_HOMIE_PREFIX_/_node-id_/serial01/debug/set</td>
  <td>(true|false)</td>
  <td>Controller → Device</td>
  <td>Enable debug mode - raw RFlink will be published, even it was not parsed by converter</td>
</tr>
<tr>
  <td>_HOMIE_PREFIX_/_node-id_/serial01/_device_name_</td>
  <td>See below JSON message format section</td>
  <td>Device → Controller</td>
  <td></td>
</tr>
<tr>
</tr>
<tr>
  <td>_HOMIE_PREFIX_/_node-id_/$online</td>
  <td><code>(true|false)</code></td>
  <td>Device → Controller</td>
  <td><code>/true</code> when the device is online, <code>false</code> when the device is offline (through LWT)</td>
</tr>
</table>

## JSON message format

Message received from RFLink is converted to JSON array. Each field is converted to array element. Name is used in topic name.

RFlink message: <code>20;1B;Keeloq;ID=e311;SWITCH=0A;CMD=ON;BAT=OK;</code> will be published in topic:
<code>_HOMIE_PREFIX_/_node-id_/serial01/Keeloq</code> with value <code>[{"msgIdx":"1B;","ID":"e311","SWITCH":"0A","CMD":"ON","BAT":"OK"}]</code>
