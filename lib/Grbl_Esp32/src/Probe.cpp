/*
  Probe.cpp - code pertaining to probing methods
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

  2023 - Roland Lautensack (G-EDM) This file was heavily edited and may no longer be compatible with the default grbl
*/

#include "Grbl.h"

static bool is_probe_away;

GRBL_PROBE gprobe;

IRAM_ATTR void GRBL_PROBE::set_probe_direction( bool is_away ){ 
    is_probe_away = is_away; 
}

IRAM_ATTR bool GRBL_PROBE::probe_get_state() {
    return probe_touched;
}

IRAM_ATTR bool GRBL_PROBE::probe_state_monitor() {
    if ( probe_get_state() ^ is_probe_away || simulate_gcode ) {
        sys_probe_state = Probe::Off;
        memcpy(sys_probe_position, sys_position, sizeof(sys_position));
        return true;
    }
    return false;
}
