#pragma once
void kpanic(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));
