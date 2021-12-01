/**
 * @file
 * SNTP client module
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Simon Goldschmidt (lwIP raw API part)
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "logs.h"
#include "rtcc.h"
#include "sntp.h"
#include "global.h"

#include <string.h>
#include <time.h>

/** This is simple "SNTP" client for socket or raw API.
 * It is a minimal implementation of SNTPv4 as specified in RFC 4330.
 * 
 * For a list of some public NTP servers, see this link :
 * http://support.ntp.org/bin/view/Servers/NTPPoolServers
 *
 * @todo:
 * - set/change servers at runtime
 * - complete SNTP_CHECK_RESPONSE checks 3 and 4
 * - support broadcast/multicast mode?
 */

/** Decide whether to build SNTP for socket or raw API
 * The socket API SNTP client is a very minimal implementation that does not
 * fully confor to the SNTPv4 RFC, especially regarding server load and error
 * procesing. */

#define SNTP_SOCKET                 1
TaskHandle_t xSntpHandle = NULL;


#if SNTP_SOCKET
//#include "lwip/sockets.h"
#endif

/**
 * SNTP_DEBUG: Enable debugging for SNTP.
 */
#ifndef SNTP_DEBUG
#define SNTP_DEBUG                  LWIP_DBG_OFF
#endif

/** SNTP server port */
#ifndef SNTP_PORT
#define SNTP_PORT                   123
#endif

/** Set this to 1 to allow SNTP_SERVER_ADDRESS to be a DNS name */
#ifndef SNTP_SERVER_DNS
#define SNTP_SERVER_DNS             1
#endif

/** Set this to 1 to support more than one server */
#ifndef SNTP_SUPPORT_MULTIPLE_SERVERS
#define SNTP_SUPPORT_MULTIPLE_SERVERS 0
#endif

/** SNTP server address:
 * - as IPv4 address in "u32_t" format
 * - as a DNS name if SNTP_SERVER_DNS is set to 1
 * May contain multiple server names (e.g. "pool.ntp.org","second.time.server")
 */
#ifndef SNTP_SERVER_ADDRESS
#if SNTP_SERVER_DNS
#define SNTP_SERVER_ADDRESS         "pool.ntp.org"
#else
#define SNTP_SERVER_ADDRESS         "213.161.194.93" /* pool.ntp.org */
#endif
#endif

/** Sanity check:
 * Define this to
 * - 0 to turn off sanity checks (default; smaller code)
 * - >= 1 to check address and port of the response packet to ensure the
 *        response comes from the server we sent the request to.
 * - >= 2 to check returned Originate Timestamp against Transmit Timestamp
 *        sent to the server (to ensure response to older request).
 * - >= 3 @todo: discard reply if any of the LI, Stratum, or Transmit Timestamp
 *        fields is 0 or the Mode field is not 4 (unicast) or 5 (broadcast).
 * - >= 4 @todo: to check that the Root Delay and Root Dispersion fields are each
 *        greater than or equal to 0 and less than infinity, where infinity is
 *        currently a cozy number like one second. This check avoids using a
 *        server whose synchronization source has expired for a very long time.
 */
#ifndef SNTP_CHECK_RESPONSE
#define SNTP_CHECK_RESPONSE         0
#endif

/** According to the RFC, this shall be a random delay
 * between 1 and 5 minutes (in milliseconds) to prevent load peaks.
 * This can be defined to a random generation function,
 * which must return the delay in milliseconds as u32_t.
 * Turned off by default.
 */
#ifndef SNTP_STARTUP_DELAY
#define SNTP_STARTUP_DELAY          0
#endif

/** SNTP receive timeout - in milliseconds
 * Also used as retry timeout - this shouldn't be too low.
 * Default is 3 seconds.
 */
#ifndef SNTP_RECV_TIMEOUT
#define SNTP_RECV_TIMEOUT           9000
#endif

/** SNTP update delay - in milliseconds
 * Default is 1 hour.
 */
#ifndef SNTP_UPDATE_DELAY
#define SNTP_UPDATE_DELAY           1800000
#endif
#if (SNTP_UPDATE_DELAY < 15000) && !SNTP_SUPPRESS_DELAY_CHECK
#error "SNTPv4 RFC 4330 enforces a minimum update time of 15 seconds!"
#endif

/** SNTP macro to change system time and/or the update the RTC clock */
#ifndef SNTP_SET_SYSTEM_TIME
#define SNTP_SET_SYSTEM_TIME(sec) ((void)sec)
#endif

