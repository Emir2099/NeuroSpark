#include "input.h"
#include "wm.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;

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

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

volatile int mouse_x = SCREEN_WIDTH / 2;
volatile int mouse_y = SCREEN_HEIGHT / 2;
volatile int mouse_buttons = 0;
static int mouse_enabled = 0;

static int key_shift = 0;
static int key_ctrl = 0;
static int key_alt = 0;
static int key_caps = 0;

static uint8_t mouse_packet[3];
static int mouse_packet_idx = 0;

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static int wait_input_empty(void) {
  for (uint32_t i = 0; i < 100000; i++) {
    if ((inb(0x64) & 0x02) == 0) {
      return 1;
    }
    outb(0x80, 0);
  }
  return 0;
}

static int wait_output_full(void) {
  for (uint32_t i = 0; i < 100000; i++) {
    if (inb(0x64) & 0x01) {
      return 1;
    }
    outb(0x80, 0);
  }
  return 0;
}

static int is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static char shift_symbol(char c) {
  switch (c) {
  case '1':
    return '!';
  case '2':
    return '@';
  case '3':
    return '#';
  case '4':
    return '$';
  case '5':
    return '%';
  case '6':
    return '^';
  case '7':
    return '&';
  case '8':
    return '*';
  case '9':
    return '(';
  case '0':
    return ')';
  case '-':
    return '_';
  case '=':
    return '+';
  case '[':
    return '{';
  case ']':
    return '}';
  case '\\':
    return '|';
  case ';':
    return ':';
  case '\'':
    return '"';
  case ',':
    return '<';
  case '.':
    return '>';
  case '/':
    return '?';
  case '`':
    return '~';
  default:
    return c;
  }
}

static char get_ascii(uint8_t scancode) {
  static char map[0x80] = {
      0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
      '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
      'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
      'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
      'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' '};
  char c = map[scancode];
  if (c == 0) {
    return 0;
  }

  if (is_alpha(c)) {
    int upper = (key_shift ^ key_caps);
    if (upper && c >= 'a' && c <= 'z') {
      c = (char)(c - ('a' - 'A'));
    }
  } else if (key_shift) {
    c = shift_symbol(c);
  }
  return c;
}

static void update_modifier(uint8_t scancode, int pressed) {
  if (scancode == 0x2A || scancode == 0x36) {
    key_shift = pressed;
  } else if (scancode == 0x1D) {
    key_ctrl = pressed;
  } else if (scancode == 0x38) {
    key_alt = pressed;
  } else if (scancode == 0x3A && pressed) {
    key_caps = !key_caps;
  }
}

void init_input_stack(void) {
  key_shift = 0;
  key_ctrl = 0;
  key_alt = 0;
  key_caps = 0;

  mouse_packet_idx = 0;
  mouse_enabled = 0;
  mouse_x = SCREEN_WIDTH / 2;
  mouse_y = SCREEN_HEIGHT / 2;
  mouse_buttons = 0;

  /* Enable auxiliary PS/2 mouse device and streaming packets (IRQ12). */
  wait_input_empty();
  outb(0x64, 0xA8);

  wait_input_empty();
  outb(0x64, 0x20);
  wait_output_full();
  uint8_t status = inb(0x60);

  status |= 0x02;
  wait_input_empty();
  outb(0x64, 0x60);
  wait_input_empty();
  outb(0x60, status);

  wait_input_empty();
  outb(0x64, 0xD4);
  wait_input_empty();
  outb(0x60, 0xF4); /* Enable streaming */
  wait_output_full();
  (void)inb(0x60); /* ACK (or controller response) */
  mouse_enabled = 1;
}

void keyboard_handler(void) {
  uint8_t scancode = inb(0x60);
  if (scancode == 0xE0 || scancode == 0xE1)
    return;

  int is_release = (scancode & 0x80) != 0;
  uint8_t code = (uint8_t)(scancode & 0x7F);

  update_modifier(code, !is_release);
  if (is_release)
    return;

  char c = get_ascii(code);

  /* Function keys always work (neural probe / system) */
  if (code == 0x3B) {
    for (int n = 0; n < 5; n++)
      os_memory_map[0].neurons[n].voltage += 300;
    return;
  }

  if (code == 0x3C) {
    switch_tasks();
    return;
  }

  if (code == 0x3D) {
    for (int n = 0; n < 5; n++)
      os_memory_map[1].neurons[n].voltage += 1000;
    return;
  }

  if (code == 0x3E) {
    sys_save_task(0, 0);
    return;
  }

  if (code == 0x3F) {
    sys_load_task(0, 0);
    return;
  }

  /* Regular input only processed if focused window needs keyboard */
  if (!wm_focused_needs_keyboard())
    return;

  if (code == 0x1C) {
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

  if (code == 0x0E && buffer_idx > 0) {
    buffer_idx--;
    shell_dirty = 1;
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
  }
}

void mouse_handler(void) {
    if (!mouse_enabled) {
      return;
    }
  int x_delta;
  int y_delta;
  uint8_t data = inb(0x60);

  if (mouse_packet_idx == 0 && (data & 0x08) == 0) {
    /* Some emulators don't set bit 3. simply ignore the check for now */
  }

  mouse_packet[mouse_packet_idx++] = data;
  if (mouse_packet_idx < 3) {
    return;
  }
  mouse_packet_idx = 0;

  x_delta = (int)((signed char)mouse_packet[1]);
  y_delta = (int)((signed char)mouse_packet[2]);

  mouse_x += x_delta;
  mouse_y -= y_delta;

  if (mouse_x < 0)
    mouse_x = 0;
  if (mouse_x > SCREEN_WIDTH - 1)
    mouse_x = SCREEN_WIDTH - 1;
  if (mouse_y < 0)
    mouse_y = 0;
  if (mouse_y > SCREEN_HEIGHT - 1)
    mouse_y = SCREEN_HEIGHT - 1;

  mouse_buttons = mouse_packet[0] & 0x07;
}
