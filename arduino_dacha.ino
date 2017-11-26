#include <EEPROM.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/pgmspace.h>
#include "types.h"
#include "const.h"
#include "dsw.h"
#include "relay.h"
#include "http.h"

extern int __bss_end;
extern void *__brkval;

#define RESET_EEPROM1

 DeviceName device_name;

Relay relay[MAX_RELAY];
DSW_Temp dsw_temp[MAX_DSW_SENSORS];


OneWire  ds;
DallasTemperature temp_sensors;

DeviceAddress Termometers;      
byte dsw_count;
 long refresh;
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte eth_ip[] = {192, 168, 1, 177};
byte mqtt[] = {192, 168, 1, 45};

IPAddress ip(eth_ip);
IPAddress mqtt_server(mqtt);

byte isRestarted = 1;

long lastReadingTime = 0;
long lastReadingTime2 = 0;
long lastReadingTime3 = 0;
long lastReadingTime4 = 0;

#ifdef MEGA
EthernetServer server(80);
#endif

EthernetClient ethClient;
PubSubClient mqtt_client(ethClient);

GlobalOptions options;

void(* resetFunc) (void) = 0;

int memoryFree()
{
   int freeValue;
   if((int)__brkval == 0)
      freeValue = ((int)&freeValue) - ((int)&__bss_end);
   else
      freeValue = ((int)&freeValue) - ((int)__brkval);
   return freeValue;
}

void load_options() {
#ifdef RESET_EEPROM1
	#ifdef MEGA
		//for (word ee=0;ee<4096;ee++) EEPROM.write(ee, 0);
	#else
		//for (word ee=0;ee<1024;ee++) EEPROM.write(ee, 0);
	#endif
#endif
	
#ifdef DEBUG1
	WRITE_TO_SERIAL(F("-------- "), F("function load_options"), F(" start"), F(""));	
#endif

	EEPROM.get(EEPROM_ADDRESS_GLOBAL_OPTIONS_START, options);	
	
	WRITE_TO_SERIAL(F("-------- "), F("options.temperature_refresh"), options.temperature_refresh, F(""));	
	
	if ( (options.pin_onewire < 2) || (options.pin_onewire > 54)) options.pin_onewire = ONE_WIRE_BUS;
	if ( (options.firstStartTimeout == 255) || (options.firstStartTimeout == 0) ) options.firstStartTimeout = FIRST_START_TIMEOUT;
	if ( (options.temperature_refresh == 255) || (options.temperature_refresh == 0) ) options.temperature_refresh = READ_TEMP_INTERVAL;
	byte t0[6] = {0, 0, 0, 0, 0, 0};
	byte t1[6] = {255, 255, 255, 255, 255, 255};
	if ( (memcpy(options.mqtt_ip, t0, 4) == 0 ) || 
	     (memcpy(options.mqtt_ip, t1, 4) == 0 ) 
			)	memcpy(options.mqtt_ip, mqtt, sizeof(mqtt));
	
	if ( (options.pin_i2c_sda < 0) || (options.pin_i2c_sda > 54) ) options.pin_i2c_sda = I2C_SDA_PIN;
	if ( (options.pin_i2c_scl < 0) || (options.pin_i2c_scl > 54) ) options.pin_i2c_scl = I2C_SCL_PIN;
/* options.firstStartTimeout = FIRST_START_TIMEOUT;
options.temperature_refresh = 10;
options.pin_onewire = ONE_WIRE_BUS; */
refresh = options.temperature_refresh;
}


void search_dsw_temp() {
	dsw_clear();
		DeviceAddress tmpTerm;
		ds.reset_search();
		uint8_t is_dsw_found = true;
		uint8_t device_idx = 0;
		while ( is_dsw_found ) {
			is_dsw_found = is_dsw_found && ds.search(tmpTerm);
			if ( is_dsw_found ) {
					dsw_temp[device_idx].index = device_idx;
					dsw_temp[device_idx].enabled = ENABLE;
					memcpy(dsw_temp[device_idx].address, tmpTerm, sizeof(DeviceAddress));
					device_idx++;
			}
		}		
}