/** SNTP macro to change system time including microseconds */
#ifdef SNTP_SET_SYSTEM_TIME_US
#define SNTP_CALC_TIME_US           1
#define SNTP_RECEIVE_TIME_SIZE      2
#else
#define SNTP_SET_SYSTEM_TIME_US(sec, us)
#define SNTP_CALC_TIME_US           0
#define SNTP_RECEIVE_TIME_SIZE      1
#endif

/** SNTP macro to get system time, used with SNTP_CHECK_RESPONSE >= 2
 * to send in request and compare in response.
 */
#ifndef SNTP_GET_SYSTEM_TIME
#define SNTP_GET_SYSTEM_TIME(sec, us)     do { (sec) = 0; (us) = 0; } while(0)
#endif

/** Default retry timeout (in milliseconds) if the response
 * received is invalid.
 * This is doubled with each retry until SNTP_RETRY_TIMEOUT_MAX is reached.
 */
#ifndef SNTP_RETRY_TIMEOUT
#define SNTP_RETRY_TIMEOUT          SNTP_RECV_TIMEOUT
#endif

/** Maximum retry timeout (in milliseconds). */
#ifndef SNTP_RETRY_TIMEOUT_MAX
#define SNTP_RETRY_TIMEOUT_MAX      (SNTP_RETRY_TIMEOUT * 10)
#endif

/** Increase retry timeout with every retry sent
 * Default is on to conform to RFC.
 */
#ifndef SNTP_RETRY_TIMEOUT_EXP
#define SNTP_RETRY_TIMEOUT_EXP      1
#endif

/* the various debug levels for this file */
#define SNTP_DEBUG_TRACE        (SNTP_DEBUG | LWIP_DBG_TRACE)
#define SNTP_DEBUG_STATE        (SNTP_DEBUG | LWIP_DBG_STATE)
#define SNTP_DEBUG_WARN         (SNTP_DEBUG | LWIP_DBG_LEVEL_WARNING)
#define SNTP_DEBUG_WARN_STATE   (SNTP_DEBUG | LWIP_DBG_LEVEL_WARNING | LWIP_DBG_STATE)
#define SNTP_DEBUG_SERIOUS      (SNTP_DEBUG | LWIP_DBG_LEVEL_SERIOUS)

#define SNTP_ERR_KOD                1

/* SNTP protocol defines */
#define SNTP_MSG_LEN                48

#define SNTP_OFFSET_LI_VN_MODE      0
#define SNTP_LI_MASK                0xC0
#define SNTP_LI_NO_WARNING          0x00
#define SNTP_LI_LAST_MINUTE_61_SEC  0x01
#define SNTP_LI_LAST_MINUTE_59_SEC  0x02
#define SNTP_LI_ALARM_CONDITION     0x03 /* (clock not synchronized) */

#define SNTP_VERSION_MASK           0x38
#define SNTP_VERSION                (4/* NTP Version 4*/<<3) 

#define SNTP_MODE_MASK              0x07
#define SNTP_MODE_CLIENT            0x03
#define SNTP_MODE_SERVER            0x04
#define SNTP_MODE_BROADCAST         0x05

#define SNTP_OFFSET_STRATUM         1
#define SNTP_STRATUM_KOD            0x00

#define SNTP_OFFSET_ORIGINATE_TIME  24
#define SNTP_OFFSET_RECEIVE_TIME    32
#define SNTP_OFFSET_TRANSMIT_TIME   40

/* number of seconds between 1900 and 1970 */
#define DIFF_SEC_1900_1970         (2208988800UL)

/**
 * SNTP packet format (without optional fields)
 * Timestamps are coded as 64 bits:
 * - 32 bits seconds since Jan 01, 1970, 00:00
 * - 32 bits seconds fraction (0-padded)
 * For future use, if the MSB in the seconds part is set, seconds are based
 * on Feb 07, 2036, 06:28:16.
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif

struct sntp_msg {
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t reference_identifier;
    uint32_t reference_timestamp[2];
    uint32_t originate_timestamp[2];
    uint32_t receive_timestamp[2];
    uint32_t transmit_timestamp[2];
};


/* function prototypes */
static int sntp_request(void *arg);

