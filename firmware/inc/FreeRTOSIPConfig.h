#ifndef FREERTOS_IP_CONFIG_H
#define FREERTOS_IP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xmem.h"
#include "priorities.h"
#ifdef DEBUG
#define FreeRTOS_printf(X) debug_printf X
#else
#define FreeRTOS_printf(X)
#endif
// #define iptraceSENDING_DHCP_DISCOVER() debug_printf("DHCP: Request\r\n")
// #define iptraceDHCP_SUCCEDEED(x) debug_printf("DHCP: address %i.%i.%i.%i\r\n", x&0xFF, (x>>8)&0xFF, (x>>16)&0xFF, (x>>24))
// #define iptraceNETWORK_INTERFACE_TRANSMIT() debug_printf("INTERFACE TX\r\n")
// #define iptraceNETWORK_INTERFACE_RECEIVE() debug_printf("INTERFACE RX\r\n")

#define ipconfigBYTE_ORDER pdFREERTOS_LITTLE_ENDIAN

#define ipconfigUSE_RMII            1

#define ipconfigUSE_WIRESHARK 0

#if ipconfigUSE_WIRESHARK == 1
extern void WireSharkCallBack(const void *pBuff, unsigned short int nBytes);
#endif

#define ipconfigPHY_INDEX 0

#define dhcpINITIAL_TIMER_PERIOD (pdMS_TO_TICKS(250))
#define dhcpINITIAL_DHCP_TX_PERIOD (pdMS_TO_TICKS(2500))

#define ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM		( 1 )
#define ipconfigDRIVER_INCLUDED_RX_IP_CHECKSUM		( 1 )

#define ipconfigUSE_LINKED_RX_MESSAGES                  1

#define ipconfigBUFFER_PADDING                          4

#define ipconfigUSE_HTTP ( 1 )


#define ipconfigSOCK_DEFAULT_RECEIVE_BLOCK_TIME	( 5000 )
#define	ipconfigSOCK_DEFAULT_SEND_BLOCK_TIME	( 5000 )

#define ipconfigZERO_COPY_RX_DRIVER			( 1 )
#define ipconfigZERO_COPY_TX_DRIVER			( 1 )


#define ipconfigUSE_LLMNR					( 0 )


#define ipconfigUSE_NBNS					( 0 )


#define ipconfigUSE_DNS_CACHE				( 1 )
#define ipconfigDNS_CACHE_NAME_LENGTH		( 16 )
#define ipconfigDNS_CACHE_ENTRIES			( 4 )
#define ipconfigDNS_REQUEST_ATTEMPTS		( 4 )

#define ipconfigIP_TASK_PRIORITY			( PRIORITY_RTOS_IP )


#define ipconfigIP_TASK_STACK_SIZE_WORDS	( configMINIMAL_STACK_SIZE * 5 )

#define ipconfigUSE_NETWORK_EVENT_HOOK 1

#define arpGRATUITOUS_ARP_PERIOD         ( pdMS_TO_TICKS( 500 ) )


#define ipconfigUDP_MAX_SEND_BLOCK_TIME_TICKS ( 5000 / portTICK_PERIOD_MS )


#define ipconfigUSE_DHCP                      1
#define ipconfigDHCP_REGISTER_HOSTNAME        0
#define ipconfigDHCP_USES_UNICAST             1
#define ipconfigUSE_DHCP_HOOK                 0

#define ipconfigMAXIMUM_DISCOVER_TX_PERIOD		( pdMS_TO_TICKS( 60000 ) )


#define ipconfigARP_CACHE_ENTRIES		6


#define ipconfigMAX_ARP_RETRANSMISSIONS ( 5 )

#define ipconfigMAX_ARP_AGE			150


#define ipconfigINCLUDE_FULL_INET_ADDR	1


#if( ipconfigZERO_COPY_RX_DRIVER != 0 )

	#define ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS		( 25 )
#else
	#define ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS		25
#endif


#define ipconfigEVENT_QUEUE_LENGTH		( ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS + 5 )


#define ipconfigALLOW_SOCKET_SEND_WITHOUT_BIND 1

#define ipconfigUDP_TIME_TO_LIVE		128
#define ipconfigTCP_TIME_TO_LIVE		128 


#define ipconfigUSE_TCP				( 1 )

#define ipconfigUSE_TCP_WIN			( 1 )

#define    ipconfigARP_STORES_REMOTE_ADDRESSES 1


#define ipconfigNETWORK_MTU					1500
#define ipconfigCAN_FRAGMENT_OUTGOING_PACKETS 0


#define ipconfigUSE_DNS								(1)