void dsw_clear() {
	for (byte i=0;i<MAX_DSW_SENSORS;i++) { 
		dsw_temp[i].enabled = DISABLE;
		dsw_temp[i].setMqttClient(&mqtt_client, device_name);
		dsw_temp[i].value = 300;
		memset(dsw_temp[i].address, 0, sizeof(DeviceAddress)) ;
	}	
}

void init_temperature_sensors() {
#ifdef DEBUG1
	WRITE_TO_SERIAL(F("-------- "), F("function init_temperature_sensors"), F(" start"), F(""));	
#endif 	
	
	dsw_clear();

   ds.begin(options.pin_onewire);
   temp_sensors = DallasTemperature(&ds);
   temp_sensors.begin();
   delay(50);
	 temp_sensors.requestTemperatures();
	 temp_sensors.setResolution(Termometers, 12);  
	 dsw_count = temp_sensors.getDeviceCount(); 
	 WRITE_TO_SERIAL(F("-------- "), F("DSW count:   "), dsw_count, F(""));
	 search_dsw_temp(); 
 
}



void init_relays() {
  for (byte i=0;i<MAX_RELAY;i++) {
		relay[i].index = i;    
		relay[i].load_eeprom();
		relay[i].setMqttClient(&mqtt_client, device_name);
		if (relay[i].begin())	delay(options.firstStartTimeout * 1000); 	
  }

	#ifdef DEBUG1
		for (byte i=0;i<MAX_RELAY;i++) {
			WRITE_TO_SERIAL(F("Relay: "), relay[i].index+1, F(" pin "), relay[i].info.pin);  
			WRITE_TO_SERIAL(F("enabled: "), relay[i].info.enabled, F(" state "), relay[i].info.state);  
			WRITE_TO_SERIAL(F("flash: "), relay[i].info.flash, F(" signal "), relay[i].info.signalType);  
		}
	#endif
  
}


void publish_all_data() {
	//relay states
	
  for (byte i=0;i<MAX_RELAY;i++) {
	if (relay[i].info.enabled == ENABLE)
		relay[i].publish();
  }
 
}

