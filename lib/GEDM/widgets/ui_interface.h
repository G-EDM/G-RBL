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

#ifndef PHANTOM_UI_INTERFACE
#define PHANTOM_UI_INTERFACE


#include <sstream>
#include <iostream>
#include <vector>
#include <functional>
#include <unordered_map>
#include <map>
#include <HardwareSerial.h>
#include <memory>
#include <list>
#include <algorithm>
#include <unordered_set>
#include "sd_card.h"
#include "widgets/phantom_page.h"
#include "widgets/widget_vertical_button_list.h"
#include "widgets/widget_button.h"
#include "widgets/widget_list.h"
#include "widgets/widget_split_list.h"
#include "widgets/widget_logger.h"
#include "widgets/widget_files.h"


static const uint8_t page_tree_depth = 20;

typedef struct SETTER_ITEM { // little bloated
    int8_t param;
    int   intValue;
    float floatValue;
    bool  boolValue;
} SETTER_ITEM;

IRAM_ATTR void fill_screen( int color );

struct PhantomTracker {
    uint8_t active_page = 0;
};

typedef struct touch_cache {
    uint16_t x, xf, xx, y, yf, yy, item_pos_x, item_pos_y, item_w, item_h;
    bool block_touch, touched, scanning;
} touch_cache;


enum sd_write_action_type {
    SD_LOAD_SETTINGS = 0, // create a custom name
    SD_SAVE_AUTONAME = 1, // auto generate name
    SD_AUTOSAVE      = 2  // save as last_settings
};

std::shared_ptr<PhantomWidget> factory_button(      int16_t w = 0, int16_t h = 0, int16_t x = 0, int16_t y = 0, int8_t t = 0, bool s = false );
std::shared_ptr<PhantomWidget> factory_split_list(  int16_t w = 0, int16_t h = 0, int16_t x = 0, int16_t y = 0, int8_t t = 0, bool s = false );
std::shared_ptr<PhantomWidget> factory_list(        int16_t w = 0, int16_t h = 0, int16_t x = 0, int16_t y = 0, int8_t t = 0, bool s = false );
std::shared_ptr<PhantomWidget> factory_vbuttonlist( int16_t w = 0, int16_t h = 0, int16_t x = 0, int16_t y = 0, int8_t t = 0, bool s = false );
std::shared_ptr<PhantomWidget> factory_nkeyboard(   int16_t w = 0, int16_t h = 0, int16_t x = 0, int16_t y = 0, int8_t t = 0, bool s = false );
std::shared_ptr<PhantomLogger> factory_logger(      int16_t w = 0, int16_t h = 0, int16_t x = 0, int16_t y = 0, int8_t t = 0, bool s = false );
std::shared_ptr<PhantomWidget> factory_akeyboard(   int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s );

class PhantomUI : public EventManager {

    public:

        info_codes msg_id;

        std::string settings_file;
        std::string gcode_file;
        bool load_settings_file_next;
        bool deep_check;
        int  sd_card_state;

        std::shared_ptr<PhantomLogger> logger;
        std::shared_ptr<PhantomWidgetItem> get_item( ItemIdentPackage &package );
        std::shared_ptr<PhantomWidget>     get_widget_by_item( PhantomWidgetItem * item );
        std::shared_ptr<PhantomWidget>     get_widget( uint8_t page_id, uint8_t page_tab_id, uint8_t widget_id, uint8_t item_id );
        std::shared_ptr<PhantomPage>       addPage( uint8_t page_id, uint8_t num_tabs );
        std::shared_ptr<PhantomPage>       get_page( uint8_t page_id );
        std::map<std::uint8_t, std::shared_ptr<PhantomPage>> pages;

        bool save_settings_writeout( std::string file_name );

        uint8_t next_page_to_load;
        bool    load_next_page;
        bool    save_param_next;
        int16_t save_param_id;
        void show_page_next( uint8_t page_id );

        IRAM_ATTR void monitor_touch( void );
        IRAM_ATTR bool touch_within_bounds(int x, int y, int xpos, int ypos, int w, int h );
        IRAM_ATTR void track_touch( void );
        IRAM_ATTR void process_update( void );
        IRAM_ATTR void wpos_update( void );
        IRAM_ATTR void reload_page_on_change( void );
        IRAM_ATTR void scope_update( void );

        bool save_settings( sd_write_action_type action = SD_AUTOSAVE ); 
        void set_gcode_file( const char* file_path );
        void set_settings_file( const char* file_path );
        bool delete_nvs_storage( void );
    
        void    emit_param_set(int param_id, const char* value);
        void    lock_touch( void );
        void    unlock_touch( void );
        void    show_page( uint8_t page );
        bool    get_touch( uint16_t *x, uint16_t *y );
        uint8_t get_previous_active_page( void );
        void    build_phantom( void );
        bool    update_firmware( void );
        void    begin( void );
        void    close_active( void );
        void    reload_page( void );
        void    get_has_sd_card( void );
        void    get_has_motion( void );
        void    init( void );
        void    restart_display( void );
        void    loader_screen( void );
        bool    get_motion_is_active( void );
        void    set_motion_is_active( bool state );
        void    sd_refresh( void );
        void    update_active_tabs( uint8_t page_id, uint8_t tab );
        uint8_t get_active_page( void );
        uint8_t get_active_tab_for_page( uint8_t page_id );
        bool    restore_last_session( void );
        bool    save_current_to_nvs( int param_id = -1 );
        bool    start( void );
        void    touch_calibrate( bool force );
        void    load_settings( std::string path_full );
        void    set_scope_and_logger( uint8_t page_id );
        void    exit( void );
        void    init_sd( void );

    private:

        uint8_t page_tree[page_tree_depth];
        uint8_t active_tabs[TOTAL_PAGES];
        uint8_t page_tree_index;

        bool           is_ready = false;
        bool           motion_is_active;
        bool           sd_card_is_available;
        uint8_t        active_page;
        uint8_t        previous_active_page;
        PhantomTracker tracker;

        //void save_settings( const char* file_name );


    protected:

        uint8_t active_tab;

};

extern PhantomUI ui;


#endif