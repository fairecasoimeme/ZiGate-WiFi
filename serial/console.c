// Copyright 2015 by Thorsten von Eicken, see LICENSE.txt

#include <esp8266.h>
#include "uart.h"
#include "cgi.h"
#include "uart.h"
#include "serbridge.h"
#include "serled.h"
#include "config.h"
#include "console.h"
#include "time.h"
#include "sntp.h"
#include "syslog.h"

// Microcontroller console capturing the last 1024 characters received on the uart so
// they can be shown on a web page

// Buffer to hold concole contents.
// Invariants:
// - console_rd==console_wr <=> buffer empty
// - *console_rd == next char to read
// - *console_wr == next char to write
// - 0 <= console_xx < BUF_MAX
// - (console_wr+1)%BUF_MAX) == console_rd <=> buffer full
#define BUF_MAX (1024)
static char console_buf[BUF_MAX];
static int console_wr, console_rd;
static int console_pos; // offset since reset of buffer
bool transcode=false;

static void ICACHE_FLASH_ATTR
console_write(char c) {
  console_buf[console_wr] = c;
  console_wr = (console_wr+1) % BUF_MAX;
  if (console_wr == console_rd) {
    // full, we write anyway and loose the oldest char
    console_rd = (console_rd+1) % BUF_MAX; // full, eat first char
    console_pos++;
  }
}

#if 0
// return previous character in console, 0 if at start
static char ICACHE_FLASH_ATTR
console_prev(void) {
  if (console_wr == console_rd) return 0;
  return console_buf[(console_wr-1+BUF_MAX)%BUF_MAX];
}
#endif



void ICACHE_FLASH_ATTR
getDate() {
	time_t now = NULL;
    struct tm *tp = NULL;
	char tmp[50];
	int lnTmp;
	uint32_t tick = WDEV_NOW();
    // create timestamp: FULL-DATE "T" PARTIAL-TIME "Z": 'YYYY-mm-ddTHH:MM:SSZ '
    // as long as realtime_stamp is 0 we use tick div 10⁶ as date
    uint32_t realtime_stamp = sntp_get_current_timestamp();
    now = (realtime_stamp == 0) ? (tick / 1000000) : realtime_stamp;
    tp = gmtime(&now);

    lnTmp = os_sprintf(tmp, "%4d-%02d-%02d %02d:%02d:%02d",tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday,tp->tm_hour, tp->tm_min, tp->tm_sec);
	for (short i=0; i<lnTmp; i++)
	{
		console_write_time(tmp[i]);
	}
	if (realtime_stamp == 0)
		lnTmp = os_sprintf(tmp, ".%06uZ ", tick % 1000000);
		
    else
      lnTmp = os_sprintf(tmp, "%+03d:00 ", flashConfig.timezone_offset);
	
	for (short i=0; i<lnTmp; i++)
	{
		console_write_time(tmp[i]);
	}
}

void ICACHE_FLASH_ATTR
console_write_time(char c) {
	//if (c == '\n' && console_prev() != '\r') console_write('\r'); // does more harm than good
	console_write(c);
}

void ICACHE_FLASH_ATTR
console_write_char(char c,bool direction) {
	//if (c == '\n' && console_prev() != '\r') console_write('\r'); // does more harm than good
	char tmp[2];
	if (c==0x01)
	{
		getDate();
		console_write(0x20);
		if (direction)
		{
			console_write('-');
			console_write('>');
		}else{
			console_write('<');
			console_write('-');
		}
		console_write(0x20);
	}else if (c==0x03)
	{
		console_write(0x0a);
	}else if (c==0x02)
	{
		transcode=true;
		
	}else{
		if (transcode)
		{
			os_sprintf(tmp,"%02X",(c^0x10));
			console_write(tmp[0]);
			console_write(tmp[1]);
			console_write(0x20);
			transcode=false;
		}else{
			os_sprintf(tmp,"%02X",c);
			console_write(tmp[0]);
			console_write(tmp[1]);
			console_write(0x20);
		}
	}
}


