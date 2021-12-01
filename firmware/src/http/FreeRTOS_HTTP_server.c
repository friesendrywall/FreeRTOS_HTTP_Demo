/*
 * FreeRTOS+TCP Labs Build 160919 (C) 2016 Real Time Engineers ltd.
 * Authors include Hein Tibosch and Richard Barry
 *
 *******************************************************************************
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 ***                                                                         ***
 ***                                                                         ***
 ***   FREERTOS+TCP IS STILL IN THE LAB (mainly because the FTP and HTTP     ***
 ***   demos have a dependency on FreeRTOS+FAT, which is only in the Labs    ***
 ***   download):                                                            ***
 ***                                                                         ***
 ***   FreeRTOS+TCP is functional and has been used in commercial products   ***
 ***   for some time.  Be aware however that we are still refining its       ***
 ***   design, the source code does not yet quite conform to the strict      ***
 ***   coding and style standards mandated by Real Time Engineers ltd., and  ***
 ***   the documentation and testing is not necessarily complete.            ***
 ***                                                                         ***
 ***   PLEASE REPORT EXPERIENCES USING THE SUPPORT RESOURCES FOUND ON THE    ***
 ***   URL: http://www.FreeRTOS.org/contact  Active early adopters may, at   ***
 ***   the sole discretion of Real Time Engineers Ltd., be offered versions  ***
 ***   under a license other than that described below.                      ***
 ***                                                                         ***
 ***                                                                         ***
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 *******************************************************************************
 *
 * FreeRTOS+TCP can be used under two different free open source licenses.  The
 * license that applies is dependent on the processor on which FreeRTOS+TCP is
 * executed, as follows:
 *
 * If FreeRTOS+TCP is executed on one of the processors listed under the Special
 * License Arrangements heading of the FreeRTOS+TCP license information web
 * page, then it can be used under the terms of the FreeRTOS Open Source
 * License.  If FreeRTOS+TCP is used on any other processor, then it can be used
 * under the terms of the GNU General Public License V2.  Links to the relevant
 * licenses follow:
 *
 * The FreeRTOS+TCP License Information Page: http://www.FreeRTOS.org/tcp_license
 * The FreeRTOS Open Source License: http://www.FreeRTOS.org/license
 * The GNU General Public License Version 2: http://www.FreeRTOS.org/gpl-2.0.txt
 *
 * FreeRTOS+TCP is distributed in the hope that it will be useful.  You cannot
 * use FreeRTOS+TCP unless you agree that you use the software 'as is'.
 * FreeRTOS+TCP is provided WITHOUT ANY WARRANTY; without even the implied
 * warranties of NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. Real Time Engineers Ltd. disclaims all conditions and terms, be they
 * implied, expressed, or statutory.
 *
 * 1 tab == 4 spaces!
 *
 * http://www.FreeRTOS.org
 * http://www.FreeRTOS.org/plus
 * http://www.FreeRTOS.org/labs
 *
 */

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

/* FreeRTOS Protocol includes. */
#include "http/FreeRTOS_HTTP_commands.h"
#include "http/FreeRTOS_TCP_server.h"
#include "http/FreeRTOS_server_private.h"

/* FreeRTOS+FAT includes. */
#include "global.h"
#include "http/httpROMFS.h"
//#include "configsave.h"
// #include "flashaddress.h"
#include "xmem.h"
#include "logs.h"
#include "debugUart.h"
//#include "wireshark.h"
// #include "circularflash.h"

#ifndef HTTP_SERVER_BACKLOG
#define HTTP_SERVER_BACKLOG (12)
#endif

#ifndef USE_HTML_CHUNKS
#define USE_HTML_CHUNKS (0)
#endif

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(x) (BaseType_t)(sizeof(x) / sizeof(x)[0])
#endif

