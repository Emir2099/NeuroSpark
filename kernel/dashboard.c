#include "dashboard.h"
#include "disk.h"
#include "pci.h"
#include "scheduler.h"
#include "storage_manager.h"
#include "net.h"
#include "profiling.h"
#include "model_manager.h"
#include "../assets/ui/icon_atlas_96x96.h"

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
extern void draw_mouse_cursor(void);
extern void disk_read_sector(uint32_t lba, uint16_t *buffer);
extern void disk_write_sector(uint32_t lba, uint16_t *buffer);
extern int find_free_slot(FileEntry *dir);

extern void gprint(char *str, uint32_t color);
extern void gprint_12(char *str, uint32_t color);
extern void gprint_dec(int val, uint32_t color);
extern void clear_region(int x0, int y0, int x1, int y1, uint32_t color);
extern void draw_hline(int y, int x0, int x1, uint32_t color);
extern void put_pixel(int x, int y, uint32_t color);
extern void flip_buffer(void);
extern volatile int flip_mutex;
extern uint32_t backbuffer[];

/* Cyber-Scientific palette */
#define P_BG_ABYSS 0x000018
#define P_BG_TEAL 0x004060
#define P_CHROME_DARK 0x001133
#define P_CHROME_LITE 0x003366
#define P_ACCENT_CYAN 0x00FFFF
#define P_DATA_GREEN 0x00FF00
#define P_CRITICAL_ORANGE 0xFF8800
extern volatile int mouse_x;
extern volatile int mouse_y;
extern volatile int mouse_buttons;

typedef struct {
  int x;
  int y;
  int w;
  int h;
  int base_x;
  int base_y;
  int base_w;
  int base_h;
  int visible;
  int minimized;
  int maximized;
} UiWindow;

static int viz_mode = VIZ_MODE_OFF;
static int viz_scrub = 0;
static int viz_compare_a = 0;
static int viz_compare_b = 1;
static int viz_autoplay = 0;
static int raster_head = 0;
static uint8_t raster_ring[64][16];

static UiWindow win_shell = {32, 30, 302, 144, 32, 30, 302, 144, 0, 0, 0};
static UiWindow win_viz = {52, 184, 370, 188, 52, 184, 370, 188, 0, 0, 0};
static UiWindow win_grid = {518, 74, 242, 222, 518, 74, 242, 222, 0, 0, 0};

static int wm_active = 0; /* 0=shell,1=viz,2=grid */
static int wm_prev_left = 0;
static int wm_dragging = -1; /* -1=not dragging, 0-2=window id being dragged */
static int wm_drag_start_x = 0;
static int wm_drag_start_y = 0;
static int wm_drag_win_x = 0;
static int wm_drag_win_y = 0;

static uint8_t prev_phase[2] = {0, 0};

void viz_set_mode(int mode) {
  if (mode < VIZ_MODE_OFF || mode > VIZ_MODE_COMPARE) {
    return;
  }
  viz_mode = mode;
}

int viz_get_mode(void) { return viz_mode; }

void viz_set_scrub(int index) {
  if (index < 0) {
    index = 0;
  }
  if (index > 63) {
    index = 63;
  }
  viz_scrub = index;
}

int viz_get_scrub(void) { return viz_scrub; }

int viz_set_compare_slots(int slot_a, int slot_b) {
  if (slot_a < 0 || slot_b < 0 || slot_a >= storage_snapshot_capacity() ||
      slot_b >= storage_snapshot_capacity()) {
    return 0;
  }
  viz_compare_a = slot_a;
  viz_compare_b = slot_b;
  return 1;
}

void viz_set_autoplay(int enabled) { viz_autoplay = enabled ? 1 : 0; }

int viz_get_autoplay(void) { return viz_autoplay; }

void viz_step_scrub(int delta) {
  int next = viz_scrub + delta;
  while (next < 0) {
    next += 64;
  }
  while (next > 63) {
    next -= 64;
  }
  viz_scrub = next;
}

static void draw_line_segment(int x0, int y0, int x1, int y1, uint32_t color) {
  int dx = x1 - x0;
  int dy = y1 - y0;
  int sx = (dx >= 0) ? 1 : -1;
  int sy = (dy >= 0) ? 1 : -1;
  int adx = dx >= 0 ? dx : -dx;
  int ady = dy >= 0 ? dy : -dy;
  int err = (adx > ady ? adx : -ady) / 2;

  while (1) {
    put_pixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) {
      break;
    }
    {
      int e2 = err;
      if (e2 > -adx) {
        err -= ady;
        x0 += sx;
      }
      if (e2 < ady) {
        err += adx;
        y0 += sy;
      }
    }
  }
}

static void draw_icon_from_atlas(int tile_r, int tile_c, int x, int y) {
  int sx0 = tile_c * UI_ICON_TILE;
  int sy0 = tile_r * UI_ICON_TILE;
  for (int yy = 0; yy < 48; yy++) {
    for (int xx = 0; xx < 48; xx++) {
      uint32_t c = ui_icon_atlas_96x96[(sy0 + yy) * UI_ICON_ATLAS_W + (sx0 + xx)];
      if (c != 0) {
        put_pixel(x + xx, y + yy, c);
      }
    }
  }
}

