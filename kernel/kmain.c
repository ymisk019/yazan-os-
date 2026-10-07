/* Yazan OS kernel entry (64-bit). Called from boot/boot.S in long mode. */
#include <stdint.h>
#include "mb2.h"
#include "bootinfo.h"
#include "gfx.h"
#include "splash.h"
#include "serial.h"
#include "vgatext.h"
#include "panic.h"
#include "io.h"
#include "kstring.h"

extern char __kernel_start[], __kernel_end[];

static void cpu_vendor(char out[13]) {
    uint32_t a, b, c, d;
    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(0));
    memcpy(out + 0, &b, 4);
    memcpy(out + 4, &d, 4);
    memcpy(out + 8, &c, 4);
    out[12] = 0;
}

static void parse_mb2(uintptr_t mbi, bootinfo_t *bi, gfx_fb_t *fb) {
    const uint8_t *p = (const uint8_t *)mbi + 8;           /* skip total_size + reserved */
    for (;;) {
        const mb2_tag_t *t = (const mb2_tag_t *)p;
        if (t->type == MB2_TAG_END) break;

        if (t->type == MB2_TAG_LOADER) {
            const char *s = (const char *)(p + 8);
            size_t n = t->size - 8; if (n > sizeof bi->loader) n = sizeof bi->loader;
            size_t i = 0; for (; i + 1 < n && s[i]; i++) bi->loader[i] = s[i];
            bi->loader[i] = 0;
        } else if (t->type == MB2_TAG_MMAP) {
            const mb2_mmap_tag_t *m = (const mb2_mmap_tag_t *)t;
            const uint8_t *e = p + sizeof *m, *end = p + t->size;
            for (; e + m->entry_size <= end; e += m->entry_size) {
                const mb2_mmap_entry_t *me = (const mb2_mmap_entry_t *)e;
                if (me->type == MB2_MMAP_AVAILABLE) bi->mem_usable += me->len;
            }
        } else if (t->type == MB2_TAG_FB) {
            const mb2_fb_tag_t *f = (const mb2_fb_tag_t *)t;
            bi->fb_w = f->width; bi->fb_h = f->height; bi->fb_bpp = f->bpp;
            if (f->type == MB2_FB_RGB && (f->bpp == 32 || f->bpp == 24 || f->bpp == 16)) {
                fb->base = (uint8_t *)(uintptr_t)f->addr;
                fb->pitch = f->pitch; fb->width = f->width; fb->height = f->height; fb->bpp = f->bpp;
                fb->rpos = f->red_pos;   fb->rsize = f->red_size;
                fb->gpos = f->green_pos; fb->gsize = f->green_size;
                fb->bpos = f->blue_pos;  fb->bsize = f->blue_size;
                bi->fb_ok = 1;
            }
        }
        p += (t->size + 7u) & ~7u;                          /* tags are 8-byte aligned */
    }
}

void kmain(uintptr_t mbi, uint32_t magic) {
    serial_init();
    klog("\n=== Yazan OS 0.1.0 (x86_64) ===\n");

    if (magic != MB2_BOOT_MAGIC) kpanic("Bad Multiboot2 magic: 0x%x", magic);

    bootinfo_t bi; memset(&bi, 0, sizeof bi);
    gfx_fb_t fb;   memset(&fb, 0, sizeof fb);
    bi.kernel_start = (uintptr_t)__kernel_start;
    bi.kernel_end   = (uintptr_t)__kernel_end;
    cpu_vendor(bi.cpu_vendor);
    parse_mb2(mbi, &bi, &fb);

    klog("[boot] loader : %s\n", bi.loader);
    klog("[boot] cpu    : %s\n", bi.cpu_vendor);
    klog("[boot] memory : %lu MB usable\n", (unsigned long)(bi.mem_usable >> 20));
    klog("[boot] kernel : 0x%lx - 0x%lx\n", (unsigned long)bi.kernel_start, (unsigned long)bi.kernel_end);

    if (bi.fb_ok) {
        gfx_init(&fb);
        klog("[boot] display: %ux%ux%u framebuffer\n", bi.fb_w, bi.fb_h, bi.fb_bpp);
        splash_draw(&bi);
    } else {
        klog("[boot] display: no RGB framebuffer, using VGA text mode\n");
        char line[80];
        vga_clear(0x1F);
        vga_puts_at(2, 1, "YAZAN OS 0.1.0 - Phase 1: Boot + Kernel", 0x1F);
        ksnprintf(line, sizeof line, "CPU: %s   RAM: %lu MB", bi.cpu_vendor, (unsigned long)(bi.mem_usable >> 20));
        vga_puts_at(2, 3, line, 0x1F);
        vga_puts_at(2, 5, "Kernel is running in 64-bit long mode.", 0x1F);
    }

    klog("[boot] Phase 1 complete. Halting.\n");
    cpu_halt_forever();
}