/* Some defines to make the code more readbale */
#define pcCOMMAND_BUFFER pxClient->pxParent->pcCommandBuffer
#define pcNEW_DIR pxClient->pxParent->pcNewDir
#define pcFILE_BUFFER pxClient->pxParent->pcFileBuffer
#define SYS_FS_ATTR_ZIP_COMPRESSED ((uint16_t)0x0001)

#ifndef ipconfigHTTP_REQUEST_CHARACTER
#define ipconfigHTTP_REQUEST_CHARACTER '?'
#endif

/*_RB_ Need comment block, although fairly self evident. */
static void prvFileClose(HTTPClient_t *pxClient);
static BaseType_t prvProcessCmd(HTTPClient_t *pxClient, BaseType_t xIndex);
static const char *pcGetContentsType(const char *apFname);
static BaseType_t prvOpenURL(HTTPClient_t *pxClient);
static BaseType_t prvSendFile(HTTPClient_t *pxClient);
static BaseType_t prvSendReply(HTTPClient_t *pxClient, BaseType_t xCode, BaseType_t ContentLength);

static const char pcEmptyString[1] = { '\0' };

typedef struct xTYPE_COUPLE {
  const char *pcExtension;
  const char *pcType;
} TypeCouple_t;

static TypeCouple_t pxTypeCouples[] = {
  { "html", "text/html" },
  { "css", "text/css" },
  { "js", "text/javascript" },
  { "png", "image/png" },
  { "jpg", "image/jpeg" },
  { "svg", "image/svg+xml" },
  { "woff", "font/woff" },
  { "gif", "image/gif" },
  { "txt", "text/plain" },
  { "mp3", "audio/mpeg3" },
  { "wav", "audio/wav" },
  { "flac", "audio/ogg" },
  { "pdf", "application/pdf" },
  { "ttf", "application/x-font-ttf" },
  { "ttc", "application/x-font-ttf" },
  { "wav", "audio/x-wave" },
  { "aac", "audio/x-aac" },
  { "mp3", "audio/mp3" },
  { "raw", "application/octect-stream; charset=Windows-1252" },
  { "json", "application/json" },
  { "log", "application/force-download" },
  { "pcap", "application/force-download" },
  { "bmp", "application/force-download" },
  { "bin", "application/force-download" }
};

void vHTTPClientDelete(TCPClient_t *pxTCPClient) {
  HTTPClient_t *pxClient = (HTTPClient_t *)pxTCPClient;

  /* This HTTP client stops, close / release all resources. */
  if (pxClient->xSocket != FREERTOS_NO_SOCKET) {
    FreeRTOS_FD_CLR(pxClient->xSocket, pxClient->pxParent->xSocketSet, eSELECT_ALL);
    FreeRTOS_closesocket(pxClient->xSocket);
    pxClient->xSocket = FREERTOS_NO_SOCKET;
  }
  prvFileClose(pxClient);
}
/*-----------------------------------------------------------*/

static void prvFileClose(HTTPClient_t *pxClient) {
  if (pxClient->pxFileHandle != NULL) {
    //FreeRTOS_printf(("Closing file: %s\n", pxClient->pcCurrentFilename));
    yaromfs_fclose(pxClient->pxFileHandle);
    pxClient->pxFileHandle = NULL;
  } else if (pxClient->pxCustomHandle != NULL) {
    //FreeRTOS_printf(("Closing custom file: %s 0x%X\n", pxClient->pcCurrentFilename, (unsigned int)pxClient));
    x_free(pxClient->pxCustomHandle);
    pxClient->pxCustomHandle = NULL;
  }
}
/*-----------------------------------------------------------*/

static void prvPostClose(HTTPClient_t *pxClient) {
  if (pxClient->pxPostBuff != NULL) {
    x_free(pxClient->pxPostBuff);
    pxClient->pxPostBuff = NULL;
    pxClient->CustomBytes = 0;
    pxClient->State = 0;
  }
  prvFileClose(pxClient);
}
/*-----------------------------------------------------------*/

