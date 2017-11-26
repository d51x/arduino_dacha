#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include "const.h"

enum State {OFF, ON};
enum Enabled {DISABLE, ENABLE};
enum SignalType {NORMAL, INVERT};

typedef uint8_t DeviceAddress[8];
typedef char DeviceName[10];

struct GlobalOptions {	
  byte pin_onewire;
  byte firstStartTimeout;
  byte temperature_refresh;
  byte mqtt_ip[4];
	byte pin_i2c_sda;
	byte pin_i2c_scl;
};


#endif //TYPES_H