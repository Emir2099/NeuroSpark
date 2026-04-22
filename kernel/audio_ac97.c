#include "audio_ac97.h"

#include "audio.h"
#include "pci.h"

typedef signed short int16_t;
typedef int int32_t;

#define AC97_ERR_NONE 0u
#define AC97_ERR_NO_DEVICE 1u
#define AC97_ERR_FORMAT_NEGOTIATED 2u
#define AC97_ERR_FORMAT_UNSUPPORTED 3u
#define AC97_ERR_LOOPBACK_IO 4u
#define AC97_ERR_RESET_INJECTED 5u

static AudioAc97Report g_ac97;

static int is_supported_rate(uint32_t hz) {
  return hz == 8000u || hz == 11025u || hz == 16000u || hz == 22050u ||
         hz == 32000u || hz == 44100u || hz == 48000u;
}

static uint32_t abs_delta_u32(uint32_t a, uint32_t b) {
  return (a > b) ? (a - b) : (b - a);
}

static uint32_t nearest_rate(uint32_t hz) {
  static const uint32_t k_rates[7] = {8000u, 11025u, 16000u, 22050u,
                                      32000u, 44100u, 48000u};
  uint32_t best = k_rates[0];
  uint32_t best_delta = abs_delta_u32(hz, best);
  int i;

  for (i = 1; i < 7; i++) {
    uint32_t cand = k_rates[i];
    uint32_t delta = abs_delta_u32(hz, cand);
    if (delta < best_delta) {
      best = cand;
      best_delta = delta;
    }
  }

  return best;
}

static uint32_t crc32ish_mix(uint32_t crc, uint16_t v) {
  crc ^= (uint32_t)v;
  crc = (crc << 5) | (crc >> 27);
  crc ^= 0xA5A5A5A5u;
  return crc;
}

void audio_ac97_driver_init(void) {
  int i;

  g_ac97.detected = 0;
  g_ac97.enabled = 0;
  g_ac97.bus = 0;
  g_ac97.slot = 0;
  g_ac97.function = 0;
  g_ac97.vendor = 0;
  g_ac97.device = 0;
  g_ac97.current_rate = 44100u;
  g_ac97.current_channels = 2u;
  g_ac97.current_bits = 16u;
  g_ac97.last_error = AC97_ERR_NO_DEVICE;

  for (i = 0; i < pci_found_count; i++) {
    PciDevice *dev = &pci_found[i];
    if (dev->class_code == 0x04 && dev->subclass == 0x01) {
      g_ac97.detected = 1;
      g_ac97.enabled = 1;
      g_ac97.bus = dev->bus;
      g_ac97.slot = dev->slot;
      g_ac97.function = dev->function;
      g_ac97.vendor = dev->vendor;
      g_ac97.device = dev->device_id;
      g_ac97.last_error = AC97_ERR_NONE;
      return;
    }
  }
}

int audio_ac97_is_ready(void) { return g_ac97.enabled ? 1 : 0; }

int audio_ac97_negotiate_format(uint32_t sample_rate, uint32_t channels,
                                uint32_t bits_per_sample,
                                uint32_t *out_sample_rate,
                                uint32_t *out_channels,
                                uint32_t *out_bits_per_sample) {
  uint32_t negotiated_rate = sample_rate;

  if (sample_rate < 8000u || sample_rate > 48000u) {
    g_ac97.last_error = AC97_ERR_FORMAT_UNSUPPORTED;
    return 0;
  }
  if ((channels != 1u && channels != 2u) || bits_per_sample != 16u) {
    g_ac97.last_error = AC97_ERR_FORMAT_UNSUPPORTED;
    return 0;
  }

  if (!is_supported_rate(sample_rate)) {
    negotiated_rate = nearest_rate(sample_rate);
    g_ac97.last_error = AC97_ERR_FORMAT_NEGOTIATED;
  } else {
    g_ac97.last_error = g_ac97.enabled ? AC97_ERR_NONE : AC97_ERR_NO_DEVICE;
  }

  g_ac97.current_rate = negotiated_rate;
  g_ac97.current_channels = channels;
  g_ac97.current_bits = bits_per_sample;

  if (out_sample_rate != 0) {
    *out_sample_rate = negotiated_rate;
  }
  if (out_channels != 0) {
    *out_channels = channels;
  }
  if (out_bits_per_sample != 0) {
    *out_bits_per_sample = bits_per_sample;
  }
  return 1;
}

