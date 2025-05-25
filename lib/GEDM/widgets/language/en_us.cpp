#include "en_us.h"

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

setget_param_enum set_get_param_enum;
page_map page_ids;



DMA_ATTR std::map<int,setting_cache_object> settings_values_cache;

//###########################################################
// Parameters added here will be stored in the settings files
// and also sued to restore the last session after boot
// Only float/bool/int paramaters are supported yet
//###########################################################
std::map<int,param_types> settings_file_param_types = {
    {PARAM_ID_FLUSHING_FLUSH_NOSPRK,PARAM_TYPE_BOOL},
    {PARAM_ID_SPINDLE_ENABLE,PARAM_TYPE_BOOL},
    {PARAM_ID_EARLY_RETR,PARAM_TYPE_BOOL},
    {PARAM_ID_USEHWTIMTER,PARAM_TYPE_BOOL},
    {PARAM_ID_RESET_QUEUE,PARAM_TYPE_BOOL},
    {PARAM_ID_USE_STOPGO,PARAM_TYPE_BOOL},
    {PARAM_ID_DPM_UART,PARAM_TYPE_BOOL},
    {PARAM_ID_SPINDLE_ONOFF,PARAM_TYPE_BOOL}, // ./bool section end
    {PARAM_ID_FREQ, PARAM_TYPE_FLOAT},
    {PARAM_ID_DUTY, PARAM_TYPE_FLOAT},
    {PARAM_ID_SETMIN, PARAM_TYPE_FLOAT},
    {PARAM_ID_SETMAX, PARAM_TYPE_FLOAT},
    {PARAM_ID_MAX_FEED, PARAM_TYPE_FLOAT},
    {PARAM_ID_SPINDLE_SPEED, PARAM_TYPE_FLOAT},
    {PARAM_ID_SPINDLE_SPEED_PROBING, PARAM_TYPE_FLOAT},
    {PARAM_ID_BROKEN_WIRE_MM,PARAM_TYPE_FLOAT},
    {PARAM_ID_I2S_RATE,PARAM_TYPE_FLOAT},
    {PARAM_ID_RETRACT_H_MM,PARAM_TYPE_FLOAT},
    {PARAM_ID_RETRACT_S_MM,PARAM_TYPE_FLOAT},
    {PARAM_ID_RETRACT_H_F,PARAM_TYPE_FLOAT},
    {PARAM_ID_RETRACT_S_F,PARAM_TYPE_FLOAT},
    {PARAM_ID_DPM_PROBE_V,PARAM_TYPE_FLOAT},
    {PARAM_ID_DPM_PROBE_C,PARAM_TYPE_FLOAT},
    {PARAM_ID_PROBE_FREQ,PARAM_TYPE_FLOAT},
    {PARAM_ID_PROBE_DUTY,PARAM_TYPE_FLOAT},
    {PARAM_ID_PROBE_TR_C,PARAM_TYPE_FLOAT},
    {PARAM_ID_REAMER_DIST,PARAM_TYPE_FLOAT},
    {PARAM_ID_REAMER_DURA,PARAM_TYPE_FLOAT},
    {PARAM_ID_FLUSHING_INTERVAL,PARAM_TYPE_FLOAT},
    {PARAM_ID_FLUSHING_DISTANCE,PARAM_TYPE_FLOAT}, 
    {PARAM_ID_SPEED_RAPID,PARAM_TYPE_FLOAT},
    {PARAM_ID_TOOLDIAMETER,PARAM_TYPE_FLOAT},//float section end
    {PARAM_ID_FLUSHING_FLUSH_OFFSTP,PARAM_TYPE_INT},
    {PARAM_ID_SHORT_DURATION, PARAM_TYPE_INT},
    {PARAM_ID_I2S_SAMPLES,PARAM_TYPE_INT},
    {PARAM_ID_I2S_BUFFER_L,PARAM_TYPE_INT},
    {PARAM_ID_I2S_BUFFER_C,PARAM_TYPE_INT},
    {PARAM_ID_RISE_BOOST,PARAM_TYPE_INT},
    {PARAM_ID_MAIN_AVG,PARAM_TYPE_INT},
    {PARAM_ID_RETRACT_CONF,PARAM_TYPE_INT},
    {PARAM_ID_MAX_REVERSE,PARAM_TYPE_INT},
    {PARAM_ID_BEST_OF,PARAM_TYPE_INT},
    {PARAM_ID_FAVG_SIZE,PARAM_TYPE_INT},
    {PARAM_ID_VDROP_THRESH,PARAM_TYPE_INT},
    {PARAM_ID_VFD_DROPS_BLOCK,PARAM_TYPE_INT},
    {PARAM_ID_VFD_SHORTS_POFF,PARAM_TYPE_INT},
    {PARAM_ID_POFF_DURATION,PARAM_TYPE_INT},
    {PARAM_ID_EARLY_X_ON,PARAM_TYPE_INT},
    {PARAM_ID_LINE_ENDS,PARAM_TYPE_INT},
    {PARAM_ID_POFF_AFTER,PARAM_TYPE_INT},
    {PARAM_ID_ZERO_JITTER,PARAM_TYPE_INT},
    {PARAM_ID_READINGS_H,PARAM_TYPE_INT},
    {PARAM_ID_READINGS_L,PARAM_TYPE_INT},
    {PARAM_ID_ZERO_THRESH,PARAM_TYPE_INT},
    {PARAM_ID_PROBE_TR_V,PARAM_TYPE_INT},
    {PARAM_ID_EDGE_THRESH,PARAM_TYPE_INT},
    {PARAM_ID_X_RES,PARAM_TYPE_INT},
    {PARAM_ID_Y_RES,PARAM_TYPE_INT},
    {PARAM_ID_Z_RES,PARAM_TYPE_INT},
    //{PARAM_ID_SINK_DIR,PARAM_TYPE_INT},
    {PARAM_ID_MODE,PARAM_TYPE_INT}, 
    {PARAM_ID_SPINDLE_STEPS_PER_MM,PARAM_TYPE_INT}// ./int section end
};


