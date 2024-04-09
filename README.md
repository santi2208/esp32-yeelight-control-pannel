# Arduino Based Yeelight control Panel 

Simple arduino based control panel using push buttons, for Yeelight/Xiaomi lights in local network

## Installation

This project uses an esp32 board, therefore, it is necessary that it be installed on an arduino ide
## Setup

Add your wifi credentials here.
```c++
#define mySSID ""
#define myPASSWORD ""
```
Add light's ips here.
```c++
char kitchen_lights_ips[2][15] = {"", ""};
char room_lights_ips[3][15] = {"", "", ""};
char all_lights_ips[5][15] = {"", "", "", "", ""};
```
Change pins ids here.
```c++
//------Buttons Config------
const int lights_buttons_pins[5] = {D5, D1, D2, D3, D4};
const int BUTTONS_CANT = 5;
int buttons_states[15] = {0, 0, 0, 0, 0};
const int switch_room_button_pin = D6;
const int switch_mode_button_pin = D7;
```
