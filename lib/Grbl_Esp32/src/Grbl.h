#pragma once

/*
  Grbl.h - Header for system level commands and real-time processes
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


#include "Config.h"
#include "NutsBolts.h"
#include "Error.h"
#include "Probe.h"
#include "System.h"
#include "GCode.h"
#include "gedm_planner/Planner.h"
#include "machine_limits.h"
#include "MotionControl.h"
#include "Protocol.h"
#include "Serial.h"
#include "Report.h"
#include "Motors/Motors.h"
#include "InputBuffer.h"
#include "Settings.h"
#include "SettingsDefinitions.h"

class GRBL_MAIN {

    public:
        GRBL_MAIN( void );
        void reset_variables( void );
        void grbl_init( void );

};

extern GRBL_MAIN grbl;

bool cartesian_to_motors(float* target, plan_line_data_t* pl_data, float* position);
bool kinematics_pre_homing(uint8_t cycle_mask);
bool limitsCheckTravel(float* target);  // weak in Limits.cpp; true if out of range
void motors_to_cartesian(float* cartestian, float* motors, int n_axis);  // weak definition