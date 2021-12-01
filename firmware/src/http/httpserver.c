/*
 * HTTP server
 * 
 * 
 */

#include <stm32f767xx.h>
#include <stdio.h>
#include <time.h>

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "http/FreeRTOS_TCP_server.h"
#include "FreeRTOS_DHCP.h"

TaskHandle_t xServerWorkTaskHandle = NULL;
void HttpServerTasks(void);

static const struct xSERVER_CONFIG xServerConfiguration[] ={
    /* Server type,		port number,	backlog, 	root dir. */
    { eSERVER_HTTP, 80, 12, ""}

};

void HttpServerInit(void){
    (void)xTaskCreate((TaskFunction_t) HttpServerTasks, "HttpWork", 800, NULL, tskIDLE_PRIORITY, &xServerWorkTaskHandle );
}

void HttpServerTasks(void){
    const TickType_t xInitialBlockTime = pdMS_TO_TICKS( 50UL );
    TCPServer_t *pxTCPServer = NULL;
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    pxTCPServer = FreeRTOS_CreateTCPServer( xServerConfiguration, sizeof( xServerConfiguration ) / sizeof( xServerConfiguration[ 0 ] ) );
    configASSERT( pxTCPServer );
    while(1){
	FreeRTOS_TCPServerWork( pxTCPServer, xInitialBlockTime );
    }
}


