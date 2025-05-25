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

#include "widget_split_list.h"


void PhantomSplitList::create( void ){};




std::shared_ptr<PhantomWidget> PhantomSplitList::set_column_left_width( int16_t width ){
    column_width_left = width;
    return shared_from_this();
};

void PhantomSplitList::show( void ){

    if( ! is_visible ){ return; }

    TFT_eSPI* display;

    if( use_sprite ){
        display = &canvas;
        canvas.createSprite( width, height );vTaskDelay(1);
    } else {
        display = &tft; // only sprite is supported for this widget right now...
    }
    display->setTextSize( 1 );

    if( num_items <= 0 ){
        num_items = 1; //
    }

    int16_t item_height = 0;
    int16_t item_width  = 0;
    int16_t current_y   = use_sprite ? 0 : ypos;
    int16_t y_offset    = 0;
    int16_t text_width  = 0;
    int16_t margin      = 20;
    int16_t xxpos       = 0;

    item_height              = 30;
    item_width               = width;
    int     width_half_onoff = 50;
    int16_t on_w = 0, off_w = 0, on_pos = 0, off_pos = 0;
    int bg_color, fg_color;

    for( auto& _item : items ){

        _item.second.emit_load();
        _item.second.set_font( *display );

        set_item_colors( _item.second.item_index, bg_color, fg_color );

        if( _item.second.output_type == OUTPUT_TYPE_TITLE ){
            item_height = 24;
            margin      = 10;
        } else {
            item_height = 30;
            margin      = 20;
        }

        y_offset            = int16_t( ( item_height - display->fontHeight() ) / 2 );
        _item.second.height = item_height; // save the item height to the item
        _item.second.width  = item_width;  // save the item widht to the item
        _item.second.truex  = xpos;        // save the true x position relative to the display
        _item.second.truey  = current_y;   // save the true y position relative to the display




        display->setTextColor( fg_color );
        display->drawString( _item.second.label.c_str(), margin, current_y+y_offset );vTaskDelay(1);
        text_width = column_width_left==-1?210:column_width_left;

        if( _item.second.disabled ){

            display->drawString( "Not available!", text_width, current_y+y_offset );vTaskDelay(1);

        } else {

            display->setTextColor( TFT_GREEN );


        switch ( _item.second.output_type ){
            case OUTPUT_TYPE_INT: // int
                display->drawNumber( _item.second.intValue, text_width, current_y+y_offset );vTaskDelay(1);
            break;
        
            case OUTPUT_TYPE_FLOAT: // float
                display->drawFloat( _item.second.floatValue, _item.second.float_decimals, text_width, current_y+y_offset );vTaskDelay(1);
            break;

            case OUTPUT_TYPE_BOOL: // bool
                on_w    = display->textWidth("ON");
                off_w   = display->textWidth("OFF");
                on_pos  = (width_half_onoff-on_w)/2;
                off_pos = (width_half_onoff-off_w)/2;
                if( _item.second.boolValue ){
                    display->fillRect( text_width,                  current_y+6, width_half_onoff, item_height-12, TFT_GREEN);   vTaskDelay(1);
                    display->fillRect( text_width+width_half_onoff, current_y+6, width_half_onoff, item_height-12, TFT_DARKGREY);vTaskDelay(1);
                } else {
                    display->fillRect( text_width,                  current_y+6, width_half_onoff, item_height-12, TFT_DARKGREY);   vTaskDelay(1);
                    display->fillRect( text_width+width_half_onoff, current_y+6, width_half_onoff, item_height-12, TFT_RED);vTaskDelay(1);
                }
                display->setTextColor(TFT_BLACK);
                display->drawString( "ON" , text_width+on_pos, current_y+y_offset, 2 );vTaskDelay(1);
                display->drawString( "OFF", text_width+width_half_onoff+off_pos, current_y+y_offset, 2 );vTaskDelay(1);
            break;

            case OUTPUT_TYPE_STRING: // string
            case OUTPUT_TYPE_CUSTOM: // string
                display->drawString( _item.second.get_visible(), text_width, current_y+y_offset );vTaskDelay(1);
            break;

            default:
            break;
        }

        }
        current_y +=item_height;
    }
    if( use_sprite ){
        canvas.pushSprite(xpos,ypos);vTaskDelay(1);
        canvas.deleteSprite();vTaskDelay(1);
    }

};
IRAM_ATTR void PhantomSplitList::redraw_item( int16_t item_index, bool reload ){
    items[item_index].changed = false;
    if( ! is_visible ){ return; }
    if( reload ){ items[item_index].emit_load(); }
    int16_t margin = 20;
    canvas.createSprite( items[item_index].width, items[item_index].height );vTaskDelay(1);
    int bg_color, fg_color;
    set_item_colors( item_index, bg_color, fg_color );
    items[item_index].set_font( canvas );
    canvas.setTextColor( fg_color ); // draw the label
    int16_t text_width = canvas.textWidth( items[item_index].label.c_str() );
    int16_t y_offset   = int16_t( ( items[item_index].height - canvas.fontHeight() ) / 2 );
    canvas.drawString( items[item_index].label.c_str(), margin, y_offset );vTaskDelay(1);
    text_width = column_width_left==-1?210:column_width_left;

    int width_half_onoff = 50;
    int16_t on_w         = canvas.textWidth("ON");
    int16_t off_w        = canvas.textWidth("OFF");
    int16_t on_pos       = (width_half_onoff-on_w)/2;
    int16_t off_pos      = (width_half_onoff-off_w)/2;

    if( items[item_index].disabled ){
        canvas.drawString( "Not available!", text_width, y_offset );vTaskDelay(1);
    } else {
        canvas.setTextColor( TFT_GREEN );
        switch ( items[item_index].output_type ){
            case OUTPUT_TYPE_INT: // int
                canvas.drawNumber( items[item_index].intValue, text_width, y_offset );vTaskDelay(1);
            break;
            case OUTPUT_TYPE_FLOAT: // float
                canvas.drawFloat( items[item_index].floatValue, items[item_index].float_decimals, text_width, y_offset );vTaskDelay(1);
            break;
            case OUTPUT_TYPE_BOOL: // bool
                if( items[item_index].boolValue ){
                    canvas.fillRect( text_width,                  6, width_half_onoff, items[item_index].height-12, TFT_GREEN);   vTaskDelay(1);
                    canvas.fillRect( text_width+width_half_onoff, 6, width_half_onoff, items[item_index].height-12, TFT_DARKGREY);vTaskDelay(1);
                } else {
                    canvas.fillRect( text_width,                  6, width_half_onoff, items[item_index].height-12, TFT_DARKGREY);   vTaskDelay(1);
                    canvas.fillRect( text_width+width_half_onoff, 6, width_half_onoff, items[item_index].height-12, TFT_RED);vTaskDelay(1);
                }
                canvas.setTextColor(TFT_BLACK);
                canvas.drawString( "ON" , text_width+on_pos, y_offset, 2 );vTaskDelay(1);
                canvas.drawString( "OFF", text_width+width_half_onoff+off_pos, y_offset, 2 );vTaskDelay(1);
            break;
            case OUTPUT_TYPE_STRING: // string
            case OUTPUT_TYPE_CUSTOM: // string
                canvas.drawString( items[item_index].get_visible(), text_width, y_offset );vTaskDelay(1);
            break;
            default:
            break;
        }
    }
    canvas.pushSprite(items[item_index].truex,items[item_index].truey);vTaskDelay(1);
    canvas.deleteSprite();vTaskDelay(1);
};