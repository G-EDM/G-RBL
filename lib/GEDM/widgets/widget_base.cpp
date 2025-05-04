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

#include "widget_base.h"
#include "tft_display/ili9341_tft.h"

PhantomWidget::~PhantomWidget(){
    //__callback = nullptr;
};

void PhantomWidget::update_page_ids_for_items(){
    for( auto& __item : items ) {
        __item.second.set_parent_page_id( page_id );
        __item.second.set_parent_page_tab_id( tab_id );
        __item.second.set_widget_id( index );
    }
};

IRAM_ATTR void PhantomWidget::show_on_change(){
    if( refreshing ){ return; }
    refreshing = true;
    int   intValue;
    float floatValue;
    bool  boolValue;
    for( auto& __item : items ) {
        if( !__item.second.refreshable ){
            continue;
        }
        intValue   = __item.second.intValue;
        floatValue = __item.second.floatValue;
        boolValue  = __item.second.boolValue;
        __item.second.emit(LOAD,&__item.second);
        __item.second.emit(CHECK_REQUIRES,&__item.second);
        if(  
               __item.second.changed
            || __item.second.boolValue  != boolValue 
            || __item.second.intValue   != intValue
            || __item.second.floatValue != floatValue
        ){ redraw_item( __item.second.item_index, false ); }
        vTaskDelay(1);
    }
    refreshing = false;
};






void PhantomWidget::close(){

}

void PhantomWidget::disable_background_color(){
    transparent_background = true;
}
void PhantomWidget::set_suffix( int16_t item_index, int _suffix ){
    items[item_index].suffix = _suffix;
}

void PhantomWidget::set_colors_active( int16_t item_index, int bg, int fg ){ // change this to references. Color definitions won't change..
    items[item_index].colors.active_bg = bg;
    items[item_index].colors.active_fg = fg;
}
void PhantomWidget::set_colors_disabled( int16_t item_index, int bg, int fg ){
    items[item_index].colors.disabled_bg = bg;
    items[item_index].colors.disabled_fg = fg;
}
void PhantomWidget::set_font( int16_t item_index, int _font ){
    items[item_index].font = _font;
}
void PhantomWidget::set_size( int16_t _width, int16_t _height ){
    width  = _width;
    height = _height;
};
void PhantomWidget::set_float_decimals( int _decimals ){
    float_decimals = _decimals;
}



void PhantomWidget::set_item_colors( int16_t item_index, int &bg, int &fg ){
    if( items[item_index].disabled ){
        bg = items[item_index].colors.disabled_bg;
        fg = items[item_index].colors.disabled_fg;
    } else if( items[item_index].is_active ){
        bg = items[item_index].colors.active_bg;
        fg = items[item_index].colors.active_fg;
    } else if( !items[item_index].touch_enabled ){ // no touch attached
        //bg = items[item_index].colors.active_bg;
        //fg = items[item_index].colors.active_fg;
        bg = items[item_index].colors.default_bg;
        fg = items[item_index].colors.default_fg;
    } else if( items[item_index].has_touch ){ // is touched
        bg = items[item_index].colors.touched_bg;
        fg = items[item_index].colors.touched_fg;
    } else {
        bg = items[item_index].colors.default_bg;
        fg = items[item_index].colors.default_fg;
    }
}

int16_t PhantomWidget::get_item_index(){
    return num_items;
}

void PhantomWidget::set_use_sprite( bool _use_sprite ){
    use_sprite = _use_sprite;
};

//void PhantomWidget::set_callback( std::function<void()> callback ){
  //  __callback = callback;
//}



std::shared_ptr<PhantomWidget> PhantomWidget::set_is_refreshable(){
    is_refreshable = true;
    return shared_from_this();
}
std::shared_ptr<PhantomWidget> PhantomWidget::disabled_emits_global(){
    no_emits = true;
    return shared_from_this();
}



PhantomWidgetItem * PhantomWidget::addItem( 
    int item_type, const char* label,int8_t param_type, int8_t param_id, 
    int16_t width, int16_t height, int16_t _posx, int16_t _posy,
    bool touch_enabled, int font
){
    
    PhantomWidgetItem item = PhantomWidgetItem();

    item.item_index    = num_items;     // index for this item in the map
    item.label         = label;         // string label for the item
    item.param_id      = param_id;      // parameter id
    item.touch_enabled = touch_enabled; // no touch event
    item.width         = width;         // with of the item
    item.height        = height;        // height of the item
    item.xpos          = _posx;         // position x
    item.ypos          = _posy;         // position y
    item.font          = font;          // font
    item.output_type   = item_type;     // output type of the data drawn on the display
    item.valueType     = param_type;    // datatype of the value 

    item.set_parent_page_tab_id( tab_id );
    item.set_widget_id( index );
    item.set_parent_page_id( page_id ); // add page id to item. if the widget is not in a page container yet it will not assign a valid page but will do so once the widget is added to a page
    
    items[num_items] = item; // add item to the map
    ++num_items;
    return &items[item.item_index];
};
