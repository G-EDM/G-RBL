//############################################################################################################
//
//  ██████        ███████ ██████  ███    ███  
// ██             ██      ██   ██ ████  ████  
// ██   ███ █████ █████   ██   ██ ██ ████ ██ 
// ██    ██       ██      ██   ██ ██  ██  ██ 
//  ██████        ███████ ██████  ██      ██ 
//  ____  _                 _                  _   _ ___ 
// |  _ \| |__   __ _ _ __ | |_ ___  _ __ ___ | | | |_ _|
// | |_) | '_ \ / _` | '_ \| __/ _ \| '_ ` _ \| | | || | 
// |  __/| | | | (_| | | | | || (_) | | | | | | |_| || | 
// |_|   |_| |_|\__,_|_| |_|\__\___/|_| |_| |_|\___/|___|
//   
//
// This is a beta version for testing purposes.
// Only for personal use. Commercial use or redistribution without permission is prohibited. 
// Copyright (c) Roland Lautensack    
//
//############################################################################################################

#include "widget_alarm.h"

const char *alarm_labels[] = {
    "Error code:",
    "Error msg:",
    "Touch to reset"
};

const char *alarm_messages[] = {
    "Something failed",
    "Hard limit was triggered",
    "Target out of reach",
    "Reset while in motion",
    "Probe failed",
    "Probe failed",
    "Homing failed",
    "Homing failed",
    "Homing failed",
    "Homing failed",
    "",
    "",
    "",
    "",
    0
};

void PhantomAlarm::set_error_code( uint8_t __error_code ){
    error_code = __error_code;
};
void PhantomAlarm::create( void ){};
IRAM_ATTR void PhantomAlarm::redraw_item( int16_t item_index, bool reload ){};

void PhantomAlarm::show( void ){
    int16_t text_width = 0;
    tft.fillScreen( TFT_BLACK );
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED);
    tft.drawString("Alarm!", 10, 50, 2);vTaskDelay(1);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY);
    text_width = tft.textWidth( alarm_labels[0] );
    tft.drawString( alarm_labels[0], 10, 100 ); vTaskDelay(1);
    tft.drawString( String( error_code ), 10+10+text_width, 100 ); vTaskDelay(1);
    text_width = tft.textWidth( alarm_labels[1] );
    tft.drawString( alarm_labels[1], 10, 120 ); vTaskDelay(1);
    tft.drawString(alarm_messages[error_code],10+10+text_width,120); 
    tft.drawString(alarm_labels[2], 10, 160, 2);vTaskDelay(1);
    uint16_t x, y;
    //while( tft.getTouch(&x, &y, 500) ){ // enter blocking until touched
    while( !tft.getTouch(&x, &y, 500) ){ // enter blocking until touched
        vTaskDelay(100);
    }
};
