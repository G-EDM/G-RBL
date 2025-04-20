//############################################################################################################
//
//  ██████        ███████ ██████  ███    ███  
// ██             ██      ██   ██ ████  ████  
// ██   ███ █████ █████   ██   ██ ██ ████ ██ 
// ██    ██       ██      ██   ██ ██  ██  ██ 
//  ██████        ███████ ██████  ██      ██ 
//
// This is a beta version for testing purposes.
// Only for personal use. Commercial use or redistribution without permission is prohibited. 
// Copyright (c) Roland Lautensack    
//
//############################################################################################################

#include "definitions.h"


volatile bool protocol_ready = false;

DMA_ATTR volatile bool lock_reference_voltage = false;

volatile float tool_diameter                    = 0.0;     // just a default tool diamter set to 0.0 if gcode takes care of it
volatile float pwm_frequency                    = 20000.0; 
volatile float pwm_duty                         = 12.0;    
volatile int   operation_mode                   = 1;       // 1 = drill/sinker, 2 = reamer (just a constant up/down movement without progressing down)
volatile bool  z_no_home                        = true;    // set to true if no homing is needed and edm process starts without homing
volatile float vsense_drop_range_min            = 2.0;     // target range bottom. We try to be between min and max and hodl there
volatile float vsense_drop_range_max            = 30.0;    // target range top. A default range from min+10 it could be min+40. 
                                                           // That produces lesser motion but a little motion is not a bad thing
                                                           // for a more steady burn use min+20/30/40
volatile float probe_trigger_current            = 6.0;  
volatile int   probe_logic_low_trigger_treshold = 90;   
volatile float VSENSE_DROP_RANGE_SHORT          = 75.0;   // voltage drops above this value are concidered a full short circuit
volatile float pwm_duty_probing                 = 20.0;   // 20% duty for probing / maybe use a higher value and lower current?
volatile int   pwm_frequency_probing            = 40000;  // 40khz for probing
volatile int   wire_spindle_speed               = 400;    // speed in step frequency
volatile int   wire_spindle_speed_probing       = 140;    // spindle speed for probing in step frequency
volatile int   finish_after_mm                  = 5;      // if the tool moves x mm without load it considers the process as done
volatile float sinker_travel_mm                 = 10.0;   // the maximum travel into the workpiece; Note: EDM has electrode wear of x percent and setting this to 1mm doesn't produce a 1mm deep hole
volatile bool  enable_spindle                   = true;


DMA_ATTR volatile float position_target[6]         = {};
DMA_ATTR volatile bool  use_stop_and_go_flags      = true;
DMA_ATTR volatile bool  enable_dpm_rxtx            = false;
DMA_ATTR volatile bool  mpos_changed               = false;
DMA_ATTR volatile bool  force_redraw               = false;
DMA_ATTR volatile bool  edm_process_done           = false; // if set to true in the edm process the task gets stopped from within the mainloop and also disables the spark PWM
DMA_ATTR volatile bool  edm_process_is_running     = false; // this is true if the machine actually is doing work
DMA_ATTR volatile bool  limit_touched              = false;
DMA_ATTR volatile bool  probe_touched              = false;
DMA_ATTR volatile bool  cutting_depth_reached      = false;
DMA_ATTR volatile bool  disable_spark_for_flushing = true;
DMA_ATTR volatile bool  simulate_gcode             = false; // runs the gcode file without moving z


DMA_ATTR volatile int64_t flush_retract_timer_us     = 0;
DMA_ATTR volatile int64_t flush_retract_after        = 0;     // retract tool after x us to allow flushing; set to 0 to disable;
DMA_ATTR volatile float   flush_retract_mm           = 5.0;
DMA_ATTR volatile int     stop_edm_task              = 0;
DMA_ATTR volatile int     start_edm_task             = 0;
DMA_ATTR volatile int     flush_offset_steps         = 20;    // after moving the axis up for flushing it goes back down to the start position minus the offset steps



DMA_ATTR volatile float   reamer_travel_mm = 10.0;  // the travel distance for the reamer up/down oscillation. 
                                                    // NOTE: In reamer moder the axis is homed and then moves down the reamer_travel_mm!!
                                                    // This is to not overschoot the home switch. It is necessary to make sure the axis can move the 
                                                    // given mm down without crashing.
DMA_ATTR volatile float   reamer_duration  = 0.0;   // the duration of the reasming process in seconds . 0.0 means it runs until manually stopped


DMA_ATTR volatile int     motion_plan           = 0;
DMA_ATTR volatile float   dpm_voltage_probing   = 15.0; // only used if dpm communication is enabled
DMA_ATTR volatile float   dpm_current_probing   = 0.2;  // only used if dpm communication is enabled






char get_axis_name(uint8_t axis) {
    switch (axis) {
        case X_AXIS:
            return 'X';
        case Y_AXIS:
            return 'Y';
        case Z_AXIS:
            return 'Z';
        default:
            return '-';
    }
}


const char* get_axis_name_const(uint8_t axis) {
    switch (axis) {
        case X_AXIS:
            return "X";
        case Y_AXIS:
            return "Y";
        case Z_AXIS:
            return "Z";
        default:
            return "-";
    }
}



DMA_ATTR volatile speeds process_speeds;
volatile speeds_mm_min   process_speeds_mm_min;
DMA_ATTR volatile gedm_conf gconf;
DMA_ATTR volatile retraction_distances retconf;








DMA_ATTR GAxis* g_axis[MAX_N_AXIS];
axis_conf axis_configuration[MAX_N_AXIS] = { 
    { "X", DEFAULT_X_MAX_TRAVEL, 0.0, X_STEPS_PER_MM }, 
    { "Y", DEFAULT_Y_MAX_TRAVEL, 0.0, Y_STEPS_PER_MM },
    { "Z", DEFAULT_Z_MAX_TRAVEL, 0.0, Z_STEPS_PER_MM },
    { "A", 100.0, 0.0, 0 },
    { "B", 100.0, 0.0, 0 },
    { "C", 100.0, 0.0, 0 } 
  };