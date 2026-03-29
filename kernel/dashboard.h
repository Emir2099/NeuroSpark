#ifndef DASHBOARD_H
#define DASHBOARD_H

typedef unsigned int uint32_t;

void draw_status_bar(void);
void draw_waveform(void);
void shell_render(void);
void neuro_task_entry(void);
void draw_neural_spike(int neuron_id, uint32_t intensity);

#define VIZ_MODE_OFF 0
#define VIZ_MODE_HEATMAP 1
#define VIZ_MODE_RASTER 2
#define VIZ_MODE_COMPARE 3

void viz_set_mode(int mode);
int viz_get_mode(void);
void viz_set_scrub(int index);
int viz_get_scrub(void);
int viz_set_compare_slots(int slot_a, int slot_b);
void viz_set_autoplay(int enabled);
int viz_get_autoplay(void);
void viz_step_scrub(int delta);

#endif
