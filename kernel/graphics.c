/* Typedefs */
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

/* Graphics Constants */
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define SCREEN_SIZE   (SCREEN_WIDTH * SCREEN_HEIGHT)

/* Backbuffer for Double Buffering (Allocated in BSS) */
uint32_t backbuffer[SCREEN_SIZE];

/* External dependency */
#include "font.h"
extern uint32_t vbe_framebuffer; // Defined in kernel.c

/* ============================================================
 * PIXEL OPERATIONS
 * ============================================================ */

/* Put pixel into the backbuffer */
void put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        backbuffer[y * SCREEN_WIDTH + x] = color;
    }
}

/* Fast Copy to VRAM (The Flip) */
void flip_buffer() {
    uint32_t* vram = (uint32_t*)vbe_framebuffer;
    __asm__ volatile (
        "cld\n\t"
        "rep movsl"
        :
        : "S"(backbuffer), "D"(vram), "c"(SCREEN_SIZE)
        : "memory"
    );
}

/* Clear the backbuffer */
void clear_screen(uint32_t color) {
    for (int i = 0; i < SCREEN_SIZE; i++) {
        backbuffer[i] = color;
    }
}

/* Clear only a rectangular region */
void clear_region(int x0, int y0, int x1, int y1, uint32_t color) {
    for (int y = y0; y < y1 && y < SCREEN_HEIGHT; y++) {
        for (int x = x0; x < x1 && x < SCREEN_WIDTH; x++) {
            backbuffer[y * SCREEN_WIDTH + x] = color;
        }
    }
}

/* Draw a horizontal line */
void draw_hline(int y, int x0, int x1, uint32_t color) {
    if (y < 0 || y >= SCREEN_HEIGHT) return;
    for (int x = x0; x < x1 && x < SCREEN_WIDTH; x++) {
        if (x >= 0) backbuffer[y * SCREEN_WIDTH + x] = color;
    }
}

/* Draw a vertical line segment */
void draw_vline(int x, int y0, int y1, uint32_t color) {
    if (x < 0 || x >= SCREEN_WIDTH) return;
    int lo = y0 < y1 ? y0 : y1;
    int hi = y0 < y1 ? y1 : y0;
    for (int y = lo; y <= hi; y++) {
        if (y >= 0 && y < SCREEN_HEIGHT) backbuffer[y * SCREEN_WIDTH + x] = color;
    }
}

/* ============================================================
 * FONT RENDERING
 * ============================================================ */

/* Global cursor position for the graphical shell */
int cursor_x = 0;
int cursor_y = 120; /* Start below the status bar (100px) + some padding */

void draw_char(char c, int x, int y, uint32_t color) {
    for (int row = 0; row < 8; row++) {
        unsigned char row_data = font_8x8_basic[(int)c][row];
        for (int col = 0; col < 8; col++) {
            if (row_data & (1 << (7 - col))) {
                put_pixel(x + col, y + row, color);
            }
        }
    }
}

/* ============================================================
 * VGA SCROLL  (pixel-level, shifts backbuffer 8px upward)
 * ============================================================ */
void vga_scroll() {
    /* scroll the "shell" region: from SCROLL_TOP downward.
     * The top 110px (status bar + divider + waveform header) is preserved. */
    const int SCROLL_TOP = 310; /* below waveform window (y: 110-300) */

    /* Shift rows up by 8 pixels */
    for (int y = SCROLL_TOP + 8; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            backbuffer[(y - 8) * SCREEN_WIDTH + x] = backbuffer[y * SCREEN_WIDTH + x];
        }
    }

    /* Clear the bottom 8 rows */
    for (int y = SCREEN_HEIGHT - 8; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            backbuffer[y * SCREEN_WIDTH + x] = 0x000033; /* NeuroSpark dark blue */
        }
    }

    /* Keep cursor inside the scroll region */
    if (cursor_y > SCREEN_HEIGHT - 16) {
        cursor_y -= 8;
    }
}

/* ============================================================
 * GPRINT  (graphical text output with auto-scroll)
 * ============================================================ */
void gprint(char* str, uint32_t color) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            cursor_x = 0;
            cursor_y += 10;
        } else {
            draw_char(str[i], cursor_x, cursor_y, color);
            cursor_x += 8;
            if (cursor_x > 792) {
                cursor_x = 0;
                cursor_y += 10;
            }
        }

        /* Scroll when cursor runs off the bottom */
        if (cursor_y >= 590) {
            vga_scroll();
        }
    }
}

/* ============================================================
 * HELPER: print an unsigned value as hex (up to 8 digits)
 * ============================================================ */
void gprint_hex(uint32_t val, int digits, uint32_t color) {
    char buf[11];
    const char *hex = "0123456789ABCDEF";
    buf[0] = '0'; buf[1] = 'x';
    for (int i = digits - 1; i >= 0; i--) {
        buf[2 + i] = hex[val & 0xF];
        val >>= 4;
    }
    buf[2 + digits] = '\0';
    gprint(buf, color);
}

/* ============================================================
 * HELPER: print a signed decimal value
 * ============================================================ */
void gprint_dec(int val, uint32_t color) {
    if (val == 0) { gprint("0", color); return; }
    char buf[12];
    int i = 0;
    int neg = (val < 0);
    if (neg) val = -val;
    while (val != 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    if (neg) buf[i++] = '-';
    buf[i] = '\0';
    /* Reverse */
    for (int a = 0, b = i - 1; a < b; a++, b--) {
        char t = buf[a]; buf[a] = buf[b]; buf[b] = t;
    }
    gprint(buf, color);
}