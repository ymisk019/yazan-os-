/* Host-side renderer: draws the real splash code into a fake framebuffer and
 * writes a PPM image. Lets you iterate on the UI without booting a VM. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gfx.h"
#include "splash.h"

int main(int argc, char **argv) {
    const char *out = argc > 1 ? argv[1] : "preview.ppm";
    uint32_t w = argc > 2 ? (uint32_t)atoi(argv[2]) : 1024;
    uint32_t h = argc > 3 ? (uint32_t)atoi(argv[3]) : 768;
    uint8_t *buf = calloc(w * h, 4);

    gfx_fb_t fb = { .base = buf, .pitch = w * 4, .width = w, .height = h, .bpp = 32,
                    .rpos = 16, .rsize = 8, .gpos = 8, .gsize = 8, .bpos = 0, .bsize = 8 };
    gfx_init(&fb);

    bootinfo_t bi; memset(&bi, 0, sizeof bi);
    strcpy(bi.loader, "GRUB 2.12");
    strcpy(bi.cpu_vendor, "GenuineIntel");
    bi.mem_usable = 255ULL << 20;
    bi.fb_w = w; bi.fb_h = h; bi.fb_bpp = 32; bi.fb_ok = 1;
    bi.kernel_start = 0x100000; bi.kernel_end = 0x118000;
    splash_draw(&bi);

    FILE *f = fopen(out, "wb");
    fprintf(f, "P6\n%u %u\n255\n", w, h);
    for (uint32_t i = 0; i < w * h; i++) {
        uint32_t px = ((uint32_t *)buf)[i];
        fputc((px >> 16) & 0xFF, f); fputc((px >> 8) & 0xFF, f); fputc(px & 0xFF, f);
    }
    fclose(f);
    printf("wrote %s (%ux%u)\n", out, w, h);
    return 0;
}
