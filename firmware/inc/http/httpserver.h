/* 
 * File:   httpserver.h
 * Author: Erik
 *
 * Created on June 7, 2018, 5:03 PM
 */

#ifndef HTTPSERVER_H
#define	HTTPSERVER_H

#ifdef	__cplusplus
extern "C" {
#endif

    extern TaskHandle_t xServerWorkTaskHandle;
    
    void HttpServerInit(void);



#ifdef	__cplusplus
}
#endif

#endif	/* HTTPSERVER_H */

