#ifndef DASHBOARD_H
#define DASHBOARD_H

typedef unsigned int uint32_t;

void draw_status_bar(void);
void draw_waveform(void);
void shell_render(void);
void neuro_task_entry(void);
void draw_neural_spike(int neuron_id, uint32_t intensity);

#endif
