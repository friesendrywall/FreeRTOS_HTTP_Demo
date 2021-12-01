#ifndef MEM_SYNC_H
#define MEM_SYNC_H
#include <stdint.h>
#include "configuration.h"

enum {
  MEM_SYNC_FADERS,
  MEM_SYNC_ALTFADER1,
  MEM_SYNC_ALTFADER2,
  MEM_SYNC_ALTFADER3,
  MEM_SYNC_ALTTOGGLE,
  MEM_SYNC_MUTE,
  MEM_SYNC_METER1,
  MEM_SYNC_METER2,
  MEM_SYNC_HG,
  MEM_SYNC_CONTROL,
  MEM_SYNC_LENGTH
};

enum {
  MEM_SYNC_DSP_EVENT_DISCONNECT,
  MEM_SYNC_DSP_EVENT_CONNECTING,
  MEM_SYNC_DSP_EVENT_CONNECTED,
  MEM_SYNC_DSP_EVENT_NO_DSP
};

/* Internal structures */
typedef uint16_t emValue;

typedef struct {
  double dspDbValue;
  emValue value;
  //Flags
  uint16_t active : 1;
  // syncedDsp no outbound items until DSP has pushed
  uint16_t syncedDsp : 1;
} internal_value;

typedef struct {
  internal_value row[CONFIG_LAYER_FADERS];
} internal_set;

typedef union {
  struct {
    internal_set all[MEM_SYNC_LENGTH];
  };
  struct {
    internal_value faders[CONFIG_LAYER_FADERS];
    internal_value altFader1[CONFIG_LAYER_FADERS];
    internal_value altFader2[CONFIG_LAYER_FADERS];
    internal_value altFader3[CONFIG_LAYER_FADERS];
    internal_value altToggle[CONFIG_LAYER_FADERS];
    internal_value mute[CONFIG_LAYER_FADERS];
    internal_value meter1[CONFIG_LAYER_FADERS];
    internal_value meter2[CONFIG_LAYER_FADERS];
    internal_value hg[CONFIG_LAYER_FADERS];
    internal_value control[CONFIG_CONTROL_COUNT];
  };
} em_int_values;

uint32_t syncInternalExternal(uint32_t sync_item, uint32_t index, emValue value);
void syncExternalInternal(uint32_t sync_item, uint32_t index, emValue value, double optVal);
void sendDspEvent(uint32_t event);
void resetDspErrors(void);
void setDspError(uint32_t index, uint32_t value);
void setDspInfo(uint8_t * str);

typedef uint32_t (*external_event_cb)(uint32_t sync_item, uint32_t index, emValue value);
typedef void (*internal_event_cb)(uint32_t sync_item, uint32_t index, emValue value, double optVal);
typedef void (*dsp_event_cb)(uint32_t event);

void registerInternalCallback(internal_event_cb cb);
void registerExternalCallback(external_event_cb cb);
void registerDspEvent(dsp_event_cb cb);

/* external dsp interfaces */

//typedef uint32_t (*dsp_task)(void);

typedef struct {
  /* incoming commands */
  void (*openDSP)(uint32_t ip, uint16_t port, uint32_t comType);
  void (*restartDSP)(uint32_t ip, uint16_t port, uint32_t comType);
  void (*closeDSP)(void);
  void (*swapSocket)(void);
  uint32_t (*em_onvalue_changed)(uint32_t sync_item, uint32_t index, emValue value);
} dsp_interface;

typedef struct {
  uint32_t sync_item;
  uint32_t index;
  emValue value;
} MEM_EVENT;

#define MEM_QUEUE_SIZE 64
#define DSP_ERROR_DELAY 30000
#define EM_OUT_MAX_BLOCK_TIME 50

#define PING_TRACK -2
#define NO_TRACK -1
#define AckTrackBits 8
#define AckTrackCount (1 << AckTrackBits)

typedef struct {
  uint32_t HeadPtr : AckTrackBits;
  uint32_t TailPtr : AckTrackBits;
  int32_t Track[AckTrackCount];
  uint32_t t_time[AckTrackCount];
} _AckTrack;

enum {
  MEM_SYNC_SITE_OK,
  MEM_SYNC_SITE_ERROR,
  MEM_SYNC_SITE_WARN,
  MEM_SYNC_SITE_UNKOWN,
  MEM_SYNC_SITE_DUPLICATE
};

#endif