static int point_in_rect(int px, int py, int x, int y, int w, int h) {
  return px >= x && px < (x + w) && py >= y && py < (y + h);
}

static UiWindow *window_by_id(int id) {
  if (id == 0)
    return &win_shell;
  if (id == 1)
    return &win_viz;
  return &win_grid;
}

static void window_apply_layout(UiWindow *w, int is_dragging) {
  if (!w)
    return;
  if (w->maximized) {
    w->x = 18;
    w->y = 22;
    w->w = 764;
    w->h = 468;
  } else if (!is_dragging) {
    /* Only reset position if NOT currently being dragged */
    w->x = w->base_x;
    w->y = w->base_y;
    w->w = w->base_w;
    w->h = w->base_h;
  } else {
    /* During drag, only update size, not position */
    w->w = w->base_w;
    w->h = w->base_h;
  }
}

static void window_open(UiWindow *w, int id) {
  if (!w)
    return;
  w->visible = 1;
  w->minimized = 0;
  wm_active = id;
}

static void window_minimize(UiWindow *w) {
  if (!w)
    return;
  w->minimized = 1;
  w->visible = 0;
}

static void window_close(UiWindow *w, int id) {
  if (!w)
    return;
  w->visible = 0;
  w->minimized = 0;
  w->maximized = 0;
  window_apply_layout(w, 0);  /* 0 = not dragging */
  
  /* Clear window-specific state when closing */
  if (id == 0) {
    /* Shell window closed - clear shell render state */
    shell_dirty = 0;
    shell_cursor_x = -100;  /* Out of bounds - stops cursor blinking */
    shell_cursor_y = -100;
  }
}

static void window_toggle_max(UiWindow *w) {
  if (!w)
    return;
  w->maximized = !w->maximized;
  window_apply_layout(w, 0);  /* 0 = not dragging */
}

static int window_handle_controls(UiWindow *w, int id, int mx, int my) {
  int bx;
  int by;
  if (!w || !w->visible) {
    return 0;
  }
  bx = w->x + w->w - 66;
  by = w->y + 3;

  if (point_in_rect(mx, my, bx, by, 16, 14)) {
    window_minimize(w);
    return 1;
  }
  if (point_in_rect(mx, my, bx + 20, by, 16, 14)) {
    window_toggle_max(w);
    wm_active = id;
    return 1;
  }
  if (point_in_rect(mx, my, bx + 40, by, 16, 14)) {
    window_close(w, id);
    return 1;
  }
  return 0;
}

static void wm_handle_clicks(void) {
  int left = (mouse_buttons & 0x1) ? 1 : 0;
  int mx = mouse_x;
  int my = mouse_y;
  int click = (left && !wm_prev_left);
  int release = (!left && wm_prev_left);
  wm_prev_left = left;

  /* Handle window dragging while button held */
  if (wm_dragging >= 0 && left) {
    UiWindow *w = window_by_id(wm_dragging);
    if (w) {
      int dx = mx - wm_drag_start_x;
      int dy = my - wm_drag_start_y;
      w->x = wm_drag_win_x + dx;
      w->y = wm_drag_win_y + dy;
      /* Clamp to screen */
      if (w->x < 0) w->x = 0;
      if (w->y < 20) w->y = 20;
      if (w->x + w->w > 800) w->x = 800 - w->w;
      if (w->y + w->h > 600) w->y = 600 - w->h;
    }
    return;
  }

  /* Stop dragging on button release */
  if (release) {
    if (wm_dragging >= 0) {
      UiWindow *w = window_by_id(wm_dragging);
      if (w) {
        w->base_x = w->x;
        w->base_y = w->y;
      }
    }
    wm_dragging = -1;
    return;
  }

  if (!click) {
    return;
  }

  /* Dock relaunch tiles - expanded zones for easier clicking */
  // Icon 1: Power
  if (point_in_rect(mx, my, 184, 544, 56, 40)) {
    window_open(&win_shell, 0);
    return;
  }
  // Icon 2: Apps
  if (point_in_rect(mx, my, 248, 544, 56, 40)) {
    window_open(&win_shell, 0);
    return;
  }
  // Icon 3: Viz
  if (point_in_rect(mx, my, 312, 544, 56, 40)) {
    window_open(&win_viz, 1);
    return;
  }
  // Icon 4: Shell
  if (point_in_rect(mx, my, 376, 544, 56, 40)) {
    window_open(&win_shell, 0);
    return;
  }
  // Icon 5: System
  if (point_in_rect(mx, my, 440, 544, 56, 40)) {
    window_open(&win_grid, 2);
    return;
  }

  /* Active-first control handling */
  {
    UiWindow *a = window_by_id(wm_active);
    if (window_handle_controls(a, wm_active, mx, my)) {
      return;
    }
  }
  for (int id = 0; id < 3; id++) {
    if (id == wm_active)
      continue;
    if (window_handle_controls(window_by_id(id), id, mx, my)) {
      return;
    }
  }

  /* Check for title bar drag (top 28 pixels of window - expanded zone) */
  for (int id = 2; id >= 0; id--) {
    UiWindow *w = window_by_id(id);
    if (w->visible && point_in_rect(mx, my, w->x, w->y, w->w, 28)) {
      /* Start dragging this window */
      wm_dragging = id;
      wm_drag_start_x = mx;
      wm_drag_start_y = my;
      wm_drag_win_x = w->x;
      wm_drag_win_y = w->y;
      wm_active = id;
      return;
    }
  }

  /* Focus by click inside window content */
  for (int id = 2; id >= 0; id--) {
    UiWindow *w = window_by_id(id);
    if (w->visible && point_in_rect(mx, my, w->x, w->y, w->w, w->h)) {
      wm_active = id;
      return;
    }
  }
}

