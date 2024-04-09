// #define ESP32_RTOS  // Uncomment this line if you want to use the code with freertos only on the ESP32
//  Has to be done before including "OTA.h"

#include "OTA.h"

#define mySSID ""
#define myPASSWORD ""

#include <ArduinoJson.h>
#include "yeelight.h"
#include <Dictionary.h>

//------Yeelight Config------
StaticJsonDocument<200> doc;
Yeelight *yeelight;

char luz_cama_ip[1][15] = {"192.168.0.6"};

char kitchen_lights_ips[2][15] = {"192.168.0.7", "192.168.0.5"};
char room_lights_ips[3][15] = {"192.168.0.6", "192.168.0.17", "192.168.0.10"};
char all_lights_ips[5][15] = {"192.168.0.7", "192.168.0.5", "192.168.0.6", "192.168.0.17", "192.168.0.10"};
const int led_wifi_pin = 13;
const int led_functions = D8;

const int rojo = 16711680;

const int loop_colors_count = 7;
const int loop_colors[loop_colors_count] = {4915330, 4129023, 2263842, 16741656, rojo, 16711935, 255};

const int loop_brightness_count = 5;
const int loop_brightness[loop_brightness_count] = {1, 25, 50, 75, 100};

const int loop_temperatures_count = 5;
const int loop_temperatures[loop_temperatures_count] = {1700, 2500, 3500, 4200, 6500};

//------Buttons Config------
const int lights_buttons_pins[5] = {D5, D1, D2, D3, D4};
const int BUTTONS_CANT = 5;
int buttons_states[15] = {0, 0, 0, 0, 0};
const int switch_room_button_pin = D6;
const int switch_mode_button_pin = D7;

//-----Actions Config-------

Dictionary<int, String> actions_by_buttons_mode_0 = Dictionary<int, String>();
Dictionary<int, String> actions_by_buttons_mode_1 = Dictionary<int, String>();

//-----Delays config------
const int led_state_on_delay = 200;
const int read_action_button_key_delay = 200;

//-----states Config--------
int current_room = 0;
int current_mode = 0;
int current_color = 0;
int current_temperature = 0;
int current_brightness = 0;
const int bright_up_value = 7;
const int bright_down_value = 5;
const int red_scene_value = 9;

void setup()
{
  //Serial.begin(115200);
  //Serial.println("Booting");

  setupOTA("NodeMCU-Arcade-Lights", mySSID, myPASSWORD);

  // Your setup code
  for (int i = 0; i < 5; i++)
  {
    pinMode(lights_buttons_pins[i], INPUT_PULLUP);
  }

  pinMode(switch_room_button_pin, INPUT_PULLUP);
  pinMode(switch_mode_button_pin, INPUT_PULLUP);
  pinMode(led_functions, OUTPUT);

  actions_by_buttons_mode_0.set(0, "turn_on");
  actions_by_buttons_mode_0.set(1, "turn_off");
  actions_by_buttons_mode_0.set(2, "loop_temperature");
  actions_by_buttons_mode_0.set(3, "white_all");
  actions_by_buttons_mode_0.set(4, "loop_color");

  actions_by_buttons_mode_1.set(0, "police");
  actions_by_buttons_mode_1.set(1, "strobe");
  actions_by_buttons_mode_1.set(2, "strobe_color");
  actions_by_buttons_mode_1.set(3, "disco");
  actions_by_buttons_mode_1.set(4, "loop");

  turn_on_off_function_led(3);
}

void loop()
{
#ifdef defined(ESP32_RTOS) && defined(ESP32)
#else // If you do not use FreeRTOS, you have to regulary call the handle method.
  ArduinoOTA.handle();
#endif
  execute_switch_buttons_special_actions();
  execute_button_switch_action(switch_room_button_pin);
  execute_button_switch_action(switch_mode_button_pin);

  int pressed_buttons = 0;
  int last_pressed_button = -1;
  int pressed_buttons_acum = 0;

  for (int i = 0; i < BUTTONS_CANT; i++)
  {
    buttons_states[i] = digitalRead(lights_buttons_pins[i]);
    if (buttons_states[i] == LOW)
    {
      pressed_buttons += 1;
      last_pressed_button = i;
      pressed_buttons_acum += i;
    }
  }
  
  if (pressed_buttons == 1)
  {
    execute_flow_by_button_id(last_pressed_button, current_room, current_mode);
  }

  if (pressed_buttons > 1)
  {
    // Execute special action
    execute_special_action(pressed_buttons_acum, current_room);
  }

  delay(read_action_button_key_delay);
}

void execute_flow_by_button_id(int button_id, int c_room, int c_mode)
{
  int lights_count = get_lights_counts_by_room(c_room);
  if (c_room == 0)
  {
    execute_action_by_mode(button_id, kitchen_lights_ips, lights_count, c_mode);
  }

  if (c_room == 1)
  {
    execute_action_by_mode(button_id, room_lights_ips, lights_count, c_mode);
  }

  if (c_room == 2)
  {
    execute_action_by_mode(button_id, all_lights_ips, lights_count, c_mode);
  }
}

