/*
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  smaucourt@gmail.com - 11/11/2018
*/

#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#include <WiFiUdp.h>
#include "yeelight.h"

WiFiUDP _udp;
WiFiClient _client;
IPAddress _ipMulti(239, 255, 255, 250);
char _packetBuffer[550];

String _location, _support;
bool _powered;
char _server[15];

uint16_t _cmdid = 0, _port = 55443;

Yeelight::Yeelight()
{
}

Yeelight::~Yeelight()
{
  _udp.stop();
}

void Yeelight::lookup()
{
#ifdef ESP32
  _udp.beginMulticast(_ipMulti, 1982);
  _udp.beginMulticastPacket();
#else
  _udp.beginMulticast(WiFi.localIP(), _ipMulti, 1982);
  _udp.beginPacketMulticast(_ipMulti, 1982, WiFi.localIP());
#endif
  _udp.print("M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1982\r\nMAN: \"ssdp:discover\"\r\nST: wifi_bulb");
  _udp.endPacket();
  _udp.begin(1982);
}

int Yeelight::feedback()
{
  int packetSize = _udp.parsePacket();
  if (packetSize)
  {
    int len = _udp.read(_packetBuffer, 550);
    if (len > 0)
    {
      _packetBuffer[len] = 0;
    }
    parseFeedback(_packetBuffer, len);
  }
  return packetSize;
}

void Yeelight::parseFeedback(char *buffer, size_t len)
{
  int i = 0, _i = 0;
  char _b[255];
  while (i < len)
  {
    if (buffer[i] == '\r' &&
        i + 1 <= len &&
        buffer[i + 1] == '\n')
    {
      _b[_i] = 0;

      // ----
      String _str = String(_b);
      if (_str.startsWith("Location: yeelight://"))
      {
        int colon = _str.indexOf(':', 21) + 1;
        _location = _str.substring(10);
        _str.substring(21, colon).toCharArray(_server, colon - 21);
        // _port = _str.substring(colon).toInt();
      }
      if (_str.startsWith("support: "))
      {
        _support = _str.substring(9);
      }
      if (_str.startsWith("power: "))
      {
        _powered = _str.substring(7) == "on";
      }
      // ----

      i = i + 2;
      _i = 0;
    }
    else
    {
      _b[_i] = buffer[i];
      i++;
      _i++;
    }
  }
}

String Yeelight::sendCommand(String method, String params, char ipLight[15])
{
  if (_client.connect(ipLight, _port))
  {
    String payload = String("") + "{\"id\":" + (++_cmdid) + ",\"method\":\"" + method + "\",\"params\":" + params + "}";
    _client.println(payload);
  }

  String result = "ERROR";
  while (_client.connected())
  {
    result = _client.readStringUntil('\r');
    _client.stop();
  }
  return result;
}

bool Yeelight::isPowered()
{
  return _powered;
}

String Yeelight::getSupport()
{
  return _support;
}

String Yeelight::getLocation()
{
  return _location;
}

String Yeelight::getServer()
{
  return _server;
}

String Yeelight::start_action(String action, String extra_params, char ipLight[15])
{
  String params = "";
  String action_name = "";
  if (action == "turn_on")
  {
    action_name = "set_power";
    params = "[\"on\",\"smooth\", 500]";
  }

  if (action == "turn_off")
  {
    action_name = "set_power";
    params = "[\"off\",\"smooth\", 500]";
  }

  if (action == "toggle")
  {
    action_name = "toggle";
    params = "[\"smooth\", 500]";
  }

  if (action == "white_all")
  {
    action_name = "set_ct_abx";
    params = "[4000, \"smooth\", 500]";
    sendCommand(action_name, params, ipLight);
    delay(100);
    action_name = "set_bright";
    params = "[100, \"smooth\", 500]";
  }

  if (action == "loop_color")
  {
    action_name = "set_rgb";
    params = String("[" +  extra_params + ", \"smooth\",500]");
  }

  if (action == "loop_temperature")
  {
    action_name = "set_ct_abx";
    params = String("[" +  extra_params + ", \"smooth\",500]");
  }

  if (action == "loop_brightness") {
    String brithness = "100";
    if (extra_params != NULL)
    {
      brithness = extra_params;
    }
    
    action_name = "set_bright";
    params = "[" + brithness + ", \"smooth\", 500]";
  }

  return sendCommand(action_name, params, ipLight);
}