static BaseType_t prvSendReply(HTTPClient_t *pxClient, BaseType_t xCode, BaseType_t ContentLength) {
  struct xTCP_SERVER *pxParent = pxClient->pxParent;
  BaseType_t xRc;

  /* A normal command reply on the main socket (port 21). */
  char *pcBuffer = pxParent->pcFileBuffer;

  xRc = snprintf(pcBuffer, sizeof(pxParent->pcFileBuffer),
      "HTTP/1.1 %d %s\r\n"
      "%s"
#if USE_HTML_CHUNKS
      "Transfer-Encoding: chunked\r\n"
#endif
      "Content-Type: %s\r\n"
      "Content-Length: %i\r\n"
      "Connection: keep-alive\r\n\r\n",
      (int)xCode,
      webCodename(xCode),
      pxClient->Compressed ? "Content-Encoding: gzip\r\n" : "",
      pxParent->pcContentsType[0] ? pxParent->pcContentsType : "text/html",
      ContentLength);

  pxParent->pcContentsType[0] = '\0';
  pxParent->pcExtraContents[0] = '\0';

  xRc = FreeRTOS_send(pxClient->xSocket, (const void *)pcBuffer, xRc, 0);
  pxClient->bits.bReplySent = pdTRUE_UNSIGNED;

  return xRc;
}

static BaseType_t prvSendRedirect(HTTPClient_t *pxClient, BaseType_t xCode, const uint8_t * location) {
  struct xTCP_SERVER *pxParent = pxClient->pxParent;
  BaseType_t xRc;

  /* A normal command reply on the main socket (port 21). */
  char *pcBuffer = pxParent->pcFileBuffer;

  xRc = snprintf(pcBuffer, sizeof(pxParent->pcFileBuffer),
      "HTTP/1.1 %d %s\r\n"
      "Location: %s\r\n\r\n",
      (int)xCode,
      webCodename(xCode),
      location);

  pxParent->pcContentsType[0] = '\0';
  pxParent->pcExtraContents[0] = '\0';

  xRc = FreeRTOS_send(pxClient->xSocket, (const void *)pcBuffer, xRc, 0);
  pxClient->bits.bReplySent = pdTRUE_UNSIGNED;

  return xRc;
}

static BaseType_t prvSendPostReply(HTTPClient_t *pxClient, BaseType_t xCode, char *Response) {
  struct xTCP_SERVER *pxParent = pxClient->pxParent;
  BaseType_t xRc;

  /* A normal command reply on the main socket (port 21). */
  char *pcBuffer = pxParent->pcFileBuffer;
  //xRc = GetHeader(pcBuffer, Response, "text/event-stream");

  xRc = snprintf(pcBuffer, sizeof(pxParent->pcFileBuffer),
      "HTTP/1.1 %d %s\r\n"
      "Content-Type: text/html\r\n"
      "Connection: keep-alive\r\n"
      "Content-Length: %i\r\n"
      "\r\n\r\n%s\r\n",
      (int)xCode,
      webCodename(xCode),
      strlen(Response) + 2,
      Response);

  xRc = FreeRTOS_send(pxClient->xSocket, (const void *)pcBuffer, xRc, 0);
  pxClient->bits.bReplySent = pdTRUE_UNSIGNED;

  return xRc;
}

/*-----------------------------------------------------------*/

