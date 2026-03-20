#include "dashboard.h"
#include "disk.h"
#include "pci.h"
#include "scheduler.h"

typedef unsigned char uint8_t;

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
  uint8_t current_phase;
  int pixel_recent_spikes;
} NeuralPixel;

typedef struct {
  int task_id;
  int priority;
  int target_pixel;
  int base_integration;
  int fire_threshold;
  const char *task_name;
  int saved_voltages[5];
  int saved_weights[5];
  int saved_thresholds[5];
  int last_spike_count;
  int spikes_per_second;
  int state;
} TaskControlBlock;

extern uint32_t tick;
extern int zoom_level;
extern int zoom_offset;
extern int potentials[16];
extern volatile int buffer_idx;
extern volatile char input_buffer[32];
extern volatile int shell_dirty;
extern int cursor_x;
extern int cursor_y;
extern int shell_cursor_x;
extern int shell_cursor_y;
extern TaskControlBlock task_list[2];
extern NeuralPixel os_memory_map[2];
extern int pmm_get_free_pages(void);
extern FileEntry root_directory[25];
extern volatile int ata_disk_available;

extern void pulse_neurons(void);
extern void draw_cursor(uint32_t tick);
extern void disk_read_sector(uint32_t lba, uint16_t *buffer);
extern void disk_write_sector(uint32_t lba, uint16_t *buffer);
extern int find_free_slot(FileEntry *dir);

extern void gprint(char *str, uint32_t color);
extern void gprint_dec(int val, uint32_t color);
extern void clear_region(int x0, int y0, int x1, int y1, uint32_t color);
extern void draw_hline(int y, int x0, int x1, uint32_t color);
extern void put_pixel(int x, int y, uint32_t color);
extern void flip_buffer(void);
extern volatile int flip_mutex;

static uint8_t prev_phase[2] = {0, 0};

void draw_status_bar(void) {
  clear_region(0, 0, 800, 100, 0x001122);
  draw_hline(100, 0, 800, 0x00FFFF);
  draw_hline(101, 0, 800, 0x006666);

  cursor_x = 4;
  cursor_y = 8;
  gprint("NEUROSPARK OS | ", 0x00FFFF);

  gprint("PCI: ", 0xFFFF00);
  gprint_dec(pci_found_count, 0xFFFFFF);
  gprint(" dev ", 0xFFFF00);

  cursor_x = 4;
  cursor_y = 22;
  gprint("MEM FREE: ", 0xAAAAAA);
  int free_pages = pmm_get_free_pages();
  gprint_dec(free_pages, 0x88FF88);
  gprint(" pages\n", 0xAAAAAA);

  cursor_x = 4;
  cursor_y = 36;
  gprint("TICK: ", 0xAAAAAA);
  gprint_dec((int)tick, 0xFF8800);
  gprint(" SPS[0]: ", 0xAAAAAA);
  gprint_dec(task_list[0].spikes_per_second, 0xFF5500);
  gprint(" SPS[1]: ", 0xAAAAAA);
  gprint_dec(task_list[1].spikes_per_second, 0xFF5500);

  cursor_x = 4;
  cursor_y = 50;
  gprint("ZOOM: ", 0xAAAAAA);
  gprint_dec(zoom_level, 0x00FFFF);
  gprint("x\n", 0xAAAAAA);

  cursor_x = 4;
  cursor_y = 64;
  for (int p = 0; p < 2; p++) {
    const char *ph_str = (os_memory_map[p].current_phase == 1) ? "RIGID" : "FLUID";
    uint32_t ph_clr = (os_memory_map[p].current_phase == 1) ? 0xFF4444 : 0x44FF44;
    gprint((char *)task_list[p].task_name, 0xFFFF00);
    gprint(": ", 0x446666);
    gprint((char *)ph_str, ph_clr);
    gprint("  ", 0x000000);
  }

  for (int p = 0; p < 2; p++) {
    if (prev_phase[p] == 0 && os_memory_map[p].current_phase == 1) {
      if (ata_disk_available) {
        disk_read_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
        int slot = find_free_slot(root_directory);
        if (slot != -1) {
          root_directory[slot].lba = TFS_DATA_START + slot;
          root_directory[slot].flags = 1;
          root_directory[slot].size = sizeof(potentials);
          root_directory[slot].name[0] = 'a';
          root_directory[slot].name[1] = 'u';
          root_directory[slot].name[2] = 't';
          root_directory[slot].name[3] = 'o';
          root_directory[slot].name[4] = '_';
          root_directory[slot].name[5] = (char)('0' + p);
          root_directory[slot].name[6] = '\0';
          disk_write_sector(root_directory[slot].lba, (uint16_t *)potentials);
          disk_write_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
        }
      }
    }
    prev_phase[p] = os_memory_map[p].current_phase;
  }
}

