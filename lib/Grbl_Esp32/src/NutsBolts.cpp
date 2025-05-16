/*
  NutsBolts.cpp - Shared functions
  Part of Grbl

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud

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

#include "Grbl.h"
#include <cstring>

const int MAX_INT_DIGITS = 8;  // Maximum number of digits in int32 (and float)

// Extracts a floating point value from a string. The following code is based loosely on
// the avr-libc strtod() function by Michael Stumpf and Dmitry Xmelkov and many freely
// available conversion method examples, but has been highly optimized for Grbl. For known
// CNC applications, the typical decimal value is expected to be in the range of E0 to E-4.
// Scientific notation is officially not supported by g-code, and the 'E' character may
// be a g-code word on some CNC systems. So, 'E' notation will not be recognized.
// NOTE: Thanks to Radu-Eosif Mihailescu for identifying the issues with using strtod().
uint8_t read_float(const char* line, uint8_t* char_counter, float* float_ptr) {
    const char*   ptr = line + *char_counter;
    unsigned char c;
    // Grab first character and increment pointer. No spaces assumed in line.
    c = *ptr++;
    // Capture initial positive/minus character
    bool isnegative = false;
    if (c == '-') {
        isnegative = true;
        c          = *ptr++;
    } else if (c == '+') {
        c = *ptr++;
    }

    // Extract number into fast integer. Track decimal in terms of exponent value.
    uint32_t intval    = 0;
    int8_t   exp       = 0;
    uint8_t  ndigit    = 0;
    bool     isdecimal = false;
    while (1) {
        c -= '0';
        if (c <= 9) {
            ndigit++;
            if (ndigit <= MAX_INT_DIGITS) {
                if (isdecimal) {
                    exp--;
                }
                intval = intval * 10 + c;
            } else {
                if (!(isdecimal)) {
                    exp++;  // Drop overflow digits
                }
            }
        } else if (c == (('.' - '0') & 0xff) && !(isdecimal)) {
            isdecimal = true;
        } else {
            break;
        }
        c = *ptr++;
    }
    // Return if no digits have been read.
    if (!ndigit) {
        return false;
    }

    // Convert integer into floating point.
    float fval;
    fval = (float)intval;
    // Apply decimal. Should perform no more than two floating point multiplications for the
    // expected range of E0 to E-4.
    if (fval != 0) {
        while (exp <= -2) {
            fval *= 0.01;
            exp += 2;
        }
        if (exp < 0) {
            fval *= 0.1;
        } else if (exp > 0) {
            do {
                fval *= 10.0;
            } while (--exp > 0);
        }
    }
    // Assign floating point value with correct sign.
    if (isnegative) {
        *float_ptr = -fval;
    } else {
        *float_ptr = fval;
    }
    *char_counter = ptr - line - 1;  // Set char_counter to next statement
    return true;
}

void delay_ms(uint16_t ms) {
    delay(ms);
}

// Non-blocking delay function used for general operation and suspend features.
bool delay_msec(int32_t milliseconds, DwellMode mode) {
    // Note: i must be signed, because of the 'i-- > 0' check below.
    int32_t i         = milliseconds / DWELL_TIME_STEP;
    int32_t remainder = i < 0 ? 0 : (milliseconds - DWELL_TIME_STEP * i);

    while (i-- > 0) {
        if (sys.abort) {
            return false;
        }
        if (mode == DwellMode::Dwell) {
            gproto.protocol_exec_rt_system();
        } else {  // DwellMode::SysSuspend
            // Execute rt_system() only to avoid nesting suspend loops.
            gproto.protocol_exec_rt_system();
        }
        delay(DWELL_TIME_STEP);  // Delay DWELL_TIME_STEP increment
    }
    delay(remainder);
    return true;
}

// Simple hypotenuse computation function.
float hypot_f(float x, float y) {
    return sqrt(x * x + y * y);
}


char* trim(char* str) {
    char* end;
    // Trim leading space
    while (::isspace((unsigned char)*str)) {
        str++;
    }
    if (*str == 0) {  // All spaces?
        return str;
    }
    // Trim trailing space
    end = str + ::strlen(str) - 1;
    while (end > str && ::isspace((unsigned char)*end)) {
        end--;
    }
    // Write new null terminator character
    end[1] = '\0';
    return str;
}
