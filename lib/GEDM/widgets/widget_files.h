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

#ifndef PHANTOM_UI_WIDGET_8LIST
#define PHANTOM_UI_WIDGET_8LIST

#include "widget_base.h"
#include "char_ringbuffer.h"

static const int FILE_ITEMS_PER_PAGE = 6;


class file_browser : public PhantomWidget {

    public:
    
        file_browser( 
            int16_t w = 0, 
            int16_t h = 0, 
            int16_t x = 0, 
            int16_t y = 0, 
            int8_t type = 0, 
            bool _use_sprite = false 
        ) : PhantomWidget(w,h,x,y,type,_use_sprite ){};
        virtual ~file_browser();
        void create( void ) override;
        void show( void ) override;
        IRAM_ATTR void redraw_item( int16_t item_index, bool reload = true ) override;
        bool touch_handler( int16_t item_index ) override;
        void set_page_title( std::string str );
        void set_folder( std::string str );
        void set_num_items( int8_t num );
        bool load_file_batch( bool forward );
        bool start( void );
        bool start_sd( void );
        void set_file_index_pointer( int index );
        void addItemCustom( const char* file_name, int file_index );
        std::string selected_file;
        void set_file_type( int type );
        int get_file_type( void );

    private:
        void draw_single_item();
        std::string title;
        std::string folder;
        int8_t      items_per_page;
        uint32_t    file_index_pointer;
        uint16_t    index_counter;
        bool        sd_success;
        int         results_success;
        int         file_type;


};

#endif