void callback(char* topic, byte* payload, unsigned int length) {
  byte* payload2 = NULL;
  payload[length] = '\0';
  payload2 = (byte*)malloc(length+1);
  if ( NULL == payload2 ) return;
  memcpy(payload2, payload, length+1);
  
#ifdef DEBUG1 
  WRITE_TO_SERIAL(F("callback: "), topic, F(" value "), (char *)payload);	
#endif 
  
  /* topic  -  <device_name>/mqtt_path/idx */
  char dev[11] = "";
  char path[40] = "";
  char path2[40] = "";
  char idx[20] = "";
  byte i = 255;
  
  // index брать с конца
  
  char *tmp = strchr(topic, '/');
  char pos;
  if ( tmp == NULL ) return;
  pos = tmp - topic + 1;
  strncpy(dev, topic, pos-1);
  //WRITE_TO_SERIAL(F("topic: "), topic, F(" |   dev: "), dev);	
  strcpy(path2, tmp);
  char *tmp2 = strrchr(tmp, '/');
  
  //WRITE_TO_SERIAL(F("tmp: "), tmp, F(" |   tmp2: "), tmp2);
  
  if (strcmp(tmp, tmp2) == 0 ) {
    // printf("idx is absent\n");
    strcpy(path, tmp2);
    
  } else {
    pos = tmp2 - tmp + 1;
    strncpy(path, tmp, pos);
    strncpy(idx, &tmp2[1], strlen(&tmp2[1]));
		i = atoi(idx);
  }

	 //WRITE_TO_SERIAL(F("topic: "), topic, F(" |   path: "), path);	 
		if ( strcmp_P(path2, P_DSW_SENSORS_LIST ) == 0 ) {
			#ifdef DEBUG  
				WRITE_TO_SERIAL(F("callback: "), path2, F(" idx "), i);	
				WRITE_TO_SERIAL(F("sensors list: "), i, F(" to "), (char*)payload2);	
			#endif 			

		}
		else if ( strcmp_P(path2, P_DSW_REFRESH_SET ) == 0 ) {
			WRITE_TO_SERIAL(F("P_DSW_REFRESH_SET = "), (char*)payload2 , F(" "), F(" "));	
			options.temperature_refresh = atoi(payload2);
			EEPROM.put(2, options.temperature_refresh);
			WRITE_TO_SERIAL(F("options.temperature_refresh = "), options.temperature_refresh , F(" "), F(" "));	
			refresh = options.temperature_refresh;
		}
		else if ( strcmp_P(path2, P_DSW_PIN_SET ) == 0 ) {
			options.pin_onewire = atoi(payload2);
			EEPROM.put(0, options.pin_onewire);			
		}
		else if ( strcmp_P(path2, P_RELAY_DELAY_SET ) == 0 ) {
			options.firstStartTimeout = atoi(payload2);
			EEPROM.put(1, options.firstStartTimeout);	
		}			
		else if ( strcmp_P(path, P_RELAY_ENABLED_SET ) == 0 ) {
			//#ifdef DEBUG  
			//	WRITE_TO_SERIAL(F("callback: "), path, F(" idx "), i);	
			//	WRITE_TO_SERIAL(F("relay enabled set: "), i, F(" to "), (char*)payload2);	
			//#endif 	
			if ( i < MAX_RELAY+1 ) relay[i-1].setEnabled(payload2);			
		}		
		else if ( strcmp_P(path, P_RELAY_PIN_SET ) == 0 ) {
			//#ifdef DEBUG  
			//	WRITE_TO_SERIAL(F("callback: "), path, F(" idx "), i);	
			//	WRITE_TO_SERIAL(F("relay pin set: "), i, F(" to "), atoi(payload2));	
			//#endif 				
			if ( i < MAX_RELAY+1 ) relay[i-1].setPin(atoi(payload2));					
		}		
		else if ( strcmp_P(path, P_RELAY_STATE_SET ) == 0 ) {
				//#ifdef DEBUG  
				//	WRITE_TO_SERIAL(F("callback: "), path, F(" idx "), i);	
				//	WRITE_TO_SERIAL(F("relay state set: "), i, F(" to "), (char*)payload2);	
				//#endif 				
				
				if ( i < MAX_RELAY+1 ) relay[i-1].switch_relay(payload2);
		}		
		else if ( strcmp_P(path, P_RELAY_TYPE_SET ) == 0 ) {
			//#ifdef DEBUG  
			//	WRITE_TO_SERIAL(F("callback: "), path, F(" idx "), i);	
			//	WRITE_TO_SERIAL(F("relay type set: "), i, F(" to "), (char*)payload2);	
			//#endif 		
			if ( i < MAX_RELAY+1 ) relay[i-1].setSignalType(payload2);					
		}		
		else if ( strcmp_P(path, P_RELAY_FLASH_SET ) == 0 ) {
			//#ifdef DEBUG  
			//	WRITE_TO_SERIAL(F("callback: "), path, F(" idx "), i);	
			//	WRITE_TO_SERIAL(F("relay flash set: "), i, F(" to "), atoi(payload2));	
			//#endif 	
			if ( i < MAX_RELAY+1 ) relay[i-1].setFlash(atoi(payload2));				
		}	
		
    else if ( strcmp_P(path, P_GPIO_OUT_CMD ) == 0) { 
			//#ifdef DEBUG1  
			//	WRITE_TO_SERIAL(F("callback: "), path, F(" idx "), i);	
			//	WRITE_TO_SERIAL(F("swtch relay: "), i, F(" to "), (char*)payload2);	
			//#endif 		
      if ( i < MAX_RELAY+1 ) relay[i-1].switch_relay(payload2);
    } 
	else if (strcmp_P(path, P_TOPIC_ARDUINO_RESET) == 0) {
		resetFunc();
	}
  free(payload2);  
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt_client.connect( device_name )) {
      //Serial.println("connected");
      // Once connected, publish an announcement...
      //mqtt_client.publish("outTopic","hello world");
      // ... and resubscribe

      //relay_state_publish( TOPIC_RELAY_F1_LIVING_STATE, Relay01_state);
	  publish_all_data();
	  char *s =(char*)malloc( strlen(device_name + 3) );
	  strcpy(s, device_name);
	  strcat(s, "/#");
		#ifdef DEBUG1  
		  WRITE_TO_SERIAL(F("reconnect mqtt: "), s, F(""), F(""));	
		#endif 		  
      mqtt_client.subscribe( s );
	  //mqtt_client.subscribe( "uno/#" );
      free(s);
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  } 
}

