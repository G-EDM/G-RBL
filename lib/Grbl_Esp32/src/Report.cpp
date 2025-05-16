/*
  Report.cpp - reporting and messaging methods
  Part of Grbl

  Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC

	2018 -	Bart Dring This file was modified for use on the ESP32
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

  2023 - Roland Lautensack (G-EDM) This file was heavily edited and may no longer be compatible with the default grbl

*/
#include "Grbl.h"
#include <map>

#ifdef REPORT_HEAP
EspClass esp;
#endif
const int DEFAULTBUFFERSIZE = 64;

static const int coordStringLen = 20;
static const int axesStringLen  = coordStringLen * MAX_N_AXIS;


void report_status_message(Error status_code) {
    /*switch (status_code) {
        case Error::Ok:  // Error::Ok
            if ( !filehandler.is_available() ) {
                SD_ready_next = true;  
            }
            break;
        default:
            // do we need to stop a running SD job?
            if ( !filehandler.is_available() ) {
                if (status_code == Error::GcodeUnsupportedCommand) {
                    // don't close file
                    SD_ready_next = true; 
                } else {
                    filehandler.close_file();
                }
                return;
            }
    }*/
}

void report_alarm_message(ExecAlarm alarm_code) {
    sys_last_alarm = alarm_code;
    delay_ms(500);
}
std::map<Message, const char*> MessageText = {
    { Message::CriticalEvent, "Reset to continue" },
    { Message::AlarmLock, "'$H'|'$X' to unlock" },
    { Message::AlarmUnlock, "Caution: Unlocked" },
    { Message::Enabled, "Enabled" },
    { Message::Disabled, "Disabled" },
    { Message::CheckLimits, "Check limits" },
    { Message::ProgramEnd, "Program End" },
    { Message::RestoreDefaults, "Restoring defaults" },
    { Message::SpindleRestore, "Restoring spindle" },
    { Message::SleepMode, "Sleeping" },
    // { Message::SdFileQuit, "Reset during SD file at line: %d" },
};

void IRAM_ATTR mpos_to_wpos(float* position) {
    float* wco    = get_wco();
    auto n_axis = N_AXIS;
    for (int idx = 0; idx < n_axis; idx++) {
        position[idx] -= wco[idx];
    }
}

float* get_wco() {
    static float wco[MAX_N_AXIS];
    auto n_axis = N_AXIS;
    for (int idx = 0; idx < n_axis; idx++) {
        // Apply work coordinate offsets and tool length offset to current position.
        wco[idx] = gc_state.coord_system[idx] + gc_state.coord_offset[idx];
        if (idx == TOOL_LENGTH_OFFSET_AXIS) {
            wco[idx] += gc_state.tool_length_offset;
        }
    }
    return wco;
}
