/* Kernel panic: clear message on screen (framebuffer or VGA text) + serial log, then halt. */
#include "panic.h"
#include "gfx.h"
#include "serial.h"
#include "vgatext.h"
#include "kstring.h"
#include "io.h"

void kpanic(const char *fmt, ...) {
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    kvsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    klog("\n*** KERNEL PANIC: %s ***\n", msg);

    if (gfx_ready()) {
        int W = (int)gfx_width();
        gfx_clear(0x8B1111);
        gfx_text(40, 40, "YAZAN OS - KERNEL PANIC", 3, 0xFFFFFF);
        gfx_text(40, 40 + 8 * 3 + 24, "The system has been halted to protect your data.", 2, 0xFFD6D6);
        int cols = (W - 80) / 16; if (cols < 8) cols = 8;
        int y = 40 + 8 * 3 + 24 + 16 * 2 + 16;
        for (const char *p = msg; *p; ) {
            char line[128]; int n = 0;
            while (p[n] && n < cols && n < 127) n++;
            memcpy(line, p, (size_t)n); line[n] = 0;
            gfx_text(40, y, line, 2, 0xFFFFFF);
            y += 8 * 2 + 6; p += n;
        }
    } else {
        vga_clear(0x4F);
        vga_puts_at(2, 2, "YAZAN OS - KERNEL PANIC", 0x4F);
        vga_puts_at(2, 4, msg, 0x4F);
    }
    cpu_halt_forever();
}