void setup() {
	//for (int t=0;t<1024;t++) EEPROM.write(t, 255);
	strcpy_P(device_name, P_DEVICE_NAME);
#ifdef DEBUG
	Serial.begin(9600);
	while (!Serial) {
		//  ; // wait for serial port to connect. Needed for native USB port only
	}  
	WRITE_TO_SERIAL(F("----------- "), F(" setup() "), F(" ------------ "), F(""));
#endif 
 
  delay(100);
  load_options();

#ifdef DEBUG
	WRITE_TO_SERIAL(F("--------- "), F(" loaded options "), F(" "), F(""));
	WRITE_TO_SERIAL(F("OneWire Pin: "), options.pin_onewire, F(" "), F(""));
	WRITE_TO_SERIAL(F("firstStartTimeout: "), options.firstStartTimeout, F(" "), F(""));	
	WRITE_TO_SERIAL(F("temperature_refresh: "), options.temperature_refresh, F(" "), F(""));
	WRITE_TO_SERIAL_BYTE_ARR(F("mqtt_ip: "), options.mqtt_ip, DEC);
	WRITE_TO_SERIAL(F("-------- "), F("function load_options"), F(" end"), F(""));	
#endif


  
  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setCallback(callback);
  
  Ethernet.begin(mac, ip);
  #ifdef MEGA
  server.begin();
  #endif


  
   // initialize dsw sensors
  init_temperature_sensors();
  delay(50);
  
  // initialize relays
  init_relays();
  delay(50);
  
  if (mqtt_client.connected()) {
    // publish
	publish_all_data();   /// ???? сейчас все закомментировано, надо ли это ????
  }
}

void temperature_update() {
#ifdef DEBUG1  
  WRITE_TO_SERIAL(F("-------- "), F("function temperature_update"), F(" start"), F(""));	
#endif 	
  temp_sensors.requestTemperatures();
  
	//	byte dsw_cnt;
	 //dsw_cnt = temp_sensors.getDeviceCount(); 
	 //WRITE_TO_SERIAL(F("-------- "), F("DSW count:   "), dsw_cnt, F(""));
	 //search_dsw_temp(); 
	 
	 
  for (byte i=0;i<MAX_DSW_SENSORS;i++) {
    //if (dsw_temp[i].index != 255) {
    if (dsw_temp[i].enabled == ENABLE) {
		//WRITE_TO_SERIAL(F("Sensor "), i, F(" state "), F("ENABLE "));	
      dsw_temp[i].value = (int)( (float)temp_sensors.getTempC(dsw_temp[i].address) * 100);
 		#ifdef DEBUG  
			char tmp[8] = "";
			Serial.print("Sensor ");	
			for (uint8_t j = 0; j < 8; j++)
			{
				Serial.print("0x");
				if (dsw_temp[i].address[j] < 0x10) Serial.print("0");
				Serial.print(dsw_temp[i].address[j], HEX);
				if (j < 7) Serial.print(", ");
			}
			WRITE_TO_SERIAL(F(""), F(""), F(" temp: "), dsw_temp[i].value);	
			
		#endif 	 	  
			dsw_temp[i].publish();
    }  
  }
	WRITE_TO_SERIAL(F("----------------------------------------- "), F(""), F(""), F(""));	
#ifdef DEBUG1  
  WRITE_TO_SERIAL(F("-------- "), F("function temperature_update"), F(" end"), F(""));	
#endif 	  
}


void uptime(const char* topic, long val) {
  byte l = strlen(topic);
  byte i = strlen(device_name);
  char* buffer = (char*)malloc( i+l+1 );
  char* sval =  (char*)malloc( 10 );
  if ( buffer == NULL) return;
  if ( sval == NULL) { free(buffer); return; }
  ltoa(val, sval, 10);
  strcpy(buffer, device_name);  
  strcat_P(buffer, (char*)topic);   
  mqtt_client.publish( buffer, sval);
  free(buffer);
  free(sval);
}


