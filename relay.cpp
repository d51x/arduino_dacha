#include "relay.h"
#include <EEPROM.h>

Relay::Relay() : index(255)
{
	info.pin = 255;
	info.enabled = DISABLE; 
	info.signalType = NORMAL;
	info.flash = false;
	info.state = OFF;  
	this->_mqtt = NULL;
	this->_device = NULL;
	//setMqttClient(mqtt);
}

void Relay::setMqttClient(PubSubClient* mqtt, const char *device){
    this->_mqtt = mqtt;
    this->_device = device;
}

void Relay::load_eeprom() {
    int addr = EEPROM_ADDRESS_RELAY_START + index*RELAY_DATA_SIZE;
	EEPROM.get(addr, info );
 //WRITE_TO_SERIAL(F("load_eeprom "), F(" index "), index, F(" index "));
 //WRITE_TO_SERIAL(F("EEPROM.get pin "), info.pin, F(" info.flash "), info.flash);
 //WRITE_TO_SERIAL(F("EEPROM.get enabled "), info.enabled, F(" info.signalType "), info.signalType);
 //WRITE_TO_SERIAL(F("EEPROM.get state "), info.state, F("  "), F("  "));
 
	info.signalType = (info.signalType != NORMAL ) ? INVERT : NORMAL;
	info.enabled = (info.enabled != ENABLE ) ? DISABLE : ENABLE;
	//info.flash = ( info.flash  ) ? true : false;
	info.state = ( info.state != ON  ) ? OFF : ON;
}

void Relay::save_eeprom() {
    RelayInfo tmp;
	int addr = EEPROM_ADDRESS_RELAY_START + index*RELAY_DATA_SIZE;
	EEPROM.get(addr, tmp ); 
	if ( memcpy(&tmp, &info, sizeof(RelayInfo)) != 0 ) {
		//WRITE_TO_SERIAL(F("save_eeprom  OK "), F("  "), F("  "), F("  "));
		EEPROM.put(addr, info ); 
	} else {
		//WRITE_TO_SERIAL(F("save_eeprom  FAIL "), F("  "), F("  "), F("  "));
		
	}
}





void Relay::save_pin_eeprom() {
  byte tmp;
	int addr = EEPROM_ADDRESS_RELAY_START + index*RELAY_DATA_SIZE + 0;
	EEPROM.get(addr, tmp );
	if ( tmp != info.pin ) EEPROM.put(addr, info.pin);		
}

void Relay::save_enabled_eeprom() {
  Enabled tmp;
	int addr = EEPROM_ADDRESS_RELAY_START + index*RELAY_DATA_SIZE + 1;
	EEPROM.get(addr, tmp );
	if ( tmp != info.enabled ) EEPROM.put(addr, info.enabled);	
}

void Relay::save_signal_type_eeprom() {
  SignalType tmp;
	int addr = EEPROM_ADDRESS_RELAY_START + index*RELAY_DATA_SIZE + 3;
	EEPROM.get(addr, tmp );
	//WRITE_TO_SERIAL(F("EEPROM.get "), tmp, F(" info.signalType "), info.signalType);	
	if ( tmp != info.signalType ) EEPROM.put(addr, info.signalType);
//EEPROM.get(addr, tmp );
	 //WRITE_TO_SERIAL(F("EEPROM.get "), tmp, F(" info.signalType "), info.signalType);		
}


void Relay::save_flash_eeprom() {
  byte tmp;
	int addr = EEPROM_ADDRESS_RELAY_START + index*RELAY_DATA_SIZE + 5;
	EEPROM.get(addr, tmp );
	 //WRITE_TO_SERIAL(F("EEPROM.get "), tmp, F(" info.flash "), info.flash);	
	 
	if ( tmp != info.flash ) EEPROM.put(addr, info.flash);

//EEPROM.get(addr, tmp );
	 //WRITE_TO_SERIAL(F("EEPROM.get "), tmp, F(" info.flash "), info.flash);	
}

void Relay::save_state_eeprom() {
  State tmp;
	int addr = EEPROM_ADDRESS_RELAY_START + index*RELAY_DATA_SIZE + 6;
	EEPROM.get(addr, tmp );
	if ( tmp != info.state ) EEPROM.put(addr, info.state);	
}

		
		
		
		