static BaseType_t prvSendFile(HTTPClient_t *pxClient) {
  size_t uxSpace;
  size_t uxCount;
  BaseType_t xRc = 0;

  if (pxClient->bits.bReplySent == pdFALSE_UNSIGNED) {
    pxClient->bits.bReplySent = pdTRUE_UNSIGNED;

    strcpy(pxClient->pxParent->pcContentsType, pcGetContentsType(yaromfs_contentType(pxClient->pxFileHandle)));
    //snprintf( pxClient->pxParent->pcExtraContents, sizeof( pxClient->pxParent->pcExtraContents ),
    //	"Content-Length: %d\r\n", ( int ) pxClient->uxBytesLeft );

    /* "Requested file action OK". */
    xRc = prvSendReply(pxClient, yaromfs_responseCode(pxClient->pxFileHandle), pxClient->uxBytesLeft);
  }

  if (xRc >= 0)
    do {
      uxSpace = FreeRTOS_tx_space(pxClient->xSocket);

      if (pxClient->uxBytesLeft < uxSpace) {
        uxCount = pxClient->uxBytesLeft;
      } else {
        uxCount = uxSpace;
      }

      if (uxCount > 0u) {
        if (uxCount > sizeof(pxClient->pxParent->pcFileBuffer)) {
          uxCount = sizeof(pxClient->pxParent->pcFileBuffer);
        }
        if (pxClient->pxFileHandle != NULL) {
          yaromfs_fread(pxClient->pxFileHandle, pxClient->pxParent->pcFileBuffer, uxCount);
        } else if (pxClient->pxCustomHandle != NULL) {
          memcpy(pxClient->pxParent->pcFileBuffer, &pxClient->pxCustomHandle[pxClient->CustomBytes], uxCount);
          pxClient->CustomBytes += uxCount;
        }
        pxClient->uxBytesLeft -= uxCount;

        xRc = FreeRTOS_send(pxClient->xSocket, pxClient->pxParent->pcFileBuffer, uxCount, 0);
        if (xRc < 0) {
          break;
        }
      }
    } while (uxCount > 0u);

  if (pxClient->uxBytesLeft == 0u) {
    /* Writing is ready, no need for further 'eSELECT_WRITE' events. */
    FreeRTOS_FD_CLR(pxClient->xSocket, pxClient->pxParent->xSocketSet, eSELECT_WRITE);
    prvFileClose(pxClient);
  } else {
    /* Wake up the TCP task as soon as this socket may be written to. */
    FreeRTOS_FD_SET(pxClient->xSocket, pxClient->pxParent->xSocketSet, eSELECT_WRITE);
  }

  return xRc;
}
/*-----------------------------------------------------------*/

static BaseType_t prvOpenURL(HTTPClient_t *pxClient) {
  BaseType_t xRc;
  char pcSlash[2];
  pxClient->bits.ulFlags = 0;

#if (ipconfigHTTP_HAS_HANDLE_REQUEST_HOOK != 0)
  {
    if (strchr(pxClient->pcUrlData, ipconfigHTTP_REQUEST_CHARACTER) != NULL) {
      size_t xResult;

      xResult = uxApplicationHTTPHandleRequestHook(pxClient->pcUrlData, pxClient->pcCurrentFilename, sizeof(pxClient->pcCurrentFilename));
      if (xResult > 0) {
        strcpy(pxClient->pxParent->pcContentsType, "text/html");
        snprintf(pxClient->pxParent->pcExtraContents, sizeof(pxClient->pxParent->pcExtraContents),
            "Content-Length: %d\r\n", (int)xResult);
        xRc = prvSendReply(pxClient, WEB_REPLY_OK); /* "Requested file action OK" */
        if (xRc > 0) {
          xRc = FreeRTOS_send(pxClient->xSocket, pxClient->pcCurrentFilename, xResult, 0);
        }
        /* Although against the coding standard of FreeRTOS, a return is
				done here  to simplify this conditional code. */
        return xRc;
      }
    }
  }
#endif /* ipconfigHTTP_HAS_HANDLE_REQUEST_HOOK */

  if (pxClient->pcUrlData[0] != '/') {
    /* Insert a slash before the file name. */
    pcSlash[0] = '/';
    pcSlash[1] = '\0';
  } else {
    /* The browser provided a starting '/' already. */
    pcSlash[0] = '\0';
  }

  pxClient->pxCustomHandle = NULL;
  snprintf(pxClient->pcCurrentFilename, sizeof(pxClient->pcCurrentFilename), "%s%s%s",
      pxClient->pcRootDir,
      pcSlash,
      strlen(pxClient->pcUrlData) > 1 ? pxClient->pcUrlData : "/index.html");
  //FreeRTOS_printf(("Open file try %s %s %s\r\n", pxClient->pcCurrentFilename, pxClient->pcRootDir, pxClient->pcUrlData));

  pxClient->pxFileHandle = yaromfs_fopen(
      pxClient->pcCurrentFilename,
      "GET",
      (uint8_t *)pxClient->pcRestData,
      (uint32_t)(pxClient->pcEndOfData - pxClient->pcRestData));

  if (pxClient->pxFileHandle == YAROMFS_HANDLE_INVALID) {
    /* "404 File not found". */
    xRc = prvSendReply(pxClient, WEB_NOT_FOUND, 0);
  } else if (pxClient->pxFileHandle->file->redirect) {
    xRc = prvSendRedirect(pxClient, WEB_REDIRECT, yaromfs_redirect(pxClient->pxFileHandle));
    prvFileClose(pxClient);
  } else {
    pxClient->uxBytesLeft = (size_t)yaromfs_f_length(pxClient->pxFileHandle);
    pxClient->Compressed = yaromfs_is_gz(pxClient->pxFileHandle);
    xRc = prvSendFile(pxClient);
  }

  return xRc;
}

