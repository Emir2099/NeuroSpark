#include "audio.h"

typedef int int32_t;

#define AUDIO_MIX_RING 1024
#define AUDIO_MAX_VOLUME 100

#define AUDIO_CTL_STOP 1u
#define AUDIO_CTL_SET_VOLUME 2u
#define AUDIO_CTL_SET_FREQ 3u
#define AUDIO_CTL_SONIFY 4u

static AudioStream g_streams[AUDIO_MAX_STREAMS];
static int16_t g_mix_ring[AUDIO_MIX_RING];
static uint32_t g_mix_r = 0;
static uint32_t g_mix_w = 0;
static AudioStats g_stats;

extern uint32_t tick;

static int clamp_i32(int v, int lo, int hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return v;
}

static uint32_t ring_next(uint32_t idx, uint32_t cap) { return (idx + 1u) % cap; }

static int stream_ring_push(AudioStream *s, int16_t sample) {
  uint32_t next_w = ring_next(s->ring_w, AUDIO_RING_SAMPLES);
  if (next_w == s->ring_r) {
    return 0;
  }
  s->ring[s->ring_w] = sample;
  s->ring_w = next_w;
  s->produced_samples++;
  return 1;
}

static int stream_ring_pop(AudioStream *s, int16_t *out) {
  if (s->ring_r == s->ring_w) {
    return 0;
  }
  *out = s->ring[s->ring_r];
  s->ring_r = ring_next(s->ring_r, AUDIO_RING_SAMPLES);
  s->mixed_samples++;
  return 1;
}

static int mix_ring_push(int16_t sample) {
  uint32_t next_w = ring_next(g_mix_w, AUDIO_MIX_RING);
  if (next_w == g_mix_r) {
    g_stats.output_overruns++;
    return 0;
  }
  g_mix_ring[g_mix_w] = sample;
  g_mix_w = next_w;
  g_stats.mix_samples++;
  g_stats.last_mixed_sample = sample;
  return 1;
}

static int16_t generate_stream_sample(AudioStream *s) {
  int16_t sample = 0;
  if (s->phase_step == 0u) {
    return 0;
  }

  if (s->phase_accum < s->phase_step) {
    sample = 12000;
  } else {
    sample = -12000;
  }
  s->phase_accum++;
  if (s->phase_accum >= (s->phase_step << 1)) {
    s->phase_accum = 0;
  }
  return sample;
}

static uint32_t normalized_sample_rate(uint32_t sample_rate) {
  if (sample_rate < 50u) {
    return g_stats.tick_hz ? g_stats.tick_hz : 100u;
  }
  if (sample_rate > 192000u) {
    return 192000u;
  }
  return sample_rate;
}

static void stream_config_tone(AudioStream *s, uint32_t freq_hz, int volume_pct) {
  if (s == 0) {
    return;
  }

  if (freq_hz < 20u) {
    freq_hz = 20u;
  }
  if (freq_hz > 4000u) {
    freq_hz = 4000u;
  }

  s->mode = AUDIO_STREAM_MODE_TONE;
  s->volume_pct = clamp_i32(volume_pct, 0, AUDIO_MAX_VOLUME);
  s->freq_hz = freq_hz;
  s->sample_rate = normalized_sample_rate(s->sample_rate);
  s->channels = 1u;
  s->bits_per_sample = 16u;
  s->phase_accum = 0;
  s->phase_step = s->sample_rate / (freq_hz * 2u);
  if (s->phase_step == 0u) {
    s->phase_step = 1u;
  }
}

void audio_init(uint32_t tick_hz) {
  int i;

  if (tick_hz == 0) {
    tick_hz = 100;
  }

  g_stats.tick_hz = tick_hz;
  g_stats.active_streams = 0;
  g_stats.mix_samples = 0;
  g_stats.mix_underruns = 0;
  g_stats.output_overruns = 0;
  g_stats.last_mixed_sample = 0;
  g_stats.sonify_enabled = 0;
  g_stats.sonify_base_hz = 220;
  g_stats.sonify_span_hz = 880;

  g_mix_r = 0;
  g_mix_w = 0;

  for (i = 0; i < AUDIO_MAX_STREAMS; i++) {
    g_streams[i].id = i;
    g_streams[i].active = 0;
    g_streams[i].volume_pct = 60;
    g_streams[i].mode = AUDIO_STREAM_MODE_TONE;
    g_streams[i].freq_hz = 440;
    g_streams[i].sample_rate = tick_hz;
    g_streams[i].channels = 1;
    g_streams[i].bits_per_sample = 16;
    g_streams[i].flags = 0;
    g_streams[i].phase_accum = 0;
    g_streams[i].phase_step = 0;
    g_streams[i].ring_r = 0;
    g_streams[i].ring_w = 0;
    g_streams[i].underruns = 0;
    g_streams[i].produced_samples = 0;
    g_streams[i].mixed_samples = 0;
  }
}

