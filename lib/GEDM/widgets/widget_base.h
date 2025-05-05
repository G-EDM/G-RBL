#pragma once
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

#ifndef PHANTOM_UI_WIDGET_BASE
#define PHANTOM_UI_WIDGET_BASE

#include <map>
#include "widget_item.h"

class PhantomWidget : 
    public EventManager, 
    public std::enable_shared_from_this<PhantomWidget> {

    public:

        virtual ~PhantomWidget();

        PhantomWidget( int16_t _width = 0, int16_t _height = 0, int16_t _xpos = 0, int16_t _ypos = 0, int8_t type = 0, bool _use_sprite = false ) 
        : width(_width),height(_height),xpos(_xpos),ypos(_ypos),num_items(0),widget_type(type),use_sprite(_use_sprite){};

        int16_t width;
        int16_t height;
        int16_t xpos;
        int16_t ypos;
        int16_t num_items;
        int8_t  widget_type;
        bool    use_sprite;
        int     intValue;
        float   floatValue;
        int     parameter_id;
        uint8_t index;
        uint8_t page_id                = 0;
        uint8_t tab_id                 = 0;
        int     float_decimals         = 3;
        bool    transparent_background = false;
        bool    is_visible             = true;
        bool    is_refreshable         = false;
        bool    changed                = false;
        bool    refreshing             = false;
        bool    no_emits               = false;

        std::shared_ptr<PhantomWidget> set_is_refreshable( void );
        std::shared_ptr<PhantomWidget> disabled_emits_global( void );

        IRAM_ATTR virtual void show_on_change( void );
        virtual bool touch_handler( int16_t item_index ){ return false; }

        virtual std::shared_ptr<PhantomWidget> reset_widget( PhantomSettingsPackage *item, uint8_t __type ){
            return shared_from_this();
        };
        virtual void reset_item( void ){};
        virtual void create( void )                                       = 0;
        virtual void show( void )                                         = 0;
        IRAM_ATTR virtual void redraw_item( int16_t item_index, bool reload = true ) = 0;

        void update_page_ids_for_items( void );

        //std::function<void()> __callback;
        //void set_callback( std::function<void()> callback );

        PhantomWidgetItem * addItem(
            int         item_type,
            const char* label,
            int8_t      param_type, // valueType: 1=int, 2= float, 3=bool
            int8_t      param_id, 
            int16_t     width,
            int16_t     height,
            int16_t     _posx,
            int16_t     _posy, 
            bool       touch_enabled, // no touch event if touch_enabled is false
            int        font           // if font is -1 it will load the icon font
        );

        void close( void );
        bool load_value( int16_t item_index );
        void set_item_colors( int16_t item_index, int &bg, int &fg );
        void disable_background_color( void );
        void set_suffix( int16_t item_index, int _suffix );
        void set_font( int16_t item_index, int _font );
        void set_colors_active( int16_t item_index, int bg, int fg );
        void set_colors_disabled( int16_t item_index, int bg, int fg );
        void set_size( int16_t _width, int16_t _height );
        void set_use_sprite( bool _use_sprite );
        void set_float_decimals( int _decimals );
        int16_t get_item_index( void );
        // map contains the widget items; no pointers
        std::map<std::int16_t, PhantomWidgetItem> items; 
};


#endif