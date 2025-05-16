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

#ifndef PHANTOM_UI_PAGE
#define PHANTOM_UI_PAGE

#include "language/en_us.h"
#include "widgets/widget_base.h"

struct PhantomTabItem { // 
    uint8_t tab_id;
    const char*  label;
};

struct PhantomTabContainer { // tab container holds all the widgets in the tab, page holds x tabs
    int8_t num_widgets = 0;
    std::map<std::int8_t, std::shared_ptr<PhantomWidget>> widgets;
};

class PhantomPage : public EventManager
                    ,public std::enable_shared_from_this<PhantomPage> 
{

    public:

        explicit PhantomPage( uint8_t id, uint8_t total_tabs = 1 );
        PhantomPage(const PhantomPage&) = delete;
        PhantomPage& operator=(const PhantomPage&) = delete;
        virtual ~PhantomPage();

        std::shared_ptr<PhantomPage> make_temporary( void );
        std::shared_ptr<PhantomPage> set_bg_color( int color );
        std::shared_ptr<PhantomPage> add_nav( uint8_t type );
        std::shared_ptr<PhantomPage> add_tab_item( uint8_t __tab_id, const char* __label );

        std::map<std::uint8_t, std::unique_ptr<PhantomTabContainer>> tabs;
        uint8_t add_widget( std::shared_ptr<PhantomWidget> widget );
        
        uint8_t get_active_tab( void );
        uint8_t get_total_tabs( void );
        uint8_t get_page_id( void );
        bool    active_tab_is_last( void );
        bool    active_tab_is_first( void );
        void    start( uint8_t id, uint8_t total_tabs );
        void    select_tab( uint8_t index, bool update_tab = false );
        bool    show( void );
        IRAM_ATTR void refresh( void );
        void    close( bool final = true );
        bool    is_temporary_page( void );


    private:

        uint8_t page_id;
        uint8_t num_tabs;
        uint8_t active_tab;
        uint8_t page_on_close;
        bool    is_active;
        bool    page_on_close_lock;
        int     tab_item_ids;
        int     bg_color;
        bool    is_temporary = false;
        bool    is_refreshing = false;

        std::map<std::uint8_t, PhantomTabItem> tab_items;

};
#endif