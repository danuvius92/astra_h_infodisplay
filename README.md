
# Astra H Infodisplay

A small project to show me the Battery Voltage, Coolant Temp and Engine Load.

BOM:
- ESP32
- 1.69 inch Screen (https://amzn.eu/d/bs5uHai)
- OBD2 Bluetooth Adapter (https://amzn.eu/d/hTLYTui)
- Some Wires
- 3D printed Files: https://makerworld.com/en/models/426179





## Getting your OBD2 Bluetooth Adapter MAC Address

For this i used my Android Tablet and the App "Serial Bluetooth Terminal". Stick your OBD2 Adapter to your cars OBD2 Port and connect to it. In the App you shoud find the MAC Address. We will need the MAC in the .ino File.
## Preparing the .ino File

Open the .ino File. In Line 38 add the MAC.

Example:

MAC: 7A:B4:84:AE:81:8D

converts into

uint8_t address[6]  = {0x7A, 0xB4, 0x84, 0xAE, 0x81, 0x8D};

## Wiring
| ESP32  | Display |
| ------------- | ------------- |
| GND  | GND  |
| 5V  | VCC  |
| GPIO19  | SCL  |
| GPIO23  | SDA  |
| RST  | RES  |
| GPIO2  | DC  |
| GPIO17  | CS  |
| not connected  | BLK  |


## Flashing the ESP32

Install the ESP32 Filesystem Uploader which you can find here: https://github.com/me-no-dev/arduino-esp32fs-plugin

Under /Documents/Arduino create a new Project Folder and place the Project Files in there.

In the Arduino IDE open the .ino File.

Under "Tools" select the appropriate Settings for your ESP32. Then click "ESP32 Sketch Data Upload". This will upload the Fonts under /data to your ESP32 Filesystem.

