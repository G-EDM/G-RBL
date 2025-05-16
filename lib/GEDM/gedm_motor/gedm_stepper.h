#pragma once

//############################################################################################################
//
//  ██████        ███████ ██████  ███    ███  
// ██             ██      ██   ██ ████  ████  
// ██   ███ █████ █████   ██   ██ ██ ████ ██ 
// ██    ██       ██      ██   ██ ██  ██  ██ 
//  ██████        ███████ ██████  ██      ██ 
//
//############################################################################################################

#include <cstdint>
#include <Config.h>
#include <config/definitions.h>
#include <driver/rmt.h>
#include <NutsBolts.h>

class G_EDM_STEPPER {

    public:

        G_EDM_STEPPER(uint8_t axis_index, uint8_t step_pin, uint8_t dir_pin);
        void init();
        bool set_homing_mode(bool is_homing);
        bool IRAM_ATTR set_direction(bool dir, bool add_dir_delay = true); // returns the number of microseconds added after a dir change
        void IRAM_ATTR step();
        void unstep();
        void read_settings();
        void init_step_dir_pins();
        void set_disable(bool);
        int  get_step_delay_for_speed( float mm_min );

    protected:
        uint8_t _axis_index;       // X_AXIS, etc
        uint8_t _dual_axis_index;  // 0 = primary 1=ganged
        rmt_channel_t _rmt_chan_num;
        bool    _invert_step_pin;
        bool    _invert_dir_pin;
        uint8_t _step_pin;
        uint8_t _dir_pin;
        uint8_t _disable_pin;
        int _direction;

    private:
        static rmt_channel_t rmt_next();
        static rmt_item32_t  rmt_item[2];
        static rmt_config_t  rmt_conf;
        bool grbl_direction_state; // used to restore the wanted grbl state after we are done
};