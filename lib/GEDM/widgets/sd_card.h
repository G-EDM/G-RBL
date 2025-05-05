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

#ifndef PHANTOM_UI_SDCARD
#define PHANTOM_UI_SDCARD

#include "SD.h"
#include <stdint.h>
#include "config/definitions.h"
#include "event_manager.h"

enum sd_errors {
    SD_OK = 0,
    SD_FIRMWARE_UPDATE_FAILED,
    SD_FILE_NOT_FOUND,
    SD_OPEN_FILE_FAILED,
    SD_BEGIN_FAILED,
    SD_MKDIR_FAILED
};

enum sd_states {
    SD_FREE          = 0, // sd is free to use
    SD_NOT_AVAILABLE = 1, // sd not found or removed
    SD_NOT_ENABLED,       // sd card support disabled by config
    SD_HAS_FILE_OPEN,     // 
    SD_BUSY_LOCK,         // locked in process
    SD_ERROR
};

enum sd_events {
    LOAD_SETTINGS,
    SAVE_SETTINGS
};

extern DMA_ATTR volatile bool sd_line_shift;
static const int DEFAULT_BUFFER_SIZE = 256;

class PhantomSD : public EventManager {

    public:

        ~PhantomSD(){};
        PhantomSD() : current_line(0), 
                      current_cursor(0),
                      state(SD_NOT_AVAILABLE), 
                      state_last(-1), 
                      has_open_file(false), 
                      ready(false), 
                      removed(false),
                      last_refresh(0),
                      file_end(false) {};

        std::string get_filename_from_path( const std::string& filePath );
        std::string generate_unique_filename( void );

        void  init( SPIClass &_spi );
        bool  reset( void );
        bool  begin( const char* path, const char* type = FILE_READ );
        // begin multiple starts an sd session and blocks sd access for everything else until
        // it is released with SD_CARD.end()
        bool  begin_multiple( std::string folder, const char* type, int max_rounds = 100 );
        bool  end( void );
        bool  exit( void );
        bool  add_line( const char* line );
        bool  is_free( void );
        IRAM_ATTR bool  refresh( void );
        bool  get_line( char *line, int size, bool add_linebreak = false );
        bool  start_gcode_job( std::string full_path );
        bool  end_gcode_job( void );
        bool  shift_gcode_line( void );
        bool  delete_file( const char* path );
        bool  exists( const char* path );
        bool  get_next_file( int tracker_index, std::function<void( const char* file_name, int file_index )> callback = nullptr );
        bool  go_to_last( int tracker_index );

        int   get_state( void );
        int   count_files_in_folder_by_extension( const char* path, const char* extension );
        int   count_files_in_folder( const char* path );
        int   get_current_tracker_index( int tracker_index );

        void  decrement_tracker( int tracker_index, int decremet_by );
        void  reset_current_file_index( int tracker_index );

    private:
        
        bool deep_check( const char* path );
        bool card_available( void );
        bool firmware_update( void );
        bool firmware_update_available( void );

        int  connect( void );
        int  build_directory_tree( void );
        int  create_folder( const char* folder );
        int  open_file( const char* file, const char* mode = FILE_READ );

        void open_folder( const char* folder );
        void close_active( void );
        void create_events( void );

        int      current_line;
        int      current_cursor;
        int      state;
        int      state_last;

        bool     has_open_file;
        bool     ready;
        bool     removed;

        int64_t  last_refresh;

        bool     file_end;

        uint32_t current_file_pointer_position;
        uint32_t previous_file_pointer_position;

        int      file_index_tracker[5];

        File     current_file;
        SPIClass __spi;

        const char* root_folder;
        const char* settings_folder;
        const char* gcode_folder;
        const char* firmware_bin;
        const char* firmware_bak;

};


extern PhantomSD SD_CARD;

#endif