//###########################################################
// Tooltips: Order needs to be exactly the same as in the
// setget_param_enum enum object. Not a beauty..
//###########################################################
const char* tooltips[TOTAL_PARAM_ITEMS] = {
    TT_PARAM_ID_FREQ,
    TT_PARAM_ID_DUTY,
    "PARAM_ID_MODE",
    "PARAM_ID_MPOSX",
    "PARAM_ID_MPOSY",
    "PARAM_ID_MPOSZ",
    "PARAM_ID_PWM_STATE",//bool
    TT_PARAM_ID_SETMIN,
    TT_PARAM_ID_SETMAX,
    TT_PARAM_ID_MAX_FEED,
    TT_PARAM_ID_SPINDLE_SPEED,
    "PARAM_ID_SPINDLE_ENABLE", //bool
    TT_PARAM_ID_SHORT_DURATION,
    TT_PARAM_ID_BROKEN_WIRE_MM,
    "PARAM_ID_DRAW_LINEAR", //bool
    TT_PARAM_ID_EDGE_THRESH,
    TT_PARAM_ID_I2S_SAMPLES,
    TT_PARAM_ID_I2S_RATE,
    TT_PARAM_ID_I2S_BUFFER_L,
    TT_PARAM_ID_I2S_BUFFER_C,
    TT_PARAM_ID_RISE_BOOST,
    TT_PARAM_ID_MAIN_AVG,
    "PARAM_ID_EARLY_RETR", //bool
    TT_PARAM_ID_RETRACT_CONF,
    TT_PARAM_ID_MAX_REVERSE,
    TT_PARAM_ID_RETRACT_H_MM,
    TT_PARAM_ID_RETRACT_S_MM,
    TT_PARAM_ID_RETRACT_H_F,
    TT_PARAM_ID_RETRACT_S_F,
    TT_PARAM_ID_BEST_OF,
    "PARAM_ID_USEHWTIMTER", //bool
    TT_PARAM_ID_FAVG_SIZE,
    TT_PARAM_ID_VDROP_THRESH,
    TT_PARAM_ID_VFD_DROPS_BLOCK,
    TT_PARAM_ID_VFD_SHORTS_POFF,
    TT_PARAM_ID_POFF_DURATION,
    TT_PARAM_ID_EARLY_X_ON,
    TT_PARAM_ID_LINE_ENDS,
    TT_PARAM_ID_RESET_QUEUE,
    TT_PARAM_ID_POFF_AFTER,
    "PARAM_ID_USE_STOPGO", //bool
    TT_PARAM_ID_ZERO_JITTER,
    TT_PARAM_ID_READINGS_H,
    TT_PARAM_ID_READINGS_L,
    TT_PARAM_ID_ZERO_THRESH,
    "PARAM_ID_DPM_UART",  //bool
    "PARAM_ID_DPM_ONOFF", //bool
    TT_PARAM_ID_DPM_VOLTAGE,
    TT_PARAM_ID_DPM_CURRENT,
    TT_PARAM_ID_X_RES,
    TT_PARAM_ID_Y_RES,
    TT_PARAM_ID_Z_RES,
    "PARAM_ID_ESP_RESTART", //none
    TT_PARAM_ID_DPM_PROBE_V,
    TT_PARAM_ID_DPM_PROBE_C,
    TT_PARAM_ID_PROBE_FREQ,
    TT_PARAM_ID_PROBE_DUTY,
    TT_PARAM_ID_PROBE_TR_V,
    TT_PARAM_ID_PROBE_TR_C,
    "PARAM_ID_SPINDLE_ONOFF", //bool
    TT_PARAM_ID_REAMER_DIST,
    TT_PARAM_ID_REAMER_DURA,
    TT_PARAM_ID_SINK_DIST,
    TT_PARAM_ID_SIMULATION, //bool
    "PARAM_ID_SINK_DIR",
    "PARAM_ID_PROBE_ACTION",
    "PARAM_ID_UNSET_PROBE_POS",
    "PARAM_ID_SET_PROBE_POS",
    "PARAM_ID_GET_PROBE_POS",
    "PARAM_ID_HOME_SEQ",
    "PARAM_ID_HOME_AXIS",
    "PARAM_ID_HOME_ENA",
    "PARAM_ID_MOVE_TO_ORIGIN",
    "PARAM_ID_MOTION_STATE",
    TT_PARAM_ID_FLUSHING_INTERVAL,
    TT_PARAM_ID_FLUSHING_DISTANCE,
    TT_PARAM_ID_FLUSHING_FLUSH_OFFSTP,
    TT_PARAM_ID_FLUSHING_FLUSH_NOSPRK,
    // this part is ugly
    // would be better to have params for different axis commands
    // separated and instead of using a function for each move have it merged into one function an pass an axis bitmask
    // but for now this is ok....
    TT_PARAM_ID_XRIGHT,
    TT_PARAM_ID_XLEFT,
    TT_PARAM_ID_YUP,
    TT_PARAM_ID_YDOWN,
    TT_PARAM_ID_ZUP,
    TT_PARAM_ID_ZDOWN,
    "PARAM_ID_WPOSX",
    "PARAM_ID_WPOSY",
    "PARAM_ID_WPOSZ",
    "PARAM_ID_SD_FILE_SET",
    "PARAM_ID_SET_PAGE",
    "PARAM_ID_SET_STOP_EDM",
    "PARAM_ID_EDM_PAUSE",
    TT_PARAM_ID_TOOLDIAMETER,
    TT_PARAM_ID_SPEED_RAPID,
    "PARAM_ID_HOME_ALL",
    TT_PARAM_ID_SPINDLE_SPEED_PROBING,
    TT_PARAM_ID_SPINDLE_STEPS_PER_MM,
    TT_PARAM_ID_SPINDLE_MOVE_UP,
    TT_PARAM_ID_SPINDLE_MOVE_DOWN

};




const char *data_sets_numeric[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", 0 };
const char *data_sets_alpha[]   = { "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "-", "_", "@", 0 };


std::map<info_codes, const char*> info_messages = {

    { NOT_FULLY_HOMED,      "* Machine needs to be fully homed!" },
    { SELECT_GCODE_FILE,    "* Please select a gcode file" },
    { AXIS_NOT_HOMED,       "* Axis needs to be homed!" },
    { TARGET_OUT_OF_REACH,  "* Position out of reach" },
    { MOTION_NOT_AVAILABLE, "* Motion is not enabled" },
    { Z_AXIS_DISABLED,      "* Z axis is disabled" }

};