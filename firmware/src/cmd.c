#include "FreeRTOS.h"
#include "list.h"
#include "stream_buffer.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_TCP_IP.h"
#include "debugUart.h"
#include "global.h"

#include <stdbool.h> // Defines true
#include <stddef.h>  // Defines NULL
#include <stdlib.h>  // Defines EXIT_FAILURE
#include <stm32f767xx.h>
#include <string.h>

#include "FreeRTOS_CLI.h"
#include "cmd.h"
#include "logs.h"
#include "xmem.h"

#define TCP_FAIL_LOGON_MSG "\r\nAccess denied"
#define TCP_USERNAME "admin"
#define TCP_PASSWORD "putty"

#define FreeRTOS_send_string(s, b, f) FreeRTOS_send(s, b, sizeof(b) - 1, f)

volatile unsigned int CPU_Load = 0;
extern StreamBufferHandle_t cmdLogStream;

static void ClearCommand(_PuttyContext *ctx);
static void SaveCommand(_PuttyContext *ctx);
static void ResetCommand(_PuttyContext *ctx);

_PuttyContext PuttyUart = {
  .EscapeSequence = 0,
  .CommandRecall = (SAVED_CMD_LEN - 1),
  .Cancel = false,
  .CmdPtr = 0,
  .CmdBuilder = { 0 },
  .ReplyBuilder = { 0 },
  .ReplyLen = 0,
  .SavedCommands = { 0 },
  .Hide = 0
};

_PuttyContext PuttyTcp = {
  .EscapeSequence = 0,
  .CommandRecall = (SAVED_CMD_LEN - 1),
  .Cancel = false,
  .CmdPtr = 0,
  .CmdBuilder = { 0 },
  .ReplyBuilder = { 0 },
  .ReplyLen = 0,
  .SavedCommands = { 0 },
  .Hide = 0
};

