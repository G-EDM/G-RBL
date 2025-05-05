#pragma once

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

#include "ili9341_pinout.h"
//#include "ili9341_config.h"
#include <SPIFFS.h>
#include "FS.h"
#include <SPI.h>
//#include <TFT_eSPI.h>
#include "gedm_sensors/sensors.h"
#include "pwm_controller/pwm_controller.h"
#include "gedm_api/api.h"
#include "gedm_api/probing_routines.h"
#include "gedm_spindle/gedm_spindle.h"
#include "gedm_planner/Planner.h"
#include "widgets/widget_item.h"
#include "widgets/char_helpers.h"

using setter_param      = std::pair<int, const char*>;
using getter_param      = PhantomSettingsPackage;
using setter_param_axis = PhantomSettingsPackage;


enum sinker_axis_modes {
    SINK_Z_NEG = 0,
    SINK_X_POS = 1,
    SINK_X_NEG = 2,
    SINK_Y_POS = 3,
    SINK_Y_NEG = 4
};

extern TaskHandle_t      ui_task_handle;
extern DRAM_ATTR int64_t last_minimal_ui_update_micros;
void   IRAM_ATTR ui_task( void * parameter );

class G_EDM_UI_CONTROLLER : 
    public PhantomWidgetItem,
    public G_EDM_SENSORS, 
    public G_EDM_PWM_CONTROLLER, 
    public G_EDM_SPINDLE, 
    public G_EDM_PROBE_ROUTINES 
{

    public:

        G_EDM_UI_CONTROLLER();
        ~G_EDM_UI_CONTROLLER(){};

        sinker_axis_modes selected_sink_mode;

        bool    is_ready;
        int     single_axis_mode_direction;
        bool    has_gcode_file;
        uint8_t active_page;

        void IRAM_ATTR reset_flush_retract_timer( void );
        bool IRAM_ATTR check_if_time_to_flush( void );

        void configure( void );
        void setup_events( void );
        void init( void );
        void run_once_if_ready( void );
        void start_ui_task( void );
        void restart_i2s_enforce( void );
        void sinker_drill_single_axis( void );
        void sd_card_task( void );
        void wire_gcode_task( void );
        void pre_process_defaults( void );
        void reset_defaults( void );
        void set_operation_mode( int mode );
        void start_process(void);
        void reset_after_job(void);
        void alarm_handler( void );
        int  change_sinker_axis_direction( int direction );
        bool move_sinker_axis_to_zero( void );
        bool get_is_ready( void );
        void set_is_ready( void );
        bool ready_to_run( void );
        bool motion_input_is_blocked(void);
        bool probe_prepare( void );
        void probe_done( void );

        int probe_dimension = 0;
        int probe_backup_frequency;

    private:
        int64_t start_time;


};

extern G_EDM_UI_CONTROLLER ui_controller;