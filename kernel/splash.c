/* Yazan OS boot splash: gradient wallpaper, original "Y" logo, boot status panel.
 * Pure integer math (no FPU/SSE needed). */
#include "splash.h"
#include "gfx.h"
#include "kstring.h"

#define BG_TOP      0x0A1430
#define BG_BOTTOM   0x1C56BE
#define TILE_TOP    0x56B6FF
#define TILE_BOTTOM 0x2563EB
#define COL_WHITE   0xFFFFFF
#define COL_PANEL   0x0B1226
#define COL_OK      0x4ADE80
#define COL_TEXT    0xE6EEFF
#define COL_DIM     0x9DB4E0

static rgb_t bg_at(int y, int h) { return gfx_mix(BG_TOP, BG_BOTTOM, (y * 256) / h); }

/* Squared distance from point p to segment a-b (all int64, no division by zero). */
static int64_t seg_dist2(int64_t px, int64_t py, int64_t ax, int64_t ay, int64_t bx, int64_t by) {
    int64_t vx = bx - ax, vy = by - ay, wx = px - ax, wy = py - ay;
    int64_t den = vx * vx + vy * vy;
    int64_t dot = wx * vx + wy * vy;
    if (dot <= 0)   return wx * wx + wy * wy;
    if (dot >= den) { int64_t ex = px - bx, ey = py - by; return ex * ex + ey * ey; }
    int64_t cross = wx * vy - wy * vx;
    return (cross * cross) / den;
}

/* Is the (2x supersampled) point inside the "Y" glyph of the logo? Unit space = 1000. */
static int in_y(int sx, int sy, int s2) {
    int64_t hw = 78LL * s2 / 1000;
    int64_t hw2 = hw * hw;
    #define P(v) ((int64_t)(v) * s2 / 1000)
    if (seg_dist2(sx, sy, P(240), P(225), P(500), P(540)) <= hw2) return 1;
    if (seg_dist2(sx, sy, P(760), P(225), P(500), P(540)) <= hw2) return 1;
    if (seg_dist2(sx, sy, P(500), P(540), P(500), P(790)) <= hw2) return 1;
    #undef P
    return 0;
}

static int in_tile(int sx, int sy, int s2) {
    int r = s2 / 4;
    int dx = 0, dy = 0;
    if (sx < r) dx = r - sx; else if (sx >= s2 - r) dx = sx - (s2 - r) + 1;
    if (sy < r) dy = r - sy; else if (sy >= s2 - r) dy = sy - (s2 - r) + 1;
    return dx * dx + dy * dy <= r * r;
}

static void draw_logo(int x0, int y0, int size, int screen_h) {
    int s2 = size * 2;
    for (int py = 0; py < size; py++) {
        rgb_t bg   = bg_at(y0 + py, screen_h);
        rgb_t tile = gfx_mix(TILE_TOP, TILE_BOTTOM, (py * 256) / size);
        for (int px = 0; px < size; px++) {
            uint32_t r = 0, g = 0, b = 0;
            for (int k = 0; k < 4; k++) {
                int sx = px * 2 + (k & 1), sy = py * 2 + (k >> 1);
                rgb_t c = bg;
                if (in_tile(sx, sy, s2)) c = in_y(sx, sy, s2) ? COL_WHITE : tile;
                r += (c >> 16) & 0xFF; g += (c >> 8) & 0xFF; b += c & 0xFF;
            }
            gfx_pixel(x0 + px, y0 + py, ((r / 4) << 16) | ((g / 4) << 8) | (b / 4));
        }
    }
}

static void log_line(int x, int y, int scale, const char *text) {
    gfx_text(x, y, "[", scale, COL_DIM);
    gfx_text(x + 8 * scale, y, " OK ", scale, COL_OK);
    gfx_text(x + 8 * 5 * scale, y, "]", scale, COL_DIM);
    gfx_text(x + 8 * 7 * scale, y, text, scale, COL_TEXT);
}

void splash_draw(const bootinfo_t *bi) {
    int W = (int)gfx_width(), H = (int)gfx_height(), cx = W / 2;

    gfx_vgradient(0, 0, W, H, BG_TOP, BG_BOTTOM);

    /* Logo + title */
    int logo = H / 4; if (logo < 96) logo = 96;
    int y = H / 16;
    draw_logo(cx - logo / 2, y, logo, H);
    y += logo + H / 36;

    int tscale = H / 110; if (tscale < 2) tscale = 2;
    gfx_text_center(cx + tscale, y + tscale, "YAZAN OS", tscale, 0x06101F);   /* soft shadow */
    gfx_text_center(cx, y, "YAZAN OS", tscale, COL_WHITE);
    y += 8 * tscale + H / 60;

    int sscale = H >= 700 ? 2 : 1;
    gfx_text_center(cx, y, "Version 0.1.0  |  Phase 1: Boot + Kernel", sscale, COL_DIM);
    y += 8 * sscale + H / 28;

    /* Boot status panel */
    char l[6][56];
    uint64_t mb = bi->mem_usable >> 20;
    ksnprintf(l[0], sizeof l[0], "Bootloader : %s", bi->loader[0] ? bi->loader : "Multiboot2");
    ksnprintf(l[1], sizeof l[1], "CPU        : %s (x86_64)", bi->cpu_vendor);
    ksnprintf(l[2], sizeof l[2], "Display    : %ux%ux%u framebuffer", bi->fb_w, bi->fb_h, bi->fb_bpp);
    ksnprintf(l[3], sizeof l[3], "Memory     : %lu MB usable RAM", (unsigned long)mb);
    ksnprintf(l[4], sizeof l[4], "Kernel     : 0x%lx - 0x%lx (%lu KB)", (unsigned long)bi->kernel_start,
              (unsigned long)bi->kernel_end, (unsigned long)((bi->kernel_end - bi->kernel_start) >> 10));
    ksnprintf(l[5], sizeof l[5], "Long mode  : 64-bit GDT, 4 GB identity map");

    int ls = W >= 900 ? 2 : 1;
    int line_h = 8 * ls + 8 * ls / 2 + 2;
    int pw = 52 * 8 * ls + 48; if (pw > W - 24) pw = W - 24;
    int ph = 6 * line_h + 32;
    int px = cx - pw / 2;
    gfx_fill_rrect(px, y, pw, ph, 14, COL_PANEL);
    for (int i = 0; i < 6; i++) log_line(px + 24, y + 16 + i * line_h, ls, l[i]);

    gfx_text_center(cx, H - 8 * sscale - H / 24,
                    "Kernel is running. Next: IDT, memory manager, timer, keyboard.", sscale, COL_DIM);
}
