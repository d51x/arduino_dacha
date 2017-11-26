#include "dsw.h"
#include "const.h"
#include <EEPROM.h>
#include "types.h"


DSW_Temp::DSW_Temp() :	value(-12700),	index(255)
{
	 memset(address, 0, sizeof(DeviceAddress)) ;
	 enabled = DISABLE;
	 
	 this->_mqtt = NULL;
	 this->_device = NULL;
}

void DSW_Temp::setMqttClient(PubSubClient *mqtt, const char *device){
    this->_mqtt = mqtt;
	this->_device = device;
}

void DSW_Temp::publish() {
	
	//mqtt:	<device>/dsw/<address>    value  (xx.yy)
	
	if (enabled == DISABLE ) return;
  if ( index > (MAX_DSW_SENSORS - 1) ) return;
  if ( value <= -12700 || value >= 8500 ) return;
  char *temp = (char*)malloc(7);
  if ( temp == NULL) return;
  sprintf_P(temp, PSTR("%d.%02d"), (int)value / 100, (int)(value) % 100);
  
  byte i = strlen(_device);
  byte l = strlen(P_DSW);
  char* buffer = (char*)malloc( i + l+1+2+16 );
  if ( buffer == NULL) return;
  
  strcpy(buffer, _device);     // mqtt topic:  <device>
  strcat_P(buffer, (char*)P_DSW);  // mqtt topic:  <device>/dsw/
  
	// в mqtt topic добавляется index
	//sprintf_P(buffer, PSTR("%s%d"), buffer, index+1);  // mqtt topic:  <device>/dsw/

	// в mqtt topic добавляется address 23 байта  28:FF:81:E9:74:16:03:41
	char *tmpAddr = (char*)malloc(16);
  if ( tmpAddr == NULL) return;
	//sprintf_P(temp, PSTR("%s:%s:%s:%s:%s:%s:%s:%s"), (int)value / 100, (int)(value) % 100);
	
  for (uint8_t k = 0; k < 8; k++)
  {
    char tmp[3] = "00";
		if (address[k] < 0x10) { String(address[k],HEX).toCharArray(tmp+1, 3); }
		else 
			String(address[k],HEX).toCharArray(tmp, 3);
    strcat(tmpAddr, tmp); //(deviceAddress[k], HEX);
  }	
  strcat(buffer, tmpAddr);  // mqtt topic:  <device>/dsw/28FF81E974160341
	
	
  _mqtt->publish( buffer, temp);
  
  #ifdef DEBUG1 
	WRITE_TO_SERIAL(F("MQTT topic: "), buffer, F(" value: "), temp);	
  #endif 	

  free(temp);
  free(buffer);
  free(tmpAddr);
}


		
		
void DSW_Temp::setEnabled(Enabled st) {
	enabled = st;
}