void PuttyTcpTasks(void *pvParameters) {
  int len;

  enum {
    SM_INIT,
    SM_WAIT_CONNECT,
    SM_PRINT_LOGIN,
    SM_GET_LOGIN,
    SM_GET_PASSWORD,
    SM_AUTHENTICATED
  };
  int task = SM_INIT;
  int BadUser = 0;
  int Tries = 0;
  int WasReset = 0;
  int err;
  BaseType_t xMoreDataToFollow;
  Socket_t tSocket = FREERTOS_INVALID_SOCKET;
  Socket_t xConnectedSocket = FREERTOS_INVALID_SOCKET;
  static struct freertos_sockaddr local, xClient;
  socklen_t xSize = sizeof(xClient);
  static const TickType_t xReceiveTimeOut = 10000;
  static TickType_t xIoTimeOut = 5000;
  CLI_Command_Context *ctx;
  ctx = calloc(1, sizeof(CLI_Command_Context));
  configASSERT(ctx);
  ctx->cmdstate.Remote = 0;
  static char buff[1024];
  memset(buff, 0, sizeof(buff));
  (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  //ClearWatchDog(WDT_TCP_PUTTY);
  //SetWatchDogTimeout(WDT_TCP_PUTTY, 60000, "Putty");
  while (1) {
    //ClearWatchDog(WDT_TCP_PUTTY);
    switch (task) {
    case SM_INIT:
      tSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP); //Port 8000
      configASSERT(tSocket != FREERTOS_INVALID_SOCKET);
      FreeRTOS_setsockopt(tSocket, 0, FREERTOS_SO_RCVTIMEO,
          &xReceiveTimeOut,
          sizeof(xReceiveTimeOut));
      memset(&local, 0, sizeof(local));
      local.sin_family = FREERTOS_AF_INET;
      local.sin_port = FreeRTOS_htons(8000);
      local.sin_addr = FreeRTOS_GetIPAddress();
      err = FreeRTOS_bind(tSocket, &local, sizeof(local));
      FreeRTOS_listen(tSocket, 1);
      task = SM_WAIT_CONNECT;
      break;
    case SM_WAIT_CONNECT:
      if (xConnectedSocket != FREERTOS_INVALID_SOCKET) {
        FreeRTOS_shutdown(xConnectedSocket, FREERTOS_SHUT_RDWR);
        vTaskDelay(250 / portTICK_PERIOD_MS);
        FreeRTOS_closesocket(xConnectedSocket);
        xConnectedSocket = FREERTOS_INVALID_SOCKET;
      }
      while (xConnectedSocket == FREERTOS_INVALID_SOCKET) {
        xConnectedSocket = FreeRTOS_accept(tSocket, &xClient, &xSize);
        //		    ClearWatchDog(WDT_TCP_PUTTY);
        if (xConnectedSocket == NULL) {
          xConnectedSocket = FREERTOS_INVALID_SOCKET;
        }
      }
      Tries = 0;
      WasReset = 0;
      xIoTimeOut = 10000;
      FreeRTOS_setsockopt(xConnectedSocket, 0, FREERTOS_SO_RCVTIMEO,
          &xIoTimeOut,
          sizeof(xIoTimeOut));
      FreeRTOS_setsockopt(xConnectedSocket, 0, FREERTOS_SO_SNDTIMEO,
          &xIoTimeOut,
          sizeof(xIoTimeOut));
      if (FreeRTOS_send_string(xConnectedSocket, "\x1b[2J\x1b[33m"
                                                 "EasyMix Server 1.0\x1b[0m",
              0) > 0) {
        task = SM_PRINT_LOGIN;
      }
      break;
    case SM_PRINT_LOGIN:
      if (FreeRTOS_send_string(xConnectedSocket, "\r\nLogin: ", 0) > 0) {
        task = SM_GET_LOGIN;
      } else {
        task = SM_WAIT_CONNECT;
      }
      break;
    case SM_GET_LOGIN:
      len = FreeRTOS_recv(xConnectedSocket, buff, sizeof(buff), 0);
      //		ClearWatchDog(WDT_TCP_PUTTY);
      if (len > 0) {
        PuttyRxHandler(buff, len, &PuttyTcp);
        if (PuttyTcp.ReplyLen) {
          if (FreeRTOS_send(xConnectedSocket, PuttyTcp.ReplyBuilder, PuttyTcp.ReplyLen, 0) > 0) {

          } else {
            task = SM_WAIT_CONNECT;
            break;
          }
          PuttyTcp.ReplyLen = 0;
        }
        if (PuttyTcp.CmdReady) {
          SysLog(LOG_DEBUG, "Username: %s\r\n", PuttyTcp.CmdBuilder);
          if (memcmp(PuttyTcp.CmdBuilder, TCP_USERNAME, strlen(TCP_USERNAME) - 1) == 0) {
            BadUser = 0;
          } else {
            BadUser = 1;
          }
          task = SM_GET_PASSWORD;
          PuttyTcp.CmdPtr = 0;
          PuttyTcp.Cancel = 0;
          PuttyTcp.CmdReady = false;
          PuttyTcp.Hide = true;
          if (FreeRTOS_send_string(xConnectedSocket, "\r\nPassword: ", 0) <= 0) {
            task = SM_WAIT_CONNECT;
          }
        }
      } else if (len == 0 || len == -pdFREERTOS_ERRNO_EWOULDBLOCK) {

      } else {
        task = SM_WAIT_CONNECT;
      }
      break;
    case SM_GET_PASSWORD:
      len = FreeRTOS_recv(xConnectedSocket, buff, sizeof(buff), 0);
      //	ClearWatchDog(WDT_TCP_PUTTY);
      if (len > 0) {
        PuttyRxHandler(buff, len, &PuttyTcp);
        if (PuttyTcp.ReplyLen) {
          if (FreeRTOS_send(xConnectedSocket, PuttyTcp.ReplyBuilder, PuttyTcp.ReplyLen, 0) <= 0) {
            task = SM_WAIT_CONNECT;
            break;
          }
          PuttyTcp.ReplyLen = 0;
        }
        if (PuttyTcp.CmdReady) {
          if (memcmp(PuttyTcp.CmdBuilder, TCP_PASSWORD, strlen(TCP_PASSWORD) - 1) == 0 && !BadUser) {
            task = SM_AUTHENTICATED;
            PuttyTcp.CmdPtr = 0;
            PuttyTcp.Cancel = 0;
            PuttyTcp.CmdReady = false;
            PuttyTcp.Hide = false;
            if (FreeRTOS_send_string(xConnectedSocket, "\r\nLogged in successfully\r\nEasyMix>", 0) <= 0) {
              task = SM_WAIT_CONNECT;
              break;
            }
            xIoTimeOut = 10; //Reset this
            FreeRTOS_setsockopt(xConnectedSocket, 0, FREERTOS_SO_RCVTIMEO,
                &xIoTimeOut,
                sizeof(xIoTimeOut));
          } else {
            if (FreeRTOS_send_string(xConnectedSocket, TCP_FAIL_LOGON_MSG, 0) <= 0) {
              task = SM_WAIT_CONNECT;
              break;
            }
            PuttyTcp.CmdPtr = 0;
            PuttyTcp.Cancel = 0;
            PuttyTcp.CmdReady = false;
            if (Tries++ < 3) {
              task = SM_PRINT_LOGIN;
            } else {
              task = SM_WAIT_CONNECT;
            }

            break;
          }
        }
      } else if (len == 0 || len == -pdFREERTOS_ERRNO_EWOULDBLOCK) {

      } else {
        task = SM_WAIT_CONNECT;
      }
      break;
    case SM_AUTHENTICATED:
      len = FreeRTOS_recv(xConnectedSocket, buff, sizeof(buff), 0);
      //		ClearWatchDog(WDT_TCP_PUTTY);
      if (len > 0) {
        PuttyRxHandler(buff, len, &PuttyTcp);
        if (PuttyTcp.ReplyLen) {
          if (FreeRTOS_send(xConnectedSocket, PuttyTcp.ReplyBuilder, PuttyTcp.ReplyLen, 0) <= 0) {
            task = SM_WAIT_CONNECT;
            break;
          }
          PuttyTcp.ReplyLen = 0;
        }

        if (PuttyTcp.CmdReady) {
          if (PuttyTcp.CmdPtr > 0) {
            if (FreeRTOS_send_string(xConnectedSocket, "\r\n", 0) <= 0) {
              task = SM_WAIT_CONNECT;
              break;
            }
            if (memcmp(PuttyTcp.CmdBuilder, "exit", PuttyTcp.CmdPtr) == 0) {
              task = SM_WAIT_CONNECT;
              break;
            }
            do {
              xMoreDataToFollow = FreeRTOS_CLIProcessCommand(PuttyTcp.CmdBuilder, buff, sizeof(buff), ctx);
              if (buff[0] != 0) {
                len = strlen(buff);
                err = FreeRTOS_send(xConnectedSocket, buff, len, 0);
                if (err != len) {
                  task = SM_WAIT_CONNECT;
                  break;
                }
              }
              vTaskDelay(1 / portTICK_PERIOD_MS);
              //				ClearWatchDog(WDT_TCP_PUTTY);
              if (xMoreDataToFollow != pdFALSE) {
                len = FreeRTOS_recv(xConnectedSocket, buff, sizeof(buff), 0);
                if (len > 0) {
                  PuttyRxHandler(buff, len, &PuttyTcp);
                } else if (len == 0 || len == -pdFREERTOS_ERRNO_EWOULDBLOCK) {

                } else {
                  task = SM_WAIT_CONNECT;
                  break;
                }
                if (PuttyTcp.ReplyLen) {
                  if (FreeRTOS_send(xConnectedSocket, PuttyTcp.ReplyBuilder, PuttyTcp.ReplyLen, 0) <= 0) {
                    task = SM_WAIT_CONNECT;
                    break;
                  }
                  PuttyTcp.ReplyLen = 0;
                }
                if (PuttyTcp.Cancel) {
                  ctx->cmdstate.Cancel = pdTRUE;
                }
              }
            } while (xMoreDataToFollow != pdFALSE); /* Until the command does not generate any more output. */
          }
          if (FreeRTOS_send_string(xConnectedSocket, "\r\nEasyMix>", 0) <= 0) {
            task = SM_WAIT_CONNECT;
            break;
          }
          ClearCommand(&PuttyTcp);
        }
        if (WasReset) {
          task = SM_WAIT_CONNECT;
        }
      } else {
        if (len == 0 || len == -pdFREERTOS_ERRNO_EWOULDBLOCK) {

        } else {
          task = SM_WAIT_CONNECT;
        }
      }
      break;
    }
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

void cmdtasks(void *pvParameters) {
  int i;
  static size_t xRxBytes;
  BaseType_t xMoreDataToFollow;
  CLI_Command_Context *ctx; //
  ctx = calloc(1, sizeof(CLI_Command_Context));
  configASSERT(ctx);
  ctx->cmdstate.Remote = 0;
  static char buff[1024];
  memset(buff, 0, sizeof(buff));
  PrintUart(
      "\x1b[2J\x1b[33m"
      "EasyMix Server 1.0\x1b[0m"
      "\r\nEasyMix>");
  //    ClearWatchDog(WDT_CMD);
  //    SetWatchDogTimeout(WDT_CMD, 60000, "Cmd");
  while (1) {
    //	ClearWatchDog(WDT_CMD);
    if (cmdLogStream) {
      xRxBytes = xStreamBufferReceive(cmdLogStream, (void *)buff, sizeof(buff) - 1, 0);
    }
    if (xRxBytes) {
      PrintUart("\r\x1b[32m");
      sendUartBytes(buff, xRxBytes);
      PrintUart("\x1b[0mEasyMix>");
      sendUartBytes(PuttyUart.CmdBuilder, PuttyUart.CmdPtr);
    }
    i = UartReadyBytes();
    if (i) {
      i = GetUartReadyBytes(buff);
      PuttyRxHandler(buff, i, &PuttyUart);
    }
    if (PuttyUart.ReplyLen) {
      sendUartBytes(PuttyUart.ReplyBuilder, PuttyUart.ReplyLen);
      PuttyUart.ReplyLen = 0;
    }
    
    if (PuttyUart.CmdReady) {
      if (PuttyUart.CmdPtr > 0) {
        PrintUart("\r\n");
        do {
          while (UartTxSpace() < sizeof(buff)) {
            vTaskDelay(1 / portTICK_PERIOD_MS);
          }
          xMoreDataToFollow = FreeRTOS_CLIProcessCommand(PuttyUart.CmdBuilder, buff, sizeof(buff), ctx);
          PrintUart(buff);
          vTaskDelay(1 / portTICK_PERIOD_MS);
          if (xMoreDataToFollow != pdFALSE) {
            i = UartReadyBytes();
            if (i) {
              i = GetUartReadyBytes(buff);
              PuttyRxHandler(buff, i, &PuttyUart);
            }
            //ClearWatchDog(WDT_CMD);
            vTaskDelay(1 / portTICK_PERIOD_MS);
            if (PuttyUart.ReplyLen) {
              sendUartBytes(PuttyUart.ReplyBuilder, PuttyUart.ReplyLen);
              PuttyUart.ReplyLen = 0;
            }
            if (PuttyUart.Cancel) {
              ctx->cmdstate.Cancel = pdTRUE;
            }
          }
        } while (xMoreDataToFollow != pdFALSE); /* Until the command does not generate any more output. */
      }
      PrintUart("\r\nEasyMix>");
      ClearCommand(&PuttyUart);
    }
    vTaskDelay(2 / portTICK_PERIOD_MS);
  }
}

void PuttyRxHandler(void *VoidRxPtr, int Length, _PuttyContext *ctx) {
  unsigned char *RxPtr = (unsigned char *)VoidRxPtr;
  int a;
  int ReplyPtr = 0;
  bool NormalChar = true;
  for (a = 0; a < Length; a++) {
    //Look for special codes
    switch (*RxPtr) {
    case CtrlE:
      strcpy(ctx->ReplyBuilder, "^E");
      return;
    case CtrlC:
      ctx->Cancel = true;
      strcpy(ctx->ReplyBuilder, "^C");
      return;
    case CtrlX:
      ctx->Cancel = true;
      strcpy(ctx->ReplyBuilder, "^X");
      return;
    case CtrlBackSpace:
      if (ctx->CmdPtr > 0) {
        ctx->CmdPtr--;
        ctx->ReplyBuilder[ReplyPtr++] = CtrlBackSpace;
      }
      NormalChar = false;
      break;
    case CtrlCR:
      ctx->CmdReady = true;
      ctx->CommandRecall = SAVED_CMD_LEN - 1;
      ctx->CmdBuilder[ctx->CmdPtr] = 0; //Null terminate
      return;                           //Toss remaining
    case CtrlEscape:
      ctx->EscapeSequence = 3;
      break;
    case '~':
      ctx->EscapeSequence = 4;
      break;
    default:
      if (*RxPtr >= ' ' && *RxPtr <= '}') {
        NormalChar = true;
      } else {
        NormalChar = false;
      }
      break;
    }
    if (ctx->EscapeSequence) {
      NormalChar = false;
      ctx->EscapeSequence--;
      if (ctx->EscapeSequence == 2 && *RxPtr != 0x1B) {
        ctx->EscapeSequence = 0; //Not a proper escape sequence
      }
      if (ctx->EscapeSequence == 1 && *RxPtr != 0x5B) {
        ctx->EscapeSequence = 0; //Not a proper escape sequence
      }
      if (ctx->EscapeSequence == 0) {
        switch (*RxPtr) {
        case 0x33: //Delete
        case 'D':  //Backspace equivalent
          if (ctx->CmdPtr > 0) {
            ctx->CmdPtr--;
            ctx->ReplyBuilder[ReplyPtr++] = CtrlBackSpace;
          }
          break;
        case 'A': //Up arrow
          if (++ctx->CommandRecall > SAVED_CMD_LEN - 1) {
            ctx->CommandRecall = 0;
          }
          if (ctx->CmdPtr > 0) {
            while (ctx->CmdPtr--) {
              ctx->ReplyBuilder[ReplyPtr++] = CtrlBackSpace;
            }
          }
          if (ctx->SavedCommands[ctx->CommandRecall].Length > 0 && ctx->SavedCommands[ctx->CommandRecall].Length <= 40) {
            memcpy(&ctx->ReplyBuilder[ReplyPtr], ctx->SavedCommands[ctx->CommandRecall].Command, ctx->SavedCommands[ctx->CommandRecall].Length);
            memcpy(ctx->CmdBuilder, ctx->SavedCommands[ctx->CommandRecall].Command, ctx->SavedCommands[ctx->CommandRecall].Length);
            ReplyPtr += ctx->SavedCommands[ctx->CommandRecall].Length;
            ctx->CmdPtr = ctx->SavedCommands[ctx->CommandRecall].Length;
          }
          break;
        case 'B': //Down arrow
          if (--ctx->CommandRecall < 0) {
            ctx->CommandRecall = SAVED_CMD_LEN - 1;
          }
          if (ctx->CmdPtr > 0) {
            while (ctx->CmdPtr--) {
              ctx->ReplyBuilder[ReplyPtr++] = CtrlBackSpace;
            }
          }
          if (ctx->SavedCommands[ctx->CommandRecall].Length > 0 && ctx->SavedCommands[ctx->CommandRecall].Length <= 40) {
            memcpy(&ctx->ReplyBuilder[ReplyPtr], ctx->SavedCommands[ctx->CommandRecall].Command, ctx->SavedCommands[ctx->CommandRecall].Length);
            memcpy(ctx->CmdBuilder, ctx->SavedCommands[ctx->CommandRecall].Command, ctx->SavedCommands[ctx->CommandRecall].Length);
            ReplyPtr += ctx->SavedCommands[ctx->CommandRecall].Length;
            ctx->CmdPtr = ctx->SavedCommands[ctx->CommandRecall].Length;
          }
          break;
        default: //No support for others
          break;
        }
      }
    }
    if (NormalChar) {
      ctx->CmdBuilder[ctx->CmdPtr++] = *RxPtr;
      if (ctx->Hide) {
        ctx->ReplyBuilder[ReplyPtr++] = '*';
      } else {
        ctx->ReplyBuilder[ReplyPtr++] = *RxPtr;
      }
      if (ctx->CmdPtr == 255) {
        ctx->CmdPtr = 0;
        break;
      }
    }
    RxPtr++;
  }
  ctx->ReplyLen = ReplyPtr;
}

static void ResetCommand(_PuttyContext *ctx) {
  ctx->EscapeSequence = 0;
  ctx->CommandRecall = (SAVED_CMD_LEN - 1);
  ctx->Cancel = false;
  ctx->CmdPtr = 0;
  memset(ctx->CmdBuilder, 0, CMD_BUILD_LEN);                         //No residual command pieces
  memset(ctx->ReplyBuilder, 0, CMD_BUILD_LEN);                       //No residual command pieces
  memset(ctx->SavedCommands, 0, sizeof(_CmdMemory) * SAVED_CMD_LEN); //No residual command pieces
  ctx->ReplyLen = 0;
  ctx->Hide = 0;
}

static void ClearCommand(_PuttyContext *ctx) {
  SaveCommand(ctx);
  memset(ctx->CmdBuilder, 0, CMD_BUILD_LEN); //No residual command pieces
  ctx->CmdPtr = 0;
  ctx->Cancel = 0;
  ctx->CmdReady = false;
}

static void SaveCommand(_PuttyContext *ctx) {
  int a;
  _CmdMemory *SavedCommands = ctx->SavedCommands;
  for (a = SAVED_CMD_LEN - 1; a > 0; a--) {
    memcpy((void *)&SavedCommands[a], (void *)&SavedCommands[a - 1], sizeof(_CmdMemory));
  }
  memcpy(SavedCommands[0].Command, ctx->CmdBuilder, 40);
  SavedCommands[0].Length = 0;
  unsigned char Length = 0;
  while (Length < 40) {
    if (SavedCommands[0].Command[Length] == 0x0D || SavedCommands[0].Command[Length] == 0) {
      break;
    }
    Length++;
  }
  SavedCommands[0].Length = Length;
}