static void fill_gradient_background(void) {
  for (int y = 0; y < 600; y++) {
    int r0 = (P_BG_ABYSS >> 16) & 0xFF;
    int g0 = (P_BG_ABYSS >> 8) & 0xFF;
    int b0 = P_BG_ABYSS & 0xFF;
    int r1 = (P_BG_TEAL >> 16) & 0xFF;
    int g1 = (P_BG_TEAL >> 8) & 0xFF;
    int b1 = P_BG_TEAL & 0xFF;
    int r = r0 + ((r1 - r0) * y) / 600;
    int g = g0 + ((g1 - g0) * y) / 600;
    int b = b0 + ((b1 - b0) * y) / 600;
    uint32_t row = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    for (int x = 0; x < 800; x++) {
      int glow = ((x - 560) * (x - 560) + (y - 160) * (y - 160)) / 18000;
      int add = (glow < 22) ? (22 - glow) : 0;
      int rr = r + add / 5;
      int gg = g + add;
      int bb = b + add;
      if (rr > 255)
        rr = 255;
      if (gg > 255)
        gg = 255;
      if (bb > 255)
        bb = 255;
      put_pixel(x, y, ((uint32_t)rr << 16) | ((uint32_t)gg << 8) | (uint32_t)bb);
    }
    if (row == 0xFFFFFFFF) {
      break;
    }
  }
}

static uint32_t blend_50(uint32_t a, uint32_t b) {
  uint32_t ar = (a >> 16) & 0xFF;
  uint32_t ag = (a >> 8) & 0xFF;
  uint32_t ab = a & 0xFF;
  uint32_t br = (b >> 16) & 0xFF;
  uint32_t bg = (b >> 8) & 0xFF;
  uint32_t bb = b & 0xFF;
  return (((ar + br) >> 1) << 16) | (((ag + bg) >> 1) << 8) | ((ab + bb) >> 1);
}

static void draw_glass_panel(int x, int y, int w, int h, uint32_t tint) {
  for (int yy = y; yy < y + h; yy++) {
    if (yy < 0 || yy >= 600)
      continue;
    for (int xx = x; xx < x + w; xx++) {
      if (xx < 0 || xx >= 800)
        continue;
      int idx = yy * 800 + xx;
      backbuffer[idx] = blend_50(backbuffer[idx], tint);
    }
  }

  draw_hline(y, x + 1, x + w - 1, 0x72B9CF);
  draw_hline(y + h - 1, x + 1, x + w - 1, 0x0A1B28);
}

static int cstr_len(const char *s) {
  int n = 0;
  if (!s)
    return 0;
  while (s[n])
    n++;
  return n;
}

static void draw_window_title_clipped(int x, int y, int w, const char *title) {
  char buf[64];
  int max_px = w - 78; /* leave space for window controls */
  int max_chars;
  int len;
  if (max_px < 12)
    return;
  max_chars = max_px / 12; /* gprint_12 is 12px wide */
  if (max_chars < 1)
    return;
  if (max_chars > 63)
    max_chars = 63;

  len = cstr_len(title);
  if (len <= max_chars) {
    int i;
    for (i = 0; i < len; i++)
      buf[i] = title[i];
    buf[len] = '\0';
  } else {
    int keep = max_chars;
    int i;
    if (keep >= 4) {
      keep -= 3;
      for (i = 0; i < keep; i++)
        buf[i] = title[i];
      buf[keep] = '.';
      buf[keep + 1] = '.';
      buf[keep + 2] = '.';
      buf[keep + 3] = '\0';
    } else {
      for (i = 0; i < keep; i++)
        buf[i] = title[i];
      buf[keep] = '\0';
    }
  }

  cursor_x = x + 6;
  cursor_y = y + 2;
  gprint_12(buf, 0x30363D);
}

static void draw_window_frame(int x, int y, int w, int h, const char *title) {
  clear_region(x + 3, y + 3, x + w + 4, y + h + 4, 0x2E4660);
  clear_region(x, y, x + w, y + h, 0xF2F5F7);
  clear_region(x, y, x + w, y + 20, 0xE2E8ED);
  clear_region(x + 2, y + 21, x + w - 2, y + 34, 0xEAF1F7);
  clear_region(x + w - 66, y + 3, x + w - 50, y + 17, P_ACCENT_CYAN);
  clear_region(x + w - 46, y + 3, x + w - 30, y + 17, P_ACCENT_CYAN);
  clear_region(x + w - 26, y + 3, x + w - 10, y + 17, P_ACCENT_CYAN);
  draw_hline(y, x, x + w, 0x9AA5B3);
  draw_hline(y + h - 1, x, x + w, 0x8794A4);
  for (int yy = y; yy < y + h; yy++) {
    put_pixel(x, yy, 0x97A4B1);
    put_pixel(x + w - 1, yy, 0x97A4B1);
  }
  draw_window_title_clipped(x, y, w, title);
  cursor_x = x + w - 63;
  cursor_y = y + 5;
  gprint("-", 0x073A44);
  cursor_x = x + w - 43;
  cursor_y = y + 5;
  gprint("O", 0x073A44);
  cursor_x = x + w - 23;
  cursor_y = y + 5;
  gprint("X", 0x073A44);
}

