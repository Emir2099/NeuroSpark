#ifndef AUDIO_AC97_H
#define AUDIO_AC97_H

#ifndef _UINT32_T_DEFINED
#define _UINT32_T_DEFINED
typedef unsigned int uint32_t;
#endif

typedef struct {
  uint32_t detected;
  uint32_t enabled;
  uint32_t bus;
  uint32_t slot;
  uint32_t function;
  uint32_t vendor;
  uint32_t device;
  uint32_t current_rate;
  uint32_t current_channels;
  uint32_t current_bits;
  uint32_t dma_frames;
  uint32_t dma_irqs;
  uint32_t loopback_runs;
  uint32_t loopback_fail;
  uint32_t reset_ok;
  uint32_t reset_fail;
  uint32_t last_error;
  uint32_t last_loopback_crc;
} AudioAc97Report;

void audio_ac97_driver_init(void);
int audio_ac97_is_ready(void);
int audio_ac97_negotiate_format(uint32_t sample_rate, uint32_t channels,
                                uint32_t bits_per_sample,
                                uint32_t *out_sample_rate,
                                uint32_t *out_channels,
                                uint32_t *out_bits_per_sample);
int audio_ac97_submit_frames(uint32_t frames);
int audio_ac97_reset(int inject_fault);
int audio_ac97_run_loopback_test(uint32_t frames, uint32_t sample_rate,
                                 uint32_t channels, uint32_t bits_per_sample,
                                 uint32_t *out_crc);
void audio_ac97_get_report(AudioAc97Report *out_report);

#endif