/** The UDP pcb used by the SNTP client */
//static struct udp_pcb* sntp_pcb;
/** Addresses of servers */
static char* sntp_server_addresses[] = {SNTP_SERVER_ADDRESS};
#if SNTP_SUPPORT_MULTIPLE_SERVERS
/** The currently used server (initialized to 0) */
static u8_t sntp_current_server;
static u8_t sntp_num_servers = sizeof(sntp_server_addresses)/sizeof(char*);
#else /* SNTP_SUPPORT_MULTIPLE_SERVERS */
#define sntp_current_server 0
#endif /* SNTP_SUPPORT_MULTIPLE_SERVERS */

#if SNTP_RETRY_TIMEOUT_EXP
#define SNTP_RESET_RETRY_TIMEOUT() sntp_retry_timeout = SNTP_RETRY_TIMEOUT
/** Retry time, initialized with SNTP_RETRY_TIMEOUT and doubled with each retry. */
static uint32_t sntp_retry_timeout;
#else /* SNTP_RETRY_TIMEOUT_EXP */
#define SNTP_RESET_RETRY_TIMEOUT()
#define sntp_retry_timeout SNTP_RETRY_TIMEOUT
#endif /* SNTP_RETRY_TIMEOUT_EXP */

#if SNTP_CHECK_RESPONSE >= 1
/** Saves the last server address to compare with response */
static ip_addr_t sntp_last_server_address;
#endif /* SNTP_CHECK_RESPONSE >= 1 */

#if SNTP_CHECK_RESPONSE >= 2
/** Saves the last timestamp sent (which is sent back by the server)
 * to compare against in response */
static u32_t sntp_last_timestamp_sent[2];
#endif /* SNTP_CHECK_RESPONSE >= 2 */

/**
 * SNTP processing of received timestamp
 */
static void
sntp_process(uint32_t *receive_timestamp) {
    /* convert SNTP time (1900-based) to unix GMT time (1970-based)
     * @todo: if MSB is 1, SNTP time is 2036-based!
     */
    time_t t = (FreeRTOS_ntohl(receive_timestamp[0]) - DIFF_SEC_1900_1970);

#if SNTP_CALC_TIME_US
    u32_t us = ntohl(receive_timestamp[1]) / 4295;
    SNTP_SET_SYSTEM_TIME_US(t, us);
    /* display local time from GMT time */
    LWIP_DEBUGF(SNTP_DEBUG_TRACE, ("sntp_process: %s, %"U32_F" us", ctime(&t), us));

#else /* SNTP_CALC_TIME_US */

    /* change system time and/or the update the RTC clock */
    //SNTP_SET_SYSTEM_TIME(t);
    static time_t ut;
    static long tdiff;
    (void) gettime(&ut);
    if (ut == 0) {
	SysLog(LOG_NORMAL, "Hardware clock time not set\r\n");
    }
    tdiff = ((long) t - (long) ut);
    if (ut == 0 || abs(tdiff) > 5) {
	SysLog(LOG_NORMAL, "Hardware time %i updated %i sec utc DIFF(%i)\r\n", ut, t, tdiff);
	ut = (long) t;
	(void) settime(&ut);
    }

    /* display local time from GMT time */
    
    SysLog(LOG_DEBUG, "SNTP: UNIX %i\r\n", t);
#endif /* SNTP_CALC_TIME_US */
}

/**
 * Initialize request struct to be sent to server.
 */
static void
sntp_initialize_request(struct sntp_msg *req)
{
  memset(req, 0, SNTP_MSG_LEN);
  req->li_vn_mode = SNTP_LI_NO_WARNING | SNTP_VERSION | SNTP_MODE_CLIENT;

#if SNTP_CHECK_RESPONSE >= 2
  {
    u32_t sntp_time_sec, sntp_time_us;
    /* fill in transmit timestamp and save it in 'sntp_last_timestamp_sent' */
    SNTP_GET_SYSTEM_TIME(sntp_time_sec, sntp_time_us);
    sntp_last_timestamp_sent[0] = htonl(sntp_time_sec + DIFF_SEC_1900_1970);
    req->transmit_timestamp[0] = sntp_last_timestamp_sent[0];
    /* we send/save us instead of fraction to be faster... */
    sntp_last_timestamp_sent[1] = htonl(sntp_time_us);
    req->transmit_timestamp[1] = sntp_last_timestamp_sent[1];
  }
#endif /* SNTP_CHECK_RESPONSE >= 2 */
}

/**
 * Send an SNTP request via sockets.
 * This is a very minimal implementation that does not fully conform
 * to the SNTPv4 RFC, especially regarding server load and error procesing.
 */
