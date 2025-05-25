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

#include "widget_numkeyboard.h"
#include <sstream>
#include "char_helpers.h"
#include "ui_interface.h"




void PhantomKeyboardNumeric::set_callback( std::function<void( PhantomSettingsPackage )> _callback ){
    callback = _callback;
}

void PhantomKeyboardNumeric::set_default_value(){
    std::stringstream sstream;
    if( type == PARAM_TYPE_INT ){
        sstream << initialIntValue;
        items[0].strValue = sstream.str();
    } else {
        sstream << initialFloatValue;
        items[0].strValue = sstream.str();
    }
    return;
};


std::shared_ptr<PhantomWidget> PhantomKeyboardNumeric::reset_widget( PhantomSettingsPackage *_package, uint8_t __type ){
    changed           = false;
    type              = __type;

    package.axis       = _package->axis;
    package.type       = _package->type;
    package.param_id   = _package->param_id;

    package.boolValue  = _package->boolValue;
    package.intValue   = _package->intValue;
    package.floatValue = _package->floatValue;

    initialIntValue   = _package->intValue;
    initialFloatValue = _package->floatValue;
    strValue          = "";
    tooltip           = "No tooltip available";
    set_default_value();
    //tooltip = tooltips[package.param_id];
    return shared_from_this();
};


void PhantomKeyboardNumeric::set_tooltip( const char* str ){
    tooltip = str;
}


bool PhantomKeyboardNumeric::touch_handler( int16_t item_index ){
        if( items[item_index].param_id == 40 ){ // del
            changed = false;
            package.intValue   = initialIntValue;
            package.floatValue = initialFloatValue;
            items[0].strValue = ""; // maybe move this to the parent and build from parent on show event insetad of accessing it via index on the subitem
            redraw_item( 0, true );
        } else if( items[item_index].param_id == 41 ){ // ok
            if( type == PARAM_TYPE_INT ){
                if( package.intValue != initialIntValue ){ 
                    changed = true; 
                }
            } else if( type == PARAM_TYPE_FLOAT ){
                if( package.floatValue != initialFloatValue ){ 
                    changed = true; 
                }
            }
            items[0].boolValue = false;
            return true;
        } else { //0-9,
            if( !items[0].boolValue ){
                if( items[item_index].param_id == 20 ){ //. as first touch
                    if( items[0].strValue.find(".") != std::string::npos ){ // already has one
                        items[0].boolValue = true;
                        items[0].strValue  = "";
                    } else {
                        items[0].boolValue = true;
                    }
                } else {
                    items[0].boolValue = true;
                    items[0].strValue  = "";
                }
            }
            if( items[item_index].param_id == 20 ){ //,
                if( items[0].strValue.find(".") != std::string::npos ){
                    // already has a comma
                    return false;
                }
                if( items[0].strValue.size() <= 0 ){
                    items[0].strValue = "0";
                }
            }
            items[0].strValue.append( items[item_index].label.c_str() );
            if( items[0].strValue != "" ){
                package.floatValue = atoff( items[0].strValue.c_str() );
                package.intValue   = atoi(  items[0].strValue.c_str() );
            } else {
                package.floatValue = initialFloatValue;
                package.intValue   = initialIntValue;
            }
            redraw_item( 0, true );
        }
        return false;
    };


void PhantomKeyboardNumeric::reset_item(){
    items.clear();
    clear_events();
}


void PhantomKeyboardNumeric::create(){

    //PhantomKeyboardNumeric & __this = *this;
    auto __this = std::dynamic_pointer_cast<PhantomKeyboardNumeric>(shared_from_this());
    std::weak_ptr<PhantomKeyboardNumeric> weak_this = __this;
    int int_numeric[] = {1,2,3,4,5,6,7,8,9,0};

    int16_t items_per_row = 6;
    int16_t button_width  = width/items_per_row;
    int16_t button_height = 45;
    int16_t current_y     = height-3*button_height;
    int16_t current_x     = 0;
    int     digit         = 0;
    int     index         = 0;
    int     outlines      = 0;

    const std::function<void(PhantomWidgetItem *item)> touch_callback = [weak_this](PhantomWidgetItem *item) {
        if (auto __this = weak_this.lock()) {
            if( __this->touch_handler( item->get_index() ) ){
                if( __this->changed ){
                    if( __this->callback != nullptr ){
                        __this->callback( __this->package );
                    }
                }
                __this->reset_item();
                ui.show_page_next( ui.get_previous_active_page() );
            }
        }
    };

    addItem( OUTPUT_TYPE_CUSTOM, "", PARAM_TYPE_STRING, 22, width, 34, 0, current_y-34, false, DEFAULT_FONT) 
    ->set_colors( 0, TFT_WHITE, BUTTON_LIST_BG )
    ->set_colors( 1, TFT_WHITE, BUTTON_LIST_BG )
    ->set_colors( 2, TFT_WHITE, BUTTON_LIST_BG );

    for( int i = 0; i < 12; ++i ){
        if( i == 5 ){
                addItem( OUTPUT_TYPE_NONE, "DEL", PARAM_TYPE_INT, 40, button_width, button_height, current_x, current_y, true, DEFAULT_FONT) 
                ->set_outlines( 0 )
                ->set_colors( 0, TFT_MAROON, TFT_LIGHTGREY )
                ->on(TOUCHED,touch_callback);
        } else if( i == 11 ){
                addItem( OUTPUT_TYPE_NONE, ".", PARAM_TYPE_INT, 20, button_width, button_height, current_x, current_y, true, DEFAULT_FONT) 
                ->set_outlines( 0 )->set_disabled( type == PARAM_TYPE_INT ? true : false )
                ->on(TOUCHED,touch_callback);
        } else {
            outlines = 1;
            digit = int_numeric[index];
            addItem( OUTPUT_TYPE_NONE, data_sets_numeric[index], PARAM_TYPE_INT, digit, button_width, button_height, current_x, current_y, true, DEFAULT_FONT) 
            ->set_outlines( outlines )
            ->on(TOUCHED,touch_callback);
            ++index;
        }
        current_x += button_width;
        if( --items_per_row <= 0 ){
            current_x = 0;
            current_y += button_height;
            items_per_row = 6;
        }   
    }
    current_y += button_height;
    current_x  = 0;
    addItem( OUTPUT_TYPE_NONE, "OK", PARAM_TYPE_INT, 41, width, button_height, 0, height-button_height, true, DEFAULT_FONT) 
    ->on(TOUCHED,touch_callback);


};

void PhantomKeyboardNumeric::show( void ){
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


    display->setTextColor(TFT_DARKGREY);
    //char* charArray;
    std::string token;
    size_t start = 0;
    size_t end = tooltip.find('\r');
    int8_t y_line = 10;
    int8_t count = 0;
    while (end != std::string::npos || ++count > 20 ) {
        token     = tooltip.substr(start, end - start);
        start = end + 1; // Move past the delimiter
        end = tooltip.find('\r', start);
        display->drawString(token.c_str(), 10, y_line, 2);vTaskDelay(1);
        y_line+=17;
        display->setTextColor(TFT_ORANGE);
    }
    token = tooltip.substr(start);
    if (!token.empty()) {
        display->drawString(token.c_str(), 10, y_line, 2);vTaskDelay(1);
    }


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
            display->setTextSize( 2 );
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
IRAM_ATTR void PhantomKeyboardNumeric::redraw_item( int16_t item_index, bool reload ){
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
        canvas.setTextSize( 2 );
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
