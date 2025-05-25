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

#include <Preferences.h>
#include <iomanip>

#include "ui_interface.h"
#include "tft_display/gpo_scope.h"
#include "tft_display/ili9341_tft.h"
#include "char_helpers.h"
#include "widgets/widget_numkeyboard.h"
#include "widgets/widget_alphakeyboard.h"
#include "widgets/widget_alarm.h"

// callback is used by the ui_controller backend class
const std::function<void(int *page_id)> show_page_ev = [](int *page_id){
    int page = *page_id;
    ui.show_page( page );
};

//############################################################################################################
// Shared pointer to base class factory functions
//############################################################################################################
std::shared_ptr<PhantomWidget> factory_button( int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){
    return std::make_shared<PhantomButton>( w, h, x, y, t, s );
}
std::shared_ptr<PhantomWidget> factory_split_list( int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){
    return std::make_shared<PhantomSplitList>( w, h, x, y, t, s );
}
std::shared_ptr<PhantomWidget> factory_list( int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){
    return std::make_shared<PhantomList>( w, h, x, y, t, s );
}
std::shared_ptr<PhantomWidget> factory_vbuttonlist( int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){
    return std::make_shared<PhantomVerticalButtonList>( w, h, x, y, t, s );
}
std::shared_ptr<PhantomWidget> factory_nkeyboard( int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){
    return std::make_shared<PhantomKeyboardNumeric>( w, h, x, y, t, s );
}
std::shared_ptr<PhantomWidget> factory_akeyboard( int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){
    return std::make_shared<PhantomKeyboardAlpha>( w, h, x, y, t, s );
}
std::shared_ptr<PhantomLogger> factory_logger( int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){
    return std::make_shared<PhantomLogger>( w, h, x, y, t, s );
}
std::shared_ptr<PhantomWidget> factory_elist( int16_t w, int16_t h, int16_t x, int16_t y, int8_t t, bool s ){
    return std::make_shared<file_browser>( w, h, x, y, t, s );
}

//############################################################################################################
// Symbols, suffixes and stuff
//############################################################################################################
const char *suffix_table[]                          = {"", " kHz", " %", " mm", " mm/min", " RPM", " kSps", " Lines", " ADC", " us", " V", " A", " s", "ms", " steps", 0};
const char icons[9][2]                              = {"i","h","j","f","?","g","d","c","e"}; // icon font
void (*page_creator_callbacks[TOTAL_PAGES])( void ) = { nullptr };

DMA_ATTR bool settings_changed = false;
DMA_ATTR touch_cache touch;
PhantomUI            ui;



void PhantomUI::restart_display(){
    if( get_active_page() != PAGE_IN_PROCESS ){ return; }
    tft.init();vTaskDelay(1);
    tft.setRotation(3);vTaskDelay(1);
    tft.setCursor(0, 0, 4);vTaskDelay(1);
    fill_screen(TFT_BLACK);
    touch_calibrate( false );vTaskDelay(1);
    tft.setCursor(0, 0, 4);
    tft.setTextColor(TFT_LIGHTGREY);
    show_page( get_active_page() );
}




bool PhantomUI::start(){ 
    if( gconf.flag_restart ){
        return true;
    }
    vTaskDelay(500);
    restore_last_session();
    return true;
}


/*
to fully delete the nvs including the namespaces
#include <nvs_flash.h>
void setup() {
  nvs_flash_erase();
  nvs_flash_init();
}
*/

//############################################################################################################
// Touch calibration
//############################################################################################################
void PhantomUI::touch_calibrate( bool force ){
    if( DISABLE_TFT_CALIBRATION && !force ){
      return;
    }
    size_t   array_size = 5;
    uint8_t  success    = 0;
    uint16_t calibration_data[array_size];

    Preferences p;
    if( p.begin( RELEASE_NAME ) ){
        if( !is_ready ){
            uint16_t z, count = 0, confirms = 0;
            while( ++count < 10 ){
                z = tft.getTouchRawZ();
                if( z > 400 ){
                    ++confirms;
                    vTaskDelay(100);
                }
            }
            if( confirms > 5 ){
                force = true;
            }
        }
        if (REPEAT_DISPLAY_CALIBRATION || force){
            p.remove("calibration");
        } else {
            size_t bytesRead = p.getBytes("calibration", calibration_data, array_size * sizeof(uint16_t));
            if( bytesRead == array_size * sizeof(uint16_t) ) {
                // found the data
                success = 1;
            } else {
                // no data found
            }
        }
    } else {
        // problem...
        return;
    }

    if (success && !REPEAT_DISPLAY_CALIBRATION){
        tft.setTouch(calibration_data);
    } else {
        fill_screen(TFT_BLACK);
        while( tft.getTouchRawZ() > 100 ){
            vTaskDelay(100);
        }
        tft.setCursor(20, 0);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE);
        tft.println("Touch corners as indicated");vTaskDelay(1);
        tft.println("It should start with the");vTaskDelay(1);
        tft.println("left top corner! If a different");vTaskDelay(1);
        tft.println("corner was the first to do");vTaskDelay(1);
        tft.println("restart the calibration!");vTaskDelay(1);
        tft.setTextFont(1);
        tft.println();vTaskDelay(1);
        if( REPEAT_DISPLAY_CALIBRATION ){
            tft.setTextColor(TFT_GREENYELLOW);
            tft.println("");
            tft.println("To prevent this calibration from running again set");vTaskDelay(1);
            tft.println("");
            tft.println("REPEAT_DISPLAY_CALIBRATION in definitions_common.h");vTaskDelay(1);
            tft.println("");
            tft.println("to false and reflash the board");vTaskDelay(1);
        }
        tft.calibrateTouch(calibration_data, TFT_MAGENTA, TFT_BLACK, 15);
        tft.setTextColor(TFT_GREENYELLOW);
        tft.println("Calibration finished.");vTaskDelay(1);
        size_t bytes_write = p.putBytes("calibration", calibration_data, array_size * sizeof(uint16_t));
        // Check if the read bytes match the expected size
        if (bytes_write == array_size * sizeof(uint16_t)) {
        } else {
        }
        fill_screen(TFT_BLACK);
    }

    p.end();

}

//############################################################################################################
// Custom fill screen function that does not fill the screen in one long run but in slices 
// Helps with high speed interrupts
//############################################################################################################
IRAM_ATTR void fill_screen( int color ){
  // drawing a full screen is a heavy task and the ISR doesn't like it
  // having an ISR running at highspeed parallel to a heavy display ui needs some hacks
  int divider = 10, display_width = 320, display_height = 240, pos_y = 0, height_fragment = display_height / divider;
  for( int i = 0; i < divider; ++i ){
    tft.fillRect( 0, pos_y, display_width, height_fragment, color );
    pos_y += height_fragment;
    vTaskDelay(1);
  }
}


//############################################################################################################
// Main touch monitor 
// Checks for a touch event and loops over all items in all widget in the current tab of the active page
// and triggers an event if the touch was done on an item thaqt is touchable
//############################################################################################################
IRAM_ATTR void PhantomUI::track_touch() {
    touch.xf = touch.x;
    touch.yf = touch.x;
    while( tft.getTouch( &touch.xx, &touch.yy, 500 ) ){ 
        touch.xf = touch.xx;
        touch.yf = touch.yy;
        vTaskDelay(1); 
    }
}

IRAM_ATTR bool PhantomUI::touch_within_bounds( int x, int y, int xpos, int ypos, int w, int h ){
    return (x >= xpos && x <= (xpos + w) &&
            y >= ypos && y <= (ypos + h));
}

IRAM_ATTR void PhantomUI::monitor_touch(){

    if( touch.scanning == true ){
        return;
    }

    touch.scanning = true;

    if( !touch.block_touch && tft.getTouch( &touch.x, &touch.y ) ){

        if( pages.find(active_page) == pages.end() ){
            return;
        }

        touch.touched = false;
        active_tab    = pages[active_page]->get_active_tab();

        auto& widgets = pages[active_page]->tabs[active_tab]->widgets;
    
        for( auto& widget : widgets ){

            if( touch.touched || touch.block_touch ){ break; }

            vTaskDelay(1); 

            if( touch_within_bounds( touch.x, touch.y, widget.second->xpos, widget.second->ypos, widget.second->width, widget.second->height ) ){

                // touch is within this widget container
                // lets scan for the specific item
                for( auto& item : widget.second->items ){

                    if( touch.block_touch ){ break; }
     
                    if( touch_within_bounds( touch.x, touch.y, item.second.truex, item.second.truey, item.second.width, item.second.height ) ){

                        vTaskDelay(1); 

                        if( item.second.touch_enabled && !item.second.disabled ){
                            item.second.has_touch = true;
                            widget.second->redraw_item( item.second.item_index, false );
                            track_touch();
                            item.second.has_touch = false;
                            widget.second->redraw_item( item.second.item_index, false );
                            // touched item found
                            // only trigger if last touch coords remained within the item

                            //if( touch_within_bounds( touch.xf, touch.yf, item.second.truex, item.second.truey, item.second.width, item.second.height ) ){
                                item.second.emit_touch();
                            //}

                            touch.touched  = true;
                            touch.scanning = false;
                            return;
                            //break;
                        } else { // touch within this item but no touch enabled. Can exit.
                            touch.touched = true;
                            break;
                        }

                    }

                }

            }

            if( touch.touched || touch.block_touch ){ break; }

        }

        if( !touch.touched ){
            ui.restart_display(); // this only resets the display if within a running edm process and only if the touch was not at any touch element
                                  // if the display goes blank just touch anywhere on the display where there is no button
                                  // also only works if no at the settings menu
        }

    }

    touch.scanning = false;

};



//############################################################################################################
// Widget control methos
//############################################################################################################
// use with care as there are no checks if an item exists!
std::shared_ptr<PhantomWidget> PhantomUI::get_widget( uint8_t page_id, uint8_t page_tab_id, uint8_t widget_id, uint8_t item_id ){
    return pages[page_id]->tabs[page_tab_id]->widgets[widget_id];
}

std::shared_ptr<PhantomWidget> PhantomUI::get_widget_by_item( PhantomWidgetItem * item ){ // use with care due to the slow speed
    if( item == nullptr ){ return nullptr; }
    auto page = pages.find( item->get_parent_page_id() );
    if( page == pages.end() ){ return nullptr; } // page not found
    auto tab = page->second->tabs.find( item->get_parent_page_tab_id() );
    if( tab == page->second->tabs.end() ){ return nullptr; } // tab not found
    auto widget = tab->second->widgets.find( item->get_widget_id() );
    if( widget == tab->second->widgets.end() ){ return nullptr; } // widget not found
    return widget->second;
}

//############################################################################################################
// Page control methods
//############################################################################################################
// get the cached active tab for a given page
uint8_t PhantomUI::get_active_tab_for_page( uint8_t page_id ){
    return active_tabs[page_id];
}
// update the cached active tab for a given page
void PhantomUI::update_active_tabs( uint8_t page_id, uint8_t tab ){
    active_tabs[page_id] = tab;
}
// fully reload the page
void PhantomUI::reload_page(){
    if( !is_ready || touch.block_touch ){return;}
    if( pages.find(active_page) == pages.end() ){ return; }
    pages[active_page]->show();
};
// reload only items flagged as refreshable
IRAM_ATTR void PhantomUI::reload_page_on_change(){
    if( !is_ready || touch.block_touch ){return;}
    if( pages.find(active_page) == pages.end() ){ return; }
    pages[active_page]->refresh();
};
// close active page and if flagged as temporary (work in progress!) it will delete the page and all the stuff in it to free memory
// Deleting pages does not fully work right now. It works with some pages but there is some black magic for some other pages
// that crashes the ESP. Maybe some pointer related issues I need to fix
void PhantomUI::close_active(){ 
    auto page = pages.find(active_page);
    if( page == pages.end() ){ 
        return; 
    }
    // backup active tab
    active_tabs[active_page] = page->second->get_active_tab();
    page->second->close(); // flags all the widgets in the page to hidden and fills the screen with bg color
    if( page->second->is_temporary_page() ){ // if temporary fully delete the page calling all destructors deleting all pointers etc.
        pages.erase(page);
    }
}
// get the previous page from the history tree to move back
uint8_t PhantomUI::get_previous_active_page(){
    if( page_tree_index > 0 ){ 
        page_tree_index--;
    } 
    uint8_t page_id = page_tree[page_tree_index];

    while( // history pages to ignore
            page_id == ALPHA_KEYBOARD 
         || page_id == NUMERIC_KEYBOARD 
         || page_id == PAGE_ALARM
         || page_id == PAGE_INFO
    ){
        if( page_tree_index == 0 ){ break; }
        --page_tree_index;
        page_id = page_tree[page_tree_index];
    }

    if( page_id == PAGE_NONE || page_tree_index == 0 ){ 
        page_id = PAGE_FRONT;
    }
    if( page_tree_index > 0 ) { page_tree_index--; }
    return page_id;
}


void PhantomUI::lock_touch( void ){
    touch.block_touch = true;
}

void PhantomUI::unlock_touch( void ){
    touch.block_touch = false;
}


void PhantomUI::show_page_next( uint8_t page_id ){
    next_page_to_load = page_id;
    load_next_page    = true;
    deep_check        = true;
}


void PhantomUI::set_scope_and_logger( uint8_t page_id ){
    // very dirty temporary solution
    // to integrate the scope etc. without rewriting it
    // called after a page was created
    if( page_id == PAGE_FRONT ){
        if( gscope.scope_is_running() ){
            gscope.stop();
        }
        gscope.init( 320-80, 75, 80, 0 );
        gscope.start();
        ui.logger->show();

    } else if( page_id == PAGE_IN_PROCESS ){
        ui.logger->show();
        if( gscope.scope_is_running() ){
            gscope.stop();
        }
        gscope.init( 320, 95, 0, 0 );
        gscope.start();
    } else {
        if( page_id == PAGE_PROBING ){
            ui.logger->show();
        }
        if( gscope.scope_is_running() ){
            gscope.stop();
        }
    }
}

