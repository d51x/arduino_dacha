#ifndef DSW_H
#define DSW_H

#include <Arduino.h>
#include "types.h"
#include <PubSubClient.h>


class DSW_Temp {
	private:
		PubSubClient *_mqtt;
		char *_device;
	public:
		int value; 
		Enabled enabled;	
	  DeviceAddress address;
		byte index;
		
		DSW_Temp();
		//~DSW_Temp(){};
		void setMqttClient(PubSubClient *client, const char *device);
		
		void publish();
		void setEnabled(Enabled enabled);


};



#endif //DSW_H
