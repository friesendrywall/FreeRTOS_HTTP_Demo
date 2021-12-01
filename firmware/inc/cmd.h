/* 
 * File:   cmd.h
 * Author: Erik
 *
 * Created on December 6, 2017, 2:07 PM
 */

#ifndef CMD_H
#define	CMD_H

#ifdef	__cplusplus
extern "C" {
#endif

    typedef struct {
        char Command[40];
        unsigned char Length;
    } _CmdMemory;

    typedef enum {
        Term_Home, Term_Confirm, Term_Ping, Term_Dig, Term_Dump, Term_Erase, Term_DoCommand, Term_LoadFlash, Term_SaveLogs
    } _CmdModes;

#define MaxParamLength 24

#define CtrlC 0x03
#define CtrlX 0x018
#define CtrlBackSpace 0x7F
#define CtrlCR 0x0D
#define CtrlE 0x05
#define CtrlEscape 0x1B

#define SAVED_CMD_LEN 8
#define CMD_BUILD_LEN 256

    typedef struct {
        int EscapeSequence;
        int CommandRecall;
        int Cancel;
        int CmdReady;
        unsigned char CmdPtr;
        unsigned char CmdBuilder[CMD_BUILD_LEN];
        unsigned char ReplyBuilder[CMD_BUILD_LEN];
        int ReplyLen;
        _CmdMemory SavedCommands[SAVED_CMD_LEN];
        int Hide;
    } _PuttyContext;

    void cmdtasks(void *pvParameters);
    void PuttyTcpTasks(void *pvParameters);
    void PuttyRxHandler(void * VoidRxPtr, int Length, _PuttyContext * ctx);

#ifdef	__cplusplus
}
#endif

#endif	/* CMD_H */