void execute_action_by_mode(int button_id, char ips[][15], int ligths_count, int c_mode)
{
  String action_name = get_action_name_by_mode_and_button_id(c_mode, button_id);
  String params = get_extra_params_by_action(action_name);

  execute_command(action_name, c_mode, params, ips, ligths_count);
}

void execute_special_action(int pressed_buttons_acum, int c_room)
{
  int lights_count = get_lights_counts_by_room(c_room);

  if (pressed_buttons_acum == bright_up_value || pressed_buttons_acum == bright_down_value)
  {
    String action_name = "";
    if (pressed_buttons_acum == bright_up_value)
    {
      action_name = "loop_brightness_up";
    }
    else
    {
      action_name = "loop_brightness_down";
    }

    String params = get_extra_params_by_action(action_name);
    action_name = "loop_brightness";

    if (c_room == 0)
    {
      execute_command(action_name, 0, params, kitchen_lights_ips, lights_count);
    }

    if (c_room == 1)
    {
      execute_command(action_name, 0, params, room_lights_ips, lights_count);
    }

    if (c_room == 2)
    {
      execute_command(action_name, 0, params, all_lights_ips, lights_count);
    }
  }

  if (pressed_buttons_acum == red_scene_value) // Rojo
  {
    execute_command("turn_off", 0, "", all_lights_ips, get_lights_counts_by_room(2));
    execute_command("turn_on", 0, "", luz_cama_ip, 1);
    execute_command("loop_color", 0, String(rojo), luz_cama_ip, 1);
    execute_command("loop_brightness", 0, "10", luz_cama_ip, 1);
  }
}

void execute_command(String action_name, int c_mode, String params, char ips[][15], int ligths_count)
{
  for (int i = 0; i < ligths_count; i++)
  {
    yeelight->start_command(action_name, c_mode, params, ips[i]);
  }
}

String get_action_name_by_mode_and_button_id(int c_mode, int button_id)
{
  if (c_mode == 0)
  {
    return actions_by_buttons_mode_0.get(button_id);
  }

  if (c_mode == 1)
  {
    return actions_by_buttons_mode_1.get(button_id);
  }

  return "";
}

String get_extra_params_by_action(String action_name)
{
  if (action_name == "loop_color")
  {
    return String(get_next_color());
  }

  if (action_name == "loop")
  {
    return "10";
  }

  if (action_name == "loop_temperature")
  {
    return String(get_next_temperature());
  }

  if (action_name == "loop_brightness_up")
  {
    return String(get_next_brightness(true));
  }

  if (action_name == "loop_brightness_down")
  {
    return String(get_next_brightness(false));
  }

  return "";
}

void execute_button_switch_action(int button_id)
{
  int led_blink_times = 0;
  if (digitalRead(button_id) == LOW)
  {
    if (button_id == switch_room_button_pin)
    {
      led_blink_times = next_room();
    }

    if (button_id == switch_mode_button_pin)
    {
      led_blink_times = next_mode();
    }

    led_blink_times += 1;
    turn_on_off_function_led(led_blink_times);
  }
}

void execute_switch_buttons_special_actions(){
  if (digitalRead(switch_room_button_pin) == LOW && digitalRead(switch_mode_button_pin) == LOW)
   {
    //Print current Room
    turn_on_off_function_led(current_room + 1);
    delay(led_state_on_delay);
    //Print current Mode
    turn_on_off_function_led(current_mode + 1);
   }
  }

int next_room()
{
  current_room += 1;
  if (current_room == 3)
  {
    current_room = 0;
  }

  return current_room;
}

int next_mode()
{
  current_mode += 1;
  if (current_mode == 2)
  {
    current_mode = 0;
  }

  return current_mode;
}

int get_next_color()
{
  current_color += 1;
  if (current_color == loop_colors_count)
  {
    current_color = 0;
  }
  return loop_colors[current_color];
}

int get_next_temperature()
{
  current_temperature += 1;
  if (current_temperature == loop_temperatures_count)
  {
    current_temperature = 0;
  }
  return loop_temperatures[current_temperature];
}

int get_next_brightness(bool up)
{
  if (up)
  {
    current_brightness += 1;
    if (current_brightness >= loop_brightness_count)
    {
      current_brightness = loop_brightness_count - 1;
    }
  }
  else
  {
    current_brightness -= 1;
    if (current_brightness < 1)
    {
      current_brightness = 0;
    }
  }

  return loop_brightness[current_brightness];
}

void turn_on_off_function_led(int led_blink_times)
{
  for (int i = 0; i < led_blink_times; i++)
  {
    digitalWrite(led_functions, HIGH);
    delay(led_state_on_delay);
    digitalWrite(led_functions, LOW);
    delay(led_state_on_delay);
  }
}

int get_lights_counts_by_room(int c_room)
{
  if (c_room == 0)
  {
    return (sizeof(kitchen_lights_ips) / sizeof(kitchen_lights_ips[0]));
  }

  if (c_room == 1)
  {
    return (sizeof(room_lights_ips) / sizeof(room_lights_ips[0]));
  }

  if (c_room == 2)
  {
    return (sizeof(all_lights_ips) / sizeof(all_lights_ips[0]));
  }

  return -1;
}
