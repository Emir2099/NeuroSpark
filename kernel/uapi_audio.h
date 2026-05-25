#ifndef UAPI_AUDIO_H
#define UAPI_AUDIO_H

typedef unsigned int uint32_t;

#define AUDIO_CTL_STOP 1u
#define AUDIO_CTL_SET_VOLUME 2u
#define AUDIO_CTL_SET_FREQ 3u
#define AUDIO_CTL_SONIFY 4u
#define AUDIO_CTL_SET_PRIORITY 5u
#define AUDIO_CTL_SET_RATE_LIMIT 6u

typedef struct {
  uint32_t sample_rate;
  uint32_t channels;
  uint32_t bits_per_sample;
  uint32_t flags;
} uapi_audio_open_t;

typedef struct {
  int stream_id;
  uint32_t cmd;
  uint32_t arg0;
  uint32_t arg1;
} uapi_audio_ctl_t;

#endif
