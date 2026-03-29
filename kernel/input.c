#include "input.h"

typedef unsigned char uint8_t;

extern volatile char input_buffer[32];
extern volatile int buffer_idx;
extern volatile int shell_dirty;
extern volatile char kb_buf[32];
extern volatile int kb_head;
extern volatile int kb_tail;

extern void process_command(char *cmd);
extern void switch_tasks(void);
extern void sys_save_task(int task_id, int slot);
extern int sys_load_task(int task_id, int slot);

typedef struct {
  int voltage;
  int spike_count;
  int id;
  int synaptic_weight;
  int refractory_timer;
  int dynamic_threshold;
} Neuron;

typedef struct {
  Neuron neurons[5];
  unsigned char current_phase;
  int pixel_recent_spikes;
} NeuralPixel;

extern NeuralPixel os_memory_map[2];

static char get_ascii(uint8_t scancode) {
  static char map[0x80] = {
      0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
      '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
      'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
      'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
      'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' '};
  return map[scancode];
}

void keyboard_handler(void) {
  uint8_t scancode = 0;
  __asm__ volatile("inb $0x60, %0" : "=a"(scancode));

  if (scancode >= 0x80)
    return;

  char c = get_ascii(scancode);

  if (scancode == 0x1C) {
    input_buffer[buffer_idx] = '\0';
    char cmd_copy[32];
    for (int i = 0; i <= buffer_idx && i < 32; i++)
      cmd_copy[i] = input_buffer[i];
    cmd_copy[31] = '\0';
    process_command(cmd_copy);
    buffer_idx = 0;
    shell_dirty = 1;
    return;
  }

  if (scancode == 0x0E && buffer_idx > 0) {
    buffer_idx--;
    shell_dirty = 1;
    return;
  }

  if (scancode == 0x3B) {
    for (int n = 0; n < 5; n++)
      os_memory_map[0].neurons[n].voltage += 300;
    return;
  }

  if (scancode == 0x3C) {
    switch_tasks();
    return;
  }

  if (scancode == 0x3D) {
    for (int n = 0; n < 5; n++)
      os_memory_map[1].neurons[n].voltage += 1000;
    return;
  }

  if (scancode == 0x3E) {
    sys_save_task(0, 0);
    return;
  }

  if (scancode == 0x3F) {
    sys_load_task(0, 0);
    return;
  }

  if (c && buffer_idx < 31) {
    input_buffer[buffer_idx++] = c;
    int next_head = (kb_head + 1) % 32;
    if (next_head != kb_tail) {
      kb_buf[kb_head] = c;
      kb_head = next_head;
    }
    shell_dirty = 1;
  } else if (!c) {
    os_memory_map[0].neurons[0].voltage += 500;
  }
}