void printAddress(DeviceAddress deviceAddress)
{
  Serial.print("Address: ");
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
  Serial.print(" ");
} 


void listenForEthernetClients() {
	// listen for incoming clients
	EthernetClient client = server.available();
	if (!client) return;
    // an http request ends with a blank line
    String request;
    boolean currentLineIsBlank = true;
    boolean requestLineReceived = false;
	
	while (client.connected()) {
		if (client.available()) {	
 			char c = client.read(); // read 1 byte (character) from client
			//HTTP_request += String(c);  // save the HTTP request 1 char at a time
			// TODO: HTTP_request += String(c); // !!!!!!!!!!!!!!!
			// last line of client request is blank and ends with \n
			// respond to client only after last line received
			if (c == '\n' && currentLineIsBlank) {
				request = getUrlFromHeader(&request);
				//WRITE_TO_SERIAL(F("Request: "), request, F(""), F(""));	
				
			  if(request=="/") {
				generate_main_page(client);
			  } else {
				  
					String page = request.substring(0, request.length() );
						//Serial.print( "page: "); Serial.print(page); Serial.println();
						if ( ispage(&page) )  {
							//Serial.print( "page: "); Serial.print(page); Serial.println();
							//Serial.print( "len: "); Serial.print(page.length()); Serial.println();
						} else {
						
						if ( page.startsWith("ajax_") ) {
							// ajax request here
							
						} 
						else {
							String slug_1 = getNextSlug(&request);	
							String slug_2 = getNextSlug(&request);
							String slug_3 = getNextSlug(&request);
							String slug_4 = getNextSlug(&request);					
							//Serial.print( "slug_1: "); Serial.print(slug_1); Serial.print( "   ");
							//Serial.print( "slug_2: "); Serial.print(slug_2); Serial.print( "   ");
							//Serial.print( "slug_3: "); Serial.print(slug_3); Serial.print( "   ");
							//Serial.print( "slug_4: "); Serial.print(slug_4); Serial.print( "   ");
							//Serial.println();
							
							if ( strcmp_P(slug_1.c_str(), HTTP_SLUG_OPTIONS ) == 0 ) {
								// парсинг настроек
								if ( strcmp_P(slug_2.c_str(), HTTP_SLUG_DSW ) == 0 ) {
									if ( strcmp_P(slug_3.c_str(), HTTP_SLUG_REFRESH ) == 0 ) {
										if ( strcmp_P(slug_4.c_str(), HTTP_SLUG_GET ) == 0 ) {	 
											successHeader(client);
											client.print( options.temperature_refresh );																	
										} else if ( strcmp_P(slug_4.c_str(), HTTP_SLUG_SET ) == 0 ) {
											String slug_5 = getNextSlug(&request);
											options.temperature_refresh = slug_5.toInt();
											EEPROM.put(2, options.temperature_refresh);
											refresh = options.temperature_refresh;
										}			
									} else if ( strcmp_P(slug_3.c_str(), HTTP_SLUG_PIN ) == 0 ) {
										if ( strcmp_P(slug_4.c_str(), HTTP_SLUG_GET ) == 0 ) { 
											successHeader(client);
											client.print( options.pin_onewire );												
										} else if ( strcmp_P(slug_4.c_str(), HTTP_SLUG_SET ) == 0 ) {
											String slug_5 = getNextSlug(&request);
											options.pin_onewire = slug_5.toInt();
											EEPROM.put(0, options.pin_onewire);
										}										
									}									
								} else if ( strcmp_P(slug_2.c_str(), HTTP_SLUG_RELAY ) == 0 ) {
									if ( strcmp_P(slug_3.c_str(), HTTP_SLUG_DELAY_TIME ) == 0 ) {
										if ( strcmp_P(slug_4.c_str(), HTTP_SLUG_GET ) == 0 ) {
											successHeader(client);
											client.print( options.firstStartTimeout );											
										} else if ( strcmp_P(slug_4.c_str(), HTTP_SLUG_SET ) == 0 ) {
											String slug_5 = getNextSlug(&request);
											options.firstStartTimeout = slug_5.toInt();
											EEPROM.put(1, options.firstStartTimeout);											
										}		
									}
								}
							}
							else if ( strcmp_P( slug_1.c_str(), HTTP_SLUG_DSW ) == 0 ) {
								// парсинг температурных датчиков
								if ( strcmp_P(slug_2.c_str(), HTTP_SLUG_LIST ) == 0 ) {
									successHeader(client);
									for (byte i=0;i<MAX_DSW_SENSORS;i++) {
										if (dsw_temp[i].enabled == ENABLE) {
											char *tmpAddr = (char*)malloc(16);
											for (uint8_t k = 0; k < 8; k++) {
												char tmp[3] = "00";
												if (dsw_temp[i].address[k] < 0x10) { String(dsw_temp[i].address[k],HEX).toCharArray(tmp+1, 3); }
													else String(dsw_temp[i].address[k],HEX).toCharArray(tmp, 3);
												strcat(tmpAddr, tmp); //(deviceAddress[k], HEX);
											}	
											client.print( tmpAddr );
											client.print( F("<br>") );											
											free(tmpAddr);
										}  
									}
								} else if ( strcmp_P(slug_2.c_str(), HTTP_SLUG_TEMP ) == 0 ) {
										if ( strcmp_P(slug_3.c_str(), HTTP_SLUG_INDEX ) == 0 ) {
											byte idx = slug_4.toInt()-1;
											successHeader(client);
											char *temp = (char*)malloc(7);
											if ( temp == NULL) return;
											sprintf_P(temp, PSTR("%d.%02d"), (int)dsw_temp[idx].value / 100, (int)(dsw_temp[idx].value) % 100);
											client.print( temp );
											free(temp);											
										} else if ( strcmp_P(slug_3.c_str(), HTTP_SLUG_ADDRESS ) == 0 ) {
											//Serial.println( "HTTP_SLUG_ADDRESS" );
											//Serial.print( "Value: " ); Serial.println( slug_4 );											

										}								
								}
							}	
							else if ( strcmp_P( slug_1.c_str(), HTTP_SLUG_RELAY ) == 0 ) {
								// парсинг реле slug_2 == index slug_4 == get/set
								String value = getNextSlug(&request);
								
								byte idx = slug_2.toInt()-1;
								if (strcmp_P( slug_3.c_str(), HTTP_SLUG_STATE ) == 0) {
									if (strcmp_P( slug_4.c_str(), HTTP_SLUG_GET ) == 0) {
										successHeader(client);
										send(client, (relay[idx].info.state == ON ) ? CONST_ON : CONST_OFF );											
									} else if (strcmp_P( slug_4.c_str(), HTTP_SLUG_SET ) == 0) {
										relay[idx].switch_relay( strcmp_P( value.c_str(), CONST_ON ) == 0 ? ON : OFF);
										relay[idx].publish();
										redirectHeader(client, "/");										
									}
								} else if (strcmp_P( slug_3.c_str(), HTTP_SLUG_SIGNAL ) == 0) {
									if (strcmp_P( slug_4.c_str(), HTTP_SLUG_GET ) == 0) {
										successHeader(client);
										send(client,  (relay[idx].info.signalType == INVERT ) ? CONST_HIGH : CONST_LOW );											
									} else if (strcmp_P( slug_4.c_str(), HTTP_SLUG_SET ) == 0) {
										if ( idx < MAX_RELAY+1 ) relay[idx].setSignalType( strcmp_P( value.c_str(), CONST_LOW ) == 0 ? NORMAL : INVERT );
										redirectHeader(client, "/");										
									}									
								} else if (strcmp_P( slug_3.c_str(), HTTP_SLUG_FLASH ) == 0) {
									if (strcmp_P( slug_4.c_str(), HTTP_SLUG_GET ) == 0) {
										successHeader(client);
										client.print( relay[idx].info.flash  );											
									} else if (strcmp_P( slug_4.c_str(), HTTP_SLUG_SET ) == 0) {	
										if ( idx < MAX_RELAY+1 ) relay[idx].setFlash(value.toInt());
										redirectHeader(client, "/");										
									}															
								} else if (strcmp_P( slug_3.c_str(), HTTP_SLUG_ENABLE ) == 0) {
									if (strcmp_P( slug_4.c_str(), HTTP_SLUG_GET ) == 0) {
										successHeader(client);
										send(client, (relay[idx].info.enabled == ENABLE ) ? CONST_ENABLE : CONST_DISABLE );										
									} else if (strcmp_P( slug_4.c_str(), HTTP_SLUG_SET ) == 0) {
										if ( idx < MAX_RELAY+1 ) relay[idx].setEnabled( strcmp_P( value.c_str(), CONST_ENABLE ) == 0 ? ENABLE : DISABLE );							
										redirectHeader(client, "/");										
									}	
								} else if (strcmp_P( slug_3.c_str(), HTTP_SLUG_PIN ) == 0) {
									if (strcmp_P( slug_4.c_str(), HTTP_SLUG_GET ) == 0) {
										successHeader(client);
										client.print(relay[idx].info.pin);	
									} else if (strcmp_P( slug_4.c_str(), HTTP_SLUG_SET ) == 0) {	
										if ( idx < MAX_RELAY+1 ) relay[idx].setPin(value.toInt());
										redirectHeader(client, "/");										
									}									
								}
							} else {

							}
						}
					}				
	
			  }
				break;
		} else if (c == '\n') {
				// you're starting a new line
				currentLineIsBlank = true;
				if ( !requestLineReceived ) { requestLineReceived = true; }          
		} else if (c != '\r') {
				if(!requestLineReceived) { request += String(c); }          
				// you've gotten a character on the current line
				currentLineIsBlank = false;
			}	 		
		}
	}
	
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    //Serial.println("client disconnected");		
}


