#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>
#include "types.h"
#include <PubSubClient.h>
#include "const.h"

struct RelayInfo {
	byte pin;
	Enabled enabled; //   Состояние - разрешено/запрещено	
	SignalType signalType;  //    Тип сигнала - прямой или инвертный
	bool flash; //   Хранить во флеш
	State state; //   Статус - вкл/выкл
};


class Relay {
	private:
		PubSubClient *_mqtt;
		char *_device;
	public:
		byte index;
		RelayInfo info;

		
		
		Relay();
		//~Relay(){};
		
		boolean begin();
		void setMqttClient(PubSubClient *client, const char *device);
		
		void load_eeprom();
		void save_eeprom();
		
		void save_state_eeprom();
		void save_pin_eeprom();
		void save_enabled_eeprom();
		void save_flash_eeprom();
		void save_signal_type_eeprom();
		
		
		void setEnabled(byte* payload);
		void setEnabled(Enabled enabled);
		void setPin(byte* payload);
		void setSignalType(byte* payload);
		void setSignalType(SignalType signal);
		void setFlash(byte* payload);
		
		void publish();
		
		//void publish_enabled();
		//void publish_pin();
		//void publish_state();
		//void publish_type();
		//void publish_flash();
		
		void turnOFF();
		void turnON();
		
		void switch_relay(byte* payload);
		void switch_relay(State state);

};
#endif //RELAY_H