// show a page based on the page id
void PhantomUI::show_page( uint8_t page_id ){ // this should only be called to draw new pages, not to draw the tabs
    if( page_id == PAGE_FRONT ){
        ui.logger->width  = 320-90;
        ui.logger->height = 75-19;
        ui.logger->ypos   = 19;
        ui.logger->xpos   = 85;
    } else if ( page_id == PAGE_SD_CARD || page_id == PAGE_FILE_MANAGER_GCODES || page_id == PAGE_FILE_MANAGER ){
        ui.logger->width  = 215;
        ui.logger->height = 75;
        ui.logger->ypos   = 100;
        ui.logger->xpos   = 95;
    } else if( page_id == PAGE_IN_PROCESS ){
        ui.logger->width  = 220;
        ui.logger->height = 75;
        ui.logger->ypos   = 110;
        ui.logger->xpos   = 90;
    } else if( page_id == PAGE_INFO ){
        ui.logger->width  = 215;
        ui.logger->height = 75;
        ui.logger->ypos   = 100;
        ui.logger->xpos   = 15;
    } else if( page_id == PAGE_PROBING ){
        ui.logger->width  = 180;
        ui.logger->height = 140;
        ui.logger->ypos   = 15;
        ui.logger->xpos   = 15;
    }


    if( page_id < TOTAL_PAGES ){
        lock_touch();
        // build page if needed
        if( pages.find(page_id) == pages.end() ){ // page not found in the map
            if( page_creator_callbacks[page_id] != nullptr ){
                page_creator_callbacks[page_id]();
                auto page = get_page( page_id );
                if( page != nullptr ){
                    page->select_tab( active_tabs[page_id], true );
                } else {
                    ui.emit(LOG_LINE,"Creating page failed!");
                    unlock_touch();
                    return;
                }
            } else {
                //ui.emit(LOG_LINE,"Page creator function not found!");
                unlock_touch();
                return;
            }
        } 
        if( active_page != page_id ){
            close_active();

            if( page_tree_index < page_tree_depth-2 ){
                page_tree_index++;
            }



        } else {
            // same page
        }

        if( page_id == PAGE_FRONT ){
            page_tree_index = 0;
        }
        page_tree[page_tree_index] = page_id;

        /*if( page_tree_index < page_tree_depth-2 ){
            page_tree_index++;
        }
        if( page_id == PAGE_FRONT ){
            page_tree_index = 0;
        }
        page_tree[page_tree_index] = page_id;*/

        active_page = page_id;
        pages[page_id]->show();
        vTaskDelay(50);

        set_scope_and_logger( page_id );

        const char * buffer = int_to_char( active_page );
        emit_param_set( PARAM_ID_SET_PAGE, buffer );
        delete[] buffer;
        buffer = nullptr;
        
        unlock_touch();
    }
};




// create a new page and add it to the pages map
// it will also create the tabs for the page
std::shared_ptr<PhantomPage> PhantomUI::addPage( uint8_t page_id, uint8_t num_tabs = 1 ){
    if( pages.find(page_id) != pages.end() ) {
        ui.emit(LOG_LINE,"Page already exists");
    }
    pages.emplace(page_id, std::make_shared<PhantomPage>( page_id, num_tabs )); //
    return pages.find(page_id)->second;
};
// get a shared pointer to the page by page id
// returns a nullptr if page does not exist
std::shared_ptr<PhantomPage> PhantomUI::get_page( uint8_t page_id ){
    auto page = pages.find(page_id);
    return page == pages.end() ? nullptr : page->second;
};





// callback used to refresh the refreshable objects in the current page
const std::function<void(int *data)> reload_page_if_changed = [](int *data){
    ui.reload_page_on_change();
};
// callback used to fully reload the current page
const std::function<void(int *data)> reload_page_lambda = [](int *data){
    ui.reload_page();
};

const std::function<void(int *data)> restart_calibration_lambda = [](int *data){
    ui.touch_calibrate( true );
};
const std::function<void(int *state)> sd_change = [](int *state){

    if( ui.sd_card_state == *state ){
        // unchanged
        return;
    }

    ui.sd_card_state = *state;

    if( SD_CARD.get_state() == SD_NOT_AVAILABLE ){
        // sd card not available 
        // this state is not a busy state and means there is no sd card or sd card could not be found
        // reset the gcode file?
        ui.set_gcode_file(""); // this would reset the gcode file and inform the backend that no file is selected
                               // it is much easier to implement for now then every other option like checking for the file
                               // before running the job after a sd swap.. Let it be for now..... #todo
    } else {
        //ui.update_firmware();
    }
    ui.reload_page_on_change(); // not very specific... it will reload all changables
                                // need to write a function that only updates the items if the requirement changed
};
const std::function<void(int *data)> restart_lambda = [](int *data){
    gconf.flag_restart = true;
};
const std::function<void(int *data)> reset_lambda = [](int *data){
    SD_CARD.reset();
};

uint8_t PhantomUI::get_active_page(){
    return active_page;
}


bool PhantomUI::get_motion_is_active( void ){
    return motion_is_active;
}
void PhantomUI::set_motion_is_active( bool state ){
    motion_is_active = state;
}
// todo: send motion state from ui_controller, default to not enabled
// and don't poll the state all the time
const std::function<void(PhantomWidgetItem *item)> check_requirements = [](PhantomWidgetItem *item){
    bool disable = false;
    if( item->dpm_required ){
        PhantomSettingsPackage package;
        package.param_id = PARAM_ID_DPM_UART;
        ui_controller.emit(PARAM_GET,&package);
        if( !package.boolValue ){
            disable = true;
        }
    }
    if( item->sd_required ){
        if( SD_CARD.get_state() == SD_NOT_AVAILABLE ){
            //disable = true;
        }
        if( !SD_CARD.is_free() ){
            disable = true;
        }
    }
    if( item->motion_required ){
        PhantomSettingsPackage package;
        package.param_id = PARAM_ID_MOTION_STATE;
        ui_controller.emit(PARAM_GET,&package);
        if( !package.boolValue ){
            disable = true;
        }
    }
    if( disable ) {
        if( !item->disabled ){
            item->changed  = true;
            item->disabled = true;
        }
    } else {
        if( item->disabled ){
            item->changed  = true;
            item->disabled = false;
        }
    }
};



//######################################################################################
// Default helper load function to get the values for given parameter, all load function should
// load the paramater with this function. If there is some custom logic needed it needs
// to be done within the callback
//######################################################################################
void param_load_raw( int param_id ){ // load the values for a parameter and stores it into the cache. Nothing else
    PhantomSettingsPackage package;
    package.intValue   = 0;
    package.floatValue = 0.0;
    package.boolValue  = false;
    package.param_id   = param_id;
    ui_controller.emit( PARAM_GET, &package );
    settings_values_cache.erase( param_id );
    settings_values_cache.emplace( param_id, setting_cache_object( package.intValue, package.floatValue, package.boolValue ) );
}

IRAM_ATTR void param_load( PhantomWidgetItem *item, std::function<void(PhantomWidgetItem *item, PhantomSettingsPackage *package)> callback = nullptr ){
    // this is a temporary solution until I figure out how to do it better
    // finally it should be done different by requesting a param
    // and get a char array in return. For some reason this does not work that easy
    // everything I tried produced unstable results that I wasn't able to debug yet
    // getting the char array was ok but converting the data to numbers or bools
    // sometimes worked and sometimes not. Could be some pointer issues. No idea.
    if( item == nullptr ){
        // no item used. 
        return;
    }

    PhantomSettingsPackage package;
    package.intValue   = item->intValue;
    package.floatValue = item->floatValue;
    package.boolValue  = item->boolValue;
    package.param_id   = item->param_id;
    ui_controller.emit( PARAM_GET, &package );
    item->intValue   = package.intValue;
    item->floatValue = package.floatValue;
    item->boolValue  = package.boolValue;

    settings_values_cache.erase( package.param_id );
    settings_values_cache.emplace( package.param_id, setting_cache_object( package.intValue, package.floatValue, package.boolValue ) );

    if( callback != nullptr ){
        callback( item, &package );
    }

}
//######################################################################################
// Default load function
//######################################################################################
const std::function<void(PhantomWidgetItem *item)> load = [](PhantomWidgetItem *item){
    param_load( item );
    if( item->suffix != 0 ){
        std::ostringstream sstream; // slow and maybe not good, does only support smaller numbers
        if( item->valueType == PARAM_TYPE_FLOAT ){
            //sstream << item->floatValue;
            sstream << std::setprecision(4) << item->floatValue;
        } else if( item->valueType == PARAM_TYPE_INT ){
            sstream << item->intValue;
        }
        item->strValue = sstream.str();
        item->strValue.append( suffix_table[item->suffix] );
    }
    vTaskDelay(1);
};
const std::function<void(PhantomWidgetItem *item)> load_mode_custom = [](PhantomWidgetItem *item) {
    int backup_int_value = item->intValue;
    item->intValue = PARAM_ID_MODE; // very dirty hack here... The intValue of the item is not the paramid in this case
                                    // backup the int value and restore it after load
    param_load( item, [backup_int_value](PhantomWidgetItem *item,PhantomSettingsPackage *package){
        bool is_active = package->intValue == backup_int_value ? true : false;
        item->intValue = backup_int_value;
        if( is_active ){
        //if( package->boolValue ){
            item->boolValue = true;
            item->is_active = true;
            item->touch_enabled = false;
        } else {
            item->boolValue = false;
            item->is_active = false;
            item->touch_enabled = true;
        }
    });
};

const std::function<void(PhantomWidgetItem *item)> load_pwm_enabled = [](PhantomWidgetItem *item){
    param_load( item, [](PhantomWidgetItem *item,PhantomSettingsPackage *package){
        bool changed = package->boolValue == item->boolValue ? true : false;
        if( package->boolValue ){
            item->boolValue = true;
            item->is_active = true;
            item->set_string_value( BTN_STOP );
        } else {
            item->boolValue = false;
            item->is_active = false;
            item->set_string_value( BTN_START );
        }

        if( !changed ){
            return;
        }

        if( gscope.scope_is_running() ){
            gscope.stop();
        }
        gscope.init( 320-80, 75, 80, 0 );
        gscope.start();

    });
};


const std::function<void(PhantomWidgetItem *item)> get_homed_state = [](PhantomWidgetItem *item) {
    param_load( item, [](PhantomWidgetItem *item,PhantomSettingsPackage *package){
        if( !item->boolValue ){
            item->is_active = false;
        } else {
            item->is_active = true;
        }
    });
};
const std::function<void(PhantomWidgetItem *item)> get_homing_enabled = [](PhantomWidgetItem *item) {
    param_load( item, [](PhantomWidgetItem *item,PhantomSettingsPackage *package){
        if( item->boolValue ){
            item->is_active = true;
            item->strValue  = "Enabled";
        } else {
            item->is_active = false;
            item->strValue  = "Disabled";
        }
    });
};
const std::function<void(PhantomWidgetItem *item)> get_homing_enabled_2 = [](PhantomWidgetItem *item) {
    param_load( item, [](PhantomWidgetItem *item,PhantomSettingsPackage *package){
        if( item->boolValue ){
            item->disabled = false;
        } else {
            item->disabled = true;
        }
    });
};

const std::function<void(PhantomWidgetItem *item)> load_toogle_active_state = [](PhantomWidgetItem *item) {
    param_load( item, [](PhantomWidgetItem *item,PhantomSettingsPackage *package){
        if( item->boolValue ){
            item->is_active = true;
            item->set_string_value( "Resume" );
            //item->strValue  = "Enabled";
        } else {
            item->is_active = false;
            item->set_string_value( "Pause" );
            //item->strValue  = "Disabled";
        }
    });
};



// Helper function
void PhantomUI::emit_param_set(int param_id, const char* value) {
    //ui.emit(LOG_LINE,value);
    std::pair<int, const char*> key_value_pair = std::make_pair(param_id, value);
    ui_controller.emit(PARAM_SET, &key_value_pair);
};

const std::function<void(PhantomWidgetItem *item)> save_settings_lambda = [](PhantomWidgetItem *item) {
    int action_id = item->intValue;
    ui.save_settings( static_cast<sd_write_action_type>(action_id) );
};

//########################################################################################
// Open the alphanumeric keybard to change float and int parameters
//########################################################################################
bool build_alpha_keyboard( std::string str = "" ) {
    auto page = ui.addPage( ALPHA_KEYBOARD, 1 ); // page for numeric keyboard
    std::shared_ptr<PhantomWidget> alpha_keyboard = factory_akeyboard( 320, 240, 0, 0, 2, false );
    auto __this = std::dynamic_pointer_cast<PhantomKeyboardAlpha>( alpha_keyboard );
    __this->set_str_value( str.c_str() );
    __this->create();
    page->make_temporary();
    page->add_widget( alpha_keyboard );
    ui.show_page( ALPHA_KEYBOARD );
    page = nullptr;
    alpha_keyboard = nullptr;
    __this = nullptr;
    return true;
};


//########################################################################################
// Open the numeric keybard to change float and int parameters
//########################################################################################
void build_keyboard( PhantomSettingsPackage * package, bool floatType, int param_type ) {
    package->type = floatType ? 1 : 0;
    auto page = ui.addPage( NUMERIC_KEYBOARD, 1 ); // page for numeric keyboard
    std::shared_ptr<PhantomWidget> num_keyboard = factory_nkeyboard( 320, 240, 0, 0, 2, false );
    auto __this = std::dynamic_pointer_cast<PhantomKeyboardNumeric>( num_keyboard );
    num_keyboard->create();
    page->make_temporary();
    num_keyboard->reset_widget( package, param_type );
    __this->set_tooltip( tooltips[package->param_id] );
    __this->set_callback([](PhantomSettingsPackage _package){
        const char* buffer = _package.type == 1 ? float_to_char( _package.floatValue ) : int_to_char( _package.intValue );
        ui.emit_param_set( _package.param_id, buffer );
        delete[] buffer;
        buffer             = nullptr;
        settings_values_cache.erase( _package.param_id );
        ui.save_param_next = true;
        ui.save_param_id   = _package.param_id;
    });
    page->add_widget( num_keyboard );
    ui.show_page( NUMERIC_KEYBOARD );
    page = nullptr;
    num_keyboard = nullptr;
};


