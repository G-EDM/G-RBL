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

#include <Update.h>
#include "sd_card.h"
#include "ui_interface.h"
#include "char_helpers.h"
#include "gedm_api/api.h"

const int MAX_FILENAME_TRIES                    = 1000;  // Prevents an infinite loop but also limits the total possible autonamed setting files to 1000
DMA_ATTR volatile     bool    sd_line_shift     = false; // if set to true it will push the lines from an sd card file to the input buffer once the buffer is empty
DMA_ATTR static const bool    enable_sd_support = ENABLE_SD_CARD;
DMA_ATTR static const int64_t refresh_interval  = 10000000; // refresh interval time in micros
DMA_ATTR              int64_t esp_micros        = 0;

PhantomSD SD_CARD;

enum sd_codes {
    SD_CNT_FAILED,
    SD_INIT,
    SD_NOT_ENA,
    SD_NOT_FND,
    SD_DIR_SKIP,
    SD_REMOVED,
    SD_MKDIR_TRY,
    SD_DIR_EXIST,
    SD_UPDATE_CK,
    SD_UPDATE_NO,
    SD_FIRMWARE_DO,
    SD_HODL,
    SD_HODL_MORE,
    SD_UPDATE_OK,
    SD_UPDATE_FAIL,
    SD_RENAME_FW,
    SD_RENAME_OK,
    SD_REBOOT,
    SD_F_NOT_FOUND,
    SD_FOPEN_ERR,
    SD_FOPEN_OK
};

std::map<sd_codes, const char*> sd_messages = {
    { SD_CNT_FAILED,  "Counting files failed" },
    { SD_INIT,        "SD init" },
    { SD_NOT_ENA,     "SD card support disabled" },
    { SD_NOT_FND,     "SD card not found" },
    { SD_DIR_SKIP,    "Building directory tree skipped" },
    { SD_REMOVED,     "SD card removed" },
    { SD_MKDIR_TRY,   "Trying to create folder:" },
    { SD_DIR_EXIST,   "Folder exists:" },
    { SD_UPDATE_CK,   "Checking for firmware update" },
    { SD_UPDATE_NO,   "    No update found" },
    { SD_FIRMWARE_DO, "Firmware updating" },
    { SD_HODL,        "    This can take some time" },
    { SD_HODL_MORE,   "    Don't disconnect the ESP" },
    { SD_UPDATE_OK,   "Update finished" },
    { SD_UPDATE_FAIL, "Update failed" },
    { SD_RENAME_FW,   "Renamed firmware.bin to .bak" },
    { SD_RENAME_OK,   "Renaming firmware failed" },
    { SD_REBOOT,      "Rebooting ESP" },
    { SD_F_NOT_FOUND, "File not found:" },
    { SD_FOPEN_ERR,   "Failed to open file:" },
    { SD_FOPEN_OK,    "File opened:" }
};

// wrapper function to ui.logger
const std::function<void(const char* line)> debug_log = [](const char *line){
    ui.emit(LOG_LINE,line);
};

bool PhantomSD::reset(){
    return true;
}

bool PhantomSD::run(){
    on(LOG_LINE,debug_log);
    emit(LOG_LINE,sd_messages[SD_INIT]);
    int error = SD_OK;
    if( !SD.begin(SS_OVERRIDE, __spi) ){
        error = SD_BEGIN_FAILED;
        state = SD_NOT_AVAILABLE;
        removed = true;
        emit(LOG_LINE,sd_messages[SD_NOT_FND]);
    } else {
        state   = SD_FREE;
        removed = false;
    }
    ui.emit(SD_CHANGE,&state);
    ready = true;
    return error == SD_BEGIN_FAILED ? false : true;
}

//##################################################################
// 
//##################################################################
bool PhantomSD::init( SPIClass &_spi ){
    root_folder     = SD_ROOT_FOLDER;
    settings_folder = SD_SETTINGS_FOLDER;
    gcode_folder    = SD_GCODE_FOLDER;
    firmware_bin    = "/firmware.bin";
    firmware_bak    = "/firmware.bak";
    last_refresh    = 0;//esp_timer_get_time();
    __spi           = _spi;
    return true;
}