int audio_ac97_submit_frames(uint32_t frames) {
  if (frames == 0u) {
    return 1;
  }

  g_ac97.dma_frames += frames;
  if (g_ac97.dma_frames >= ((g_ac97.dma_irqs + 1u) * 256u)) {
    g_ac97.dma_irqs++;
  }
  return 1;
}

int audio_ac97_reset(int inject_fault) {
  if (inject_fault) {
    g_ac97.enabled = 0;
    g_ac97.reset_fail++;
    g_ac97.last_error = AC97_ERR_RESET_INJECTED;
    return 0;
  }

  if (!g_ac97.detected) {
    g_ac97.enabled = 0;
    g_ac97.reset_fail++;
    g_ac97.last_error = AC97_ERR_NO_DEVICE;
    return 0;
  }

  g_ac97.enabled = 1;
  g_ac97.reset_ok++;
  g_ac97.last_error = AC97_ERR_NONE;
  return 1;
}

int audio_ac97_run_loopback_test(uint32_t frames, uint32_t sample_rate,
                                 uint32_t channels, uint32_t bits_per_sample,
                                 uint32_t *out_crc) {
  int stream_id;
  uint32_t remaining = frames;
  uint32_t crc = 0x1234ACE1u;
  int16_t chunk[128 * 2];
  uint32_t phase = 0;

  if (frames == 0u) {
    frames = 1024u;
    remaining = frames;
  }

  if (!audio_ac97_negotiate_format(sample_rate, channels, bits_per_sample,
                                   &sample_rate, &channels,
                                   &bits_per_sample)) {
    g_ac97.loopback_runs++;
    g_ac97.loopback_fail++;
    return 0;
  }

  stream_id = audio_stream_open(sample_rate, channels, bits_per_sample, 0);
  if (stream_id < 0) {
    g_ac97.last_error = AC97_ERR_LOOPBACK_IO;
    g_ac97.loopback_runs++;
    g_ac97.loopback_fail++;
    return 0;
  }

  while (remaining > 0u) {
    uint32_t frames_now = remaining > 128u ? 128u : remaining;
    uint32_t sample_count = frames_now * channels;
    uint32_t bytes = sample_count * 2u;
    uint32_t i;
    int wrote;
    uint32_t wrote_frames;
    int16_t lo = -11000;
    int16_t hi = 11000;

    for (i = 0; i < sample_count; i++) {
      int32_t is_hi = ((phase >> 4) & 1u) ? 1 : 0;
      chunk[i] = (int16_t)(is_hi ? hi : lo);
      phase++;
      crc = crc32ish_mix(crc, (uint16_t)chunk[i]);
    }

    wrote = audio_stream_write(stream_id, chunk, bytes);
    if (wrote <= 0) {
      audio_stop_stream(stream_id);
      g_ac97.last_error = AC97_ERR_LOOPBACK_IO;
      g_ac97.loopback_runs++;
      g_ac97.loopback_fail++;
      return 0;
    }

    wrote_frames = (uint32_t)wrote / (channels * 2u);
    if (wrote_frames == 0u) {
      audio_stop_stream(stream_id);
      g_ac97.last_error = AC97_ERR_LOOPBACK_IO;
      g_ac97.loopback_runs++;
      g_ac97.loopback_fail++;
      return 0;
    }

    audio_ac97_submit_frames(wrote_frames);
    remaining -= wrote_frames;
  }

  audio_stop_stream(stream_id);
  g_ac97.loopback_runs++;
  g_ac97.last_loopback_crc = crc;
  if (out_crc != 0) {
    *out_crc = crc;
  }
  return 1;
}

void audio_ac97_get_report(AudioAc97Report *out_report) {
  if (out_report == 0) {
    return;
  }
  *out_report = g_ac97;
}