//########################################################################################
// Main gateway to change setting. Creates a key, value pair and emits it to the backend
//########################################################################################
const std::function<void(PhantomWidgetItem *item)> change_setting = [](PhantomWidgetItem *item){
    item->has_touch = true;

    uint8_t page_id   = item->get_parent_page_id();
    uint8_t tab_id    = item->get_parent_page_tab_id();
    uint8_t widget_id = item->get_widget_id();
    uint8_t item_id   = item->get_index();
    uint8_t param_id  = item->param_id;

    PhantomSettingsPackage package;
    package.intValue   = item->intValue;
    package.floatValue = item->floatValue;
    package.boolValue  = item->boolValue;
    package.param_id   = item->param_id;
    uint8_t param_type = item->valueType;

    std::shared_ptr<PhantomWidget> widget = ui.get_widget(page_id, tab_id, widget_id, item_id);

    switch (item->valueType) {

        case PARAM_TYPE_BOOL:
        case PARAM_TYPE_BOOL_RELOAD_WIDGET:
        case PARAM_TYPE_BOOL_RELOAD_WIDGET_OC:
            item->boolValue = !item->boolValue;
            ui.emit_param_set( item->param_id, item->boolValue?"true":"false" );
            item->has_touch = false;
            if( item->valueType == PARAM_TYPE_BOOL_RELOAD_WIDGET ){
                if( widget->is_refreshable ){
                    item->changed = true;
                    widget->show_on_change();
                } else { 
                    widget->show(); 
                }
            } else if( item->valueType == PARAM_TYPE_BOOL_RELOAD_WIDGET_OC ){
                item->changed = true;
                widget->show_on_change();
            } else {
                widget->redraw_item( item->item_index, true );
            }
            ui.save_current_to_nvs( item->param_id );
            break;

        case PARAM_TYPE_INT:
        case PARAM_TYPE_INT_RAW_RELOAD_WIDGET:
        case PARAM_TYPE_INT_INCR:
            if( item->valueType == PARAM_TYPE_INT_INCR ){ // incremental value (currently without ranges...)

            } else if( item->valueType == PARAM_TYPE_INT_RAW_RELOAD_WIDGET ){ // no keyboard input
                const char* buffer = int_to_char( item->intValue );
                ui.emit_param_set( item->param_id, buffer );
                delete[] buffer;
                buffer = nullptr;
                item->has_touch = false;
                widget->show();
                vTaskDelay(1);
                ui.save_current_to_nvs( item->param_id );
            } else { // keyboard input
                build_keyboard( &package, false, param_type );
            }
        break;

        case PARAM_TYPE_FLOAT:
            build_keyboard( &package, true, param_type );    
            break;

        case PARAM_TYPE_NONE:
        case PARAM_TYPE_NONE_REDRAW_WIDGET:
        case PARAM_TYPE_NONE_REDRAW_PAGE:
            ui.emit_param_set( item->param_id, "" );
            item->has_touch = false;

            if (item->valueType == PARAM_TYPE_NONE_REDRAW_PAGE) {
                widget = nullptr;
                ui.reload_page();
            } else {
                if (item->valueType == PARAM_TYPE_NONE_REDRAW_WIDGET) {
                    widget->show();
                } else {
                    widget->redraw_item(item->item_index, true);
                }
            }
            break;
    }
    widget = nullptr;
    vTaskDelay(1);
    return;
};
const std::function<void(PhantomWidgetItem *item)> navigation_callback = [](PhantomWidgetItem *item) {
    // open keyboard for float value with no default value
    // submit action command with axis or ?
    item->has_touch = false;
    std::string tooltip;
    int  axis   = X_AXIS;
    switch( item->param_id ){
        case PARAM_ID_XLEFT:
            axis    = X_AXIS;
        break;
        case PARAM_ID_XRIGHT:
            axis    = X_AXIS;
        break;
        case PARAM_ID_YUP:
            axis    = Y_AXIS;
        break;
        case PARAM_ID_YDOWN:
            axis    = Y_AXIS;
        break;
        case PARAM_ID_ZUP:
            axis    = Z_AXIS;
        break;
        case PARAM_ID_ZDOWN:
            axis    = Z_AXIS;
        break;
        default: // wire spindle
            axis = -1;
        break;
    }
    item->floatValue = 0.0; // reset
    item->intValue   = axis;

    PhantomSettingsPackage package;
    package.axis       = item->intValue;
    package.intValue   = item->intValue;
    package.floatValue = item->floatValue;
    package.boolValue  = item->boolValue;
    package.param_id   = item->param_id;
    package.type       = 1;// float input
    uint8_t param_type = item->valueType;


    build_keyboard( &package, true, param_type );

};










// callback used to open a page
const std::function<void(PhantomWidgetItem *item)> open_page = [](PhantomWidgetItem *item) {
    //emit_param_set();
    ui.show_page( item->intValue );
};
const std::function<void(PhantomWidgetItem *item)> open_page_pause_process = [](PhantomWidgetItem *item) {
    //ui.restart_display();
    ui.emit_param_set( PARAM_ID_EDM_PAUSE, "true" );
    ui.show_page( item->intValue );
};













const std::function<void(int *data)> monitor_touch_lambda = [](int *data){
    if( gconf.flag_restart ){
        return;
    }

    if( sd_line_shift ){
        if( !SD_CARD.shift_gcode_line() ){
            sd_line_shift = false;
            ui.emit(LOG_LINE,"GCode file end");
        }
    }

    gscope.draw_scope();

    if( ui.load_settings_file_next ){
        ui.load_settings( ui.settings_file );
    }

    if( ui.deep_check ){

        ui.deep_check = false; // prevent all the if checks if not needed using the deep check flag

        vTaskDelay(10);

        if( ui.load_settings_file_next ){ // needs to be before the page load
            ui.load_settings_file_next = false;
            ui.load_settings( ui.settings_file );
            vTaskDelay(1);
        }

        if( ui.load_next_page ){
            ui.load_next_page = false;
            ui.show_page( ui.next_page_to_load );
            vTaskDelay(1);
        }

        if( ui.save_param_next ){ // this needs to be after the page load!
            ui.save_param_next = false;
            ui.save_current_to_nvs( ui.save_param_id );
            vTaskDelay(1);
        }

        ui.deep_check = false;

        return;
    
    }

    ui.monitor_touch();
    if( SD_CARD.refresh() ){ 
        ui.reload_page_on_change(); 
    }
    //ui.reload_page_on_change();
};


const std::function<void(PhantomWidgetItem *item)> mode_load = [](PhantomWidgetItem *item) {
    // load values here!!!!
    switch (item->intValue){
        case 1:
        item->strValue = DICT_MODE1;
        break;
        case 2:
        item->strValue = DICT_MODE2;
        break;
        case 3:
        item->strValue = DICT_MODE3;
        break;
        case 4:
        item->strValue = DICT_MODE4;
        break;
        default:
        item->strValue = DICT_UNDEF;
        break;
    }
    vTaskDelay(1);
};

static const std::function<void(int *data)> alarm_lamda = [](int *data){
    PhantomAlarm * alarm_handler = new PhantomAlarm( 320, 240, 0, 0, 2, false );
    int erro_code = *data;
    alarm_handler->set_error_code( erro_code );
    alarm_handler->show();
    delete alarm_handler;
    alarm_handler = nullptr;
};

const std::function<void(PhantomWidgetItem *item)> sinker_dir_lambda = [](PhantomWidgetItem *item) {
    // load values here!!!!
    switch(item->intValue){
        case 0:
        item->strValue = "Z down";
        break;
        case 1:
        item->strValue = "X right";
        break;
        case 2:
        item->strValue = "X left";
        break;
        case 3:
        item->strValue = "Y forward";
        break;
        case 4:
        item->strValue = "Y back";
        break;
        default:
        item->strValue = "Not found";
        break;
    }
    vTaskDelay(1);
};




const std::function<void(PhantomWidgetItem *item)> num_to_string_lambda = [](PhantomWidgetItem *item) {
    std::ostringstream sstream; // slow and maybe not good, does only support smaller numbers
    if( item->valueType == PARAM_TYPE_FLOAT ){
        sstream << item->floatValue;
    } else if( item->valueType == PARAM_TYPE_INT ){
        sstream << item->intValue;
    }
    item->strValue = sstream.str();
    if( item->suffix > 0 ){
        item->strValue += " ";
        item->strValue += item->suffix;
    }
};

const std::function<void(PhantomWidgetItem *item)> set_probe_pos_string = [](PhantomWidgetItem *item) {
    if( !item->boolValue ){
          item->strValue = DICT_NOTPROBE;
    } else {
        std::ostringstream sstream; // slow and maybe not good, does only support smaller numbers
        if( item->valueType == PARAM_TYPE_FLOAT ){
            sstream << item->floatValue;
        } else if( item->valueType == PARAM_TYPE_INT ){
            sstream << item->intValue;
        }
        item->strValue = sstream.str();
        //item->strValue.append( suffix_table[item->suffix] );
    }
    vTaskDelay(1);
};