static int
sntp_request(void *arg) {
    int retval = 0;
    Socket_t sock;
    struct freertos_sockaddr local;
    struct freertos_sockaddr to;
    int tolen;
    int size;
    int timeout;
    struct sntp_msg sntpmsg;
    uint32_t sntp_server_address;

    /* if we got a valid SNTP server address... */
    sntp_server_address = FreeRTOS_gethostbyname(SNTP_SERVER_ADDRESS);
    if (sntp_server_address) {
	/* create new socket */
	sock = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);
	if (sock != FREERTOS_INVALID_SOCKET) {
	    /* prepare local address */
	    memset(&local, 0, sizeof (local));
	    local.sin_family = FREERTOS_AF_INET;
	    local.sin_port = FREERTOS_INADDR_ANY;
	    local.sin_addr = FREERTOS_INADDR_ANY;

	    /* bind to local address */
	    if (FreeRTOS_bind(sock, (struct freertos_sockaddr *) &local, sizeof (local)) == 0) {
		/* set recv timeout */
		timeout = SNTP_RECV_TIMEOUT;
		FreeRTOS_setsockopt(sock, 0, FREERTOS_SO_RCVTIMEO, (char *) &timeout, sizeof (timeout));

		/* prepare SNTP request */
		sntp_initialize_request(&sntpmsg);

		/* prepare SNTP server address */
		memset(&to, 0, sizeof (to));
		to.sin_family = FREERTOS_AF_INET;
		to.sin_port = FreeRTOS_htons(SNTP_PORT);
		to.sin_addr = sntp_server_address;
		//inet_addr_from_ipaddr(&to.sin_addr, &sntp_server_address);

		/* send SNTP request to server */
		if (FreeRTOS_sendto(sock, &sntpmsg, SNTP_MSG_LEN, 0, (struct freertos_sockaddr *) &to, sizeof (to)) >= 0) {
		    /* receive SNTP server response */
		    tolen = sizeof (to);
		    size = FreeRTOS_recvfrom(sock, &sntpmsg, SNTP_MSG_LEN, 0, (struct freertos_sockaddr *) &to, (socklen_t *) & tolen);
		    /* if the response size is good */
		    if (size == SNTP_MSG_LEN) {
			/* if this is a SNTP response... */
			if (((sntpmsg.li_vn_mode & SNTP_MODE_MASK) == SNTP_MODE_SERVER) ||
				((sntpmsg.li_vn_mode & SNTP_MODE_MASK) == SNTP_MODE_BROADCAST)) {
			    /* do time processing */
			    sntp_process(sntpmsg.receive_timestamp);
			} else {
			    SysLog(LOG_DEBUG, "SNTP: no response frame code\r\n");
			    retval = -1;
			}
		    } else if (size < 0) {
			if (size == -pdFREERTOS_ERRNO_EWOULDBLOCK) {
			    SysLog(LOG_DEBUG, "SNTP: recv timeout\r\n");
			} else {
			    SysLog(LOG_DEBUG, "SNTP: recv err %i\r\n", size);
			}
			retval = -1;
		    } else {
			SysLog(LOG_DEBUG, "SNTP: size != SNTP_MSG_LEN %i\r\n", size);
			retval = -1;
		    }
		} else {
		    SysLog(LOG_DEBUG, "SNTP: no sendto\r\n");
		    retval = -1;
		}
	    } else {
		SysLog(LOG_DEBUG, "SNTP: bind error\r\n");
		retval = -1;
	    }
	    /* close the socket */
	    FreeRTOS_closesocket(sock);
	}
    } else {
	SysLog(LOG_DEBUG, "SNTP: Dns issue\r\n");
	retval = -1;
    }
    return retval;
}

/**
 * SNTP thread
 */
static void
sntp_thread(void *arg) {
    (void) ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    //ClearWatchDog(WDT_TIME);
    //SetWatchDogTimeout(WDT_TIME, SNTP_UPDATE_DELAY * 2, "Time");
    while (1) {
      //ClearWatchDog(WDT_TIME);
      if (USE_DHCP) {
        if (sntp_request(NULL)) {
          vTaskDelay(30000 / portTICK_PERIOD_MS);
        } else {
          vTaskDelay(SNTP_UPDATE_DELAY / portTICK_PERIOD_MS);
        }
      } else {
        vTaskDelay(30000 / portTICK_PERIOD_MS);
      }
    }
}

/**
 * Initialize this module when using sockets
 */
void
sntp_init(void)
{
    (void) xTaskCreate((TaskFunction_t) sntp_thread, "Sntp", 256, NULL, 1, &xSntpHandle);
}