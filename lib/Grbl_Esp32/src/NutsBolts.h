#pragma once

/*
  NutsBolts.h - Header for system level commands and real-time processes
  Part of Grbl
  Copyright (c) 2014-2016 Sungeun K. Jeon for Gnea Research LLC

	2018 -	Bart Dring This file was modifed for use on the ESP32
					CPU. Do not use this with Grbl for atMega328P

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Config.h"

// #define false 0
// #define true 1

enum class DwellMode : uint8_t {
    Dwell      = 0,  // (Default: Must be zero)
    SysSuspend = 1,  //G92.1 (Do not alter value)
};

const int MAX_AXES = 6;

#define X2_AXIS (X_AXIS + MAX_AXES)
#define Y2_AXIS (Y_AXIS + MAX_AXES)
#define Z2_AXIS (Z_AXIS + MAX_AXES)
#define A2_AXIS (A_AXIS + MAX_AXES)
#define B2_AXIS (B_AXIS + MAX_AXES)
#define C2_AXIS (C_AXIS + MAX_AXES)
static inline int toMotor2(int axis) {
    return axis + MAX_AXES;
}

// Conversions
const double MM_PER_INCH = (25.40);
const double INCH_PER_MM = (0.0393701);

// Useful macros
#define clear_vector(a) memset(a, 0, sizeof(a))
#define clear_vector_float(a) memset(a, 0.0, sizeof(float) * MAX_N_AXIS)
// #define clear_vector_long(a) memset(a, 0.0, sizeof(long)*MAX_N_AXIS)
#define MAX(a, b) (((a) > (b)) ? (a) : (b))  // changed to upper case to remove conflicts with other libraries
#define MIN(a, b) (((a) < (b)) ? (a) : (b))  // changed to upper case to remove conflicts with other libraries
#define isequal_position_vector(a, b) !(memcmp(a, b, sizeof(float) * MAX_N_AXIS))

// Bit field and masking macros
// bit(n) is defined in Arduino.h.  We redefine it here so we can apply
// the static_cast, thus making it work with scoped enums
#undef bit
#define bit(n) (1 << static_cast<unsigned int>(n))

#define bit_true(x, mask) (x) |= (mask)
#define bit_false(x, mask) (x) &= ~(mask)
#define bit_istrue(x, mask) ((x & mask) != 0)
#define bit_isfalse(x, mask) ((x & mask) == 0)
#define bitnum_true(x, num) (x) |= bit(num)
#define bitnum_istrue(x, num) ((x & bit(num)) != 0)

// Read a floating point value from a string. Line points to the input buffer, char_counter
// is the indexer pointing to the current character of the line, while float_ptr is
// a pointer to the result variable. Returns true when it succeeds
uint8_t read_float(const char* line, uint8_t* char_counter, float* float_ptr);

// Non-blocking delay function used for general operation and suspend features.
bool delay_msec(int32_t milliseconds, DwellMode mode);

// Delays variable-defined milliseconds. Compiler compatibility fix for _delay_ms().
void delay_ms(uint16_t ms);

// Computes hypotenuse, avoiding avr-gcc's bloated version and the extra error checking.
float hypot_f(float x, float y);

char* trim(char* value);

template <class T>
void swap(T& a, T& b) {
    T c(a);
    a = b;
    b = c;
}
