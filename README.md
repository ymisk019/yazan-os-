# Yazan OS

نظام تشغيل x86_64 حقيقي يُبنى من الصفر ويقلع كملف ISO على QEMU وVirtualBox (وبإذن الله على أجهزة حقيقية لاحقًا).

## الحالة: المرحلة 1 ✅ — Boot → Kernel → شاشة Yazan OS

| الجزء | الملف | الوصف |
|---|---|---|
| Bootloader | GRUB 2 (Multiboot2) + `boot/boot.S` | GRUB يحمّل النواة ويجهّز Framebuffer؛ `boot.S` يتحقق من CPUID/Long Mode، يبني Page Tables (4GB identity map)، يفعّل PAE+LME+Paging، يحمّل GDT 64-bit ويقفز إلى C |
| Kernel | `kernel/kmain.c` | يقرأ معلومات الإقلاع (Multiboot2): اسم الـBootloader، خريطة الذاكرة، الـFramebuffer، والـCPU |
| Graphics | `kernel/gfx.c` | رسم بكسلات، مستطيلات، تدرّجات، نص 8x8 — بدون FPU وبدون Heap |
| Splash | `kernel/splash.c` | شاشة Yazan OS بشعار أصلي ولوحة حالة الإقلاع |
| Debug | `kernel/serial.c` | سجل على COM1 (يظهر في الطرفية مع QEMU) |
| Panic | `kernel/panic.c` | شاشة Kernel Panic واضحة (Framebuffer أو VGA text) + سجل serial |
| Fallback | `kernel/vgatext.c` | وضع نص 80x25 إن لم يتوفر Framebuffer |

## المتطلبات (Ubuntu / Debian / WSL2)

```bash
./scripts/setup-ubuntu.sh
# أو يدويًا:
sudo apt install build-essential grub-pc-bin grub-efi-amd64-bin xorriso mtools qemu-system-x86
```

> macOS: استخدم مترجمًا متقاطعًا `x86_64-elf-gcc` ثم `make CROSS=x86_64-elf-`.
> Windows: استخدم WSL2 للبناء، ثم شغّل الـISO في VirtualBox من Windows.

## البناء والتشغيل

```bash
make            # يبني build/yazan.elf
make check      # يتحقق من الـELF وترويسة Multiboot2
make iso        # ينتج build/yazanos.iso
make run        # تشغيل في QEMU (سجل serial في نفس الطرفية)
make run-uefi   # تشغيل في QEMU بنمط UEFI (يحتاج ovmf)
make vbox       # ينشئ ويشغّل VM في VirtualBox
make preview    # يرسم الشاشة على جهازك مباشرة إلى build/preview.ppm بدون VM
```

أمر QEMU اليدوي:
```bash
qemu-system-x86_64 -cdrom build/yazanos.iso -m 256M -vga std -serial stdio
```

VirtualBox يدويًا: New → Type: Other / Version: Other/Unknown (64-bit) → RAM 256MB → بدون قرص → Settings → Storage → اختر `yazanos.iso` → Start.
(يجب أن يكون Graphics Controller = VBoxVGA، وبدون EFI في هذه المرحلة.)

## النتيجة المتوقعة عند الإقلاع
شاشة 1024x768 بتدرّج أزرق، شعار Y، عنوان YAZAN OS، ولوحة فيها ست خطوات `[ OK ]`. وفي السجل (serial):
```
=== Yazan OS 0.1.0 (x86_64) ===
[boot] loader : GRUB 2.x
[boot] cpu    : GenuineIntel
[boot] memory : ~255 MB usable
...
[boot] Phase 1 complete. Halting.
```
إذا ظهر على الشاشة `ERR:M` أو `ERR:C` أو `ERR:L` فهذا خطأ مبكر من `boot.S` (Magic / CPUID / Long Mode).

## هيكل المشروع
```
boot/boot.S        مدخل Multiboot2 + الانتقال إلى 64-bit
boot/grub.cfg      إعداد GRUB داخل الـISO
linker.ld          النواة عند 1MB
kernel/            كود النواة بلغة C
tools/preview.c    معاينة الواجهة على الجهاز المضيف
scripts/           إعداد البيئة + VirtualBox
Makefile
```

## حدود معروفة في المرحلة 1 (مقصودة)
- لا توجد IDT بعد، لذا أي استثناء CPU سيعيد التشغيل (Triple Fault) بدل Panic. **هذا أول بند في المرحلة 2.**
- النواة والـPaging بنمط identity-map لأول 4GB؛ الفصل بين Kernel/User (Higher-half + Ring 3) يأتي مع إدارة الذاكرة والعمليات.
- الـFramebuffer مخزّن مؤقتًا بنمط WB الافتراضي؛ سنضبط PAT/Write-Combining لاحقًا للأجهزة الحقيقية.
- الـFramebuffer يجب أن يكون تحت 4GB (كل الأجهزة الشائعة وVM كذلك).

## خارطة الطريق
1. ✅ Boot → Kernel + شاشة Yazan OS
2. ⏭ IDT + استثناءات + Panic حقيقي + طباعة نصية (console) فوق الـFramebuffer
3. Physical Memory Manager + Paging + Heap (kmalloc)
4. PIC/APIC + Timer (PIT/HPET) + Keyboard
5. Framebuffer مزدوج (Back buffer) ورسم أسرع
6. Mouse (PS/2)
7. GUI toolkit
8. Window Manager
9. Desktop + Taskbar + Start menu
10. File System (RAM disk ثم FAT32)
11. التطبيقات (Terminal, File Manager, Settings, Text Editor, Calculator, About)
12. تحسين الأداء والاستقرار (Ring 3, syscalls, Threads)
