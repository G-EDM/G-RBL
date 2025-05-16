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

#include "widget_list.h"

void PhantomList::create( void ){};

void PhantomList::show( void ){
    if( ! is_visible ){ return; }
    TFT_eSPI* display;
    if( use_sprite ){
        display = &canvas;
        canvas.createSprite( width, height );vTaskDelay(1);
        canvas.fillSprite(TFT_TRANSPARENT);
    } else {
        display = &tft;
    }
    display->setTextSize( 1 );
    if( num_items <= 0 ){
        num_items = 1; //
    }
    int16_t item_height = 0;
    int16_t item_width  = 0;
    int16_t current_y   = 0;
    int16_t current_x   = 0;
    int16_t y_offset    = 0;
    int16_t text_width  = 0;
    int16_t margin      = 5;
    if( widget_type <= 1 ){
        // single row horizintal list
        item_height = height;
        item_width  = width/num_items;
    } else if( widget_type == 2 ){
        // vertical list
        item_height = height/num_items;
        item_width  = width;
    }
    for( auto& _item : items ){
        _item.second.emit_load();
        _item.second.set_font( *display );
        margin = _item.second.text_align==1?0:5;
        y_offset            = int16_t( ( item_height - display->fontHeight() ) / 2 );
        _item.second.height = item_height;    // save the item height to the item
        _item.second.width  = item_width;     // save the item widht to the item
        _item.second.truex  = xpos+current_x; // save the true x position relative to the display
        _item.second.truey  = ypos+current_y; // save the true y position relative to the display
        int bg_color, fg_color;
        set_item_colors( _item.second.item_index, bg_color, fg_color );
        display->setTextColor( fg_color );
        display->drawString( _item.second.label.c_str(), current_x+margin, current_y+y_offset );vTaskDelay(1);
        text_width = display->textWidth( _item.second.label.c_str() );
        switch ( _item.second.output_type ){
            case OUTPUT_TYPE_INT: // int
                display->drawNumber( _item.second.intValue, (current_x+text_width+margin*2), current_y+y_offset );vTaskDelay(1);
            break;
            case OUTPUT_TYPE_FLOAT: // float
                display->drawFloat( _item.second.floatValue, _item.second.float_decimals, (current_x+text_width+margin*2), current_y+y_offset );vTaskDelay(1);
            break;
            case OUTPUT_TYPE_BOOL: // bool
            break;
            case OUTPUT_TYPE_STRING: // string
            case OUTPUT_TYPE_CUSTOM: // string
                display->drawString( _item.second.get_visible(), (current_x+text_width+margin*2), current_y+y_offset );vTaskDelay(1);
            break;
            default:
            break;
        }
        if( widget_type <= 1 ){
            current_x +=item_width;
        } else if( widget_type == 2 ){
            current_y +=item_height;
        }
    }
    if( use_sprite ){
        canvas.pushSprite(xpos,ypos,TFT_TRANSPARENT);vTaskDelay(1);
        canvas.deleteSprite();vTaskDelay(1);
    }

};
IRAM_ATTR void PhantomList::redraw_item( int16_t item_index, bool reload ){
    items[item_index].changed = false;
    if( ! is_visible ){ return; }
    if( reload ){ items[item_index].emit_load(); }
    TFT_eSPI* display = &canvas;
    int16_t margin = items[item_index].text_align==1?0:5;
    canvas.createSprite( items[item_index].width, items[item_index].height );vTaskDelay(1);
    int bg_color, fg_color;
    set_item_colors( item_index, bg_color, fg_color );
    items[item_index].set_font( *display );
    canvas.setTextColor( fg_color ); // draw the label
    //const char* label  = items[item_index].label.c_str();
    int16_t text_width = canvas.textWidth( items[item_index].label.c_str() );
    int16_t y_offset   = int16_t( ( items[item_index].height - canvas.fontHeight() ) / 2 );
    canvas.drawString( items[item_index].label.c_str(), margin, y_offset );vTaskDelay(1);
    text_width = canvas.textWidth( items[item_index].label.c_str() );


        switch ( items[item_index].output_type ){
            case OUTPUT_TYPE_INT: // int
                canvas.drawNumber( items[item_index].intValue, text_width+margin*2, y_offset );vTaskDelay(1);
            break;
        
            case OUTPUT_TYPE_FLOAT: // float
                canvas.drawFloat( items[item_index].floatValue, items[item_index].float_decimals, text_width+margin*2, y_offset );vTaskDelay(1);
            break;

            case OUTPUT_TYPE_BOOL: // bool
            break;

            case OUTPUT_TYPE_STRING: // string
            case OUTPUT_TYPE_CUSTOM: // string
                canvas.drawString( items[item_index].get_visible(), text_width+margin*2, y_offset );vTaskDelay(1);
            break;

            default:
            break;
        }

    canvas.pushSprite(items[item_index].truex,items[item_index].truey);vTaskDelay(1);
    canvas.deleteSprite();vTaskDelay(1);
};
