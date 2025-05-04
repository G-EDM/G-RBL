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

#include "driver/mcpwm.h"

#include "Config.h"
#include "Error.h"
#include "Probe.h"
#include "System.h"

class G_EDM_SPINDLE {

    protected:

        int  dir_pin;
        int  step_pin;
        bool is_running;
        bool dir_inverted;
        int  frequency;
        int  default_frequency;
        int  backup_frequency;
        int  steps_per_mm;

    public:

        G_EDM_SPINDLE();
        void  setup_spindle( int _dir_pin, int _step_pin );
        void  start_spindle( void );
        void  stop_spindle( void );
        void  set_spindle_direction( bool direction );
        void  reverse_direction( void );
        void  reset_spindle( void );
        void  backup_spindle_speed( void );
        void  restore_spindle_speed( void );
        bool  spindle_is_running( void );
        bool  dir_is_inverted( void );
        bool  set_speed( int __frequency, bool backup = true );
        bool  restore_backup_speed( void );
        int   get_speed( void );
        int   convert_mm_min_to_frequency( float mm_min );
        float convert_frequency_to_mm_min( float frequency );
        int   get_wire_steps_per_mm( void );
        int   set_wire_steps_per_mm( int __steps_per_mm );
        bool  manual_motion( float total_mm, bool negative = false );
        int get_step_pin( void );
        void init_gpio_input( void );
};


extern G_EDM_SPINDLE gedm_spindle;