static void refresh_sonify_stream(void) {
  uint32_t tone_seed;
  uint32_t freq;

  if (!g_stats.sonify_enabled) {
    return;
  }

  tone_seed = tick;
  freq = g_stats.sonify_base_hz +
      (tone_seed & 0xFFu) * g_stats.sonify_span_hz / 255u;

  if (!g_streams[0].active) {
    audio_play_tone(freq, 45);
  } else {
    g_streams[0].mode = AUDIO_STREAM_MODE_TONE;
    g_streams[0].freq_hz = freq;
    g_streams[0].phase_step = g_streams[0].sample_rate / (freq * 2u);
    if (g_streams[0].phase_step == 0u) {
      g_streams[0].phase_step = 1u;
    }
  }
}

void audio_tick(void) {
  int i;
  int32_t mixed = 0;
  int contributing = 0;

  refresh_sonify_stream();

  for (i = 0; i < AUDIO_MAX_STREAMS; i++) {
    AudioStream *s = &g_streams[i];
    int16_t sample;

    if (!s->active) {
      continue;
    }

    if (s->mode != AUDIO_STREAM_MODE_TONE) {
      continue;
    }

    sample = generate_stream_sample(s);
    if (!stream_ring_push(s, sample)) {
      s->underruns++;
      g_stats.mix_underruns++;
    }
  }

  for (i = 0; i < AUDIO_MAX_STREAMS; i++) {
    AudioStream *s = &g_streams[i];
    int16_t sample = 0;

    if (!s->active) {
      continue;
    }

    if (!stream_ring_pop(s, &sample)) {
      s->underruns++;
      g_stats.mix_underruns++;
      continue;
    }

    mixed += (int32_t)sample * s->volume_pct / AUDIO_MAX_VOLUME;
    contributing++;
  }

  if (contributing > 0) {
    mixed = clamp_i32(mixed, -32768, 32767);
    mix_ring_push((int16_t)mixed);
  }

  g_stats.active_streams = 0;
  for (i = 0; i < AUDIO_MAX_STREAMS; i++) {
    if (g_streams[i].active) {
      g_stats.active_streams++;
    }
  }
}

int audio_play_tone(uint32_t freq_hz, int volume_pct) {
  int i;

  for (i = 0; i < AUDIO_MAX_STREAMS; i++) {
    AudioStream *s = &g_streams[i];
    if (s->active) {
      continue;
    }

    s->active = 1;
    s->flags = 0;
    s->ring_r = 0;
    s->ring_w = 0;
    s->underruns = 0;
    s->produced_samples = 0;
    s->mixed_samples = 0;
    stream_config_tone(s, freq_hz, volume_pct);
    return s->id;
  }

  return -1;
}

int audio_stream_open(uint32_t sample_rate, uint32_t channels,
                      uint32_t bits_per_sample, uint32_t flags) {
  int i;

  if ((channels != 1u && channels != 2u) || bits_per_sample != 16u) {
    return -2;
  }

  for (i = 0; i < AUDIO_MAX_STREAMS; i++) {
    AudioStream *s = &g_streams[i];
    if (s->active) {
      continue;
    }

    s->active = 1;
    s->volume_pct = 80;
    s->mode = AUDIO_STREAM_MODE_PCM;
    s->freq_hz = 0;
    s->sample_rate = normalized_sample_rate(sample_rate);
    s->channels = channels;
    s->bits_per_sample = bits_per_sample;
    s->flags = flags;
    s->phase_accum = 0;
    s->phase_step = 0;
    s->ring_r = 0;
    s->ring_w = 0;
    s->underruns = 0;
    s->produced_samples = 0;
    s->mixed_samples = 0;
    return s->id;
  }

  return -1;
}