/*-----------------------------------------------------------*/

void Reboot_Callback1000ms(uintptr_t context, uint32_t currTick) {
  SysLog(LOG_DEBUG, "SoftReset()\r\n");
  while (uartTxInProgress())
    ;
  NVIC_SystemReset();
}

static BaseType_t prvHandlePost(HTTPClient_t *pxClient) {
  BaseType_t xRc = 0;
  BaseType_t xPreservePost;
  YAROMFSFILE_HANDLE fh;
  pxClient->pxFileHandle = yaromfs_fopen(
      pxClient->pcCurrentFilename,
      "POST",
      pxClient->pxPostBuff,
      pxClient->CustomBytes);
  if (YAROMFS_HANDLE_INVALID != pxClient->pxFileHandle) {
    pxClient->uxBytesLeft = (size_t)yaromfs_f_length(pxClient->pxFileHandle);
    pxClient->Compressed = yaromfs_is_gz(pxClient->pxFileHandle);
    xPreservePost = yaromfs_preservePost(pxClient->pxFileHandle);
    xRc = prvSendFile(pxClient);
    if (pxClient->pxPostBuff != NULL) {
      //Check if handler is reqesting to handle freeing the memory block passed to it.
      if (!xPreservePost) {
        x_free(pxClient->pxPostBuff);
      }
      pxClient->pxPostBuff = NULL;
      pxClient->CustomBytes = 0;
      pxClient->State = 0;
      //prvPostClose(pxClient);
    }
  } else {
    FreeRTOS_printf(("No Post handler found for %s\n", pxClient->pcCurrentFilename));
    prvSendPostReply(pxClient, WEB_NOT_FOUND, "No file found");
    prvPostClose(pxClient);
    xRc = 0;
  }
  return xRc;
}