void loop() {
  
  // TODO something
  // update temp 
 

//Serial.print("refresh: ");
//Serial.print( refresh * 1000);
//Serial.println();
   //WRITE_TO_SERIAL(F("(millis() - lastReadingTime2): "), (millis() - lastReadingTime2) / 1000, F("               "), F("               "));

  if ((millis() - lastReadingTime2) > (refresh*1000) ) {
    lastReadingTime2 = millis();
		WRITE_TO_SERIAL(F("temperature_update: "), lastReadingTime2 / 1000, F(" sec"), F(""));	
    temperature_update();
	
	//WRITE_TO_SERIAL(F("FreeMem: "), memoryFree(), F(""), F(""));	
	//	
  }
  
  if ((millis() - lastReadingTime4) > (10 * 1000)) {
    lastReadingTime4 = millis();
    uptime(P_TOPIC_ARDUINO_UPTIME, lastReadingTime4);
	free_memory_publish();
	  //publish_all_data();
  }
  
  if (!mqtt_client.connected()) {
    reconnect();
  }
  
  mqtt_client.loop();
  
  // listen for incoming clients
#ifdef MEGA
  listenForEthernetClients();  
#endif
  

  
}


void free_memory_publish() {
#ifdef DEBUG1  
  WRITE_TO_SERIAL(F("-------- "), F("function free_memory_publish"), F(" start"), F(""));	
#endif 	
  int m = memoryFree();

  byte i = strlen(device_name);
  byte l = strlen(P_TOPIC_ARDUINO_MEMORY_FREE);
  char* buffer = (char*)malloc( i + l+1+2 );
  if ( buffer == NULL) return;
  
  strcpy(buffer, device_name);
  strcat_P(buffer, (char*)P_TOPIC_ARDUINO_MEMORY_FREE);

  char mm[5];
  itoa(m, mm, 10);
  mqtt_client.publish( buffer, mm);
  
  #ifdef DEBUG1 
	WRITE_TO_SERIAL(F("MQTT topic: "), buffer, F(" value: "), mm);	
  #endif 	

  free(buffer);

  
#ifdef DEBUG1  
  WRITE_TO_SERIAL(F("-------- "), F("function free_memory_publish"), F(" end"), F(""));	
#endif 	 	

}