static void draw_synapse_grid_panel(void) {
  int inner_x0 = win_grid.x + 2;
  int inner_y0 = win_grid.y + 20;
  int inner_x1 = win_grid.x + win_grid.w - 2;
  int inner_y1 = win_grid.y + win_grid.h - 2;
  int cx = (inner_x0 + inner_x1) / 2;
  int cy = (inner_y0 + inner_y1) / 2;
  int ring = ((inner_x1 - inner_x0) < (inner_y1 - inner_y0) ?
              (inner_x1 - inner_x0) : (inner_y1 - inner_y0)) /
             2 - 12;
  if (ring < 32)
    ring = 32;
  for (int i = 0; i < 96; i++) {
    int a = (i * 41) & 255;
    int b = ((i * 67) + 53) & 255;
    int x0 = cx + (((a - 128) * ring) / 128);
    int y0 = cy + ((((a ^ 53) - 128) * ring) / 128);
    int x1 = cx + (((b - 128) * ring) / 128);
    int y1 = cy + ((((b ^ 53) - 128) * ring) / 128);
    if ((x0 - cx) * (x0 - cx) + (y0 - cy) * (y0 - cy) < ring * ring &&
        (x1 - cx) * (x1 - cx) + (y1 - cy) * (y1 - cy) < ring * ring) {
      draw_line_segment(x0, y0, x1, y1, 0x7F95A8);
      clear_region(x0 - 2, y0 - 2, x0 + 3, y0 + 3, 0x2E86AD);
    }
  }
}

static void draw_rect_outline(int x0, int y0, int x1, int y1, uint32_t color) {
  draw_hline(y0, x0, x1, color);
  draw_hline(y1 - 1, x0, x1, color);
  for (int y = y0; y < y1; y++) {
    put_pixel(x0, y, color);
    put_pixel(x1 - 1, y, color);
  }
}

static void draw_glow_box(int x0, int y0, int x1, int y1, uint32_t color) {
  draw_rect_outline(x0 - 2, y0 - 2, x1 + 2, y1 + 2, (color & 0xFEFEFE) >> 1);
  draw_rect_outline(x0 - 1, y0 - 1, x1 + 1, y1 + 1, (color & 0xFCFCFC) >> 2);
  draw_rect_outline(x0, y0, x1, y1, color);
}

static void draw_icon_tile(int x, int y, int w, int h, uint32_t accent) {
  clear_region(x, y, x + w, y + h, 0x324555);
  draw_rect_outline(x, y, x + w, y + h, 0x5E7387);
  clear_region(x + 1, y + 1, x + w - 1, y + 3, 0x3D5266);
  clear_region(x + 4, y + h - 5, x + w - 4, y + h - 3, 0x2B3C4B);
  clear_region(x + 6, y + 6, x + w - 6, y + 8, accent);
}

static void draw_icon_apps(int x, int y) {
  draw_icon_from_atlas(2, 0, x, y);
}

static void draw_icon_bolt(int x, int y) {
  draw_icon_from_atlas(0, 0, x, y);
}

static void draw_icon_terminal(int x, int y) {
  draw_icon_from_atlas(1, 0, x, y);
}

static void draw_icon_viz(int x, int y) {
  draw_icon_from_atlas(0, 1, x, y);
}

static void draw_icon_gear(int x, int y) {
  draw_icon_from_atlas(1, 1, x, y);
}

static void draw_tray_mini_graph(int x, int y) {
  clear_region(x, y, x + 58, y + 24, 0x30465D);
  clear_region(x + 60, y, x + 118, y + 24, 0x30465D);
  draw_rect_outline(x, y, x + 58, y + 24, 0x6E8498);
  draw_rect_outline(x + 60, y, x + 118, y + 24, 0x6E8498);
  for (int i = 0; i < 11; i++) {
    int h = (i * 7 + (int)tick) & 13;
    clear_region(x + 3 + i * 5, y + 20 - h, x + 6 + i * 5, y + 21, 0x61C18A);
    h = (i * 5 + (int)tick / 2) & 11;
    clear_region(x + 63 + i * 5, y + 20 - h, x + 66 + i * 5, y + 21, 0x4FA7E6);
  }
}

static void draw_tray_clock(int x, int y) {
  clear_region(x, y, x + 26, y + 24, 0x30465D);
  draw_rect_outline(x, y, x + 26, y + 24, 0x6E8498);
  draw_rect_outline(x + 6, y + 4, x + 20, y + 18, 0xA4B8C8);
  draw_line_segment(x + 13, y + 11, x + 13, y + 7, 0xA4B8C8);
  draw_line_segment(x + 13, y + 11, x + 16, y + 11, 0xA4B8C8);
}

