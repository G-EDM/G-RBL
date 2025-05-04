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

#include "gedm_spindle.h"
#include "config/definitions.h"
#include "driver/pcnt.h"

G_EDM_SPINDLE gedm_spindle = G_EDM_SPINDLE();

G_EDM_SPINDLE::G_EDM_SPINDLE(){}

void G_EDM_SPINDLE::setup_spindle( int _dir_pin, int _step_pin ){
    default_frequency = 5000;
    backup_frequency  = default_frequency;
    dir_pin           = _dir_pin;
    step_pin          = _step_pin;
    frequency         = default_frequency;
    dir_inverted      = false;

    #ifndef USE_IDF_650

    //if( dir_pin != 1 ){ // problem on espressif 6.5
        pinMode(dir_pin, OUTPUT);   
        digitalWrite(dir_pin,LOW);
    //}

    #endif
    
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, step_pin);
    mcpwm_config_t pwm_config = {};
    pwm_config.frequency      = frequency;
    pwm_config.cmpr_a         = 0;
    pwm_config.cmpr_b         = 0;
    pwm_config.counter_mode   = MCPWM_UP_COUNTER;
    pwm_config.duty_mode      = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
    is_running = true;
    stop_spindle();

    #ifndef USE_IDF_650
        pinMode(step_pin, GPIO_MODE_INPUT_OUTPUT); // problem on espressif 6.5
    #endif

}

void G_EDM_SPINDLE::reset_spindle(){
    set_speed( default_frequency );
}
int G_EDM_SPINDLE::get_speed(){
    return frequency;
}
bool G_EDM_SPINDLE::restore_backup_speed(){
    return set_speed( backup_frequency, false );
}

bool G_EDM_SPINDLE::set_speed( int __frequency, bool backup ){
    if( frequency == __frequency ){ return true; }

    if( __frequency > frequency ){
        // faster 
    } else {
        // slower
    }

    frequency = __frequency;
    if( backup ){
        backup_frequency = frequency;
    }
    mcpwm_set_frequency( MCPWM_UNIT_0, MCPWM_TIMER_0, __frequency );
    if( !is_running ){ return false; }
    mcpwm_set_duty( MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 50.0 );
    return true;
}
void G_EDM_SPINDLE::start_spindle(){
    if( is_running ){ return; }
    int __frequency = 0;
    mcpwm_set_frequency( MCPWM_UNIT_0, MCPWM_TIMER_0, 1 );
    mcpwm_set_duty( MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 50.0 );
    while( __frequency < frequency ){
        // soft start
        if( sys.abort || sys_rt_exec_state.bit.motionCancel ){ break; }
        __frequency += 1;
        mcpwm_set_frequency( MCPWM_UNIT_0, MCPWM_TIMER_0, __frequency );
        mcpwm_set_duty( MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 50.0 );
        vTaskDelay(1);
    }
    mcpwm_set_frequency( MCPWM_UNIT_0, MCPWM_TIMER_0, frequency );
    mcpwm_set_duty( MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 50.0 );
    is_running = true;
}
void G_EDM_SPINDLE::stop_spindle(){
    if( ! is_running ){ return; }
    //mcpwm_set_frequency( MCPWM_UNIT_0, MCPWM_TIMER_2, 0 );
    mcpwm_set_duty( MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0.0 );
    is_running = false;
}
void G_EDM_SPINDLE::set_spindle_direction( bool direction ){
}
bool G_EDM_SPINDLE::spindle_is_running(){
    return is_running;
}
void G_EDM_SPINDLE::reverse_direction(){
    if( dir_inverted ){
        digitalWrite(dir_pin,LOW);
        dir_inverted = false;
    } else{
        digitalWrite(dir_pin,HIGH);
        dir_inverted = true;
    }
}
bool G_EDM_SPINDLE::dir_is_inverted(){
    return dir_inverted;
}


int G_EDM_SPINDLE::convert_mm_min_to_frequency( float mm_min ){
    int   steps_per_mm     = get_wire_steps_per_mm();
    float steps_per_minute = mm_min * float( steps_per_mm );
    // needed is the step frequency in hertz; steps per second
    float steps_per_second = steps_per_minute / 60.0;
    int frequency_hz = round( steps_per_second );
    if( frequency_hz <= WIRE_MIN_SPEED_HZ ){
        frequency_hz = WIRE_MIN_SPEED_HZ;
    }
    return frequency_hz;
}

float G_EDM_SPINDLE::convert_frequency_to_mm_min( float frequency ){
    float current_steps_per_minute = frequency * 60.0; // freq is in hertz
    float current_mm_min = current_steps_per_minute / float( get_wire_steps_per_mm() );
    return current_mm_min;
}

int G_EDM_SPINDLE::get_wire_steps_per_mm(){
    return steps_per_mm;
}

int G_EDM_SPINDLE::set_wire_steps_per_mm( int __steps_per_mm ){
    steps_per_mm = __steps_per_mm;
    return steps_per_mm;
}


volatile int  total_steps_do = 0;

void IRAM_ATTR wire_pwm_isr( void ){

    if( total_steps_do <= 0 ){ return; }
    --total_steps_do;

}


bool G_EDM_SPINDLE::manual_motion( float total_mm, bool negative ){
    int   steps_per_mm = get_wire_steps_per_mm();
    float total_steps  = total_mm * float( steps_per_mm );
    stop_spindle();
    delayMicroseconds(20);
    int dir_pin_state = digitalRead( dir_pin );
    if( !negative ){
        digitalWrite( dir_pin, false );
    } else {
        digitalWrite( dir_pin, true );
    }
    delayMicroseconds(20);
    float duration_micros = ( total_steps / float( frequency ) ) * 1000;
    total_steps_do = round( total_steps );
    attachInterrupt(step_pin, wire_pwm_isr, RISING);
    start_spindle();
    while( total_steps_do > 0 ){
        if( sys.abort || sys_rt_exec_state.bit.motionCancel ){ break; }
        vTaskDelay(10);
    }
    stop_spindle();
    detachInterrupt( step_pin );
    digitalWrite( dir_pin, dir_pin_state );
    delayMicroseconds(20);
    return true;
}