#define ipconfigREPLY_TO_INCOMING_PINGS				1


#define ipconfigSUPPORT_OUTGOING_PINGS				1


#define ipconfigSUPPORT_SELECT_FUNCTION				1
#define ipconfigSOCKET_HAS_USER_SEMAPHORE 1


#define ipconfigFILTER_OUT_NON_ETHERNET_II_FRAMES  0


#define ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES	1


#define configWINDOWS_MAC_INTERRUPT_SIMULATOR_DELAY ( 2 / portTICK_PERIOD_MS )


#define ipconfigPACKET_FILLER_SIZE 2

#define ipconfigETHERNETMINIMUMPACKET_BYTES       60


#define ipconfigTCP_WIN_SEG_COUNT 64


#define ipconfigTCP_RX_BUFFER_LENGTH			( 3 * 1460 )

#define ipconfigTCP_TX_BUFFER_LENGTH			( 2 * 1460 )


#define ipconfigIS_VALID_PROG_ADDRESS(x) ( (x) != NULL )


#define ipconfigTCP_HANG_PROTECTION				( 1 )
#define ipconfigTCP_HANG_PROTECTION_TIME		( 30 )


#define ipconfigTCP_KEEP_ALIVE				( 1 )
#define ipconfigTCP_KEEP_ALIVE_INTERVAL		( 20 ) 

#define ipconfigFTP_TX_BUFSIZE				( 4 * ipconfigTCP_MSS )
#define ipconfigFTP_TX_WINSIZE				( 2 )
#define ipconfigFTP_RX_BUFSIZE				( 8 * ipconfigTCP_MSS )
#define ipconfigFTP_RX_WINSIZE				( 4 )
#define ipconfigHTTP_TX_BUFSIZE				( 3 * ipconfigTCP_MSS )
#define ipconfigHTTP_TX_WINSIZE				( 2 )
#define ipconfigHTTP_RX_BUFSIZE				( 4 * ipconfigTCP_MSS )
#define ipconfigHTTP_RX_WINSIZE				( 4 )



BaseType_t xApplicationDNSQueryHook( const char *pcName );

#define ipconfigDNS_USE_CALLBACKS			(1)
#define ipconfigSUPPORT_SIGNALS				1

#define pvPortMallocLarge( x )				x_malloc( x ,XMEM_HEAP_SLOW)
#define vPortFreeLarge(ptr)				x_free(ptr)
#define pvPortMallocSocket( x )				x_malloc( x ,XMEM_HEAP_SLOW)
#define vPortFreeSocket(ptr)				x_free(ptr)


#define USE_IPERF                                   1
#define ipconfigIPERF_DOES_ECHO_UDP                 0

#define ipconfigIPERF_VERSION                       3

#define ipconfigIPERF_STACK_SIZE_IPERF_TASK         680  // or more?

#define ipconfigIPERF_TX_BUFSIZE                    ( 8 * ipconfigTCP_MSS )
#define ipconfigIPERF_TX_WINSIZE                    ( 6 )
#define ipconfigIPERF_RX_BUFSIZE                    ( 8 * ipconfigTCP_MSS )
#define ipconfigIPERF_RX_WINSIZE                    ( 6 )

/* The iperf module declares a character buffer to store its send data. */
#define ipconfigIPERF_RECV_BUFFER_SIZE              ( 2 * ipconfigTCP_MSS )


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* FREERTOS_IP_CONFIG_H */

// #ifndef FREERTOS_IP_CONFIG_H
// #define FREERTOS_IP_CONFIG_H

// #ifdef __cplusplus
// extern "C" {
// #endif

// //Constants Affecting the TCP/IP Stack Task Execution Behaviour

// #define ipconfigIP_TASK_PRIORITY                ( configMAX_PRIORITIES - 2 )
// #define ipconfigIP_TASK_STACK_SIZE_WORDS        ( configMINIMAL_STACK_SIZE * 5 )
// #define ipconfigUSE_NETWORK_EVENT_HOOK          1 
// #define ipconfigEVENT_QUEUE_LENGTH              ( ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS + 5 )

// //Debug, Trace and Logging Settings
// //See also TCP/IP Trace Macros.

// #define ipconfigHAS_DEBUG_PRINTF                0  
// #define FreeRTOS_debug_printf                   0 
// #define ipconfigHAS_PRINTF                      0 
// #define FreeRTOS_printf                         0 
// //#define ipconfigTCP_MAY_LOG_PORT
// //#define ipconfigCHECK_IP_QUEUE_SPACE     
// //#define ipconfigWATCHDOG_TIMER()

// //Hardware and Driver Specific Settings