static void draw_tray_battery(int x, int y, int pct) {
  int fill = (pct * 18) / 100;
  if (fill < 0)
    fill = 0;
  if (fill > 18)
    fill = 18;
  clear_region(x, y, x + 28, y + 12, 0x30465D);
  draw_rect_outline(x, y, x + 24, y + 12, 0xA4B8C8);
  clear_region(x + 24, y + 3, x + 27, y + 9, 0xA4B8C8);
  clear_region(x + 2, y + 2, x + 2 + fill, y + 10, pct < 30 ? 0xE27272 : 0x73C890);
}

static int heat_color(int v, int lo, int hi) {
  if (hi <= lo) {
    return 0x335577;
  }
  int span = hi - lo;
  int x = (v - lo) * 255 / span;
  if (x < 0) {
    x = 0;
  }
  if (x > 255) {
    x = 255;
  }

  int r = x;
  int g = 255 - ((x - 128) < 0 ? (128 - x) : (x - 128));
  int b = 255 - x;
  if (g < 0) {
    g = 0;
  }
  return ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

static void draw_viz_heatmap(void) {
  const int x0 = win_grid.x + 16;
  const int y0 = win_grid.y + win_grid.h - 48;
  const int cell_w = 22;
  const int cell_h = 32;
  int min_w = 32767;
  int max_w = -32768;

  for (int p = 0; p < 2; p++) {
    for (int n = 0; n < 5; n++) {
      int w = os_memory_map[p].neurons[n].synaptic_weight;
      if (w < min_w)
        min_w = w;
      if (w > max_w)
        max_w = w;
    }
  }

  clear_region(x0 - 6, y0 - 20, x0 + (10 * cell_w) + 6, y0 + cell_h + 22,
               0xEDF3F8);
  cursor_x = x0;
  cursor_y = y0 - 16;
  gprint("Heatmap", 0x335E7A);

  for (int p = 0; p < 2; p++) {
    for (int n = 0; n < 5; n++) {
      int idx = (p * 5) + n;
      int cx = x0 + idx * cell_w;
      int cy = y0;
      int color = heat_color(os_memory_map[p].neurons[n].synaptic_weight, min_w, max_w);
      clear_region(cx, cy, cx + cell_w - 2, cy + cell_h, color);
      if (n == 0) {
        cursor_x = cx;
        cursor_y = y0 + cell_h + 2;
        gprint((p == 0) ? "T0" : "T1", 0xAACCEE);
      }
    }
  }
}

static void record_raster_sample(void) {
  int row = raster_head & 63;
  for (int i = 0; i < 16; i++) {
    int p = i / 8;
    int n = i % 5;
    uint8_t spike = 0;
    if (n < 5 && p < 2) {
      spike = (os_memory_map[p].neurons[n].voltage >=
               os_memory_map[p].neurons[n].dynamic_threshold)
                  ? 1
                  : 0;
    }
    raster_ring[row][i] = spike;
  }
  raster_head = (raster_head + 1) & 63;
}

static void draw_viz_raster(void) {
  const int x0 = win_viz.x + 16;
  const int y0 = win_viz.y + 36;
  const int w = win_viz.w - 36;
  const int h = win_viz.h - 58;
  int scrub = viz_scrub;
  if (scrub < 0)
    scrub = 0;
  if (scrub > 63)
    scrub = 63;

  clear_region(x0 - 6, y0 - 16, x0 + w + 8, y0 + h + 22, 0xEFF4F8);
  cursor_x = x0;
  cursor_y = y0 - 14;
  gprint("Neuro-Viz Raster", 0x3A5E78);
  gprint(" scrub:", 0xAACCEE);
  gprint_dec(scrub, 0xFFFFFF);

  for (int t = 0; t < 64; t++) {
    int ring_idx = (raster_head + t) & 63;
    int x = x0 + (t * w) / 64;
    for (int n = 0; n < 10; n++) {
      int y = y0 + (n * h) / 10;
      uint32_t c = raster_ring[ring_idx][n] ? 0x3CC082 : 0x6F879A;
      put_pixel(x, y, c);
      if (x + 1 < x0 + w)
        put_pixel(x + 1, y, c);
    }
  }

  {
    int sx = x0 + (scrub * w) / 64;
    for (int y = y0; y < y0 + h; y++) {
      put_pixel(sx, y, 0xFFE066);
    }
  }
}

static void draw_viz_compare(void) {
  int dv = 0, dw = 0, dt = 0;
  int dvv[5], dww[5], dtt[5];
  clear_region(win_grid.x + 10, win_grid.y + 24, win_grid.x + win_grid.w - 10,
               win_grid.y + win_grid.h - 12, 0xEFF4F8);
  cursor_x = win_grid.x + 16;
  cursor_y = win_grid.y + 28;
  gprint("Compare", 0x3A5E78);
  gprint(" slots ", 0xAACCEE);
  gprint_dec(viz_compare_a, 0xFFFFFF);
  gprint(" vs ", 0xAACCEE);
  gprint_dec(viz_compare_b, 0xFFFFFF);

  cursor_x = win_grid.x + 16;
  cursor_y = win_grid.y + 44;
  if (!storage_diff_snapshots(viz_compare_a, viz_compare_b, &dv, &dw, &dt)) {
    gprint("No snapshot data for selected slots", 0xFF7777);
    return;
  }

  if (!storage_diff_snapshots_vector(viz_compare_a, viz_compare_b, dvv, dww, dtt, 5)) {
    gprint("Vector diff unavailable", 0xFF7777);
    return;
  }

  gprint("drift V:", 0xAACCEE);
  gprint_dec(dv, 0xFFFFFF);
  gprint("  W:", 0xAACCEE);
  gprint_dec(dw, 0xFFFFFF);
  gprint("  T:", 0xAACCEE);
  gprint_dec(dt, 0xFFFFFF);

  cursor_x = win_grid.x + 16;
  cursor_y = win_grid.y + 58;
  gprint("divergence markers: ", 0xAACCEE);
  gprint((dv > 2000 || dv < -2000) ? "V! " : "v ", (dv > 2000 || dv < -2000) ? 0xFF7777 : 0x77FF77);
  gprint((dw > 600 || dw < -600) ? "W! " : "w ", (dw > 600 || dw < -600) ? 0xFF7777 : 0x77FF77);
  gprint((dt > 1200 || dt < -1200) ? "T!" : "t", (dt > 1200 || dt < -1200) ? 0xFF7777 : 0x77FF77);

  cursor_x = win_grid.x + 16;
  cursor_y = win_grid.y + 72;
  gprint("param diff summary", 0xAACCEE);
  cursor_x = win_grid.x + 16;
  cursor_y = win_grid.y + 84;
  gprint("task0 model:", 0x88E0FF);
  gprint((char *)model_name(0), 0xFFFFFF);
  cursor_x = win_grid.x + 16;
  cursor_y = win_grid.y + 96;
  gprint("task1 model:", 0x88E0FF);
  gprint((char *)model_name(1), 0xFFFFFF);

  cursor_x = win_grid.x + 16;
  cursor_y = win_grid.y + 110;
  gprint("N dV dW dT", 0x9AF0C8);
  for (int i = 0; i < 5; i++) {
    cursor_x = win_grid.x + 16;
    cursor_y = win_grid.y + 122 + (i * 10);
    gprint_dec(i, 0xFFFFFF);
    gprint(" ", 0x666666);
    gprint_dec(dvv[i], 0xFFFFFF);
    gprint(" ", 0x666666);
    gprint_dec(dww[i], 0xFFFFFF);
    gprint(" ", 0x666666);
    gprint_dec(dtt[i], 0xFFFFFF);
  }
}

static void draw_phase13_viz(void) {
  if ((viz_mode == VIZ_MODE_HEATMAP) && win_grid.visible && wm_active == 2) {
    draw_viz_heatmap();
  } else if ((viz_mode == VIZ_MODE_RASTER) && win_viz.visible && wm_active == 1) {
    draw_viz_raster();
  } else if ((viz_mode == VIZ_MODE_COMPARE) && win_grid.visible && wm_active == 2) {
    draw_viz_compare();
  }
}

static void draw_hud_telemetry(void) {
  const int total_user_pages = (0x2000000 - 0x100000) / 4096;
  int free_pages = pmm_get_free_pages();
  int used_pages = total_user_pages - free_pages;
  int pressure_pct = 0;
  int running = 0;
  int ready = 0;
  int blocked = 0;
  int sleeping = 0;
  int safe_task_count = os_task_count;
  int safe_current_task = os_current_task;

  if (safe_task_count < 0) {
    safe_task_count = 0;
  }
  if (safe_task_count > MAX_TASKS) {
    safe_task_count = MAX_TASKS;
  }
  if (safe_current_task < 0 || safe_current_task >= safe_task_count) {
    safe_current_task = 0;
  }

  if (used_pages < 0) {
    used_pages = 0;
  }
  if (total_user_pages > 0) {
    pressure_pct = (used_pages * 100) / total_user_pages;
  }

  {
    uint32_t secs = tick / 100;
    uint32_t mins = secs / 60;
    uint32_t hrs = mins / 60;
    int battery = 62 + (int)((tick / 300) % 32);
    int free_mb = pmm_get_free_pages() * 4 / 1024;
    int wx = 646;
    int wy = 540;

    draw_glass_panel(wx, wy, 150, 54, 0x27435A);

    cursor_x = wx + 10;
    cursor_y = wy + 8;
    gprint_dec((int)(hrs % 100), 0xEAF4FC);
    gprint(":", 0xEAF4FC);
    if ((mins % 60) < 10)
      gprint("0", 0xEAF4FC);
    gprint_dec((int)(mins % 60), 0xEAF4FC);

    draw_hline(wy + 28, wx + 9, wx + 140, 0x2D5268);

    clear_region(wx + 72, wy + 8, wx + 104, wy + 18, 0x2A4456);
    draw_hline(wy + 8, wx + 72, wx + 104, 0x86B8D3);
    draw_hline(wy + 17, wx + 72, wx + 104, 0x86B8D3);
    put_pixel(wx + 105, wy + 11, 0x86B8D3);
    put_pixel(wx + 105, wy + 12, 0x86B8D3);
    put_pixel(wx + 105, wy + 13, 0x86B8D3);
    put_pixel(wx + 105, wy + 14, 0x86B8D3);
    clear_region(wx + 74, wy + 10, wx + 74 + (battery * 28) / 100, wy + 16,
                 0x51D37D);

    cursor_x = wx + 112;
    cursor_y = wy + 8;
    gprint_dec(battery, 0xCDE3F2);
    gprint("%", 0xCDE3F2);

    cursor_x = wx + 10;
    cursor_y = wy + 34;
    gprint("Space:", 0x9FC2D8);
    gprint_dec(free_mb, 0xBDEBFF);
    gprint("MB", 0x9FC2D8);
  }

  for (int i = 0; i < safe_task_count; i++) {
    if (os_tasks[i].state == TASK_STATE_RUNNING) {
      running++;
    } else if (os_tasks[i].state == TASK_STATE_READY) {
      ready++;
    } else if (os_tasks[i].state == TASK_STATE_BLOCKED) {
      blocked++;
    } else if (os_tasks[i].state == TASK_STATE_SLEEPING) {
      sleeping++;
    }
  }

  {
    ProfileSnapshot snap;
    profile_snapshot(&snap);
    cursor_x = 676;
    cursor_y = 584;
    gprint("PRF:", 0xAACCEE);
    gprint(snap.enabled ? "ON" : "OFF", snap.enabled ? 0x77FF77 : 0xFFAA66);
    cursor_x = 46;
    cursor_y = 582;
    gprint("OS", 0xA8B8C8);
    gprint("  T:", 0xA8B8C8);
    gprint_dec((int)tick, 0xE7EDF2);
    gprint("  RUN:", 0xA8B8C8);
    gprint_dec(running, 0xE7EDF2);
    gprint(" RDY:", 0xA8B8C8);
    gprint_dec(ready, 0xE7EDF2);
  }
}

static void draw_command_overlay(void) {
  draw_glass_panel(160, 536, 480, 58, 0x274761);

  draw_glass_panel(184, 544, 56, 40, 0x2E6076);
  draw_glass_panel(248, 544, 56, 40, 0x3E5A73);
  draw_glass_panel(312, 544, 56, 40, 0x2D6378);
  draw_glass_panel(376, 544, 56, 40, 0x356D74);
  draw_glass_panel(440, 544, 56, 40, 0x495F72);

  draw_glow_box(184, 544, 240, 584, 0x5BE8EF);

  draw_icon_bolt(188, 540);
  draw_icon_apps(252, 540);
  draw_icon_viz(316, 540);
  draw_icon_terminal(380, 540);
  draw_icon_gear(444, 540);
}

void draw_status_bar(void) {
  fill_gradient_background();
  window_apply_layout(&win_shell, wm_dragging == 0);
  window_apply_layout(&win_viz, wm_dragging == 1);
  window_apply_layout(&win_grid, wm_dragging == 2);

  if (win_shell.visible && wm_active != 0) {
    draw_window_frame(win_shell.x, win_shell.y, win_shell.w, win_shell.h,
                      "Axiom Control Shell");
    clear_region(win_shell.x + 2, win_shell.y + 20, win_shell.x + win_shell.w - 2,
                 win_shell.y + win_shell.h - 2, 0x04143E);
  }
  if (win_viz.visible && wm_active != 1) {
    draw_window_frame(win_viz.x, win_viz.y, win_viz.w, win_viz.h,
                      "Neuro-Viz - AXIS 1: SNN LIVE FEED");
    clear_region(win_viz.x + 2, win_viz.y + 20, win_viz.x + win_viz.w - 2,
                 win_viz.y + win_viz.h - 2, 0xEFF4F8);
  }
  if (win_grid.visible && wm_active != 2) {
    draw_window_frame(win_grid.x, win_grid.y, win_grid.w, win_grid.h,
                      "Grid View - Project Synapse");
    clear_region(win_grid.x + 2, win_grid.y + 20, win_grid.x + win_grid.w - 2,
                 win_grid.y + win_grid.h - 2, 0xF2F6F9);
    draw_synapse_grid_panel();
  }

  if (window_by_id(wm_active)->visible) {
    UiWindow *a = window_by_id(wm_active);
    const char *title = (wm_active == 0) ? "Axiom Control Shell"
                        : (wm_active == 1) ? "Neuro-Viz - AXIS 1: SNN LIVE FEED"
                                           : "Grid View - Project Synapse";
    uint32_t fill = (wm_active == 0) ? 0x04143E : (wm_active == 1) ? 0xEFF4F8 : 0xF2F6F9;
    draw_window_frame(a->x, a->y, a->w, a->h, title);
    clear_region(a->x + 2, a->y + 20, a->x + a->w - 2, a->y + a->h - 2, fill);
    if (wm_active == 2) {
      draw_synapse_grid_panel();
    }
  }

  if (win_grid.visible && wm_active == 2) {
    for (int p = 0; p < 2; p++) {
      const char *ph_str = (os_memory_map[p].current_phase == 1) ? "RIGID" : "FLUID";
      uint32_t ph_clr = (os_memory_map[p].current_phase == 1) ? 0xFF7A7A : 0x77FFAA;
      cursor_x = win_grid.x + 18 + (p * 92);
      cursor_y = win_grid.y + win_grid.h - 22;
      gprint((char *)task_list[p].task_name, 0x3A5E78);
      gprint(":", 0x9AA8B4);
      gprint((char *)ph_str, ph_clr);
    }
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
  if (!win_viz.visible || wm_active != 1) {
    return;
  }
  const int WIN_Y0 = win_viz.y + 30;
  const int WIN_Y1 = win_viz.y + win_viz.h - 14;
  const int WIN_X0 = win_viz.x + 14;
  const int WIN_X1 = win_viz.x + win_viz.w - 16;
  int safe_zoom = zoom_level;
  if (safe_zoom < 1) {
    safe_zoom = 1;
  }

  clear_region(WIN_X0, WIN_Y0, WIN_X1, WIN_Y1, 0xF3F6F9);
  draw_hline(WIN_Y0 - 1, WIN_X0, WIN_X1, 0xCAD4DE);
  draw_hline(WIN_Y1, WIN_X0, WIN_X1, 0xCAD4DE);

  cursor_x = WIN_X0 + 4;
  cursor_y = WIN_Y0 - 12;
  gprint("+40mV", 0x4E6478);
  cursor_x = WIN_X0 + 4;
  cursor_y = WIN_Y0 + 10;
  gprint("0", 0x4E6478);
  cursor_x = WIN_X0 + 4;
  cursor_y = WIN_Y0 + 28;
  gprint("-40mV", 0x4E6478);

  int neurons_visible = 16 / safe_zoom;
  if (neurons_visible < 1)
    neurons_visible = 1;
  int start_n = zoom_offset;
  if (start_n + neurons_visible > 16)
    start_n = 16 - neurons_visible;
  if (start_n < 0)
    start_n = 0;

  for (int t = 0; t < 6; t++) {
    int prev_x = -1;
    int prev_y = -1;
    uint32_t c = (t == 1) ? 0x2D8CB5 : 0x5C788E;
    for (int x = WIN_X0 + 48; x < WIN_X1 - 8; x++) {
      int idx = start_n + ((x - WIN_X0) / 18) % neurons_visible;
      int p = potentials[idx];
      int wave = ((p + (int)tick + (t * 37) + (x * (t + 3))) & 63) - 31;
      int amp = (t == 1) ? 24 : (10 + t);
      int y = WIN_Y0 + 18 + (t * 16) + (wave * amp) / 64;
      if (prev_x >= 0) {
        draw_line_segment(prev_x, prev_y, x, y, c);
      }
      prev_x = x;
      prev_y = y;
    }
  }
}

void shell_render(void) {
  if (!win_shell.visible || wm_active != 0) {
    return;
  }
  extern char cmd_output[256];
  extern int cmd_output_valid;

  const int term_x = win_shell.x + 8;
  const int term_y = win_shell.y + 24;
  const int term_w = win_shell.w - 16;
  const int prompt_y = win_shell.y + win_shell.h - 28;
  clear_region(term_x, term_y, term_x + term_w, term_y + 108, 0x04143E);

  int saved_cx = cursor_x;
  int saved_cy = cursor_y;
  cursor_x = term_x + 10;
  cursor_y = prompt_y;
  gprint("> ", 0x55FF88);

  for (int i = 0; i < buffer_idx && i < 32; i++) {
    char ch[2] = {input_buffer[i], '\0'};
    gprint(ch, 0xEAF2F8);
  }

  shell_cursor_x = term_x + 26 + (buffer_idx * 8);
  shell_cursor_y = prompt_y;

  /* Persistent command output line below prompt */
  clear_region(term_x + 8, term_y + 66, term_x + term_w - 8, term_y + 84, 0x04143E);
  if (cmd_output_valid) {
    cursor_x = term_x + 10;
    cursor_y = term_y + 68;
    gprint(cmd_output, 0x5DE1A0);
  }

  cursor_x = saved_cx;
  cursor_y = saved_cy;
  shell_dirty = 0;
}

void neuro_task_entry(void) {
  while (1) {
    uint32_t render_profile_stamp = profile_begin();

    wm_handle_clicks();

    pulse_neurons();
    record_raster_sample();
    if (viz_mode == VIZ_MODE_RASTER && viz_autoplay) {
      viz_step_scrub(1);
    }

    clear_region(0, 102, 800, 310, 0x000033);
    draw_status_bar();
    /* Keep desktop clean; optional viz windows render only when explicitly used. */

    shell_render();
    draw_command_overlay();
    if (win_shell.visible && wm_active == 0) {
      draw_cursor(tick);
    }
    draw_mouse_cursor();

    cursor_x = 0;
    cursor_y = 312;

    /* Kernel dashboard task flips directly to avoid syscall privilege ambiguity. */
    __asm__ volatile("cli");
    flip_mutex = 1;
    flip_buffer();
    flip_mutex = 0;
    __asm__ volatile("sti");

    profile_end(PROFILE_SLOT_RENDER_PASS, render_profile_stamp);

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