void create_settings_page(){

    int current_tab = 0; // settings pages
    auto page       = ui.addPage( PAGE_MAIN_SETTINGS, 12 );
    // basic settings
    std::shared_ptr<PhantomWidget> settings_1_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_1_ptr );
    // title
    settings_1_ptr->addItem(OUTPUT_TYPE_TITLE,DICT_BASIC_PAGE_TITLE,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
           ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    // items
    settings_1_ptr->addItem(OUTPUT_TYPE_STRING,DICT_FREQUENCY,PARAM_TYPE_FLOAT,PARAM_ID_FREQ,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(1) //1=kHz
           ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_1_ptr->addItem(OUTPUT_TYPE_STRING,DICT_DUTY,PARAM_TYPE_FLOAT,PARAM_ID_DUTY,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(2) //2=%
           ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_1_ptr->addItem(OUTPUT_TYPE_STRING,DICT_SETPOINT_MIN,PARAM_TYPE_FLOAT,PARAM_ID_SETMIN,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(2) //2=%
           ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_1_ptr->addItem(OUTPUT_TYPE_STRING,DICT_SETPOINT_MAX,PARAM_TYPE_FLOAT,PARAM_ID_SETMAX,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(2) //2=%
           ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_1_ptr->addItem(OUTPUT_TYPE_STRING,DICT_VDROP_THRESH,PARAM_TYPE_INT,PARAM_ID_VDROP_THRESH,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(8) //8=ADC
           ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_1_ptr->addItem(OUTPUT_TYPE_STRING,DICT_MAX_FEED,PARAM_TYPE_FLOAT,PARAM_ID_MAX_FEED,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(4) //4=mm/min
           ->on(LOAD,load).on(TOUCHED,change_setting);





    // retraction settings
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_6_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_6_ptr );
    settings_6_ptr->addItem(OUTPUT_TYPE_TITLE,DICT_PAGE_R,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
            ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    settings_6_ptr->addItem(OUTPUT_TYPE_BOOL,DICT_EARLY_RETR_X,PARAM_TYPE_BOOL,PARAM_ID_EARLY_RETR,0,0,0,0,true,DEFAULT_FONT)
            ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(TOUCHED,change_setting).on(LOAD,load);

    settings_6_ptr->addItem(OUTPUT_TYPE_INT,DICT_EARLY_X_ON,PARAM_TYPE_INT,PARAM_ID_EARLY_X_ON,0,0,0,0,true,DEFAULT_FONT)
            ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);

    settings_6_ptr->addItem(OUTPUT_TYPE_INT,DICT_RETR_CONFIRM,PARAM_TYPE_INT,PARAM_ID_RETRACT_CONF,0,0,0,0,true,DEFAULT_FONT)
            ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_6_ptr->addItem(OUTPUT_TYPE_STRING,DICT_MAX_REVERSE,PARAM_TYPE_INT,PARAM_ID_MAX_REVERSE,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(7) //7=Lines
           ->on(LOAD,load).on(TOUCHED,change_setting);


    // protection settings
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_8_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_8_ptr );
    settings_8_ptr->addItem(OUTPUT_TYPE_TITLE,DICT_PAGE_PROT,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
           ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    settings_8_ptr->addItem(OUTPUT_TYPE_STRING,DICT_SHORT_DURATION,PARAM_TYPE_INT,PARAM_ID_SHORT_DURATION,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(13) //13=ms
           ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_8_ptr->addItem(OUTPUT_TYPE_STRING,DICT_BROKEN_WIRE_MM,PARAM_TYPE_FLOAT,PARAM_ID_BROKEN_WIRE_MM,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(3) //3=mm
           ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_8_ptr->addItem(OUTPUT_TYPE_INT,DICT_VDROPS_BLOCK,PARAM_TYPE_INT,PARAM_ID_VFD_DROPS_BLOCK,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_8_ptr->addItem(OUTPUT_TYPE_INT,DICT_VDROPS_POFF,PARAM_TYPE_INT,PARAM_ID_VFD_SHORTS_POFF,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_8_ptr->addItem(OUTPUT_TYPE_INT,DICT_POFF_DURATION,PARAM_TYPE_INT,PARAM_ID_POFF_DURATION,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);

    // forward blocker
    ++current_tab;
    page->select_tab( current_tab );
    std::shared_ptr<PhantomWidget> settings_9_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_9_ptr );
    settings_9_ptr->addItem(OUTPUT_TYPE_TITLE,DICT_PAGE_FBLOCK,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
           ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    settings_9_ptr->addItem(OUTPUT_TYPE_BOOL,DICT_RESET_QUEUE,PARAM_TYPE_BOOL,PARAM_ID_RESET_QUEUE,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(TOUCHED,change_setting).on(LOAD,load);
    settings_9_ptr->addItem(OUTPUT_TYPE_INT,DICT_BEST_OF,PARAM_TYPE_INT,PARAM_ID_BEST_OF,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_9_ptr->addItem(OUTPUT_TYPE_STRING,DICT_ETHRESH,PARAM_TYPE_INT,PARAM_ID_EDGE_THRESH,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(8) //8=ADC
           ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_9_ptr->addItem(OUTPUT_TYPE_INT,DICT_LINE_ENDS,PARAM_TYPE_INT,PARAM_ID_LINE_ENDS,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_9_ptr->addItem(OUTPUT_TYPE_INT,DICT_POFF_AFTER,PARAM_TYPE_INT,PARAM_ID_POFF_AFTER,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);

    // stop go signales
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_10_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_10_ptr );
    settings_10_ptr->addItem(OUTPUT_TYPE_TITLE,DICT_STOPGO_PAGE_TITLE,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
           ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    settings_10_ptr->addItem(OUTPUT_TYPE_BOOL,DICT_USE_STOPGO,PARAM_TYPE_BOOL,PARAM_ID_USE_STOPGO,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(TOUCHED,change_setting).on(LOAD,load);
    settings_10_ptr->addItem(OUTPUT_TYPE_INT,DICT_ZERO_JITTER,PARAM_TYPE_INT,PARAM_ID_ZERO_JITTER,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_10_ptr->addItem(OUTPUT_TYPE_INT,DICT_READINGS_H,PARAM_TYPE_INT,PARAM_ID_READINGS_H,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)
           ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_10_ptr->addItem(OUTPUT_TYPE_INT,DICT_EADINGS_L,PARAM_TYPE_INT,PARAM_ID_READINGS_L,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_10_ptr->addItem(OUTPUT_TYPE_STRING,DICT_ZERO_THRESH,PARAM_TYPE_INT,PARAM_ID_ZERO_THRESH,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(8) //8=ADC
           ->on(LOAD,load).on(TOUCHED,change_setting);


    // Feedback settings
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_3_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_3_ptr );
    settings_3_ptr->addItem(OUTPUT_TYPE_TITLE,DICT_PAGE_FEEDBACK,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
           ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    settings_3_ptr->addItem(OUTPUT_TYPE_BOOL,DICT_DRAW_LINEAR,PARAM_TYPE_BOOL,PARAM_ID_DRAW_LINEAR,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(TOUCHED,change_setting).on(LOAD,load);
    settings_3_ptr->addItem(OUTPUT_TYPE_INT,DICT_MAIN_AVG,PARAM_TYPE_INT,PARAM_ID_MAIN_AVG,0,0,0,0,true,DEFAULT_FONT)
            ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_3_ptr->addItem(OUTPUT_TYPE_INT,DICT_RISE_BOOST,PARAM_TYPE_INT,PARAM_ID_RISE_BOOST,0,0,0,0,true,DEFAULT_FONT)
            ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_3_ptr->addItem(OUTPUT_TYPE_INT,DICT_FAVG_SIZE,PARAM_TYPE_INT,PARAM_ID_FAVG_SIZE,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);

    // Spindle settings
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_4_ptr = factory_split_list( 320, 150, 0, 0, 2, false );
    page->add_widget( settings_4_ptr );
    settings_4_ptr->addItem(OUTPUT_TYPE_TITLE,DICT_SPINDLE_PAGE_TITLE,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
           ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    settings_4_ptr->addItem(OUTPUT_TYPE_BOOL,DICT_SPINDLE_ONOFF,PARAM_TYPE_BOOL,PARAM_ID_SPINDLE_ONOFF,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_4_ptr->addItem(OUTPUT_TYPE_BOOL,DICT_SPINDLE_ENABLE,PARAM_TYPE_BOOL,PARAM_ID_SPINDLE_ENABLE,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_4_ptr->addItem(OUTPUT_TYPE_STRING,DICT_SPINDLE_SPEED_B,PARAM_TYPE_FLOAT,PARAM_ID_SPINDLE_SPEED,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(4) //4=mm/min//5=RPM
           ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_4_ptr->addItem(OUTPUT_TYPE_STRING,DICT_SPINDLE_SPM,PARAM_TYPE_INT,PARAM_ID_SPINDLE_STEPS_PER_MM,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(14) //4=mm/min
           ->on(LOAD,load).on(TOUCHED,change_setting);



    std::shared_ptr<PhantomWidget> spindle_move_btns = factory_button( 30, 60, 20, 150, 2, true );
    int w = 30, h = 30;
    spindle_move_btns->disable_background_color();
    spindle_move_btns->addItem(OUTPUT_TYPE_NONE,NAV_UP, PARAM_TYPE_FLOAT,PARAM_ID_SPINDLE_MOVE_UP,w,h,0,0,true,-1)
                           ->set_colors( 1, -1, TFT_GREEN )
                           ->set_colors( 3, -1, BUTTON_LIST_BG_DEACTIVATED )
                           ->set_require_motion()
                           ->set_is_refreshable()
                           ->on(TOUCHED,navigation_callback).on(CHECK_REQUIRES,check_requirements);
    spindle_move_btns->addItem(OUTPUT_TYPE_NONE,NAV_DOWN, PARAM_TYPE_FLOAT,PARAM_ID_SPINDLE_MOVE_DOWN,w,h,0,30,true,-1)
                           ->set_colors( 1, -1, TFT_GREEN )
                           ->set_colors( 3, -1, BUTTON_LIST_BG_DEACTIVATED )
                           ->set_require_motion()
                           ->set_is_refreshable()
                           ->on(TOUCHED,navigation_callback).on(CHECK_REQUIRES,check_requirements);
    //spindle_move_btns->set_is_refreshable();
    page->add_widget( spindle_move_btns ); // add widget to active tab; default active tab is the first one









    // Flushing settings
    ++current_tab;
    page->select_tab( current_tab );
    std::shared_ptr<PhantomWidget> settings_f_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_f_ptr );
    settings_f_ptr->addItem(OUTPUT_TYPE_TITLE,DICT_PAGE_FLUSHING,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
                  ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    settings_f_ptr->addItem(OUTPUT_TYPE_BOOL,DICT_FLUSH_NOSPARK,PARAM_TYPE_BOOL,PARAM_ID_FLUSHING_FLUSH_NOSPRK,0,0,0,0,true,DEFAULT_FONT)
                  ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_f_ptr->addItem(OUTPUT_TYPE_STRING,DICT_FLUSH_DIST,PARAM_TYPE_FLOAT,PARAM_ID_FLUSHING_DISTANCE,0,0,0,0,true,DEFAULT_FONT)
                  ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(3)->on(LOAD,load).on(TOUCHED,change_setting); 
    settings_f_ptr->addItem(OUTPUT_TYPE_STRING,DICT_FLUSH_INTV,PARAM_TYPE_FLOAT,PARAM_ID_FLUSHING_INTERVAL,0,0,0,0,true,DEFAULT_FONT)
                  ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(12)->on(LOAD,load).on(TOUCHED,change_setting); 
    settings_f_ptr->addItem(OUTPUT_TYPE_INT,DICT_FLUSH_OFFSTP,PARAM_TYPE_INT,PARAM_ID_FLUSHING_FLUSH_OFFSTP,0,0,0,0,true,DEFAULT_FONT)
                  ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting); 


    // I2S settings
    ++current_tab;
    page->select_tab( current_tab );
    std::shared_ptr<PhantomWidget> settings_5_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_5_ptr );
    settings_5_ptr->addItem(OUTPUT_TYPE_TITLE,DICT_I2S_PAGE_TITLE,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
             ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    settings_5_ptr->addItem(OUTPUT_TYPE_INT,DICT_I2S_SAMPLES,PARAM_TYPE_INT,PARAM_ID_I2S_SAMPLES,0,0,0,0,true,DEFAULT_FONT)
             ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_5_ptr->addItem(OUTPUT_TYPE_STRING,DICT_I2S_RATE,PARAM_TYPE_FLOAT,PARAM_ID_I2S_RATE,0,0,0,0,true,DEFAULT_FONT)
             ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(6) //6=kSps
             ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_5_ptr->addItem(OUTPUT_TYPE_INT,DICT_I2S_BUFFER_L,PARAM_TYPE_INT,PARAM_ID_I2S_BUFFER_L,0,0,0,0,true,DEFAULT_FONT)
             ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_5_ptr->addItem(OUTPUT_TYPE_INT,DICT_I2S_BUFFER_C,PARAM_TYPE_INT,PARAM_ID_I2S_BUFFER_C,0,0,0,0,true,DEFAULT_FONT)
             ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_5_ptr->addItem(OUTPUT_TYPE_BOOL,DICT_ENABLE_HWTIMER,PARAM_TYPE_BOOL,PARAM_ID_USEHWTIMTER,0,0,0,0,true,DEFAULT_FONT)
             ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(TOUCHED,change_setting).on(LOAD,load);

    // retraction speeds and distances
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_7_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_7_ptr );
    settings_7_ptr->addItem(OUTPUT_TYPE_TITLE,DICT_PAGE_RS,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
            ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    settings_7_ptr->addItem(OUTPUT_TYPE_STRING,DICT_RETR_HARD_MM,PARAM_TYPE_FLOAT,PARAM_ID_RETRACT_H_MM,0,0,0,0,true,DEFAULT_FONT)
            ->set_float_decimals(3)->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(3) //3=mm
            ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_7_ptr->addItem(OUTPUT_TYPE_STRING,DICT_RETR_HARD_F,PARAM_TYPE_FLOAT,PARAM_ID_RETRACT_H_F,0,0,0,0,true,DEFAULT_FONT)
            ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(4) //4=mm/min
            ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_7_ptr->addItem(OUTPUT_TYPE_STRING,DICT_RETR_SOFT_MM,PARAM_TYPE_FLOAT,PARAM_ID_RETRACT_S_MM,0,0,0,0,true,DEFAULT_FONT)
            ->set_float_decimals(3)->set_suffix(3) //3=mm
            ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_7_ptr->addItem(OUTPUT_TYPE_STRING,DICT_RETR_SOFT_F,PARAM_TYPE_FLOAT,PARAM_ID_RETRACT_S_F,0,0,0,0,true,DEFAULT_FONT)
            ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(4) //4=mm/min
            ->on(LOAD,load).on(TOUCHED,change_setting);

    // DPM main
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_11_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_11_ptr );
    settings_11_ptr->addItem(OUTPUT_TYPE_TITLE,DICT_DPM_PAGE_TITLE,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
           ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);

    settings_11_ptr->addItem(OUTPUT_TYPE_BOOL,DICT_DPM_UART,PARAM_TYPE_BOOL_RELOAD_WIDGET_OC,PARAM_ID_DPM_UART,0,0,0,0,true,DEFAULT_FONT)
           ->set_is_refreshable()
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(TOUCHED,change_setting).on(LOAD,load);
    settings_11_ptr->addItem(OUTPUT_TYPE_BOOL,DICT_DPM_ONOFF,PARAM_TYPE_BOOL,PARAM_ID_DPM_ONOFF,0,0,0,0,true,DEFAULT_FONT)
           ->set_is_refreshable()
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_require_dpm()
           ->on(TOUCHED,change_setting).on(LOAD,load).on(CHECK_REQUIRES,check_requirements);
    settings_11_ptr->addItem(OUTPUT_TYPE_STRING,DICT_DPM_VOLTAGE,PARAM_TYPE_FLOAT,PARAM_ID_DPM_VOLTAGE,0,0,0,0,true,DEFAULT_FONT)
           ->set_is_refreshable()
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_require_dpm()->set_suffix(10) //10=V
           ->on(LOAD,load).on(TOUCHED,change_setting).on(CHECK_REQUIRES,check_requirements);
    settings_11_ptr->addItem(OUTPUT_TYPE_STRING,DICT_DPM_CURRENT,PARAM_TYPE_FLOAT,PARAM_ID_DPM_CURRENT,0,0,0,0,true,DEFAULT_FONT)
           ->set_is_refreshable()
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_require_dpm()->set_suffix(11) //11=A
           ->on(LOAD,load).on(TOUCHED,change_setting).on(CHECK_REQUIRES,check_requirements);

    settings_11_ptr->addItem(OUTPUT_TYPE_STRING,DICT_DPM_PROBE_C,PARAM_TYPE_FLOAT,PARAM_ID_DPM_PROBE_C,0,0,0,0,true,DEFAULT_FONT)
           ->set_is_refreshable()
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_require_dpm()->set_suffix(11) //11=A
           ->on(LOAD,load).on(TOUCHED,change_setting).on(CHECK_REQUIRES,check_requirements);
    settings_11_ptr->addItem(OUTPUT_TYPE_STRING,DICT_DPM_PROBE_V,PARAM_TYPE_FLOAT,PARAM_ID_DPM_PROBE_V,160,0,0,0,true,DEFAULT_FONT)
           ->set_is_refreshable()
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_require_dpm()->set_suffix(10) //10=V
           ->on(LOAD,load).on(TOUCHED,change_setting).on(CHECK_REQUIRES,check_requirements);

    //settings_11_ptr->set_is_refreshable(); // if the widget itself is set to refreshable it will check for refreshable items with each refresh event
                                             // but in this special case the widget should not selfrefresh but the items should if we manually trigger a refresh
                                             // therefore the items are tagged refreshable but the widget not


    // Probing settings
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_13_ptr = factory_split_list( 320, 205, 0, 0, 2, false );
    page->add_widget( settings_13_ptr );

    settings_13_ptr->addItem(OUTPUT_TYPE_TITLE,DICT_PAGE_PROBING,PARAM_TYPE_NONE,160,0,0,0,0,false,DEFAULT_FONT)
           ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    settings_13_ptr->addItem(OUTPUT_TYPE_STRING,DICT_PROBE_FREQ,PARAM_TYPE_FLOAT,PARAM_ID_PROBE_FREQ,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(1) //1=kHz
           ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_13_ptr->addItem(OUTPUT_TYPE_STRING,DICT_PROBE_DUTY,PARAM_TYPE_FLOAT,PARAM_ID_PROBE_DUTY,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(2) //2=%
           ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_13_ptr->addItem(OUTPUT_TYPE_STRING,DICT_PROBE_TR_V,PARAM_TYPE_INT,PARAM_ID_PROBE_TR_V,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(8) //8=ADC
           ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_13_ptr->addItem(OUTPUT_TYPE_STRING,DICT_PROBE_TR_C,PARAM_TYPE_FLOAT,PARAM_ID_PROBE_TR_C,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(2) //2=%
           ->on(LOAD,load).on(TOUCHED,change_setting);
    settings_13_ptr->addItem(OUTPUT_TYPE_STRING,DICT_SPINDLE_SPEED,PARAM_TYPE_FLOAT,PARAM_ID_SPINDLE_SPEED_PROBING,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(4) //4=mm/min;5=RPM
           ->on(LOAD,load).on(TOUCHED,change_setting);



    page->add_nav( 1 );  // add navigation button for tab selection
    page->select_tab( ui.get_active_tab_for_page( PAGE_MAIN_SETTINGS ), true ); // select first tab of this page again 
    page->make_temporary();

    page = nullptr;
    
}




void create_motion_page(){

    auto page = ui.addPage( PAGE_MOTION, 4 ); 
    page->set_bg_color( COLOR555555 );
    //page->set_bg_color( TFT_DARKGREY );
    int current_tab = 0; // reset
    int w = 40; 
    int h = 40;
    int16_t grid_w = 3, grid_h = 3, cc_y = 0, btw_h = w;
    int16_t r_w = 320-(w*grid_w+grid_w);
    // create widgets

    for( int i = 0; i < N_AXIS; ++i ){

        std::shared_ptr<PhantomWidget> container = factory_button( r_w, btw_h, w*grid_w+grid_w+10, 45+cc_y, 2, true );
       int16_t hcw  = round( float( r_w ) / 2.0 );
       int16_t hhcw = round( float( hcw ) / 2.0 );

       container->addItem(OUTPUT_TYPE_NONE,get_axis_name_const(i),PARAM_TYPE_NONE,PARAM_ID_PROBE_ACTION,88,btw_h/2,0,0,false,DEFAULT_FONT)
                ->set_colors(0, BUTTON_LIST_BG, TFT_WHITE )
                ->set_colors(1, BUTTON_LIST_BG, TFT_WHITE )
                ->set_colors(2, BUTTON_LIST_BG, TFT_WHITE );

       container->addItem(OUTPUT_TYPE_STRING,get_axis_name_const(i),PARAM_TYPE_FLOAT,PARAM_ID_GET_PROBE_POS,88,btw_h/2,0,w/2,false,DEFAULT_FONT)
                ->set_int_value( i ) // show current probe pos
                ->set_colors(0, BUTTON_LIST_BG, TFT_WHITE )
                ->set_colors(1, BUTTON_LIST_BG, TFT_WHITE )
                ->set_colors(2, BUTTON_LIST_BG, TFT_WHITE )
                ->on(LOAD,load).on(BUILDSTRING,set_probe_pos_string);

       container->addItem(OUTPUT_TYPE_NONE,"L",PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_UNSET_PROBE_POS,38,btw_h,89,0,true,-1)
                ->set_int_value( i ) // set axis probe pos to zero
                ->set_colors(0, TFT_MAROON, TFT_WHITE )
                ->on(TOUCHED,change_setting);

       container->addItem(OUTPUT_TYPE_NONE,"Set",PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_SET_PROBE_POS,48,btw_h,128,0,true,DEFAULT_FONT)
                ->set_int_value( i ) // set axis probe pos to current
                ->set_colors(0, COLORGREENY, TFT_BLACK )
                ->on(TOUCHED,change_setting);

       cc_y += btw_h+1;
       page->add_widget( container );

    }

    std::shared_ptr<PhantomWidget> ph_btn5 = factory_button( w*grid_w+grid_w, h*grid_h+grid_h, 5, 45, 2, true );
    int h_h = 0, c_y = 0, c_x = 0;
    int icon_color_default    = TFT_WHITE;
    int icon_bg_color_default = BUTTON_LIST_BG;
    for(int i = 0; i < grid_h*grid_w; ++i){
       switch (i){
           case 0:
           case 2:
           case 6:
           case 8:
               icon_color_default    = TFT_DARKGREY;
               icon_bg_color_default = BUTTON_LIST_BG;
           break;
           case 4:
               icon_color_default    = TFT_WHITE;
               icon_bg_color_default = BUTTON_LIST_BG;
           break;
           default:
               icon_color_default    = TFT_WHITE;
               icon_bg_color_default = COLOR333333;
           break;
       }
       ph_btn5->addItem(OUTPUT_TYPE_NONE,icons[i],PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_PROBE_ACTION,w,h,c_x,c_y,true,-1)
                   ->set_int_value( i )
                   ->set_require_motion()
                   ->set_colors(0,icon_bg_color_default,icon_color_default)
                   ->on(CHECK_REQUIRES,check_requirements).on(TOUCHED,change_setting);
       c_x += w+1;
       if( ++h_h == grid_h ){ // next row
              c_y += h+1;
              c_x  = 0;
              h_h  = 0;
       }
    }
    page->add_widget( ph_btn5 );


    std::shared_ptr<PhantomWidget> ph_btn6 = factory_button( ph_btn5->width, 28, 5, cc_y+h+10, 2, true );
    ph_btn6->addItem(OUTPUT_TYPE_NONE,"Move to 0,0",PARAM_TYPE_NONE,PARAM_ID_MOVE_TO_ORIGIN,ph_btn5->width,28,0,0,true,DEFAULT_FONT)
             ->set_require_motion()
             ->on(TOUCHED,change_setting).on(CHECK_REQUIRES,check_requirements);
    page->add_widget( ph_btn6 );


    ++current_tab; // next tab
    page->select_tab(current_tab); // select tab
    c_y   = 0;
    h     = 35;
    r_w   = 202;
    cc_y  = 45;
    for( int i = 0; i < N_AXIS; ++i ){
        std::shared_ptr<PhantomWidget> container = factory_button( r_w, h, 5, cc_y, 2, true );
       container->addItem(OUTPUT_TYPE_NONE,get_axis_name_const(i),PARAM_TYPE_NONE,PARAM_ID_HOME_AXIS,40,h,0,0,false,DEFAULT_FONT)
                ->set_colors(0, BUTTON_LIST_BG, TFT_WHITE )
                ->set_colors(1, BUTTON_LIST_BG, TFT_WHITE )
                ->set_colors(2, BUTTON_LIST_BG, TFT_WHITE );
       container->addItem(OUTPUT_TYPE_NONE,"N",PARAM_TYPE_NONE,PARAM_ID_HOME_AXIS,40,h,40,0,false,-1)
                ->set_int_value( i ) 
                ->set_colors(0, BUTTON_LIST_BG, COLORORANGERED )
                ->set_colors(1, BUTTON_LIST_BG, COLORGREENY )
                ->set_colors(2, BUTTON_LIST_BG, COLORGREENY )                
                ->on(LOAD,get_homed_state);
       container->addItem(OUTPUT_TYPE_STRING,"",PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_HOME_ENA,60,h,81,0,true,DEFAULT_FONT)
                ->set_int_value( i ) 
                ->set_colors(0, BUTTON_LIST_BG, COLORORANGERED )
                //->set_colors(1, BUTTON_LIST_BG, COLORGREENY )
                ->set_colors(2, BUTTON_LIST_BG, COLORGREENY )                
                ->on(LOAD,get_homing_enabled).on(TOUCHED,change_setting);
       container->addItem(OUTPUT_TYPE_NONE,"5",PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_HOME_SEQ,60,h,142,0,true,-1)
                ->set_int_value( i ) 
                ->set_require_motion()
                ->set_colors(0, COLORGREENY, TFT_BLACK )
                ->on(LOAD,get_homing_enabled_2).on(TOUCHED,change_setting).on(CHECK_REQUIRES,check_requirements);
       page->add_widget( container );
       cc_y += h+1;
    }


    std::shared_ptr<PhantomWidget> ph_btn7 = factory_button( r_w, h, 5, cc_y+10, 2, true );
    ph_btn7->addItem(OUTPUT_TYPE_NONE,"Home All",PARAM_TYPE_NONE_REDRAW_PAGE,PARAM_ID_HOME_ALL,r_w,h,0,0,true,DEFAULT_FONT)
             ->set_int_value( -1 )
             ->set_require_motion()
             ->on(TOUCHED,change_setting).on(CHECK_REQUIRES,check_requirements);
    page->add_widget( ph_btn7 );


    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> speed_settings = factory_split_list( 320, 160, 0, 45, 2, false );
    page->add_widget( speed_settings );
    speed_settings->addItem(OUTPUT_TYPE_TITLE,"Speeds",PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT);
    speed_settings->addItem(OUTPUT_TYPE_STRING,"Rapid (mm/min)",PARAM_TYPE_FLOAT,PARAM_ID_SPEED_RAPID,0,0,0,0,true,DEFAULT_FONT)
           ->set_suffix(4)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);





    // motor settings
    ++current_tab;
    page->select_tab( current_tab ); 
    std::shared_ptr<PhantomWidget> settings_12_ptr = factory_split_list( 320, 160, 0, 45, 2, false );
    page->add_widget( settings_12_ptr );
    settings_12_ptr->addItem(OUTPUT_TYPE_TITLE,"Step settings",PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT);
    settings_12_ptr->addItem(OUTPUT_TYPE_INT,DICT_X_RES,PARAM_TYPE_INT,PARAM_ID_X_RES,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_12_ptr->addItem(OUTPUT_TYPE_INT,DICT_Y_RES,PARAM_TYPE_INT,PARAM_ID_Y_RES,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);
    settings_12_ptr->addItem(OUTPUT_TYPE_INT,DICT_Z_RES,PARAM_TYPE_INT,PARAM_ID_Z_RES,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(TOUCHED,change_setting);

    std::shared_ptr<PhantomWidget> settings_12b_ptr = factory_button( 140, 24, 20, 170, 2, true );
    page->add_widget( settings_12b_ptr );
    settings_12b_ptr->addItem(OUTPUT_TYPE_NONE,DICT_RESTART,PARAM_TYPE_NONE,PARAM_ID_ESP_RESTART,140,24,0,0,true,DEFAULT_FONT)
               ->set_radius(4)
               ->set_colors( 0, TFT_RED, TFT_LIGHTGREY )   // touched
               ->set_colors( 1, BUTTON_LIST_BG, TFT_LIGHTGREY ) // default
               ->set_colors( 2, TFT_SKYBLUE, TFT_BLACK )        // active
               ->on(TOUCHED,change_setting);





    page->add_tab_item(0,DICT_PROBE_TAB);
    page->add_tab_item(1,DICT_HOMING_TAB);
    page->add_tab_item(2,"Speeds");
    page->add_tab_item(3,"Steps");


    page->add_nav( 2 ); 
    page->select_tab( ui.get_active_tab_for_page( PAGE_MOTION ), true ); // select first tab of this page again 
    page->make_temporary();

    page = nullptr;

}





void create_mode_page(){


    /*const std::function<void(PhantomWidgetItem *item)> change_tab_lambda = [](PhantomWidgetItem *item) {
        auto page = ui.get_page( item->get_parent_page_id() );
        if( page == nullptr ){ return; }
        uint8_t target_tab = 0;

        switch( item->intValue ){

            case MODE_SINKER:
            target_tab = 0;
            break;

            case MODE_2D_WIRE:
            target_tab = 1;
            break;

            case MODE_REAMER:
            target_tab = 2;
            break;

        }
        page->close( false );
        page->select_tab( target_tab, true );
        page->show();
        page = nullptr;
    };*/


    auto page = ui.addPage( PAGE_MODES, 3 ); // create new page with x tabs and index for mode (PAGE_MODE)
    int current_tab = 0;
    std::shared_ptr<PhantomWidget> ph_btn3 = factory_button( 300, 30, 10, 5, 2, true );
    ph_btn3->addItem(OUTPUT_TYPE_NONE,DICT_MODE1, PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_MODE,100,30,0,0,true,DEFAULT_FONT)
                 ->set_outlines( 1 )->set_int_value( MODE_SINKER )->on(LOAD,load_mode_custom).on(TOUCHED,change_setting);
    ph_btn3->addItem(OUTPUT_TYPE_NONE,DICT_MODE4, PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_MODE,100,30,100,0,true,DEFAULT_FONT)
                 ->set_outlines( 1 )->set_int_value( MODE_2D_WIRE )->on(LOAD,load_mode_custom).on(TOUCHED,change_setting);
    ph_btn3->addItem(OUTPUT_TYPE_NONE,DICT_MODE2, PARAM_TYPE_INT_RAW_RELOAD_WIDGET,PARAM_ID_MODE,100,30,200,0,true,DEFAULT_FONT)
                 ->set_outlines( 1 )->set_int_value( MODE_REAMER )->on(LOAD,load_mode_custom).on(TOUCHED,change_setting);

    // tab 1 = sinker specific
    // tab 2 = wire
    // tab 3 = reamer


    // tab 1 - mode settings sinker
    std::shared_ptr<PhantomWidget> ph_sli = factory_split_list( 320, 165, 0, 40, 2, false );
    ph_sli->addItem(OUTPUT_TYPE_TITLE,DICT_PAGE_MODE_S,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
                        ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    ph_sli->addItem(OUTPUT_TYPE_BOOL,DICT_SIMULATION,PARAM_TYPE_BOOL,PARAM_ID_SIMULATION,0,0,0,0,true,DEFAULT_FONT)
           ->on(LOAD,load).on(TOUCHED,change_setting); 
    ph_sli->addItem(OUTPUT_TYPE_STRING,DICT_SINK_DISTANCE,PARAM_TYPE_FLOAT,PARAM_ID_SINK_DIST,0,0,0,0,true,DEFAULT_FONT)
                        ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(3) //3=mm;
                        ->on(LOAD,load).on(TOUCHED,change_setting); 
    ph_sli->addItem(OUTPUT_TYPE_CUSTOM,DICT_SINK_DIR,PARAM_TYPE_NONE,PARAM_ID_SINK_DIR,0,0,0,0,true,DEFAULT_FONT)
                        ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->on(LOAD,load).on(BUILDSTRING,sinker_dir_lambda).on(TOUCHED,change_setting); 



    page->add_widget( ph_sli ); // add widget to pages active tab
    page->add_widget( ph_btn3 ); // mode buttons on all tabs


    // tab 2 - mode settings 2d wire
    ++current_tab; // next tab
    std::shared_ptr<PhantomWidget> ph_sli2 = factory_split_list( 320, 165, 0, 40, 2, false );
    page->select_tab(current_tab); // select tab
    ph_sli2->addItem(OUTPUT_TYPE_TITLE,DICT_PAGE_MODE_W,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
           ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    ph_sli2->addItem(OUTPUT_TYPE_BOOL,DICT_SIMULATION,PARAM_TYPE_BOOL,PARAM_ID_SIMULATION,0,0,0,0,true,DEFAULT_FONT)
           ->on(LOAD,load).on(TOUCHED,change_setting); 
    ph_sli2->addItem(OUTPUT_TYPE_STRING,DICT_TOOLDIA,PARAM_TYPE_FLOAT,PARAM_ID_TOOLDIAMETER,0,0,0,0,true,DEFAULT_FONT)
           ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)->set_suffix(3) //3=mm;
           ->on(LOAD,load).on(TOUCHED,change_setting);


    page->add_widget( ph_sli2 );
    page->add_widget( ph_btn3 ); // mode buttons on all tabs

    // tab 3 - mode settings reamer
    ++current_tab; // next tab
    std::shared_ptr<PhantomWidget> ph_sli3 = factory_split_list( 320, 165, 0, 40, 2, false );
    page->select_tab(current_tab); // select tab
    ph_sli3->addItem(OUTPUT_TYPE_TITLE,DICT_PAGE_MODE_R,PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
                        ->set_colors(0,BUTTON_LIST_BG,TFT_OLIVE)
                        ->set_colors(1,BUTTON_LIST_BG,TFT_OLIVE)
                        ->set_colors(2,BUTTON_LIST_BG,TFT_OLIVE);
    ph_sli3->addItem(OUTPUT_TYPE_STRING,DICT_REAMER_DIS,PARAM_TYPE_FLOAT,PARAM_ID_REAMER_DIST,0,0,0,0,true,DEFAULT_FONT)
                        ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)
                        ->set_suffix(3) //3=mm;
                        ->on(LOAD,load).on(TOUCHED,change_setting); 
    ph_sli3->addItem(OUTPUT_TYPE_STRING,DICT_REAMER_DUR,PARAM_TYPE_FLOAT,PARAM_ID_REAMER_DURA,0,0,0,0,true,DEFAULT_FONT)
                        ->set_colors(1,BUTTON_LIST_BG,TFT_ORANGE)
                        ->set_suffix(12) //12=s;
                        ->on(LOAD,load).on(TOUCHED,change_setting);
    page->add_widget( ph_sli3 );
    page->add_widget( ph_btn3 ); // mode buttons on all tabs
    page->add_nav( 1 );   
    page->select_tab( ui.get_active_tab_for_page( PAGE_MODES ), true ); // select first tab of this page again 
    page->make_temporary();

    page = nullptr;

}


const std::function<void(PhantomWidgetItem *item)> load_previous_page = [](PhantomWidgetItem *item){
    ui.show_page( ui.get_previous_active_page() );
};



void create_info_page( info_codes msg_id ){
    auto page = ui.addPage( PAGE_INFO, 1 ); // create page with only 1 tab
    std::shared_ptr<PhantomWidget> infobox = factory_list( 280, (4+tft.fontHeight(DEFAULT_FONT))*4, 10, 10, 2, true );
    infobox->addItem(OUTPUT_TYPE_STRING,"P",PARAM_TYPE_NONE,0,0,0,0,0,false,-1)->set_colors( 0, -1, TFT_DARKGREY );
    infobox->addItem(OUTPUT_TYPE_STRING,"Info",PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)->set_colors( 0, -1, TFT_GREENYELLOW );
    infobox->addItem(OUTPUT_TYPE_STRING,info_messages[msg_id],PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)->set_colors( 0, -1, TFT_LIGHTGREY );
    page->add_widget( infobox );
    std::shared_ptr<PhantomWidget> backbtn = factory_button( 300, 40, 10, 190, 1, true );
    backbtn->addItem(OUTPUT_TYPE_STRING,DICT_CLOSE,PARAM_TYPE_NONE,0,300,40,0,0,true,DEFAULT_FONT)->set_radius(4)->set_string_value(DICT_CLOSE)->on(TOUCHED,load_previous_page);
    page->add_widget( backbtn );
    page->select_tab( ui.get_active_tab_for_page( PAGE_INFO ), true ); // select first tab of this page again 
    page->make_temporary();
    page = nullptr;
}


static const std::function<void(info_codes *data)> info_box_lambda = [](info_codes *data){
    if( ui.get_active_page() == PAGE_INFO ){ return; }
    create_info_page( *data );
    ui.show_page( PAGE_INFO );
};


void create_front_page(){

    //#################################################
    // Front page
    //#################################################
    auto page = ui.addPage( PAGE_FRONT, 1 ); // create page with only 1 tab
    std::shared_ptr<PhantomWidget> front_menu = factory_vbuttonlist( 80, 240, 0, 0, 1, true );
    front_menu->addItem(OUTPUT_TYPE_NONE,ICON_COGS,0,0,0,0,0,0,true,-1)->set_int_value( PAGE_MAIN_SETTINGS )->on(TOUCHED,open_page);
    front_menu->addItem(OUTPUT_TYPE_NONE,MM_MODE, PARAM_TYPE_NONE,0,0,0,0,0,true,DEFAULT_FONT)->set_int_value( PAGE_MODES )->on(TOUCHED,open_page);
    front_menu->addItem(OUTPUT_TYPE_NONE,MM_MOTION, PARAM_TYPE_NONE,0,0,0,0,0,true,DEFAULT_FONT)->set_int_value(PAGE_MOTION)->on(TOUCHED,open_page);
    front_menu->addItem(OUTPUT_TYPE_NONE,MM_SD, PARAM_TYPE_NONE,0,0,0,0,0,true,DEFAULT_FONT)->set_int_value( PAGE_SD_CARD )->set_require_sd()->set_is_refreshable()->on(TOUCHED,open_page).on(CHECK_REQUIRES,check_requirements);
    front_menu->set_is_refreshable();
    page->add_widget( front_menu );

    std::shared_ptr<PhantomWidget> mpos = factory_list( 110, (4+tft.fontHeight(DEFAULT_FONT))*3, 85, 90, 2, true );
    mpos->addItem(OUTPUT_TYPE_FLOAT,MPOS_X,PARAM_TYPE_FLOAT,PARAM_ID_MPOSX,0,0,0,0,false,DEFAULT_FONT)->set_is_refreshable()->on(LOAD,load);
    mpos->addItem(OUTPUT_TYPE_FLOAT,MPOS_Y,PARAM_TYPE_FLOAT,PARAM_ID_MPOSY,0,0,0,0,false,DEFAULT_FONT)->set_is_refreshable()->on(LOAD,load);
    mpos->addItem(OUTPUT_TYPE_FLOAT,MPOS_Z,PARAM_TYPE_FLOAT,PARAM_ID_MPOSZ,0,0,0,0,false,DEFAULT_FONT)->set_is_refreshable()->on(LOAD,load);
    //front_mpos->on(LOAD,load); // load all mpos at once.... todo
    mpos->set_is_refreshable();
    page->add_widget( mpos );


    std::shared_ptr<PhantomWidget> mdata = factory_list( 110, (4+tft.fontHeight(DEFAULT_FONT))*3, 195, 90, 2, true );
    mdata->addItem(OUTPUT_TYPE_CUSTOM,MM_MODE,PARAM_TYPE_INT,PARAM_ID_MODE,0,0,0,0,false,DEFAULT_FONT)
              ->on(BUILDSTRING,mode_load).on(LOAD,load);
    mdata->addItem(OUTPUT_TYPE_STRING,DICT_FREQ,PARAM_TYPE_FLOAT,PARAM_ID_FREQ,0,0,0,0,false,DEFAULT_FONT)
              ->set_suffix(1) //1=kHz
              ->on(LOAD,load).on(TOUCHED,change_setting);
    mdata->addItem(OUTPUT_TYPE_STRING,DICT_DUTY,PARAM_TYPE_FLOAT,PARAM_ID_DUTY,0,0,0,0,false,DEFAULT_FONT)
              ->set_suffix(2) //2=%
              ->on(LOAD,load).on(TOUCHED,change_setting);
    page->add_widget( mdata );       // add widget to active tab; default active tab is the first one



    std::shared_ptr<PhantomWidget> ph_btn = factory_button( 60, 60, 250, 170, 1, true );
    ph_btn->addItem(OUTPUT_TYPE_STRING,BTN_START,PARAM_TYPE_BOOL,PARAM_ID_PWM_STATE,60,60,0,0,true,DEFAULT_FONT)
                ->set_string_value( BTN_START )
                //->set_is_refreshable()
                ->set_radius(4)
                ->set_colors( 0, TFT_DARKGREY, TFT_LIGHTGREY )
                ->set_colors( 1, BUTTON_LIST_BG, TFT_LIGHTGREY )
                ->set_colors( 2, COLORORANGERED, TFT_WHITE )
                ->on(TOUCHED,change_setting).on(LOAD,load_pwm_enabled);
    //ph_btn->set_is_refreshable();
    page->add_widget( ph_btn );     // add widget to active tab; default active tab is the first one



    std::shared_ptr<PhantomWidget> ph_btn2 = factory_button( 145, 80, 90, 160, 2, true );
    int w = 30, h = 40;
    ph_btn2->disable_background_color();
    ph_btn2->addItem(OUTPUT_TYPE_NONE,NAV_LEFT, PARAM_TYPE_FLOAT,PARAM_ID_XLEFT,w,h,0,20,true,-1)
                    //->set_colors( 0, -1, COLORORANGERED )
                    ->set_colors( 1, -1, TFT_GREEN )
                    ->set_colors( 3, -1, BUTTON_LIST_BG_DEACTIVATED )
                    ->set_require_motion()
                    ->set_is_refreshable()
                    ->on(TOUCHED,navigation_callback).on(CHECK_REQUIRES,check_requirements);
    ph_btn2->addItem(OUTPUT_TYPE_NONE,NAV_RIGHT, PARAM_TYPE_FLOAT,PARAM_ID_XRIGHT,w,h,70,20,true,-1)
                    //->set_colors( 0, -1, COLORORANGERED )
                    ->set_colors( 1, -1, TFT_GREEN )
                    ->set_colors( 3, -1, BUTTON_LIST_BG_DEACTIVATED )
                    ->set_require_motion()
                    ->set_is_refreshable()
                    ->on(TOUCHED,navigation_callback).on(CHECK_REQUIRES,check_requirements);
    ph_btn2->addItem(OUTPUT_TYPE_NONE,NAV_UP, PARAM_TYPE_FLOAT,PARAM_ID_YUP,w,h,35,0,true,-1)
                    //->set_colors( 0, -1, COLORORANGERED )
                    ->set_colors( 1, -1, TFT_GREEN )
                    ->set_colors( 3, -1, BUTTON_LIST_BG_DEACTIVATED )
                    ->set_require_motion()
                    ->set_is_refreshable()
                    ->on(TOUCHED,navigation_callback).on(CHECK_REQUIRES,check_requirements);
    ph_btn2->addItem(OUTPUT_TYPE_NONE,NAV_DOWN, PARAM_TYPE_FLOAT,PARAM_ID_YDOWN,w,h,35,40,true,-1)
                    //->set_colors( 0, -1, COLORORANGERED )
                    ->set_colors( 1, -1, TFT_GREEN )
                    ->set_colors( 3, -1, BUTTON_LIST_BG_DEACTIVATED )
                    ->set_require_motion()
                    ->set_is_refreshable()
                    ->on(TOUCHED,navigation_callback).on(CHECK_REQUIRES,check_requirements);
    ph_btn2->addItem(OUTPUT_TYPE_NONE,NAV_UP, PARAM_TYPE_FLOAT,PARAM_ID_ZUP,w,h,110,0,true,-1)
                    //->set_colors( 0, -1, COLORORANGERED )
                    ->set_colors( 1, -1, TFT_GREEN )
                    ->set_colors( 3, -1, BUTTON_LIST_BG_DEACTIVATED )
                    ->set_require_motion()
                    ->set_is_refreshable()
                    ->on(TOUCHED,navigation_callback).on(CHECK_REQUIRES,check_requirements);
    ph_btn2->addItem(OUTPUT_TYPE_NONE,NAV_DOWN, PARAM_TYPE_FLOAT,PARAM_ID_ZDOWN,w,h,110,40,true,-1)
                    //->set_colors( 0, -1, COLORORANGERED )
                    ->set_colors( 1, -1, TFT_GREEN )
                    ->set_colors( 3, -1, BUTTON_LIST_BG_DEACTIVATED )
                    ->set_require_motion()
                    ->set_is_refreshable()
                    ->on(TOUCHED,navigation_callback).on(CHECK_REQUIRES,check_requirements);
    ph_btn2->set_is_refreshable();
    page->add_widget( ph_btn2 ); // add widget to active tab; default active tab is the first one
    
    page->select_tab( ui.get_active_tab_for_page( PAGE_FRONT ), true ); // select first tab of this page again 
    page->make_temporary();

    page = nullptr;

}


void build_file_manager( int type ){
    int  folder_page = type == 0 ? PAGE_FILE_MANAGER : PAGE_FILE_MANAGER_GCODES;
    auto page = ui.addPage( folder_page, 1 );
    // precalculate based on the number of file on the card matching the extension
    // less trouble for now compared to keeping track of the file index and return stuff
    int total_files = SD_CARD.count_files_in_folder( type == 0 ? SD_SETTINGS_FOLDER : SD_GCODE_FOLDER );
    if( total_files == -1 ){
        //error. SD not available... Not handled well yet
        total_files = 0;
    }
    std::shared_ptr<PhantomWidget> file_manager = factory_elist( 300, 205, 10, 0, 0, false );
    std::shared_ptr<file_browser> __this = std::dynamic_pointer_cast<file_browser>(file_manager);
    std::string page_title = type == 0 ? SD_SETTINGS_FOLDER : SD_GCODE_FOLDER;
    page_title += " (";
    page_title += intToString( total_files );
    page_title += " files)";
    __this->set_file_type( type );
    __this->set_page_title( page_title );                                   // set page title
    __this->set_num_items( FILE_ITEMS_PER_PAGE );                           // number of files to view per page
    __this->set_folder( type == 0 ? SD_SETTINGS_FOLDER : SD_GCODE_FOLDER ); // set the folder to look for files
    __this->create();                                                       // setup some internal callbacks and initialize the sd folder
    __this->start();
    page->add_widget( file_manager );
    int page_id   = page->get_page_id();
    int page_tab  = page->get_active_tab();
    int widget_id = file_manager->index;
    //##########################################################################################
    // Bottom navigation buttons 
    //##########################################################################################
    const std::function<void(PhantomWidgetItem *item)> button_is_active = [widget_id,total_files](PhantomWidgetItem *item){
        if( item->param_id == PARAM_ID_PAGE_NAV_LEFT ){
            // if current file is first disable
            if( SD_CARD.get_current_tracker_index(0) <= FILE_ITEMS_PER_PAGE || !SD_CARD.is_free() ){
                item->disabled = true;
            } else {
                item->disabled = false;
            }
        } else {
            if( SD_CARD.get_current_tracker_index(0) >= total_files || !SD_CARD.is_free() ){
                item->disabled = true;
            } else {
                item->disabled = false;
            }
        }
    };

    const std::function<void(PhantomWidgetItem *item)> previous = [widget_id,folder_page](PhantomWidgetItem *item){
        // load previous batch 
        if( ui.get_active_page() != folder_page ){ return; }
        auto widget = ui.pages[folder_page]->tabs[0]->widgets.find( widget_id );
        std::shared_ptr<file_browser> __this = std::dynamic_pointer_cast<file_browser>(widget->second);
        __this->load_file_batch( false );
        ui.reload_page();
    };
    const std::function<void(PhantomWidgetItem *item)> next = [widget_id,folder_page](PhantomWidgetItem *item){
        // load next batch
        if( ui.get_active_page() != folder_page ){ return; }
        auto widget = ui.pages[folder_page]->tabs[0]->widgets.find( widget_id );
        std::shared_ptr<file_browser> __this = std::dynamic_pointer_cast<file_browser>(widget->second);
        __this->load_file_batch( true );
        ui.reload_page();
    };
    std::shared_ptr<PhantomWidget> nav = factory_button( 320, 30, 0, 210, 0, true );

    nav->addItem(OUTPUT_TYPE_NONE,NAV_LEFT, PARAM_TYPE_INT, PARAM_ID_PAGE_NAV_LEFT,60,35,200,0,true,-1)
       ->set_is_refreshable()->set_require_sd()->set_outlines(2)->on(TOUCHED,previous).on(CHECK_REQUIRES,button_is_active);
    nav->addItem(OUTPUT_TYPE_NONE,NAV_RIGHT, PARAM_TYPE_INT, PARAM_ID_PAGE_NAV_RIGHT,60,35,260,0,true,-1)
       ->set_is_refreshable()->set_require_sd()->on(TOUCHED,next).on(CHECK_REQUIRES,button_is_active);
    nav->addItem(OUTPUT_TYPE_NONE,DICT_CLOSE, PARAM_TYPE_NONE, PARAM_ID_PAGE_NAV_CLOSE,200,35,0,0,true,DEFAULT_FONT)
       ->set_outlines(2)->on(TOUCHED,load_previous_page);
    nav->set_is_refreshable();

    page->add_widget( nav );

    page->select_tab( ui.get_active_tab_for_page( folder_page ), true ); // select first tab of this page again 
    page->make_temporary();
    page   = nullptr;
    __this = nullptr;


}

void create_file_manager_page_gcode(){
    build_file_manager( 1 );
}

void create_file_manager_page(){
    build_file_manager( 0 );
}

void create_probing_page(){
    auto page = ui.addPage( PAGE_PROBING, 1 );
    std::shared_ptr<PhantomWidget> nav = factory_button( 320, 30, 0, 210, 0, true );
    nav->addItem(OUTPUT_TYPE_NONE,DICT_CLOSE, PARAM_TYPE_NONE, PARAM_ID_PAGE_NAV_CLOSE,320,30,0,0,true,DEFAULT_FONT)
       ->set_outlines(2)->on(TOUCHED,load_previous_page);
    page->add_widget( nav );
    page->select_tab( ui.get_active_tab_for_page( PAGE_PROBING ), true ); // select first tab of this page again 
    page->make_temporary();
    page   = nullptr;
}

void create_process_page(){

    auto page = ui.addPage( PAGE_IN_PROCESS, 1 );

    int w = 160, h = 20;
    std::shared_ptr<PhantomWidget> mpos_wpos = factory_button( 80, h, 15, 110, 2, true );
    mpos_wpos->disabled_emits_global()->disable_background_color();
    mpos_wpos->addItem(OUTPUT_TYPE_NONE,"WPos",PARAM_TYPE_NONE,0,80,h,0,0,false,DEFAULT_FONT)
             ->set_colors( 0, -1, TFT_LIGHTGREY )->set_colors( 1, -1, TFT_LIGHTGREY )->set_colors( 2, -1, TFT_LIGHTGREY )
             ->set_int_value( -1 )
             ->set_text_align( 1 );
    page->add_widget(mpos_wpos);
    std::shared_ptr<PhantomWidget> axis_names = factory_list( 15, 60, 0, 130, 2, true );
    axis_names->disabled_emits_global()->disable_background_color();
    for( int i = 0; i < N_AXIS; ++i ){
        std::string axis_name;
        axis_name = get_axis_name(i);
        axis_names->addItem(OUTPUT_TYPE_STRING,"",PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
                  ->set_int_value( -1 )
                  ->set_string_value( axis_name )
                  ->set_colors( 0, -1, TFT_LIGHTGREY )
                  ->set_text_align(1);
    }
    page->add_widget(axis_names);


    std::shared_ptr<PhantomWidget> axis_wpos = factory_list( 70, 60, 15, 130, 2, true );
    axis_wpos->disable_background_color();
    axis_wpos->set_is_refreshable();
    axis_wpos->addItem(OUTPUT_TYPE_FLOAT,"",PARAM_TYPE_FLOAT,PARAM_ID_WPOSX,0,0,0,0,false,DEFAULT_FONT)
             ->set_float_decimals( 3 )->set_colors( 0, -1, TFT_YELLOW )->set_text_align(1)->set_is_refreshable()->on(LOAD,load);
    axis_wpos->addItem(OUTPUT_TYPE_FLOAT,"",PARAM_TYPE_FLOAT,PARAM_ID_WPOSY,0,0,0,0,false,DEFAULT_FONT)
             ->set_float_decimals( 3 )->set_colors( 0, -1, TFT_YELLOW )->set_text_align(1)->set_is_refreshable()->on(LOAD,load);
    axis_wpos->addItem(OUTPUT_TYPE_FLOAT,"",PARAM_TYPE_FLOAT,PARAM_ID_WPOSZ,0,0,0,0,false,DEFAULT_FONT)
             ->set_float_decimals( 3 )->set_colors( 0, -1, TFT_YELLOW )->set_text_align(1)->set_is_refreshable()->on(LOAD,load);
    page->add_widget(axis_wpos);


    std::shared_ptr<PhantomWidget> bottom_controls = factory_button( 320, 30, 0, 210, 0, true );
    bottom_controls->set_is_refreshable();
    bottom_controls->addItem(OUTPUT_TYPE_NONE,"Settings",PARAM_TYPE_NONE,0,80,30,0,0,true,DEFAULT_FONT)
                   ->set_int_value( PAGE_MAIN_SETTINGS )->on(TOUCHED,open_page_pause_process);

    bottom_controls->addItem(OUTPUT_TYPE_STRING,"Pause",PARAM_TYPE_BOOL,PARAM_ID_EDM_PAUSE,239,30,81,0,true,DEFAULT_FONT)
                   ->set_is_refreshable()
                   ->set_string_value( "Pause" )->set_colors( 2, COLORORANGERED, TFT_LIGHTGREY )->on(TOUCHED,change_setting).on(LOAD,load_toogle_active_state);

    page->add_widget(bottom_controls);
    page->select_tab( ui.get_active_tab_for_page( PAGE_IN_PROCESS ), true ); // select first tab of this page again 
    page->make_temporary();
    page = nullptr;

}

const std::function<void(PhantomWidgetItem *item)> set_gcode_name_lambda = [](PhantomWidgetItem *item){
    if( ui.gcode_file == "" ){
        item->set_string_value( "None" );
    }
};


const std::function<void(PhantomWidgetItem *item)> factory_reset_lambda = [](PhantomWidgetItem *item){
    ui.lock_touch();
    ui.delete_nvs_storage();
    gconf.flag_restart = true;
};


void create_sd_page(){

    auto page = ui.addPage( PAGE_SD_CARD, 3 );

    // fill tab 0 = save/load settings
    std::shared_ptr<PhantomWidget> settings_save_container = factory_button( 310, 40, 5, 50, 2, true );
    settings_save_container->addItem(OUTPUT_TYPE_NONE,"Save Settings",PARAM_TYPE_NONE,0,154,40,0,0,true,DEFAULT_FONT)
             ->set_int_value( SD_SAVE_AUTONAME )
             ->set_require_sd()->set_is_refreshable()
             ->on(CHECK_REQUIRES,check_requirements).on(TOUCHED,save_settings_lambda);

    settings_save_container->addItem(OUTPUT_TYPE_NONE,"Load Settings",PARAM_TYPE_NONE,0,154,40,155,0,true,DEFAULT_FONT)
             ->set_int_value( PAGE_FILE_MANAGER )
             ->set_require_sd()->set_is_refreshable()
             ->on(CHECK_REQUIRES,check_requirements).on(TOUCHED,open_page);

    settings_save_container->set_is_refreshable();
    page->add_widget(settings_save_container);


    std::shared_ptr<PhantomWidget> settings_current_container = factory_list( 310, 50, 5, 110, 2, true );
    settings_current_container->disable_background_color();
    settings_current_container->addItem(OUTPUT_TYPE_STRING,"",PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
             ->set_int_value( -1 )
             ->set_string_value( "Loaded settings:" )
             ->set_require_sd()
             ->set_colors( 0, -1, TFT_LIGHTGREY )
             ->on(CHECK_REQUIRES,check_requirements);
    settings_current_container->addItem(OUTPUT_TYPE_STRING,"",PARAM_TYPE_NONE,0,0,0,0,0,true,DEFAULT_FONT)
             ->set_int_value( PAGE_FILE_MANAGER )
             ->set_string_value( SD_CARD.get_filename_from_path( ui.settings_file ) )
             ->set_require_sd()->set_is_refreshable()
             ->set_colors( 0, -1, TFT_YELLOW )
             ->on(CHECK_REQUIRES,check_requirements).on(TOUCHED,open_page);
    page->add_widget(settings_current_container);
    settings_current_container->set_is_refreshable();





    page->select_tab( 1 ); // gcode tab
    std::shared_ptr<PhantomWidget> gcode_load_container = factory_button( 310, 40, 5, 50, 2, true );
    gcode_load_container->addItem(OUTPUT_TYPE_NONE,"Set GCode file",PARAM_TYPE_NONE,0,154,40,0,0,true,DEFAULT_FONT)
             ->set_int_value( PAGE_FILE_MANAGER_GCODES )
             ->set_require_sd()->set_is_refreshable()
             ->on(CHECK_REQUIRES,check_requirements).on(TOUCHED,open_page);
    page->add_widget(gcode_load_container);

    std::shared_ptr<PhantomWidget> gcode_current_container = factory_list( 310, 50, 5, 110, 2, true );
    gcode_current_container->disable_background_color();
    gcode_current_container->addItem(OUTPUT_TYPE_STRING,"",PARAM_TYPE_NONE,0,0,0,0,0,false,DEFAULT_FONT)
             ->set_int_value( -1 )
             ->set_string_value( "Loaded GCode file:" )
             ->set_require_sd()
             ->set_colors( 0, -1, TFT_LIGHTGREY )
             ->on(CHECK_REQUIRES,check_requirements);
    gcode_current_container->addItem(OUTPUT_TYPE_STRING,"",PARAM_TYPE_NONE,0,0,0,0,0,true,DEFAULT_FONT)
             ->set_int_value( PAGE_FILE_MANAGER_GCODES )
             ->set_string_value( SD_CARD.get_filename_from_path( ui.gcode_file ) )
             ->set_require_sd()->set_is_refreshable()
             ->set_colors( 0, -1, TFT_YELLOW )
             ->on(CHECK_REQUIRES,check_requirements).on(TOUCHED,open_page).on(LOAD,set_gcode_name_lambda);
    page->add_widget(gcode_current_container);
    gcode_current_container->set_is_refreshable();




    page->select_tab( 2 ); // others
    std::shared_ptr<PhantomWidget> settings_factory_reset = factory_button( 310, 24, 5, 50, 0, true ); // widget_type is not used in the button widget. The type/t value doesn't matter
    settings_factory_reset->addItem(OUTPUT_TYPE_NONE,"Factory Reset",PARAM_TYPE_NONE,0,310,24,0,0,true,DEFAULT_FONT)
             ->set_colors( 0, COLORORANGERED, TFT_WHITE )
             ->on(TOUCHED,factory_reset_lambda);
    page->add_widget(settings_factory_reset);



    page->add_tab_item(0,DICT_SD_SET_LABEL);
    page->add_tab_item(1,DICT_SD_GCODE_LABEL);
    page->add_tab_item(2,"Others");
    page->add_nav( 2 ); 
    page->select_tab( ui.get_active_tab_for_page( PAGE_SD_CARD ), true ); // select first tab of this page again 
    page->make_temporary();
    page = nullptr;

}
void create_alarm_page(){
}


const std::function<void(int *data)> end_sd_job_lambda = [](int *data) {
    ui.emit(LOG_LINE," Unload SD card");
    SD_CARD.end_gcode_job();
};
const std::function<void(int *data)> shift_line_sd_lambda = [](int *data) {
    if( sd_line_shift ){ 
        return; 
    }
    if( !SD_CARD.shift_gcode_line() ){
        ui.emit(LOG_LINE,"@ GCode file end");
        sd_line_shift = false;
    }
};
const std::function<void(int *data)> start_sd_job_lambda = [](int *data) {

    edm_stop_codes error = STOP_CLEAN; // default no error...

    if( ui.gcode_file.length() <= 0 ){ // should not happen easily

        error = STOP_NO_FILE_GIVEN;
        ui.emit(LOG_LINE,"No GCode file given!");

    } else {

        if( !SD_CARD.start_gcode_job( ui.gcode_file ) ){

            // failed to open file
            error         = STOP_SD_FAILED_TO_INIT;
            ui.emit(LOG_LINE,"Failed to start sd shiftout!");

        }

    }

    if( error != STOP_CLEAN ){

        sd_line_shift = false; // disable line shifting
        const char * buffer = int_to_char( error );
        ui.emit_param_set( PARAM_ID_SET_STOP_EDM, buffer );
        delete[] buffer;
        buffer = nullptr;

    } else {

        sd_line_shift = true; // enable line shifting

    }


};

//##############################################
// As long as the ui runs on the same esp
// it needs some faster solutions in the process
// skipping all the emitter stuff etc
//##############################################
IRAM_ATTR void PhantomUI::wpos_update( void ){
    ui.reload_page_on_change();
}
IRAM_ATTR void PhantomUI::scope_update( void ){
    gscope.draw_scope();
}
IRAM_ATTR void PhantomUI::process_update( void ){
    if( sd_line_shift ){ // load next gcode line
        if( !SD_CARD.shift_gcode_line() ){
            sd_line_shift = false;
            ui.emit(LOG_LINE,"GCode file end");
        }
    }
    ui.monitor_touch(); // touch monitor
}

const std::function<void(int *data)> firmware_update_done_lambda = [](int *data) {
    // this just removes the flag for the cached settings
    // on restart it will then write the initial or overwrite the existing cache values
    Preferences p;
    if( p.begin(RELEASE_NAME, false) ){
        p.putBool( "has_settings", false );
        p.end();
    }
};

const std::function<void(const char* line)> logger_fill = [](const char *line){
    ui.logger->add_line( line );vTaskDelay(1);
    //if( !ui.get_is_ready() ){ return; }
    ui.logger->show(); //tmp needs to be specified to prevent it from popping up
};

void PhantomUI::exit(){
    SD_CARD.exit();
}


bool PhantomUI::update_firmware(){
    if( SD_CARD.get_state() == SD_NOT_AVAILABLE || gconf.flag_restart ){
        return false;
    }
    if( SD_CARD.firmware_update_available() ){
        ui.emit(LOG_LINE,"Flagging NVS storage reset");
        Preferences p;
        if( p.begin(RELEASE_NAME, false) ){
            p.putBool( "has_settings", false );
            p.end();
        }
        if( SD_CARD.firmware_update() ){
            int data = 1;
            ui.emit(FIRMWARE_UPDATE_DONE, &data);
            gconf.flag_restart = true;
        }
    }
    return true;
}
//############################################################################################################
// Boot screen
//############################################################################################################
void PhantomUI::loader_screen(){
    canvas.setTextSize(3);
    canvas.setTextColor(TFT_DARKGREY);
    canvas.setTextSize(3);
    canvas.setTextFont(2);
    canvas.textWidth(BRAND_NAME);
    canvas.createSprite( canvas.textWidth(BRAND_NAME), canvas.fontHeight()+20 );
    canvas.fillScreen(TFT_BLACK);
    canvas.drawString(BRAND_NAME,0,20);
    int16_t x = 20, y = -60;
    do { canvas.pushSprite(x,y); vTaskDelay(5); } while (++y < 0);
    canvas.deleteSprite();
    canvas.setTextSize(2);
    canvas.createSprite( 320, canvas.fontHeight() );
    canvas.fillScreen(TFT_BLACK);
    canvas.setTextColor(COLORORANGERED);
    canvas.setTextSize(2);
    canvas.drawString(RELEASE_NAME, 20, 0, 2);vTaskDelay(1);
    canvas.setTextColor(BUTTON_LIST_BG);
    x = 320, y = 70;
    do { canvas.pushSprite(x,60); vTaskDelay(1); } while (--x != 0);
    canvas.deleteSprite();
    vTaskDelay(100);
}

//############################################################################################################
// PhantomUI init function
//############################################################################################################
void PhantomUI::init(){
    msg_id        = INFO_NONE;
    sd_card_state = -1;
    settings_file = "Last Session";
    gcode_file    = "";
    memset( page_tree, 0, sizeof(page_tree) );
    memset( active_tabs, 0, sizeof(active_tabs) );
    page_tree_index    = 0;
    gconf.flag_restart = false;
    tft.init();vTaskDelay(1);
    tft.setRotation(3);vTaskDelay(1);
    tft.setCursor(0, 0, 2);vTaskDelay(1);
    fill_screen(TFT_BLACK);
    touch_calibrate( false );vTaskDelay(1);
    tft.setCursor(0, 0, 2);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.setCursor(0, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
}

/* 
    This is a hacky bugfix for now. If sd card is initialized after the new boot screen it has 
    problems with updates via sd card and creating the initial directorytree after a power is turned on.
    Strange thing: It does work after a software triggered reboot or a reboot via the small button on the ESP board.
    This one makes all the SD stuff before showing the boot screen and uses only basic tft features.
    Not a beauty but it works. 
*/
void PhantomUI::init_sd(){
    tft.println("SD init");vTaskDelay(1);
    SD_CARD.init( tft.getSPIinstance() );
    if( !SD_CARD.run() ){
        tft.println("No SD card found");
        return;
    }
    tft.println("Checking SD directories");
    SD_CARD.build_directory_tree();
    if( SD_CARD.firmware_update_available() ){
        tft.println("Updating firmware!");
        tft.println("This can take some time");
        tft.println("Don't disconnect the ESP!");
        if( SD_CARD.firmware_update() ){
            int data = 1;
            firmware_update_done_lambda( &data );
            gconf.flag_restart = true;
        }
    }
}


bool PhantomUI::get_is_ready(){
    return is_ready;
}



// setup events and everything and start sd card
void PhantomUI::build_phantom(){

    init();
    init_sd(); // setup basic sd card stuff
    fill_screen(TFT_BLACK);

    if( !gconf.flag_restart ){
        loader_screen();vTaskDelay(1);
    }

    page_creator_callbacks[PAGE_FRONT]               = create_front_page;
    page_creator_callbacks[PAGE_MAIN_SETTINGS]       = create_settings_page;
    page_creator_callbacks[PAGE_MODES]               = create_mode_page;
    page_creator_callbacks[PAGE_MOTION]              = create_motion_page;
    page_creator_callbacks[PAGE_SD_CARD]             = create_sd_page;
    page_creator_callbacks[PAGE_FILE_MANAGER]        = create_file_manager_page;
    page_creator_callbacks[PAGE_FILE_MANAGER_GCODES] = create_file_manager_page_gcode;
    page_creator_callbacks[PAGE_IN_PROCESS]          = create_process_page;
    page_creator_callbacks[PAGE_PROBING]             = create_probing_page;

    set_motion_is_active( false ); // default to motion not active until loaded
    gscope.setup(); // create the scope queue etc.
    //gscope.set_full_range_size( i2sconf.I2S_full_range );
    //gscope.set_vdrop_treshhold( adc_critical_conf.voltage_feedback_treshhold );
    //gscope.update_setpoint( setpoints.lower, setpoints.upper );
    logger = factory_logger( 320, 120, 20, 100, 1, true );
    logger->build_ringbuffer( 0 );


    ui.on(EVENT_TOUCH_PULSE,monitor_touch_lambda);     //const std::function<void (int *data)> monitor_touch_lambda
    ui.on(SHOW_PAGE,show_page_ev);                     //const std::function<void (int *data)> show_page_ev
    ui.on(SD_CHANGE,sd_change);                        //const std::function<void (int *data)> sd_change
    ui.on(ALARM,alarm_lamda);                          //static const std::function<void (int *data)> alarm_lamda
    ui.on(RELOAD,reload_page_lambda);                  //const std::function<void (int *data)> reload_page_lambda
    ui.on(RELOAD_ON_CHANGE,reload_page_if_changed);    //const std::function<void (int *data)> reload_page_if_changed
    ui.on(LOG_LINE,logger_fill);                       //const std::function<void (const char *line)> log_line
    ui.on(RESET,reset_lambda);                         //const std::function<void (int *data)> restart_lambda
    ui.on(INFOBOX,info_box_lambda);
    ui.on(RUN_SD_CARD_JOB,start_sd_job_lambda);
    ui.on(END_SD_CARD_JOB,end_sd_job_lambda);
    ui.on(SHIFT_SD_CARD_LINE,shift_line_sd_lambda);
    ui.on(FIRMWARE_UPDATE_DONE,firmware_update_done_lambda);

    vTaskDelay(100);

    is_ready = true;

}




//#####################################################################################################
// Iterate over all settings that are storable and pass them to a callback lambda function
//#####################################################################################################
bool param_loop( std::function<void(int param_id, setting_cache_object *package)> callback = nullptr ){
    if( callback == nullptr ){ return false; }
    for( auto& param : settings_file_param_types ){
        int param_id = param.first;
        if( settings_values_cache.find( param_id ) == settings_values_cache.end() ){
            // no cached value found
            param_load_raw( param_id ); // load param and update the cached data
            if( settings_values_cache.find( param_id ) == settings_values_cache.end() ){
                // still not found, skip
                continue;
            }
        }
        // param package available
        callback( param_id, &settings_values_cache.find( param_id )->second );
    }
    return true;
}



bool PhantomUI::delete_nvs_storage(){
    emit(LOG_LINE,"### Factory reset called ###");
    Preferences p;
    if( p.begin(RELEASE_NAME, false) ){
        if( p.clear() ){
            emit(LOG_LINE,"  NVS deleted!");
        } else {
            emit(LOG_LINE,"  Failed to delete NVS!");
        }
        p.end();
    } else {
        emit(LOG_LINE,"  Failed to open storage ###");
    }
    return true;
}


//########################################################
// Last session settings are stored in the NVS on the ESP
// and don't need an SD card 
//########################################################

// restore last session on boot
bool PhantomUI::restore_last_session(){
    //return delete_nvs_storage();
    Preferences p;
    p.begin(RELEASE_NAME, true);
    if( !p.getBool( "has_settings" ) ){
        // no valid settings stored yet
        ui.emit(LOG_LINE,"* No valid last session data");
        save_current_to_nvs(-1);
        return false;
    }
    int param_id;
    for( auto& param : settings_file_param_types ){
        param_id = param.first;
        const char* key_buffer = int_to_char( param_id );

        auto param_type = settings_file_param_types[param_id]; 

        switch( param_type ){
            case PARAM_TYPE_NONE:
            case PARAM_TYPE_STRING:
            case PARAM_TYPE_NONE_REDRAW_WIDGET:
            case PARAM_TYPE_NONE_REDRAW_PAGE:
            case PARAM_TYPE_BOOL_RELOAD_WIDGET:
            case PARAM_TYPE_INT_RAW_RELOAD_WIDGET:
            case PARAM_TYPE_INT_INCR:
            case PARAM_TYPE_BOOL_RELOAD_WIDGET_OC:
                break;
            case PARAM_TYPE_BOOL:
                emit_param_set( param_id, p.getBool( key_buffer ) ? "true" : "false" );
                break;
            case PARAM_TYPE_INT:
            case PARAM_TYPE_FLOAT:
                const char* buffer = param_type == PARAM_TYPE_INT 
                                     ? int_to_char( p.getInt( key_buffer ) ) 
                                     : float_to_char( p.getFloat( key_buffer ) );
                emit_param_set( param_id, buffer );
                delete[] buffer;
                buffer = nullptr;
                break;

        }
        delete[] key_buffer;
        key_buffer = nullptr;
    }
    p.end();
    return true;
}
// save specific paramater to NVS or save all parameters if no param_id is passed
bool PhantomUI::save_current_to_nvs( int _param_id ){

    const std::function<void(int param_id, setting_cache_object *package)> update_param = [](int param_id, setting_cache_object *package) {
        if( 
            package == nullptr ||
            settings_file_param_types.find( param_id ) == settings_file_param_types.end()
        ){ return; }

        Preferences p;
        p.begin(RELEASE_NAME, false);
        const char* key_buffer = int_to_char( param_id );
        switch( settings_file_param_types[param_id] ){
            case PARAM_TYPE_BOOL:
                    p.putBool( key_buffer, package->boolValue );
                break;
            case PARAM_TYPE_INT:
                    p.putInt( key_buffer, package->intValue );
                break;
            case PARAM_TYPE_FLOAT:
                    p.putFloat( key_buffer, package->floatValue );
                break;
            default:
                break;
        }
        delete[] key_buffer;
        key_buffer = nullptr;
        p.end();
    };


    if( _param_id != -1 ){
        //ui.emit(LOG_LINE,"Updatingparam");
        bool change = true;
        if( settings_values_cache.find( _param_id ) == settings_values_cache.end() ){
            // no cached value found
            // load param
            param_load_raw( _param_id );
            if( settings_values_cache.find( _param_id ) != settings_values_cache.end() ){
                // still not found
                change = false;
            }
        }
        if( change ){
            update_param( _param_id, &settings_values_cache.find( _param_id )->second );
        }
    } else { 

        param_loop( update_param ); 
        Preferences p;
        p.begin(RELEASE_NAME, false);
        p.putBool( "has_settings", true );
        p.end();

    }


    return true;
}













// takes a filename without extension and path
bool PhantomUI::save_settings_writeout( std::string file_name ){
    //return true;
    std::string file_path = SD_SETTINGS_FOLDER;
    file_path += "/";
    file_path += file_name;
    file_path += ".conf";
    //ui.emit(LOG_LINE,file_path.c_str());
    SD_CARD.delete_file( file_path.c_str() ); // delete the settings file if it already exists
    if( SD_CARD.begin( file_path.c_str(), FILE_WRITE ) ){
        param_loop([](int param_id, setting_cache_object *package){ // iterate over saveable settings
            std::string key_value_str = "";
            std::string key_str       = intToString( param_id );
            key_value_str += key_str.c_str();
            key_value_str += ":";
            switch( settings_file_param_types[param_id] ){
                case PARAM_TYPE_NONE:
                case PARAM_TYPE_STRING:
                case PARAM_TYPE_NONE_REDRAW_PAGE:
                case PARAM_TYPE_BOOL_RELOAD_WIDGET:
                case PARAM_TYPE_INT_RAW_RELOAD_WIDGET:
                case PARAM_TYPE_INT_INCR:
                case PARAM_TYPE_NONE_REDRAW_WIDGET:
                case PARAM_TYPE_BOOL_RELOAD_WIDGET_OC:
                    break;
                case PARAM_TYPE_BOOL:
                    key_value_str += package->boolValue ? "true" : "false";
                    SD_CARD.add_line( key_value_str.c_str() );
                    break;
                case PARAM_TYPE_INT:
                    key_value_str += intToString( package->intValue );
                    SD_CARD.add_line( key_value_str.c_str() );
                    break;
                case PARAM_TYPE_FLOAT:
                    key_value_str += floatToString( package->floatValue );
                    SD_CARD.add_line( key_value_str.c_str() );
                    break;
            }
        });
        SD_CARD.end();
        return true;
    } else { 
        ui.emit(LOG_LINE,"Failed to open file for write"); 
    }
    return false;
}


//###################################################################
// Save settings to SD card with auto generated name or custom name
//###################################################################
bool PhantomUI::save_settings( sd_write_action_type action ){

    std::string file_name;
    std::string file_path;
    std::string file_name_no_ext;

    try {
        file_path = SD_CARD.generate_unique_filename();
        file_name = SD_CARD.get_filename_from_path( file_path );
    } catch ( const std::exception& e ){
        // looped out of max range
        ui.emit(LOG_LINE,"Failed to create filename");
        return false;
    }

    return build_alpha_keyboard( remove_extension( file_name ) );

}

void PhantomUI::set_settings_file( const char* file_path ){
    ui.settings_file           = file_path;
    ui.load_settings_file_next = true;
}

void PhantomUI::set_gcode_file( const char* file_path ){
    bool update = false;
    if( ui.gcode_file != file_path ){
        update = true;
    }
    ui.gcode_file = file_path;
    if( !update ){ return; }
    ui.emit_param_set( PARAM_ID_SD_FILE_SET, strlen( file_path ) > 0?"true":"false" );
}

// requires full path to file not just the name
void PhantomUI::load_settings( std::string path_full ){

    if( SD_CARD.begin_multiple( path_full, FILE_READ ) ){

        // file opened and sd state locked
        const int buffer_size = 256;
        char buffer[buffer_size];
        // iterate over the lines from the file
        // each line contains a key:value pair
        int param_id;
        while( SD_CARD.get_line( buffer, buffer_size, false ) ){
            auto kvpair = split_key_value( buffer );
            if( kvpair.first > -1 ){
                param_id        = kvpair.first;
                auto param_type = settings_file_param_types[param_id]; 
                switch( param_type ){
                    case PARAM_TYPE_NONE:
                    case PARAM_TYPE_STRING:
                    case PARAM_TYPE_NONE_REDRAW_PAGE:
                    case PARAM_TYPE_NONE_REDRAW_WIDGET:
                    case PARAM_TYPE_BOOL_RELOAD_WIDGET:
                    case PARAM_TYPE_INT_RAW_RELOAD_WIDGET:
                    case PARAM_TYPE_INT_INCR:
                    case PARAM_TYPE_BOOL_RELOAD_WIDGET_OC:
                    break;
                    case PARAM_TYPE_BOOL:
                        emit_param_set( param_id, kvpair.second.c_str() );
                        break;
                    case PARAM_TYPE_INT:
                    case PARAM_TYPE_FLOAT:
                        emit_param_set( param_id, kvpair.second.c_str() );
                        break;
                }
            }
        } 

        SD_CARD.end();
        ui.emit(LOG_LINE,"Settingsfile loaded");
        ui.emit(LOG_LINE,path_full.c_str());

    } else {
    
        ui.emit(LOG_LINE,"Failed to load settings from SD");
    
    }

}

