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


#include "widget_logger.h"

IRAM_ATTR void PhantomLogger::build_ringbuffer( size_t capacity ){
    if( capacity <= 0 ){
        capacity = 14;//height/15;
    } 
    create_ringbuffer( capacity );
};


IRAM_ATTR void PhantomLogger::create( void ){};

IRAM_ATTR void PhantomLogger::show( void ){
    if( ! is_visible || height <= 0 ){ return; }
    TFT_eSPI* display;
    if( use_sprite ){
        display = &canvas;
        canvas.createSprite( width, height );vTaskDelay(1);
    } else {
        display = &tft; // only sprite is supported for this widget right now...
    }
    display->setTextFont( MINI_FONT ); 
    display->setTextSize( 1 );
    display->setTextColor(TFT_DARKGREY);

    int16_t item_height, current_y, items_num;//item_width
    current_y   = use_sprite ? 0 : ypos;
    //item_width  = width;
    item_height = 15;
    items_num   = height/item_height; // max possible items for given height
    //total_log_items
    size_t size = get_buffer_size();  // capacity of the ring buffer
    int16_t text_height = display->fontHeight();
    int16_t y_offset    = int16_t( ( item_height - display->fontHeight() ) / 2 );
    if( items_num > size ){
        items_num = size;
    }
    current_y = height-item_height;

    reset_work_index();

    for( int i=0; i < items_num; ++i ){
        const char* line = pop_line();
        if( line[0] == ' '){
            display->setTextColor( TFT_LIGHTGREY );
        } else if( line[0] == '@' ){
            display->setTextColor( TFT_GREEN );
        } else if( line[0] == '*' ){
            display->setTextColor( COLORORANGERED );
        } else {
            display->setTextColor( TFT_DARKGREY );
        }
        display->drawString( line, 0, current_y+y_offset );
        current_y -= item_height;
        vTaskDelay(1);
    }

    //reset_work_index();

    if( use_sprite ){
        canvas.pushSprite(xpos,ypos,TFT_TRANSPARENT);vTaskDelay(1);
        canvas.deleteSprite();vTaskDelay(1);
    }

};

IRAM_ATTR void PhantomLogger::redraw_item( int16_t item_index, bool reload ){};