boolean Relay::begin() {
	if ( info.pin > 60 ) return false;	
	if ( info.enabled == DISABLE) return false;
	pinMode( info.pin, OUTPUT);

	if ( info.flash  ) {
		digitalWrite( info.pin, (info.signalType == INVERT) ? ( (info.state == ON) ? LOW : HIGH ) : ( (info.state == ON) ? HIGH : LOW ));
		save_state_eeprom();
		return true;
	} else {
		digitalWrite(info.pin, (info.signalType == INVERT) ? HIGH : LOW);	
		return false;		
	}
	publish();
	
}

void Relay::publish() {
	//(const char* topic, int value) {
  if (index == 255) return;
  byte i = strlen(_device);
  //byte l = strlen(P_GPIO_OUT_STATE);
  byte l = strlen(P_RELAY_STATE);
  char* buffer = (char*)malloc( i+l+1+2 );
  if ( buffer == NULL) return;
  WRITE_TO_SERIAL(F("-------- "), _device, F(""), F(""));	
  strcpy(buffer, _device);
  //strcat_P(buffer, (char*)P_GPIO_OUT_STATE);
  strcat_P(buffer, (char*)P_RELAY_STATE);
  sprintf_P(buffer, PSTR("%s%d"), buffer, index+1);

  char* s = (char*)malloc( 4 );
  strcpy_P(s, (info.state == ON) ? CONST_ON : CONST_OFF);
   WRITE_TO_SERIAL(F("relay_publish: "), (char*)buffer, F(" val "), (char*)s);
  _mqtt->publish( buffer, s);
  //mqtt_client->publish( buffer, status);
  
//#ifdef DEBUG  
  WRITE_TO_SERIAL(F("Relay::publish MQTT topic: "), buffer, F(" value: "), s);	
  Serial.println();
//#endif 
  
  free(buffer);
  free(s);
  
}

void Relay::turnOFF() {
	info.state = OFF;
	digitalWrite(info.pin, (info.signalType == INVERT) ? HIGH : LOW);
	if (info.flash ) { save_state_eeprom(); }
	publish();
}

void Relay::turnON() {
	info.state = ON;
	digitalWrite(info.pin, (info.signalType == INVERT) ? LOW : HIGH);
	if (info.flash ) { save_state_eeprom(); }
	publish();
}

void Relay::switch_relay(byte* payload) {
	  char* cstring = (char*)payload;
	  ( strcmp_P( cstring, CONST_ON) == 0 ) ? turnON() : turnOFF();
	  // (payload[0] == 1 ) ? turnON() : turnOFF();
}

void Relay::switch_relay(State state) {
	  ( state == ON ) ? turnON() : turnOFF();
}

void Relay::setEnabled(byte* payload) {
	char* cstring = (char*)payload;	
	( strcmp_P( cstring, CONST_ENABLE) == 0 ) ? info.enabled = ENABLE : info.enabled = DISABLE;
	//WRITE_TO_SERIAL(F("Relay::setEnabled: "), info.enabled , F(" "),  F(" "));
	save_enabled_eeprom();
	//load_eeprom();
}

void Relay::setEnabled(Enabled enable) {
	//( enable == CONST_ENABLE) ? info.enabled = ENABLE : info.enabled = DISABLE;
	info.enabled = enable;
	save_enabled_eeprom();
}

void Relay::setPin(byte* payload) {
	info.pin = payload;
	pinMode( info.pin, OUTPUT);
	save_pin_eeprom();
	//WRITE_TO_SERIAL(F("Relay::setPin: "), info.pin , F(" "),  F(" "));	
	//load_eeprom();
}

void Relay::setSignalType(byte* payload) {
	char* cstring = (char*)payload;	
	( strcmp_P( cstring, CONST_HIGH) == 0 ) ? info.signalType = INVERT : info.signalType = NORMAL;
	//WRITE_TO_SERIAL(F("Relay::setSignalType: "), info.signalType , F(" "),  F(" "));	
	save_signal_type_eeprom();
	//load_eeprom();
}

void Relay::setSignalType(SignalType signal) {
	//( signal == CONST_HIGH) ? info.signalType = INVERT : info.signalType = NORMAL;
	info.signalType = signal;
	save_signal_type_eeprom();
}

void Relay::setFlash(byte* payload) {
	
 info.flash = payload;
 //WRITE_TO_SERIAL(F("Relay::setFlash: "), info.flash , F(" "),  F(" "));	
  save_flash_eeprom(); 
  //load_eeprom();
}
		
		