static BaseType_t prvGetPost(HTTPClient_t *pxClient) {
  BaseType_t xRc = pdFALSE;
  if (pxClient->State == 0) {
    //Parse http info
    if (!pxClient->pcRestData) {
      //TODO: fail here
      FreeRTOS_printf(("No rest data\n"));
      prvPostClose(pxClient);
      return 0;
    }
    //FreeRTOS_printf(("%s\r\n",pxClient->pcRestData));
    char *StartPointer = strstr(pxClient->pcRestData, "\r\n\r\n");
    char *LengthPointer = strstr(pxClient->pcRestData, "Content-Length: ");
    if (LengthPointer != NULL && StartPointer != NULL) {
      StartPointer += 4;
      LengthPointer += 16;
      int ContentLength = strtol(LengthPointer, NULL, 10);
      int ActualLength = pxClient->pcEndOfData - StartPointer;
      if (ActualLength < 0) {
        ActualLength = 0;
      }
      if (ContentLength > pxClient->MaxPostLength || ActualLength > pxClient->MaxPostLength) {
        FreeRTOS_printf(("POST Content length error %i\n", ContentLength));
        prvSendPostReply(pxClient, WEB_BAD_REQUEST, "POST too long");
        prvPostClose(pxClient);
        return 0;
      }
      pxClient->uxBytesLeft = ContentLength;
      if (ActualLength < ContentLength) {
        if (ActualLength > 0) {
          memcpy(pxClient->pxPostBuff, StartPointer, ActualLength);
        }
        pxClient->State = 1;
        pxClient->CustomBytes = ActualLength;
        pxClient->uxBytesLeft -= ActualLength;
        return ActualLength;
      } else {
        memcpy(pxClient->pxPostBuff, StartPointer, ContentLength);
        pxClient->CustomBytes = ContentLength;
        xRc = prvHandlePost(pxClient);
        return xRc;
      }
    } else {
      FreeRTOS_printf(("Missing length and start\n"));
      prvPostClose(pxClient);
      return 0;
    }
  } else {
    xRc = FreeRTOS_recv(pxClient->xSocket, (void *)&pxClient->pxPostBuff[pxClient->CustomBytes], pxClient->uxBytesLeft, 0);
    if (xRc > 0) {
      pxClient->CustomBytes += xRc;
      pxClient->uxBytesLeft -= xRc;
    }
    if (pxClient->uxBytesLeft == 0) {
      xRc = prvHandlePost(pxClient);
    }
  }
  return xRc;
}

static BaseType_t prvOpenPostUrl(HTTPClient_t *pxClient) {
  BaseType_t xRc;
  char pcSlash[2];
  pxClient->bits.ulFlags = 0;

  if (pxClient->pcUrlData[0] != '/') {
    /* Insert a slash before the file name. */
    pcSlash[0] = '/';
    pcSlash[1] = '\0';
  } else {
    /* The browser provided a starting '/' already. */
    pcSlash[0] = '\0';
  }
  snprintf(pxClient->pcCurrentFilename, sizeof(pxClient->pcCurrentFilename), "%s%s",
      pcSlash,
      pxClient->pcUrlData);
  pxClient->CustomBytes = 0;

  pxClient->MaxPostLength = yaromfs_postExists(pxClient->pcCurrentFilename);
  if (pxClient->MaxPostLength) {
    pxClient->pxPostBuff = x_malloc(pxClient->MaxPostLength + 64, XMEM_HEAP_SLOW | XMEM_HEAP_ALIGN32 | XMEM_HEAP_SKIP_HOOK);
    if (pxClient->pxPostBuff == NULL) {
      xRc = prvSendPostReply(pxClient, WEB_INTERNAL_SERVER_ERROR, "Insufficient memory");
    } else {
      xRc = prvGetPost(pxClient);
    }
  } else {
    xRc = prvSendPostReply(pxClient, WEB_NOT_FOUND, pxClient->pcCurrentFilename);
  }
  return xRc;
}
/*-----------------------------------------------------------*/
static BaseType_t prvProcessCmd(HTTPClient_t *pxClient, BaseType_t xIndex) {
  BaseType_t xResult = 0;

  /* A new command has been received. Process it. */
  switch (xIndex) {
  case ECMD_GET:
    xResult = prvOpenURL(pxClient);
    break;
  case ECMD_POST:
    xResult = prvOpenPostUrl(pxClient);
    break;
  case ECMD_HEAD:
  case ECMD_PUT:
  case ECMD_DELETE:
  case ECMD_TRACE:
  case ECMD_OPTIONS:
  case ECMD_CONNECT:
  case ECMD_PATCH:
  case ECMD_UNK: {
    FreeRTOS_printf(("prvProcessCmd: Not implemented: %s\n",
        xWebCommands[xIndex].pcCommandName));
  } break;
  }

  return xResult;
}
/*-----------------------------------------------------------*/

