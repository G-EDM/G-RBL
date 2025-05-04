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

#include "widget_alphakeyboard.h"
#include <sstream>
#include "char_helpers.h"
#include "ui_interface.h"

void PhantomKeyboardAlpha::set_str_value( const char* str ){
    strValue        = str;
    initialStrValue = str;
}

void PhantomKeyboardAlpha::set_default_value(){
    items[0].strValue = strValue;
    return;
};

std::shared_ptr<PhantomWidget> PhantomKeyboardAlpha::reset_widget( PhantomSettingsPackage *item, uint8_t __type ){
    changed = false;
    return shared_from_this();
};

void PhantomKeyboardAlpha::reset_item(){
    items.clear();
    clear_events();
}

bool PhantomKeyboardAlpha::touch_handler( int16_t item_index ){
    if( items[item_index].param_id == 39 ){ // undo
        items[0].boolValue = true;
        items[0].strValue.pop_back(); 
        redraw_item( 0, true );
    } else if( items[item_index].param_id == 40 ){ // del
        changed = false;
        //strValue   = initialStrValue;
        items[0].strValue = ""; 
        redraw_item( 0, true );
    } else if( items[item_index].param_id == 41 ){ // ok
        if( items[0].strValue != initialStrValue && items[0].strValue.length() > 0 ){
            changed = true;
        }
        strValue = items[0].strValue;
        items[0].boolValue = false;
        return true;
    } else { //
        if( !items[0].boolValue ){
            items[0].boolValue = true;
            items[0].strValue  = "";
        }
        std::string str = items[item_index].label;
        std::transform(str.begin(), str.end(), str.begin(),[](unsigned char c) { return std::tolower(c); });
        items[0].strValue.append( str.c_str() );
        redraw_item( 0, true );
    }
    return false;
};

void PhantomKeyboardAlpha::create(){

    block_touch                                   = false;
    auto __this                                   = std::dynamic_pointer_cast<PhantomKeyboardAlpha>(shared_from_this());
    std::weak_ptr<PhantomKeyboardAlpha> weak_this = __this;
    int int_numeric[]                             = {1,2,3,4,5,6,7,8,9,0};

    const std::function<void(PhantomWidgetItem *item)> touch_callback = [weak_this](PhantomWidgetItem *item) {
        if (auto __this = weak_this.lock()) {
            if( __this->block_touch ){
                return;
            }
            if( __this->touch_handler( item->get_index() ) ){
                __this->block_touch = true;
                if( __this->changed ){
                    ui.save_settings_writeout( __this->strValue );
                }
                __this->reset_item();
                ui.show_page_next( ui.get_previous_active_page() );
            }
        }
    };

    const int ITEMS_PER_ROW_A = 10;
    const int ITEMS_PER_ROW_B = 8;
    int16_t items_per_row = ITEMS_PER_ROW_A; // counter
    int16_t button_width  = width/items_per_row;
    int16_t button_height = button_width;
    int16_t current_y     = 0;
    int16_t current_x     = 0;
    int     digit         = 0;
    int     index         = 0;
    int     outlines      = 0;

    //##########################################################
    // Add the text field to view the input; needs to be item[0]
    //##########################################################
    addItem( OUTPUT_TYPE_CUSTOM, "", PARAM_TYPE_STRING, 22, width, button_height, current_x, current_y, false, DEFAULT_FONT) 
    ->set_string_value( strValue )
    ->set_colors( 0, BUTTON_LIST_BG, BUTTON_LIST_FG )
    ->set_colors( 1, BUTTON_LIST_BG, BUTTON_LIST_FG )
    ->set_colors( 2, BUTTON_LIST_BG, BUTTON_LIST_FG );

    current_y += button_height;
    current_y += 1;

    //########################################################
    // Draw the numeric section of the keyboard 1,2,3...0
    //########################################################
    while( data_sets_numeric[index] != NULL ){
        outlines = 1;
        digit    = int_numeric[index];
        addItem( OUTPUT_TYPE_NONE, data_sets_numeric[index], PARAM_TYPE_INT, digit, button_width, button_height, current_x, current_y, true, DEFAULT_FONT) 
        ->set_outlines( outlines )
        ->set_colors( 0, TFT_DARKGREY,  TFT_WHITE )
        ->set_colors( 1, BUTTON_LIST_BG, TFT_WHITE )
        ->on(TOUCHED,touch_callback);
        ++index;
        current_x += button_width;
        if( --items_per_row <= 0 ){
            current_x = 0;
            current_y += button_height;
            items_per_row = ITEMS_PER_ROW_A;
        }   
    }

    //########################################################
    // Draw the alpha section of the keyboard a,b,c...
    //########################################################
    items_per_row = ITEMS_PER_ROW_B;
    button_width  = width/items_per_row;
    index = 0;
    current_x  = 0;
    current_y += 1;
    while( data_sets_alpha[index] != NULL ){
        outlines = 1;
        digit    = int_numeric[index];
        addItem( OUTPUT_TYPE_NONE, data_sets_alpha[index], PARAM_TYPE_INT, digit, button_width, button_height, current_x, current_y, true, DEFAULT_FONT) 
        //->set_int_value(  )
        //->set_string_value()
        ->set_outlines( outlines )
        //->set_colors()
        ->on(TOUCHED,touch_callback);
        
        ++index;
        current_x += button_width;
        if( --items_per_row <= 0 ){
            current_x = 0;
            current_y += button_height;
            items_per_row = ITEMS_PER_ROW_B;
        }   
    }

    //########################################################
    // Del button
    //########################################################
    uint16_t nwidth = ( width-current_x ) / 2;
    addItem( OUTPUT_TYPE_NONE, "DEL", PARAM_TYPE_INT, 40, nwidth-1, button_height, current_x+1, current_y, true, DEFAULT_FONT) 
    ->set_colors( 0, TFT_RED, TFT_WHITE )
    ->on(TOUCHED,touch_callback);

    current_x += nwidth;

    addItem( OUTPUT_TYPE_NONE, "UNDO", PARAM_TYPE_INT, 39, nwidth, button_height, current_x+1, current_y, true, DEFAULT_FONT) 
    ->set_colors( 0, TFT_ORANGE, TFT_BLACK )
    ->on(TOUCHED,touch_callback);

    current_y += button_height;

    //########################################################
    // Cancel and confirm buttons
    //########################################################
    addItem( OUTPUT_TYPE_NONE, "Cancel", PARAM_TYPE_INT, 41, width-80, button_height, 0, height-button_height, true, DEFAULT_FONT) 
    ->set_colors( 0, TFT_MAROON, TFT_WHITE )
    ->on(TOUCHED,touch_callback);

    addItem( OUTPUT_TYPE_NONE, "Ok", PARAM_TYPE_INT, 41, 79, button_height, width-79, height-button_height, true, DEFAULT_FONT) 
    ->set_colors( 0, TFT_GREEN, TFT_BLACK )
    ->on(TOUCHED,touch_callback);

};