int PhantomSD::connect(){
    int error = SD_OK;
    if( !SD.begin(SS_OVERRIDE, __spi) ){
        error = SD_BEGIN_FAILED;
        state = SD_NOT_AVAILABLE;
        removed = true;
    } else {
        state   = SD_FREE;
        removed = false;
        if( firmware_update() ){
            int data = 1;
            ui.emit(FIRMWARE_UPDATE_DONE, &data);
            gconf.flag_restart = true;
        }
    }
    ui.emit(SD_CHANGE,&state);
    vTaskDelay(1);
    return error;
}

int PhantomSD::build_directory_tree(){
    if( state != SD_FREE ){
        emit(LOG_LINE,sd_messages[SD_DIR_SKIP]);
        return SD_OK;
    }

    int error = create_folder( root_folder );
    if( error == SD_OK ){
        error = create_folder( settings_folder );
        error = create_folder( gcode_folder );
    } 
    return error;
};

int PhantomSD::create_folder( const char* folder ){
    int error  = SD_OK;
    if( !SD.exists(folder) ){
        emit(LOG_LINE,sd_messages[SD_MKDIR_TRY]);
        emit(LOG_LINE,folder);
        if( !SD.mkdir(folder) ){
            error = SD_MKDIR_FAILED;
        }
        vTaskDelay(1);
    }
    return error;
}

//##################################################################
// Firmware update section
//##################################################################
void update_progress_callback(size_t currSize, size_t totalSize) {
	return; 
}

bool PhantomSD::firmware_update_available(){ // check if firmware binary is available
    emit(LOG_LINE,sd_messages[SD_UPDATE_CK]);
    if( SD.exists( firmware_bin ) ){
        return true;
    } return false;
}
bool PhantomSD::firmware_update(){
    bool failed = false;
    if( state != SD_FREE || !firmware_update_available() ){ 
        emit(LOG_LINE,sd_messages[SD_UPDATE_NO]);
        return false; 
    } 
    state = SD_BUSY_LOCK;
    if( open_file( firmware_bin, FILE_READ ) == SD_OK ){
        emit(LOG_LINE,sd_messages[SD_FIRMWARE_DO]);
        emit(LOG_LINE,sd_messages[SD_HODL]);
        emit(LOG_LINE,sd_messages[SD_HODL_MORE]);
        Update.onProgress( update_progress_callback );
        Update.begin( current_file.size(), U_FLASH );
        Update.writeStream( current_file );
	    if( !Update.end() ){
            failed = true;
	    } 
        close_active();
        SD.remove(firmware_bak);
        SD.rename( firmware_bin, firmware_bak );
        delay(1000);
    }
    state = SD_FREE;
    return failed?false:true;
}



bool PhantomSD::start_gcode_job( std::string full_path ){
    // lock sd reader
    // reset everything needed
    // load gcode file
    emit(LOG_LINE,"@ SD GCode start called");
    file_end = false;
    if( begin_multiple( full_path, FILE_READ ) ){ // open the file and lock sd reader
        // success
        sd_line_shift = true; // set flag to shift out lines from the file
        return true;
    } else {
        // failed to open file
        sd_line_shift = false; // unset flag to shift lines from file
    }
    return false;
}
bool PhantomSD::end(){
    file_end     = false;
    current_line = 0;
    close_active();
    state = SD_FREE;
    return true;
}
bool PhantomSD::end_gcode_job( void ){
    sd_line_shift = false; // unset flag to shift lines from file
    end();                 // close file and unlock reader
    return true;
}

// will try to begin the sd session multiple times. Default 100 attemps
bool PhantomSD::begin_multiple( std::string full_path, const char* type, int max_rounds ){
    bool success = false;
    while( --max_rounds > 0 ){
        if( begin( full_path.c_str(), type ) ){
            success = true;
            break;
        } else {
            vTaskDelay(50);
        }
    }
    if( !success ){
        emit(LOG_LINE,"SD start looped out");
    }
    return success;
}

bool PhantomSD::begin( const char* path, const char* type ){ // default type = read aka "r"
    if( state > SD_FREE ){
        emit(LOG_LINE,"SD not available or busy");
        return false; // sd not available or busy
    }
    state = SD_BUSY_LOCK;
    int error = open_file( path, type );
    bool success = error == SD_OK ? true : false;
    if( success ){
        current_line = 0;
    } else {
        emit(LOG_LINE,"SD open failed");
        state = SD_FREE;
    }
    return success;
}