// #define ipconfigBYTE_ORDER                      pdFREERTOS_LITTLE_ENDIAN
// #define ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM  1 
// #define ipconfigDRIVER_INCLUDED_RX_IP_CHECKSUM  1
// #define ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES 1
// #define ipconfigETHERNET_DRIVER_FILTERS_PACKETS 0
// #define ipconfigNETWORK_MTU                     1500
// #if( ipconfigZERO_COPY_RX_DRIVER != 0 )
// 	/* _HT_ Actually we should know the value of 'configNUM_RX_DESCRIPTORS' here. */
// 	#define ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS		( 25 + 6 )
// #else
// 	#define ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS		25
// #endif
// #define ipconfigFILTER_OUT_NON_ETHERNET_II_FRAMES       0
// #define ipconfigUSE_LINKED_RX_MESSAGES                  0
// #define ipconfigZERO_COPY_TX_DRIVER                     1
// #define ipconfigBUFFER_PADDING                          0
// #define ipconfigPACKET_FILLER_SIZE                      2

// //TCP Specific Constants

// #define ipconfigUSE_TCP                                 1
// #define ipconfigTCP_TIME_TO_LIVE                        128
// #define ipconfigTCP_RX_BUFFER_LENGTH                    ( 3 * 1460 )
// #define ipconfigTCP_TX_BUFFER_LENGTH                    ( 3 * 1460 )
// #define ipconfigUSE_TCP_WIN                             1
// #define ipconfigTCP_WIN_SEG_COUNT                       64
// //#define ipconfigUSE_TCP_TIMESTAMPS                      
// //#define ipconfigTCP_MSS
// #define ipconfigTCP_KEEP_ALIVE                          1
// #define ipconfigTCP_KEEP_ALIVE_INTERVAL                 20
// #define ipconfigTCP_HANG_PROTECTION                     1
// #define ipdefineonfigTCP_HANG_PROTECTION_TIME           30
// #define ipconfigIGNORE_UNKNOWN_PACKETS                  0

// //UDP Specific Constants

// #define    ipconfigUDP_TIME_TO_LIVE                     128
// #define    ipconfigUDP_MAX_SEND_BLOCK_TIME_TICKS        ( 5000 / portTICK_PERIOD_MS )
// //#define    ipconfigUDP_MAX_RX_PACKETS           

// //Other Constants Effecting Socket Behaviour

// #define    ipconfigINCLUDE_FULL_INET_ADDR               1
// #define    ipconfigALLOW_SOCKET_SEND_WITHOUT_BIND       1
// #define    ipconfigSOCK_DEFAULT_RECEIVE_BLOCK_TIME      
// #define    ipconfigSOCK_DEFAULT_SEND_BLOCK_TIME
// #define    ipconfigSOCKET_HAS_USER_SEMAPHORE
// #define    ipconfigSUPPORT_SIGNALS
// #define    ipconfigSUPPORT_SELECT_FUNCTION

// //Constants Affecting the ARP Behaviour

// #define    ipconfigARP_CACHE_ENTRIES
// #define    ipconfigMAX_ARP_RETRANSMISSIONS
// #define    ipconfigMAX_ARP_AGE
// #define    ipconfigARP_REVERSED_LOOKUP
// #define    ipconfigARP_STORES_REMOTE_ADDRESSES
// #define    ipconfigUSE_ARP_REVERSED_LOOKUP
// #define    ipconfigUSE_ARP_REMOVE_ENTRY

// //Constants Affecting DHCP and Name Service Behaviour

// #define    ipconfigUSE_DNS
// #define    ipconfigUSE_DNS_CACHE
// #define    ipconfigDNS_CACHE_ENTRIES
// #define    ipconfigDNS_CACHE_NAME_LENGTH
// #define    ipconfigUSE_LLMNR
// #define    ipconfigUSE_NBNS
// #define    ipconfigDNS_REQUEST_ATTEMPTS
// #define    ipconfigUSE_DHCP
// #define    ipconfigMAXIMUM_DISCOVER_TX_PERIOD
// #define    ipconfigUSE_DHCP_HOOK
// #define    ipconfigDHCP_REGISTER_HOSTNAME

// //Constants Affecting IP and ICMP Behaviour

// #define    ipconfigCAN_FRAGMENT_OUTGOING_PACKETS
#define    ipconfigREPLY_TO_INCOMING_PINGS 1
//#define    ipconfigSUPPORT_OUTGOING_PINGS 1

// //Constants Providing Target Support

// #define    ipconfigHAS_INLINE_FUNCTIONS
//#define    ipconfigRAND32 111

// }

//***************************** ALL SETTINGS ******************************//





