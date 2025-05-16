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

#include <WString.h>
#include <InputBuffer.h>  
#include "Config.h"
#include <Serial.h>
#include <Error.h>
#include <Probe.h>
#include <System.h>
#include <Motors/Motors.h>
#include <GCode.h>
#include <Settings.h>
#include <SettingsDefinitions.h>
#include <machine_limits.h>


namespace api{

    IRAM_ATTR extern float* get_wpos( void );

    extern int32_t get_step_position( int axis );
    extern float   get_max_travel_possible( int axis, int direction );
    extern float   probe_pos_to_mpos( int axis );
    extern void    update_speeds( void );
    extern void    autohome_at_center( void );
    extern void    push_cmd(const char *text, bool block_until_idle = false);
    extern void    machine_to_work_pos( float *position );
    extern void    home_machine( void );
    extern void    auto_set_probepositions( void );
    extern void    set_reprobe_points( void );
    extern void    set_reprobe_point_x( float x );
    extern void    set_reprobe_point_y( float y );
    extern void    home_x( void );
    extern void    home_y( void );
    extern void    home_z( void );
    extern void    force_home_z( void );
    extern void    force_home( int axis );
    extern void    home_axis( int axis );
    extern void    block_while_homing( int axis_index );
    extern void    unlock_machine( void );
    extern void    cancel_running_job( void );
    extern void    block_while_not_idle( void );
    extern void    probe_block( void );
    extern void    reset_probe_points( void );
    extern void    set_current_position_as_probe_position( void );
    extern void    execute_retraction( void );
    extern void    auto_set_probepositions_for( int axis );
    extern void    unset_probepositions_for( int axis );
    extern bool    buffer_available( void );
    extern bool    machine_is_idle( void );
    extern bool    machine_is_fully_homed( void );
    extern bool    machine_is_fully_probed( void );
    extern bool    jog_axis(    float mm, const char *axis_name, int speed, int axis );
    extern bool    jog_up(      float mm );
    extern bool    jog_down(    float mm );
    extern bool    jog_back(    float mm );
    extern bool    jog_forward( float mm );
    extern bool    jog_left(    float mm );
    extern bool    jog_right(   float mm );

};