// warning: not thread safe
// sd is locked in the process and it should be safe to call
// this from core 1 too
// but only from one core 
bool PhantomSD::shift_gcode_line(){

    if( file_end ){
        vTaskDelay(1);
        return false;
    }

    if( !api::buffer_available() ){
        // buffer not free; skip this round
        // move this one out of the sd logic
        // best would be to trigger this in the protocol loop...
        vTaskDelay(1);
        return true;
    }

    char buffer[DEFAULT_LINE_BUFFER_SIZE]; // stack memory allocation // char * ... heap = dynamic
    
    if( SD_CARD.get_line( buffer, DEFAULT_LINE_BUFFER_SIZE, true ) ){
        // submit the line
        api::push_cmd( buffer, false );
        return true;
    } else {
        file_end = true;
        emit(LOG_LINE,"  Filend flagged");
    }
    
    return false;
}

// this function is used to get gcode lines and also normal lines
// the gcode lines need a linebreak while normal lines don't
// gcode lines are pushed to the inputbuffer and the protocol collects them
// without linebreak the protocol will not be able to parse the line
bool PhantomSD::get_line( char *line, int size, bool add_linebreak ){
    if(!current_file){
        emit(LOG_LINE,"Get line failed. File not open");
        return false;
    }
    current_line += 1;
    int length = 0;
    char c;
    while( current_file.available() ){
        //current_file.readStringUntil();
        c = current_file.read();
        if (c == '\n' || c == '\r'){
            if( length == 0 ){
                // linebreak at the beginning
                continue;
            }
            break;
        }
        line[length] = c;
        if( ++length >= size-1 ){
            emit(LOG_LINE,"Unhandled: Line size exceeded buffer length");
            return false;
        }
    }

    if( length > 0 && add_linebreak ){
        line[length] = '\n'; // line end needed for the protocol
        ++length;
    } 
    
    line[length] = '\0';

    int _continue = length || current_file.available();

    return _continue; 
}

IRAM_ATTR bool PhantomSD::refresh(){
    esp_micros = esp_timer_get_time();
    if( state > SD_NOT_AVAILABLE || esp_micros - last_refresh < refresh_interval ){ 
        vTaskDelay(1);
        return false; 
    } 

    if( !exists(settings_folder) || SD.cardSize() <= 0 ){
        state = SD_NOT_AVAILABLE;
        if( !removed ){
            removed = true;
            emit(LOG_LINE,sd_messages[SD_REMOVED]);
        }
        close_active();
        SD.end();
        int error = connect();
        last_refresh = esp_timer_get_time();
        return true;
    } else { 
        state = state==SD_NOT_AVAILABLE?SD_FREE:state;
    }
    last_refresh = esp_timer_get_time();
    return false;
}

int PhantomSD::get_state(){
    return state;
}

bool PhantomSD::is_free(){
    return state == SD_FREE ? true : false;
}

void PhantomSD::reset_current_file_index( int tracker_index ){
    file_index_tracker[tracker_index] = 0;
}

int PhantomSD::get_current_tracker_index( int tracker_index ){
    return file_index_tracker[tracker_index];
}

void PhantomSD::decrement_tracker( int tracker_index, int decremet_by ){
    file_index_tracker[tracker_index] -= decremet_by;
    if( file_index_tracker[tracker_index] < 0 ){
        file_index_tracker[tracker_index] = 0;
    }
}

bool PhantomSD::go_to_last( int tracker_index ){
    if( current_file && current_file.isDirectory() ){
        for( int i = 0; i < file_index_tracker[tracker_index]; ++i ){
            current_file.openNextFile();
        }
        return true;
    } return false;
}

bool PhantomSD::get_next_file( int tracker_index, std::function<void( const char* file_name, int file_index )> callback ){
    if( callback != nullptr && current_file && current_file.isDirectory() ){
        File file = current_file.openNextFile();
        if( !file ){
            callback( "", file_index_tracker[tracker_index] );
        } else {
            if( file.isDirectory() ){}
            ++file_index_tracker[tracker_index];
            callback( get_filename_from_path( file.name() ).c_str(), file_index_tracker[tracker_index] );
            file.close();
        }
        //file.close();
        return true;
    }
    return false;
}

//##########################################
// Core SD card section
//##########################################
bool PhantomSD::add_line( const char* line ){
    if( !has_open_file || !current_file ){
        return false; // no open file
    }
    current_file.println(line);vTaskDelay(1);
    current_file.flush();vTaskDelay(1);
    return true;
}

bool PhantomSD::delete_file( const char* path ){
    if( exists( path ) ){
        return SD.remove(path);
    }
    return false;
}