int ICACHE_FLASH_ATTR
ajaxConsoleReset(HttpdConnData *connData) {
  if (connData->conn==NULL) return HTTPD_CGI_DONE; // Connection aborted. Clean up.
  jsonHeader(connData, 200);
  console_rd = console_wr = console_pos = 0;
  serbridgeReset();
  return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR
ajaxConsoleBaud(HttpdConnData *connData) {
  if (connData->conn==NULL) return HTTPD_CGI_DONE; // Connection aborted. Clean up.
  char buff[512];
  int len, status = 400;
  len = httpdFindArg(connData->getArgs, "rate", buff, sizeof(buff));
  if (len > 0) {
    int rate = atoi(buff);
    if (rate >= 300 && rate <= 1000000) {
      uart0_baud(rate);
      flashConfig.baud_rate = rate;
      status = configSave() ? 200 : 400;
    }
  } else if (connData->requestType == HTTPD_METHOD_GET) {
    status = 200;
  }

  jsonHeader(connData, status);
  os_sprintf(buff, "{\"rate\": %d}", flashConfig.baud_rate);
  httpdSend(connData, buff, -1);
  return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR
ajaxConsoleFormat(HttpdConnData *connData) {
  if (connData->conn==NULL) return HTTPD_CGI_DONE; // Connection aborted. Clean up.
  char buff[16];
  int len, status = 400;

  len = httpdFindArg(connData->getArgs, "fmt", buff, sizeof(buff));
  if (len >= 3) {
    int c = buff[0];
    if (c >= '5' && c <= '8') flashConfig.data_bits = c - '5' + FIVE_BITS;
    if (buff[1] == 'N') flashConfig.parity = NONE_BITS;
    if (buff[1] == 'E') flashConfig.parity = EVEN_BITS;
    if (buff[1] == 'O') flashConfig.parity = ODD_BITS;
    if (buff[2] == '1') flashConfig.stop_bits = ONE_STOP_BIT;
    if (buff[2] == '2') flashConfig.stop_bits = TWO_STOP_BIT;
    uart0_config(flashConfig.data_bits, flashConfig.parity, flashConfig.stop_bits);
    status = configSave() ? 200 : 400;
  } else if (connData->requestType == HTTPD_METHOD_GET) {
    status = 200;
  }

  jsonHeader(connData, status);
  os_sprintf(buff, "{\"fmt\": \"%c%c%c\"}", flashConfig.data_bits + '5',
      flashConfig.parity ? 'E' : 'N', flashConfig.stop_bits ? '2': '1');
  httpdSend(connData, buff, -1);
  return HTTPD_CGI_DONE;
}


int ICACHE_FLASH_ATTR
ajaxConsoleSend(HttpdConnData *connData) {
  if (connData->conn==NULL) return HTTPD_CGI_DONE; // Connection aborted. Clean up.
  char buff[2048];
  int len, status = 400;

  // figure out where to start in buffer based on URI param
  len = httpdFindArg(connData->getArgs, "text", buff, sizeof(buff));
  if (len > 0) {
    serledFlash(50); // short blink on serial LED
    uart0_tx_buffer(buff, len);
	for (short i=0; i<len; i++)
	{
		console_write_char(buff[i],true);
	}
    status = 200;
  }

  jsonHeader(connData, status);
  return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR
ajaxConsole(HttpdConnData *connData) {
  if (connData->conn==NULL) return HTTPD_CGI_DONE; // Connection aborted. Clean up.
  char buff[2048];
  int len; // length of text in buff
  int console_len = (console_wr+BUF_MAX-console_rd) % BUF_MAX; // num chars in console_buf
  int start = 0; // offset onto console_wr to start sending out chars

  jsonHeader(connData, 200);

  // figure out where to start in buffer based on URI param
  len = httpdFindArg(connData->getArgs, "start", buff, sizeof(buff));
  if (len > 0) {
    start = atoi(buff);
    if (start < console_pos) {
      start = 0;
    } else if (start >= console_pos+console_len) {
      start = console_len;
    } else {
      start = start - console_pos;
    }
  }

  // start outputting
  len = os_sprintf(buff, "{\"len\":%d, \"start\":%d, \"text\": \"",
      console_len-start, console_pos+start);

  int rd = (console_rd+start) % BUF_MAX;
  while (len < 2040 && rd != console_wr) {
    uint8_t c = console_buf[rd];
    if (c == '\\' || c == '"') {
      buff[len++] = '\\';
      buff[len++] = c;
    } else if (c == '\r') {
      // this is crummy, but browsers display a newline for \r\n sequences
    } else if (c < ' ') {
      len += os_sprintf(buff+len, "\\u%04x", c);
    } else {
      buff[len++] = c;
    }
    rd = (rd + 1) % BUF_MAX;
  }
  os_strcpy(buff+len, "\"}"); len+=2;
  httpdSend(connData, buff, len);
  return HTTPD_CGI_DONE;
}

void ICACHE_FLASH_ATTR consoleInit() {
  console_wr = 0;
  console_rd = 0;
}


