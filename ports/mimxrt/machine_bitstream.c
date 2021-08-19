/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Jim Mussared
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// This is a translation of the cycle counter implementation in ports/stm32/machine_bitstream.c.

#include "py/mpconfig.h"
#include "py/mphal.h"

#if MICROPY_PY_MACHINE_BITSTREAM

#define NS_TICKS_OVERHEAD (6)

void machine_bitstream_high_low(mp_hal_pin_obj_t pin, uint32_t *timing_ns, const uint8_t *buf, size_t len) __attribute__((section(".ram_functions")));
void machine_bitstream_high_low(mp_hal_pin_obj_t pin, uint32_t *timing_ns, const uint8_t *buf, size_t len) {
    uint32_t fcpu_mhz = mp_hal_get_cpu_freq() / 1000000;
    // Convert ns to us ticks [high_time_0, period_0, high_time_1, period_1].
    for (size_t i = 0; i < 4; ++i) {
        timing_ns[i] = fcpu_mhz * timing_ns[i] / 1000;
        if (timing_ns[i] > NS_TICKS_OVERHEAD) {
            timing_ns[i] -= NS_TICKS_OVERHEAD;
        }
        if (i % 2 == 1) {
            // Convert low_time to period (i.e. add high_time).
            timing_ns[i] += timing_ns[i - 1];
        }
    }
    // Enable the CPU cycle counter, which is not always enabled.
    mp_hal_ticks_cpu_init();

    uint32_t irq_state = mp_hal_quiet_timing_enter();

    for (size_t i = 0; i < len; ++i) {
        uint8_t b = buf[i];
        for (size_t j = 0; j < 8; ++j) {
            uint32_t start_ticks = mp_hal_ticks_cpu();
            mp_hal_pin_high(pin);
            uint32_t *t = &timing_ns[b >> 6 & 2];
            while ((mp_hal_ticks_cpu() - start_ticks) < t[0]) {
            }
            mp_hal_pin_low(pin);
            b <<= 1;
            while ((mp_hal_ticks_cpu() - start_ticks) < t[1]) {
            }
        }
    }

    mp_hal_quiet_timing_exit(irq_state);
}

#endif // MICROPY_PY_MACHINE_BITSTREAM
