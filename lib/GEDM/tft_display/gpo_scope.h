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

#ifndef GSCOPE_LOCK
#define GSCOPE_LOCK

#include <stdint.h>

#include <freertos/queue.h>

extern xQueueHandle scope_queue; 

struct gpo_config {
    bool enable_ksps  = true;
    bool enablewave   = true;
    bool drawlinear   = false;
};

struct gpo_scope_data {

    float    scope_ratio          = 1.0;
    bool     scope_active         = false;
    bool     scope_frame_ready    = false;
    bool     canvas_processing    = false;
    bool     show_values          = true;
    bool     header_drawn         = false;
    bool     has_task_queue       = false;
    bool     skip_push            = false;

    int      full_range_tresh     = 0;
    int      vrop_tresh           = 1500;
    int      motion_signal        = 0;
    int      dstate               = 1;
    int      favg_size            = 0;

    uint32_t setpoint_min         = 0;
    uint32_t setpoint_max         = 0;
    int16_t  voltage_feedback     = 0;
    int16_t  right_bar_width      = 5;
    int16_t  scope_current_cursor = 0;
    int16_t  scope_width          = 0;
    int16_t  scope_height         = 0;
    int16_t  scope_height_total   = 0;
    int16_t  scope_width_total    = 0;
    int16_t  scope_pos_x          = 0;
    int16_t  scope_pos_y          = 0;
    int16_t  scope_last_x         = 0;
    int16_t  scope_last_y         = 0;
    int16_t  resolution           = 0;
    int16_t  sample_rate          = 0;
    int16_t  us_per_read          = 0;
    int16_t  last_adc_value       = 0;
    int16_t  motion_plan          = 0;
    int16_t  total_avg            = 0;

};

extern DMA_ATTR volatile gpo_scope_data gpo_data;
extern DMA_ATTR volatile gpo_config gpconf;

class GPO_SCOPE {

    private:
    
        IRAM_ATTR int16_t adc_to_voltage( int16_t adc_value );

    public:

        GPO_SCOPE( void );
        IRAM_ATTR int16_t get_total_avg( void );
        IRAM_ATTR void    start( void );
        IRAM_ATTR void    set_motion_plan( int16_t plan );
        IRAM_ATTR void    set_sample_rate( int16_t sample_rate );
        IRAM_ATTR void    set_us_per_read( int16_t us_per_read );
        IRAM_ATTR void    set_allow_motion( int motion_signal );
        IRAM_ATTR void    set_digital_state( int state );
        IRAM_ATTR void    set_total_avg( int16_t adc_value );
        IRAM_ATTR void    stop( void );
        IRAM_ATTR void    draw_scope();
        IRAM_ATTR void    reset( void );
        IRAM_ATTR void    init( int16_t width, int16_t height, int16_t posx, int16_t posy );
        IRAM_ATTR int     add_to_scope( int16_t adc_value, int16_t plan );
        IRAM_ATTR bool    is_blocked( void );
        IRAM_ATTR void    zoom( void );
        IRAM_ATTR void    set_scope_resolution( float ratio );
        IRAM_ATTR void    draw_header( void );
        IRAM_ATTR void    toogle_values( void );
        IRAM_ATTR bool    scope_is_running( void );
        IRAM_ATTR float   get_zoom( void );
        IRAM_ATTR void    setup( void );
        IRAM_ATTR void    update_setpoint( uint32_t smin, uint32_t smax );
        IRAM_ATTR void    draw_setpoint( void );
        IRAM_ATTR void    toggle_scope( bool enable = true );
        IRAM_ATTR void    draw_wave( int steps = 10 );
        IRAM_ATTR void    draw_scope_meta( void );
        IRAM_ATTR void    refresh_scope_canvas( void );
        IRAM_ATTR void    draw_error( int error_code );
        IRAM_ATTR int     avg_control( void );
        IRAM_ATTR void    set_vdrop_treshhold( int tresh );
        IRAM_ATTR void    set_full_range_size( int size );
};


extern GPO_SCOPE gscope;

#endif