BaseType_t xHTTPClientWork(TCPClient_t *pxTCPClient) {
  BaseType_t xRc;
  HTTPClient_t *pxClient = (HTTPClient_t *)pxTCPClient;

  if (!pxClient->ClientInitialized) {
    pxClient->ClientInitialized = 1;
    pxClient->pxFileHandle = YAROMFS_HANDLE_INVALID;
    pxClient->pxCustomHandle = NULL;
  }

  //The following handles completion of file sending.
  if (pxClient->pxFileHandle != YAROMFS_HANDLE_INVALID || pxClient->pxCustomHandle != NULL) {
    xRc = prvSendFile(pxClient);
    return xRc;
  }

  if (pxClient->pxPostBuff != NULL) {
    xRc = prvGetPost(pxClient);
    return xRc;
  } else {
    xRc = FreeRTOS_recv(pxClient->xSocket, (void *)pcCOMMAND_BUFFER, sizeof(pcCOMMAND_BUFFER), 0);
  }

  if (xRc > 0 && pxClient->pxPostBuff == NULL) {
    BaseType_t xIndex;
    //const char *pcEndOfCmd;
    const struct xWEB_COMMAND *curCmd;
    char *pcBuffer = pcCOMMAND_BUFFER;

    if (xRc < (BaseType_t)sizeof(pcCOMMAND_BUFFER)) {
      pcBuffer[xRc] = '\0';
    }
    pxClient->pcEndOfData = pcBuffer + xRc;
    while (xRc && (pcBuffer[xRc - 1] == 13 || pcBuffer[xRc - 1] == 10)) {
      --xRc; //pcBuffer[ --xRc ] = '\0';
    }
    pxClient->pcEndOfCmd = pcBuffer + xRc;

    curCmd = xWebCommands;

    /* Pointing to "/index.html HTTP/1.1". */
    pxClient->pcUrlData = pcBuffer;

    /* Pointing to "HTTP/1.1". */
    pxClient->pcRestData = pcEmptyString;

    /* Last entry is "ECMD_UNK". */
    for (xIndex = 0; xIndex < WEB_CMD_COUNT - 1; xIndex++, curCmd++) {
      BaseType_t xLength;

      xLength = curCmd->xCommandLength;
      if ((xRc >= xLength) && (memcmp(curCmd->pcCommandName, pcBuffer, xLength) == 0)) {
        char *pcLastPtr;

        pxClient->pcUrlData += xLength + 1;
        for (pcLastPtr = (char *)pxClient->pcUrlData; pcLastPtr < pxClient->pcEndOfCmd; pcLastPtr++) {
          char ch = *pcLastPtr;
          if ((ch == '\0') || (strchr("\n\r \t", ch) != NULL)) {
            *pcLastPtr = '\0';
            pxClient->pcRestData = pcLastPtr + 1;
            break;
          }
        }
        break;
      }
    }

    if (xIndex < (WEB_CMD_COUNT - 1)) {
      xRc = prvProcessCmd(pxClient, xIndex);
    }
  } else if (xRc < 0) {
    /* The connection will be closed and the client will be deleted. */
    FreeRTOS_printf(("xHTTPClientWork: rc = %ld\n", xRc));
  }
  return xRc;
}
/*-----------------------------------------------------------*/

static const char *pcGetContentsType(const char *apFname) {
  const char *slash = NULL;
  const char *dot = NULL;
  const char *ptr;
  const char *pcResult = "text/html";
  BaseType_t x;

  dot++;
  for (x = 0; x < ARRAY_SIZE(pxTypeCouples); x++) {
    if (strcasecmp(apFname, pxTypeCouples[x].pcExtension) == 0) {
      pcResult = pxTypeCouples[x].pcType;
      break;
    }
  }
  return pcResult;
}