void PhantomKeyboardAlpha::show( void ){
    if( ! is_visible ){ return; }
    TFT_eSPI* display;
    if( use_sprite ){
        display = &canvas;
        canvas.createSprite( width, height );vTaskDelay(1);
    } else {
        display = &tft; // only sprite is supported for this widget right now...
    }
    display->setTextSize( 1 );
    if( num_items <= 0 ){ num_items = 1; }
    int16_t text_width = 0;
    int16_t y_offset   = 0;


    for( auto& _item : items ){
        _item.second.emit_load();
        _item.second.set_font( *display );
        _item.second.truex = use_sprite ? xpos + _item.second.xpos : _item.second.xpos;
        _item.second.truey = use_sprite ? ypos + _item.second.ypos : _item.second.ypos;
        int bg_color, fg_color;
        set_item_colors( _item.second.item_index, bg_color, fg_color );
        display->fillRect( _item.second.xpos, _item.second.ypos, _item.second.width, _item.second.height, bg_color );
        if( _item.second.outline != 0 ){
            switch (_item.second.outline){
                case 1:
                    display->drawFastVLine( _item.second.xpos+_item.second.width-1, _item.second.ypos, _item.second.height, TFT_BLACK );
                break;

                default:
                break;
            }
        }
        display->setTextColor( fg_color ); 
        if( _item.second.output_type == OUTPUT_TYPE_CUSTOM ){
            //display->setTextSize( 2 );
            y_offset   = int16_t( ( _item.second.height - display->fontHeight() ) / 2 );
            display->drawString( _item.second.get_visible(), (_item.second.xpos+10), _item.second.ypos+y_offset );vTaskDelay(1);
        } else {
            display->setTextSize( 1 );
            y_offset   = int16_t( ( _item.second.height - display->fontHeight() ) / 2 );
            display->drawFastHLine( _item.second.xpos, _item.second.ypos+_item.second.height-1, _item.second.width, TFT_BLACK );
            text_width = display->textWidth( _item.second.label.c_str() );
            display->drawString( _item.second.label.c_str(), _item.second.xpos+int16_t((_item.second.width-text_width)/2), _item.second.ypos+y_offset );vTaskDelay(1);
        }
    }
    if( use_sprite ){
        canvas.pushSprite(xpos,ypos);vTaskDelay(1);
        canvas.deleteSprite();vTaskDelay(1);
    }
};
IRAM_ATTR void PhantomKeyboardAlpha::redraw_item( int16_t item_index, bool reload ){
    if( ! is_visible ){ return; }
    if( reload ){
        items[item_index].emit_load();
        //items[item_index].emit(LOAD,&items[item_index]);
    }
    TFT_eSPI *display = &canvas;
    items[item_index].set_font( *display );
    canvas.createSprite( items[item_index].width, items[item_index].height );vTaskDelay(1);
    int bg_color, fg_color;
    set_item_colors( item_index, bg_color, fg_color );
    canvas.fillRect(0,0,items[item_index].width, items[item_index].height, bg_color);vTaskDelay(1); // draw the item background
    if( items[item_index].outline != 0 ){
        switch (items[item_index].outline){
            case 1:
                canvas.drawFastVLine( items[item_index].width-1, 0, items[item_index].height, TFT_BLACK );
            break;
            default:
            break;
        }
    }
    canvas.setTextColor( fg_color ); 
    if( items[item_index].output_type == OUTPUT_TYPE_CUSTOM ){
        //canvas.setTextSize( 2 );
        int16_t y_offset   = int16_t( ( items[item_index].height - canvas.fontHeight() ) / 2 );
        canvas.drawString( items[item_index].get_visible(), 10, y_offset );vTaskDelay(1);
    } else {
        canvas.setTextSize( 1 );
        int16_t y_offset   = int16_t( ( items[item_index].height - canvas.fontHeight() ) / 2 );
        int16_t text_width = canvas.textWidth( items[item_index].label.c_str() );
        canvas.drawFastHLine( 0, items[item_index].height-1, items[item_index].width, TFT_BLACK );
        canvas.drawString( items[item_index].label.c_str(), int16_t((items[item_index].width-text_width)/2), y_offset );vTaskDelay(1);
    }
    canvas.pushSprite(items[item_index].truex,items[item_index].truey);vTaskDelay(1);
    canvas.deleteSprite();vTaskDelay(1);
};
