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

#ifndef PHANTOM_UI_WIDGET_NKEY
#define PHANTOM_UI_WIDGET_NKEY


#include "language/en_us.h"
#include "widgets/widget_base.h"






class PhantomKeyboardNumeric 
        : public PhantomWidget
        //,public std::enable_shared_from_this<PhantomKeyboardNumeric>
    {
    public:
        PhantomKeyboardNumeric( 
            int16_t w = 0, 
            int16_t h = 0, 
            int16_t x = 0, 
            int16_t y = 0, 
            int8_t type = 0, 
            bool _use_sprite = false 
        ) : PhantomWidget(w,h,x,y,type,_use_sprite ), callback(nullptr) {};
        virtual ~PhantomKeyboardNumeric(){};
        void build( std::function<void()> __callback );
        void create( void )                   override;
        void show( void )                     override;
        void IRAM_ATTR redraw_item( int16_t item_index, bool reload = true ) override;
        bool touch_handler( int16_t item_index ) override;

        void reset_item( void ) override;
        std::shared_ptr<PhantomWidget>          reset_widget( PhantomSettingsPackage *_package, uint8_t __type ) override;
        std::shared_ptr<PhantomKeyboardNumeric> get_shared_ptr( void );

        void set_tooltip( const char* str );

        void set_callback( std::function<void( PhantomSettingsPackage )> _callback );
        std::function<void( PhantomSettingsPackage _package )> callback;

        
        void close( void );
        void hide( void );

        void set_default_value( void );
        
        int         initialIntValue;
        float       initialFloatValue;
        std::string strValue;

    private:

        PhantomSettingsPackage package;

        std::string tooltip = "No tooltip available.";
        int   type;

};


#endif