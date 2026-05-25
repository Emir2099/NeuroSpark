#ifndef AUDIO_H
#define AUDIO_H

#ifndef _INT16_T_DEFINED
#define _INT16_T_DEFINED
typedef signed short int16_t;
#endif
#ifndef _UINT8_T_DEFINED
#define _UINT8_T_DEFINED
typedef unsigned char uint8_t;
#endif
#ifndef _UINT32_T_DEFINED
#define _UINT32_T_DEFINED
typedef unsigned int uint32_t;
#endif

#define AUDIO_MAX_STREAMS 8
#define AUDIO_RING_SAMPLES 512

#define AUDIO_STREAM_MODE_TONE 1u
#define AUDIO_STREAM_MODE_PCM 2u
#define AUDIO_STREAM_MODE_CAPTURE 3u

#define AUDIO_STREAM_FLAG_CAPTURE 0x1u

#define AUDIO_CTL_STOP 1u
#define AUDIO_CTL_SET_VOLUME 2u
#define AUDIO_CTL_SET_FREQ 3u
#define AUDIO_CTL_SONIFY 4u
#define AUDIO_CTL_SET_PRIORITY 5u
#define AUDIO_CTL_SET_RATE_LIMIT 6u

typedef struct {
  int id;
  int active;
  int volume_pct;
  uint32_t mode;
  uint32_t freq_hz;
  uint32_t sample_rate;
  uint32_t channels;
  uint32_t bits_per_sample;
  uint32_t flags;
  uint32_t priority_qos;
  uint32_t max_samples_per_tick;
  uint32_t phase_accum;
  uint32_t phase_step;
  int16_t ring[AUDIO_RING_SAMPLES];
  uint32_t ring_r;
  uint32_t ring_w;
  uint32_t underruns;
  uint32_t policy_drops;
  uint32_t produced_samples;
  uint32_t mixed_samples;
} AudioStream;

typedef struct {
  uint32_t tick_hz;
  uint32_t active_streams;
  uint32_t mix_samples;
  uint32_t mix_underruns;
  uint32_t output_overruns;
  int16_t last_mixed_sample;
  uint32_t sonify_enabled;
  uint32_t sonify_base_hz;
  uint32_t sonify_span_hz;
} AudioStats;

void audio_init(uint32_t tick_hz);
void audio_tick(void);

int audio_play_tone(uint32_t freq_hz, int volume_pct);
int audio_stream_open(uint32_t sample_rate, uint32_t channels,
                      uint32_t bits_per_sample, uint32_t flags);
int audio_stream_write(int stream_id, const void *pcm, uint32_t bytes);
int audio_stream_read(int stream_id, void *pcm, uint32_t bytes);
int audio_stream_ctl(int stream_id, uint32_t cmd, uint32_t arg0,
                     uint32_t arg1);
int audio_stop_stream(int stream_id);
void audio_stop_all(void);
int audio_set_stream_volume(int stream_id, int volume_pct);
int audio_set_stream_freq(int stream_id, uint32_t freq_hz);

int audio_get_streams(AudioStream *out_streams, int max_count);
void audio_get_stats(AudioStats *out_stats);

void audio_sonify_set(int enabled, uint32_t base_hz, uint32_t span_hz);

#endif
