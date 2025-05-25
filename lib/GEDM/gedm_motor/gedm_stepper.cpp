//############################################################################################################
//
//  ██████        ███████ ██████  ███    ███  
// ██             ██      ██   ██ ████  ████  
// ██   ███ █████ █████   ██   ██ ██ ████ ██ 
// ██    ██       ██      ██   ██ ██  ██  ██ 
//  ██████        ███████ ██████  ██      ██ 
//
//############################################################################################################
#include "gedm_stepper.h"
    
rmt_item32_t G_EDM_STEPPER::rmt_item[2];
rmt_config_t G_EDM_STEPPER::rmt_conf;


bool G_EDM_STEPPER::set_homing_mode(bool is_homing){
  return true;
}


rmt_channel_t G_EDM_STEPPER::rmt_next() {
    static uint8_t next_RMT_chan_num = uint8_t(RMT_CHANNEL_0);  // channels 0-7 are valid
    if (next_RMT_chan_num < RMT_CHANNEL_MAX) {
        next_RMT_chan_num++;
    }
    return rmt_channel_t(next_RMT_chan_num);
}


G_EDM_STEPPER::G_EDM_STEPPER(uint8_t axis_index, uint8_t step_pin, uint8_t dir_pin) :
    _axis_index(axis_index % MAX_AXES)
    ,_dual_axis_index(axis_index / MAX_AXES)
    , _step_pin(step_pin)
    , _dir_pin(dir_pin) 
{

    _rmt_chan_num = rmt_next();

}
void G_EDM_STEPPER::init() {
    _direction = 2; // initial
    read_settings();
}
void G_EDM_STEPPER::read_settings() { init_step_dir_pins(); }
void G_EDM_STEPPER::init_step_dir_pins() {
    _invert_step_pin = bitnum_istrue(DEFAULT_STEPPING_INVERT_MASK, _axis_index);
    _invert_dir_pin  = bitnum_istrue(DEFAULT_DIRECTION_INVERT_MASK, _axis_index);
    pinMode(_dir_pin, OUTPUT);
    rmt_conf.rmt_mode                       = RMT_MODE_TX;
    rmt_conf.clk_div                        = 20;
    rmt_conf.mem_block_num                  = 2;
    rmt_conf.tx_config.loop_en              = false;
    rmt_conf.tx_config.carrier_en           = false;
    rmt_conf.tx_config.carrier_freq_hz      = 0;
    rmt_conf.tx_config.carrier_duty_percent = 50;
    rmt_conf.tx_config.carrier_level        = RMT_CARRIER_LEVEL_LOW;
    rmt_conf.tx_config.idle_output_en       = true;
    auto stepPulseDelay                      = STEP_PULSE_DELAY;
    rmt_item[0].duration0                     = stepPulseDelay < 1 ? 1 : stepPulseDelay * 4;
    rmt_item[0].duration1                     = 4 * DEFAULT_STEP_PULSE_MICROSECONDS;
    rmt_item[1].duration0                     = 0;
    rmt_item[1].duration1                     = 0;
    if (_rmt_chan_num == RMT_CHANNEL_MAX) {
        return;
    }
    rmt_set_source_clk(_rmt_chan_num, RMT_BASECLK_APB);
    rmt_conf.channel              = _rmt_chan_num;
    rmt_conf.tx_config.idle_level = _invert_step_pin ? RMT_IDLE_LEVEL_HIGH : RMT_IDLE_LEVEL_LOW;
    rmt_conf.gpio_num             = gpio_num_t(_step_pin);
    rmt_item[0].level0              = rmt_conf.tx_config.idle_level;
    rmt_item[0].level1              = !rmt_conf.tx_config.idle_level;
    rmt_config(&rmt_conf);
    rmt_fill_tx_items(rmt_conf.channel, &rmt_item[0], rmt_conf.mem_block_num, 0);
    //pinMode(_disable_pin, OUTPUT);
}
void IRAM_ATTR G_EDM_STEPPER::step() {
    //mpos_changed = true;
    RMT.conf_ch[_rmt_chan_num].conf1.mem_rd_rst = 1;
    RMT.conf_ch[_rmt_chan_num].conf1.tx_start   = 1;
}
void G_EDM_STEPPER::unstep(){}
bool IRAM_ATTR G_EDM_STEPPER::set_direction(bool dir,bool add_dir_delay) { 
    if( dir == _direction ){ return false; } 
    _direction = dir;
    digitalWrite(_dir_pin, dir ^ _invert_dir_pin); 
    if( ! add_dir_delay ){ return true; }
    delayMicroseconds(STEPPER_DIR_CHANGE_DELAY);
    return true;
}
void G_EDM_STEPPER::set_disable(bool disable) { 
}

int G_EDM_STEPPER::get_step_delay_for_speed( float mm_min ){
  // mm per single step
  double mm_per_step = 1.0 / ( double ) g_axis[_axis_index]->steps_per_mm.get();
  double delay = mm_per_step / ( double ) mm_min * 60000000;
  return round( delay );
}