String Yeelight::start_flow(String flow, String extra_params, char ipLight[15])
{
  String params = "";
  if (flow == "slowdown")
  {
    params = "[0, 0, \"2000, 1, 58367, 100, 4000, 1, 16711774, 100, 6000, 1, 25599, 100, 8000, 1, 16711825, 100, 10000, 1, 65333, 100, 12000, 1, 65486, 100, 14000, 1, 3342080, 100, 16000, 1, 16711710, 100\"]";
  }

  if (flow == "temp")
  {
    params = "[0, 0, \"40000, 2, 1700, 100, 40000, 2, 6500, 100\"]";
  }

  if (flow == "lsd")
  {
    params = "[0, 0, \"3000, 1, 16724262, 100, 3000, 1, 16737817, 100, 3000, 1, 16771853, 100, 3000, 1, 12124032, 100, 3000, 1, 570367, 100\"]";
  }

  if (flow == "police")
  {
    params = "[0, 0, \"300, 1, 16711680, 100, 300, 1, 255, 100\"]";
  }

  if (flow == "strobe")
  {
    params = "[0, 0, \"50, 1, 16777215, 100, 50, 1, 16777215, 1\"]";
  }

  if (flow == "strobe_color")
  {
    params = "[0, 0, \"50, 1, 196863, 100, 50, 1, 16711424, 100, 50, 1, 16711804, 100, 50, 1, 16711680, 100, 50, 1, 65507, 100, 50, 1, 16744448, 100\"]";
  }

  if (flow == "disco")
  {
    params = "[0, 0, \"500, 1, 16711680, 100, 500, 1, 16711680, 1, 500, 1, 8322816, 100, 500, 1, 8322816, 1, 500, 1, 65023, 100, 500, 1, 65023, 1, 500, 1, 8585471, 100, 500, 1, 8585471, 1\"]";
  }

  if (flow == "police_2")
  {
    params = "[0, 0, \"250, 1, 16711680, 100, 250, 1, 16711680, 1, 250, 1, 16711680, 100, 250, 7, 1, 2, 250, 1, 255, 100, 250, 1, 255, 1, 250, 1, 255, 100, 250, 7, 1, 2\"]";
  }

  if (flow == "crhistmas")
  {
    params = "[0, 0, \"250, 1, 16711680, 100, 3000, 7, 1, 2, 250, 1, 65281, 100, 3000, 7, 1, 2\"]";
  }

  if (flow == "rgb")
  {
    params = "[0, 0, \"250, 1, 16711680, 100, 3000, 7, 1, 2, 250, 1, 65281, 100, 3000, 7, 1, 2, 250, 1, 196863, 100, 3000, 7, 1, 2\"]";
  }

  if (flow == "loop")
  {
    String brithness = "100";
    if (extra_params != NULL)
    {
      brithness = extra_params;
    }
    String duration = "5000";
    params = String("[0, 0, \"" + duration + ", 1, 65507," + brithness + ", " + duration + ", 1, 65452, " + brithness + ", " + duration + ", 1, 13041919, " + brithness + ", " + duration + ", 1, 786176, " + brithness + ", " + duration + ", 1, 16734464, " + brithness + ", " + duration + ", 1, 16187136, " + brithness + ", " + duration + ", 1, 65520, " + brithness + ", " + duration + ", 1, 13566207, " + brithness + "\"]");
  }

  if (flow == "slowdown")
  {
    params = "[0, 0, \"750, 1, 65486, 100, 750, 1, 6357247, 100, 750, 1, 5570304, 100, 750, 1, 9961727, 100, 750, 1, 6143, 100, 750, 1, 1638144, 100, 750, 1, 16711902, 100, 750, 1, 16711684, 100, 750, 1, 16765184, 100\"]";
  }

  if (params == "")
  {
    return "";
  }

  return sendCommand("start_cf", params, ipLight);
}

String Yeelight::start_command(String action, int action_type, String extra_params, char ipLight[15])
{
  String res = "";
  if (action_type == 0)
  {
    res = start_action(action, extra_params, ipLight);
  }

  if (action_type == 1)
  {
    res = start_flow(action, extra_params, ipLight);
  }
  return res;
}
