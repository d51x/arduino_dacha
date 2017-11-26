#ifndef CONST_H
#define CONST_H
#include <avr/pgmspace.h>
#include <Arduino.h>

#define DEBUG
#define RESET_EEPROM
#define TEST

#define MEGA

#define CONST_ON (const char *)F("ON")
#define CONST_OFF (const char *)F("OFF")

#define CONST_LOW (const char *)F("LOW")
#define CONST_HIGH (const char *)F("HIGH")

#define CONST_ENABLE (const char *)F("ENABLE")
#define CONST_DISABLE (const char *)F("DISABLE")

#define ONE_WIRE_BUS 8
#define I2C_SDA_PIN	2
#define I2C_SCL_PIN	0

#define FIRST_START_TIMEOUT 2//*1000
#define READ_TEMP_INTERVAL 30//*1000 //30sec


#ifdef MEGA
	#define MAX_RELAY 16
	#define MAX_DSW_SENSORS 16
#else
	#define MAX_RELAY 5
	#define MAX_DSW_SENSORS 10	
#endif

#define EEPROM_ADDRESS_GLOBAL_OPTIONS_START 0 

#define RELAY_DATA_SIZE 8 //5
#define EEPROM_ADDRESS_RELAY_START 	50

#define I2C_DEVICE_DATA_SIZE 3
#define EEPROM_ADDRESS_I2C_DEVICE_START 	210


#ifdef DEBUG
  #define WRITE_TO_SERIAL(val1,val2, val3, val4) { Serial.print(val1); Serial.print(val2); Serial.print(val3); Serial.println(val4); }
  #define WRITE_TO_SERIAL_BYTE_ARR(val0, val1, val2) { Serial.print(val0); for (byte i=0;i<sizeof(val1);i++) {Serial.print(val1[i], val2); Serial.print(" ");} Serial.println();}
  #define WRITE_TO_SERIAL_CHAR_ARR(val0, val1) { Serial.print(val0); for (byte i=0;i<sizeof(val1);i++) {Serial.write( val1[i]);} Serial.println();}
#endif


	const char P_DEVICE_NAME[] PROGMEM         = "arduino";


const char P_DSW[] PROGMEM         = "/dsw/";
const char P_RELAY[] PROGMEM         = "/relay/";

const char P_SET[] PROGMEM         = "/set";

const char P_GPIO_OUT_CMD[] PROGMEM         = "/output/gpio/cmd/";
const char P_GPIO_OUT_STATE[] PROGMEM         = "/output/gpio/state/";

const char P_DSW_SENSORS_LIST[] PROGMEM         = "/dsw/list";
const char P_DSW_REFRESH_SET[] PROGMEM         = "/options/dsw/refresh/set";
const char P_DSW_PIN_SET[] PROGMEM         = "/options/dsw/pin/set";

const char P_RELAY_ENABLED_SET[] PROGMEM         = "/relay/enabled/set/";
const char P_RELAY_PIN_SET[] PROGMEM         = "/relay/pin/set/";
const char P_RELAY_STATE_SET[] PROGMEM         = "/relay/state/set/";
const char P_RELAY_STATE[] PROGMEM         = "/relay/state/";
const char P_RELAY_TYPE_SET[] PROGMEM         = "/relay/type/set/";
const char P_RELAY_FLASH_SET[] PROGMEM         = "/relay/flash/set/";
const char P_RELAY_DELAY_SET[] PROGMEM         = "/options/relay/delay-time/set";

/*
const char P_THERM_STATE_CMD[] PROGMEM         = "/therm/state/cmd/";
const char P_THERM_STATUS_CMD[] PROGMEM         = "/therm/status/cmd/";
const char P_THERM_SET_TEMP_CMD[] PROGMEM         = "/therm/set/cmd/";
const char P_THERM_MODE_CMD[] PROGMEM         = "/therm/mode/cmd/";
*/

const char P_TOPIC_ARDUINO_RESTART[] PROGMEM           = "/restart";
const char P_TOPIC_ARDUINO_RESET[] PROGMEM           = "/reset";
const char P_TOPIC_ARDUINO_UPTIME[] PROGMEM           = "/uptime";
const char P_TOPIC_ARDUINO_MEMORY_FREE[] PROGMEM           = "/memoryfree";

#endif //CONST_H