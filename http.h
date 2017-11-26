#ifndef HTTP_H
#define HTTP_H

#include <Ethernet.h>
#include <avr/pgmspace.h>

const char PROGMEM header_http_ok[]=
	"HTTP/1.1 200 OK\r\n"\
	"Content-Type: text/html\r\n"\
	"<meta http-equiv='content-type' content='text/html; charset=UTF-8'>\r\n"\
	"Connection: keep-alive\r\n"\
	"\r\n";

	
//----------------------------------------------------------------------
const char HTTP_SLUG_RELAY[] PROGMEM = "relay";
const char HTTP_SLUG_DSW[] PROGMEM = "dsw";
const char HTTP_SLUG_OPTIONS[] PROGMEM = "options";

const char HTTP_SLUG_REFRESH[] PROGMEM = "refresh";
const char HTTP_SLUG_INDEX[] PROGMEM = "index";
const char HTTP_SLUG_ADDRESS[] PROGMEM = "address";

const char HTTP_SLUG_PIN[] PROGMEM = "pin";
const char HTTP_SLUG_LIST[] PROGMEM = "list";
const char HTTP_SLUG_TEMP[] PROGMEM = "temp";
const char HTTP_SLUG_SET[] PROGMEM = "set";
const char HTTP_SLUG_GET[] PROGMEM = "get";

const char HTTP_SLUG_DELAY_TIME[] PROGMEM = "delay-time";

const char HTTP_SLUG_STATUS[] PROGMEM = "status";
const char HTTP_SLUG_STATE[] PROGMEM = "state";
const char HTTP_SLUG_SIGNAL[] PROGMEM = "signal";
const char HTTP_SLUG_FLASH[] PROGMEM = "flash";
const char HTTP_SLUG_MODE[] PROGMEM = "mode";
const char HTTP_SLUG_AUTO[] PROGMEM = "auto";
const char HTTP_SLUG_ON[] PROGMEM = "on";
const char HTTP_SLUG_ENABLE[] PROGMEM = "enable";


void successHeader(EthernetClient client);
void redirectHeader(EthernetClient client, const char *path);
void send(EthernetClient client, const char *ptr);
void generate_main_page(EthernetClient client);
bool ispage(String *page);
/*
request - input url
result - slug
*/
String getUrlFromHeader(String *request);
String getNextSlug(String *request);


#endif //HTTP_H