int audio_stream_write(int stream_id, const void *pcm, uint32_t bytes) {
  const uint8_t *src = (const uint8_t *)pcm;
  AudioStream *s;
  uint32_t frames;
  uint32_t written = 0;

  if (stream_id < 0 || stream_id >= AUDIO_MAX_STREAMS || pcm == 0 || bytes == 0u) {
    return -1;
  }

  s = &g_streams[stream_id];
  if (!s->active || s->mode != AUDIO_STREAM_MODE_PCM || s->bits_per_sample != 16u) {
    return -1;
  }

  if (s->channels == 0u) {
    return -1;
  }

  frames = bytes / (2u * s->channels);
  if (frames == 0u) {
    return 0;
  }

  for (uint32_t i = 0; i < frames; i++) {
    int16_t mixed;
    const int16_t *frame = (const int16_t *)(src + (i * s->channels * 2u));

    if (s->channels == 1u) {
      mixed = frame[0];
    } else {
      mixed = (int16_t)(((int32_t)frame[0] + (int32_t)frame[1]) / 2);
    }

    if (!stream_ring_push(s, mixed)) {
      break;
    }
    written++;
  }

  if (written == 0u && frames > 0u) {
    return -3;
  }

  return (int)(written * s->channels * 2u);
}

int audio_stop_stream(int stream_id) {
  if (stream_id < 0 || stream_id >= AUDIO_MAX_STREAMS) {
    return 0;
  }

  g_streams[stream_id].active = 0;
  g_streams[stream_id].mode = AUDIO_STREAM_MODE_TONE;
  g_streams[stream_id].freq_hz = 440;
  g_streams[stream_id].channels = 1;
  g_streams[stream_id].bits_per_sample = 16;
  g_streams[stream_id].flags = 0;
  g_streams[stream_id].phase_accum = 0;
  g_streams[stream_id].phase_step = 0;
  g_streams[stream_id].ring_r = 0;
  g_streams[stream_id].ring_w = 0;
  return 1;
}

void audio_stop_all(void) {
  int i;
  for (i = 0; i < AUDIO_MAX_STREAMS; i++) {
    audio_stop_stream(i);
  }
}

int audio_set_stream_volume(int stream_id, int volume_pct) {
  if (stream_id < 0 || stream_id >= AUDIO_MAX_STREAMS) {
    return 0;
  }

  g_streams[stream_id].volume_pct = clamp_i32(volume_pct, 0, 100);
  return 1;
}

int audio_set_stream_freq(int stream_id, uint32_t freq_hz) {
  AudioStream *s;

  if (stream_id < 0 || stream_id >= AUDIO_MAX_STREAMS) {
    return 0;
  }

  s = &g_streams[stream_id];
  if (!s->active) {
    return 0;
  }

  stream_config_tone(s, freq_hz, s->volume_pct);
  return 1;
}

int audio_stream_ctl(int stream_id, uint32_t cmd, uint32_t arg0, uint32_t arg1) {
  (void)arg1;

  if (cmd == AUDIO_CTL_SONIFY) {
    if (arg0 == 0u) {
      audio_sonify_set(0, g_stats.sonify_base_hz, g_stats.sonify_span_hz);
    } else {
      audio_sonify_set(1, g_stats.sonify_base_hz, g_stats.sonify_span_hz);
    }
    return 1;
  }

  if (stream_id < 0 || stream_id >= AUDIO_MAX_STREAMS || !g_streams[stream_id].active) {
    return 0;
  }

  if (cmd == AUDIO_CTL_STOP) {
    return audio_stop_stream(stream_id);
  }
  if (cmd == AUDIO_CTL_SET_VOLUME) {
    return audio_set_stream_volume(stream_id, (int)arg0);
  }
  if (cmd == AUDIO_CTL_SET_FREQ) {
    return audio_set_stream_freq(stream_id, arg0);
  }

  return 0;
}

int audio_get_streams(AudioStream *out_streams, int max_count) {
  int i;
  int copied = 0;

  if (out_streams == 0 || max_count <= 0) {
    return 0;
  }

  for (i = 0; i < AUDIO_MAX_STREAMS && copied < max_count; i++) {
    out_streams[copied++] = g_streams[i];
  }

  return copied;
}

void audio_get_stats(AudioStats *out_stats) {
  if (out_stats == 0) {
    return;
  }

  *out_stats = g_stats;
}

void audio_sonify_set(int enabled, uint32_t base_hz, uint32_t span_hz) {
  g_stats.sonify_enabled = enabled ? 1u : 0u;

  if (base_hz < 40) {
    base_hz = 40;
  }
  if (base_hz > 1200) {
    base_hz = 1200;
  }
  if (span_hz < 40) {
    span_hz = 40;
  }
  if (span_hz > 2000) {
    span_hz = 2000;
  }

  g_stats.sonify_base_hz = base_hz;
  g_stats.sonify_span_hz = span_hz;

  if (!g_stats.sonify_enabled) {
    audio_stop_stream(0);
  }
}