void draw_waveform(void) {
  const int WIN_Y0 = 110;
  const int WIN_Y1 = 300;
  const int WIN_H = WIN_Y1 - WIN_Y0;
  const int BASE_Y = WIN_Y0 + WIN_H - 10;
  int BAR_W = 4 * zoom_level;
  if (BAR_W > 20)
    BAR_W = 20;

  draw_hline(WIN_Y0 - 1, 0, 800, 0x334455);
  draw_hline(WIN_Y1, 0, 800, 0x334455);

  cursor_x = 4;
  cursor_y = WIN_Y0;
  gprint("NEURAL WAVEFORM [SRNEM]", 0x3366AA);

  int neurons_visible = 16 / zoom_level;
  if (neurons_visible < 1)
    neurons_visible = 1;
  int start_n = zoom_offset;
  if (start_n + neurons_visible > 16)
    start_n = 16 - neurons_visible;
  if (start_n < 0)
    start_n = 0;

  int spacing = 800 / (neurons_visible + 1);
  int prev_px = -1, prev_py = -1;

  for (int idx = 0; idx < neurons_visible; idx++) {
    int i = start_n + idx;
    int potential = potentials[i];
    if (potential < 0)
      potential = 0;
    if (potential > 1000)
      potential = 1000;

    int bar_h = (potential * (WIN_H - 20)) / 1000;
    int px = spacing + idx * spacing;
    int py = BASE_Y - bar_h;

    uint32_t color;
    if (potential < 333)
      color = 0x00CC44;
    else if (potential < 666)
      color = 0xFFDD00;
    else
      color = 0xFF3300;

    for (int dy = 0; dy < bar_h && dy < WIN_H - 10; dy++) {
      for (int dx = 0; dx < BAR_W; dx++) {
        put_pixel(px - BAR_W / 2 + dx, BASE_Y - dy, color);
      }
    }

    if (prev_px >= 0) {
      int dx = px - prev_px;
      int ddx = dx > 0 ? 1 : -1;
      for (int xi = prev_px; xi != px; xi += ddx) {
        int t_y = prev_py + (py - prev_py) * (xi - prev_px) / (dx ? dx : 1);
        put_pixel(xi, t_y, 0x005588);
      }
    }
    prev_px = px;
    prev_py = py;
  }
}

void shell_render(void) {
  clear_region(0, shell_cursor_y, 800, shell_cursor_y + 10, 0x000033);

  int saved_cx = cursor_x;
  int saved_cy = cursor_y;
  cursor_x = 0;
  cursor_y = shell_cursor_y;
  gprint("> ", 0x00FFFF);

  for (int i = 0; i < buffer_idx && i < 32; i++) {
    char ch[2] = {input_buffer[i], '\0'};
    gprint(ch, 0xFFFFFF);
  }

  shell_cursor_x = 16 + (buffer_idx * 8);
  cursor_x = saved_cx;
  cursor_y = saved_cy;
  shell_dirty = 0;
}

void neuro_task_entry(void) {
  while (1) {
    pulse_neurons();

    clear_region(0, 102, 800, 310, 0x000033);
    draw_status_bar();
    draw_waveform();

    shell_render();
    draw_cursor(tick);

    cursor_x = 0;
    cursor_y = 312;

    /* Kernel dashboard task flips directly to avoid syscall privilege ambiguity. */
    __asm__ volatile("cli");
    flip_mutex = 1;
    flip_buffer();
    flip_mutex = 0;
    __asm__ volatile("sti");

    for (volatile int d = 0; d < 400000; d++)
      ;

    schedule();
  }
}

void draw_neural_spike(int neuron_id, uint32_t intensity) {
  int start_x = 100 + (neuron_id * 40);
  for (int i = 0; i < 30; i++) {
    for (int j = 0; j < (int)(intensity / 10); j++) {
      put_pixel(start_x + i, 500 - j, 0x00FF00);
    }
  }
}