bool PhantomSD::exists( const char* path ){
    return SD.exists(path);
}

void PhantomSD::close_active(){
    if( ! has_open_file ){
        return; // no open file
    }
    current_file.close();vTaskDelay(1);
    has_open_file = false;
}

int PhantomSD::open_file( const char* file, const char* mode ){ // full path to the file 
    if( strcmp(mode, FILE_READ) == 0 && !exists( file ) ){
        return SD_FILE_NOT_FOUND;
    }
    close_active();
    current_file = SD.open( file, mode ); vTaskDelay(1);
    if( ! current_file ){
        return SD_OPEN_FILE_FAILED;
    }
    has_open_file = true;
    return SD_OK;
}

void PhantomSD::open_folder( const char* folder ){
    if( open_file( folder, FILE_READ ) == SD_OK ){
        current_file.rewindDirectory();vTaskDelay(1);
    } else {
    }
}


bool PhantomSD::exit(){
    state = SD_NOT_AVAILABLE;
    close_active();
    SD.end();
    return true;
}





//############################################################################
// Takes a full path and extracts the filename with extension
//############################################################################
std::string PhantomSD::get_filename_from_path( const std::string& filePath ) {
    size_t lastSlashPos = filePath.find_last_of("/\\");
    if (lastSlashPos == std::string::npos) {
        return filePath;
    }    
    return filePath.substr(lastSlashPos + 1);
}

//############################################################################
// Creates a unique file path like Phantom-settings-999.conf
// returns the full path like /Phantom/settings/Phantom-settings-999.conf
//############################################################################
std::string PhantomSD::generate_unique_filename(){
    int start = 0;
    int vTaskDelayAfter = 10;

    while( ++start < MAX_FILENAME_TRIES ){
        if(--vTaskDelayAfter<=0){vTaskDelay(1);vTaskDelayAfter=10;}
        std::string unique_path = RELEASE_NAME;
        unique_path += "-settings-";
        unique_path += intToString( start );
        unique_path += ".conf";
        std::string full_path = SD_SETTINGS_FOLDER;
        full_path += "/";
        full_path += unique_path;
        if (!SD_CARD.exists(full_path.c_str())) {
            return full_path; // file name available
        }
    }
    throw std::runtime_error("too many files");
}

//###########################################################################
// the count files in folder by extension counts files in folder by extension
//###########################################################################
int PhantomSD::count_files_in_folder_by_extension( const char* path, const char* extension ){
    // open folder and store it in current_file

    int max_rounds = 200;
    bool success   = false;

    while( --max_rounds >= 0 && !success ){
        if( begin( path) ){
            success = true;
            break;
        } else {
            vTaskDelay( 20 );
        }
    }

    if( ! success ){
        // problem
        emit(LOG_LINE,sd_messages[SD_CNT_FAILED]);
        emit(LOG_LINE,path);
        return -1; // return -1 on error
    }

    current_file.rewindDirectory();
    int vTaskDelayAfter = 10; // add a vTaskDelay(1); after given number of files
    int num_files       = 0;
    File entry          = current_file.openNextFile();
    while( entry ){
        if( !entry.isDirectory() ){
            // check if the file has the correct extension
            if( path_has_extension( entry.name(), extension ) ){
                ++num_files;
            }
        }
        entry = current_file.openNextFile();
        if( --vTaskDelayAfter <= 0 ){
            vTaskDelayAfter = 10;
            //vTaskDelay(1);
        }
    }
    end(); // close the current cile and reset the state etc.
    return num_files;
}

int PhantomSD::count_files_in_folder( const char* path ){
    // open folder and store it in current_file

    int max_rounds = 200;
    std::string full_path = path;
    bool success = begin_multiple( full_path, FILE_READ );

    if( ! success ){
        // problem
        emit(LOG_LINE,sd_messages[SD_CNT_FAILED]);
        emit(LOG_LINE,path);
        return -1; // return -1 on error
    }

    current_file.rewindDirectory();
    int vTaskDelayAfter = 10; // add a vTaskDelay(1); after given number of files
    int num_files       = 0;
    File entry          = current_file.openNextFile();
    while( entry ){
        if( !entry.isDirectory() ){
            // check if the file has the correct extension
            ++num_files;
        }
        entry = current_file.openNextFile();
        if( --vTaskDelayAfter <= 0 ){
            vTaskDelayAfter = 10;
        }
    }
    end(); // close the current cile and reset the state etc.
    return num_files;
}