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

#include "widget_item.h"
#include "widget_base.h"

PhantomWidgetItem::PhantomWidgetItem(){};

PhantomWidgetItem::~PhantomWidgetItem(){

}
ItemIdentPackage create_package( PhantomWidgetItem * item ){
    ItemIdentPackage package; // contains all the data to access the item later without passing the item to the emitter
    package.page_id   = item->get_parent_page_id();
    package.tab_id    = item->get_parent_page_tab_id();
    package.widget_id = item->get_widget_id();
    package.item_id   = item->get_index();
    return package;
}
void PhantomWidgetItem::emit_touch( void ){
    emit(TOUCHED, this );
}
void PhantomWidgetItem::emit_load( void ){
    emit(LOAD,this);
    emit(CHECK_REQUIRES,this);
}
void PhantomWidgetItem::emit_check( void ){

}
void PhantomWidgetItem::set_font( TFT_eSPI &display ){
    if( font == -1 ){ // if item font == -1 we select the icon font
        display.setFreeFont(FONT_A);
    } else {
        display.setTextFont( font ); 
        display.setTextSize( 1 );
    }
}
uint8_t PhantomWidgetItem::get_parent_page_id(){
    return parent_page_id;
}
uint8_t PhantomWidgetItem::get_parent_page_tab_id(){
    return page_tab_index;
}
uint8_t PhantomWidgetItem::get_widget_id(){
    return widget_index;
}
void PhantomWidgetItem::set_parent_page_id( uint8_t page_id ){
    parent_page_id = page_id;
}
void PhantomWidgetItem::set_parent_page_tab_id( uint8_t tab_id ){
    page_tab_index = tab_id;
}
void PhantomWidgetItem::set_widget_id( uint8_t widget_id ){
    widget_index = widget_id;
}
PhantomWidgetItem* PhantomWidgetItem::set_disabled( bool disable ){
    disabled = disable;
    return this;
}
PhantomWidgetItem* PhantomWidgetItem::set_is_refreshable( void ){
    refreshable = true;
    return this;
}
PhantomWidgetItem* PhantomWidgetItem::set_radius( uint8_t value ){
    radius = value;
    return this;
}
PhantomWidgetItem* PhantomWidgetItem::set_float_decimals( int8_t value ){
    float_decimals = value;
    return this;
}
// returns the value from strValue via c_str()
// returned pointer is invalidated if the item gets deleted
const char* PhantomWidgetItem::get_visible( void ){
    emit(BUILDSTRING,this);
    return strValue.c_str();
}
//{"", " kHz", " %", " mm", " mm/min", " RPM", " kSps", " Lines", " ADC", " us", " V", " A", " s", 0};
PhantomWidgetItem* PhantomWidgetItem::set_suffix( uint8_t __suffix ){
    suffix = __suffix;
    return this;
}
// returns the key index of the item in the map
int PhantomWidgetItem::get_index(){
    return item_index;
}
PhantomWidgetItem* PhantomWidgetItem::set_require_dpm( void ){
    dpm_required = true;
    return this;
}
PhantomWidgetItem* PhantomWidgetItem::set_require_motion( void ){
    motion_required = true;
    return this;
}
PhantomWidgetItem* PhantomWidgetItem::set_require_sd( void ){
    sd_required = true;
    return this;
}
PhantomWidgetItem* PhantomWidgetItem::set_outlines( uint value ){
    outline = value;
    return this;
}
// 1=touched, 0=default,2=active,3=disabled
PhantomWidgetItem* PhantomWidgetItem::set_colors( int state, int bg, int fg ){
    //for( auto item = items.begin(); item != items.end(); item++ ) {
        if( state == 1 ){
            colors.touched_bg = bg;
            colors.touched_fg = fg;
        } else if( state == 0 ){
            colors.default_bg = bg;
            colors.default_fg = fg;
        } else if( state == 2 ){
            colors.active_bg = bg;
            colors.active_fg = fg;
        } else if( state == 3 ){
            colors.disabled_bg = bg;
            colors.disabled_fg = fg;
        }
    //}
    return this;
}
PhantomWidgetItem* PhantomWidgetItem::set_int_value( int value ){
    intValue = value;
    return this;
}
PhantomWidgetItem* PhantomWidgetItem::set_string_value( std::string str ){
    strValue = str;
    return this;
}
PhantomWidgetItem* PhantomWidgetItem::set_float_value( float value ){
    floatValue = value;
    return this;
}
PhantomWidgetItem* PhantomWidgetItem::set_text_align( int align ){
    text_align = align;
    return this;
}

