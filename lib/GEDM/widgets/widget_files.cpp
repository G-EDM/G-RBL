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

#include "widget_files.h"
#include "ui_interface.h"
#include "sd_card.h"
#include "char_helpers.h"

file_browser::~file_browser(){
    SD_CARD.end();
}

void file_browser::create(){
    sd_success    = false;
    index_counter = 0;
}

void draw_single_item( TFT_eSPI* display, const char* str, int16_t w, int16_t h, int16_t x, int16_t y ){
    display->drawFastHLine( x, y+h-1, w, BUTTON_LIST_BG );vTaskDelay(1); //draw separator
    int16_t y_offset = int16_t( ( h - display->fontHeight() ) / 2 );
    display->drawString( str, x, y+y_offset );
}
void file_browser::set_page_title( std::string str ){
    title = str;
}
void file_browser::set_num_items( int8_t num ){
    items_per_page = num;
}

void file_browser::set_folder( std::string str ){
    folder = str;
}

bool file_browser::start(){
    results_success = 0;
    SD_CARD.reset_current_file_index( 0 );
    load_file_batch( true );
    return false;
}

void file_browser::set_file_type( int type ){
    file_type = type;
}

int file_browser::get_file_type(){
    return file_type;
}

void file_browser::addItemCustom( const char* file_name, int file_index ){ // currently needed as the files don't use a normal 0-xxx order

    auto __this = std::dynamic_pointer_cast<file_browser>(shared_from_this());
    std::weak_ptr<file_browser> weak_this = __this;
    const std::function<void(PhantomWidgetItem *item)> touch_callback = [weak_this](PhantomWidgetItem *item) {
        if (auto __this = weak_this.lock()) {
            if( __this->get_file_type() == 0 ){
                ui.set_settings_file( item->strValue.c_str() );
            } else {
                ui.set_gcode_file( item->strValue.c_str() );
            }
            ui.show_page_next( ui.get_previous_active_page() );
        }
    };

    PhantomWidgetItem item = PhantomWidgetItem();
    item.item_index    = file_index;     // index for this item in the map
    item.label         = file_name;     // string label for the item
    item.intValue      = file_index;    // parameter id
    item.param_id      = file_index;    // parameter id
    item.strValue      = folder;
    item.strValue     += "/";
    item.strValue     += file_name;
    item.touch_enabled = true;          // no touch event
    item.width         = width;         // with of the item
    item.height        = round( float(height-30) / float(items_per_page) ); // height of the item
    item.xpos          = 0;             // position x
    item.ypos          = 0;             // position y
    item.font          = DEFAULT_FONT;  // font
    item.output_type   = OUTPUT_TYPE_CUSTOM; // output type of the data drawn on the display
    item.valueType     = PARAM_TYPE_INT;     // datatype of the value 
    item.set_parent_page_tab_id( tab_id );
    item.set_widget_id( index );
    item.set_parent_page_id( page_id ); // add page id to item. if the widget is not in a page container yet it will not assign a valid page but will do so once the widget is added to a page
    item.set_colors( 0, -1, TFT_LIGHTGREY );
    item.set_colors( 1, -1, TFT_ORANGE );
    item.set_colors( 2, -1, TFT_ORANGE );
    item.set_colors( 3, -1, TFT_DARKGREY );
    item.on(TOUCHED, touch_callback);
    items[file_index] = item;           // add item to the map
    ++num_items;

}

bool file_browser::touch_handler( int16_t item_index ){
    if( items[item_index].param_id == 41 ){ // close
        return true;
    } 
    return false;
}

bool file_browser::load_file_batch( bool forward ){
    //file_browser & __this = *this;
    auto __this = std::dynamic_pointer_cast<file_browser>(shared_from_this());
    std::weak_ptr<file_browser> weak_this = __this;
    const int tracker_id = 0;
    // reset items
    int items_in_map = items.size();
    items.clear();
    num_items = 0;
    bool success = SD_CARD.begin_multiple( folder, FILE_READ );
    if( success ){
        if( forward ) { 
            SD_CARD.go_to_last( tracker_id ); 
        } else {
            SD_CARD.decrement_tracker( tracker_id, items_per_page+items_in_map );
            SD_CARD.go_to_last( tracker_id ); 
        }
        const std::function<void(const char* file_name, int file_index )> item_callback = [weak_this](const char* file_name, int file_index ){
            if (auto __this = weak_this.lock()) {
                if( strcmp(file_name, "") != 0 && sizeof( file_name ) > 0 ){ 
                    __this->addItemCustom( file_name, file_index );
                }
            }
        };
        for( int i = 0; i < items_per_page; ++i ){
            if( forward ){
                SD_CARD.get_next_file( tracker_id, item_callback );
            } else {
                SD_CARD.get_next_file( tracker_id, item_callback );
            }
        }
        SD_CARD.end();
        return true;
    }
    return false;
}

IRAM_ATTR void file_browser::redraw_item( int16_t item_index, bool reload ){
    if( ! is_visible ){ return; }
    TFT_eSPI* display = &canvas;
    canvas.createSprite( items[item_index].width, items[item_index].height );vTaskDelay(1);
    int bg_color, fg_color;
    set_item_colors( item_index, bg_color, fg_color );
    items[item_index].set_font( *display ); 
    canvas.setTextColor(fg_color);
    int16_t y_offset = int16_t( ( items[item_index].height - display->fontHeight() ) / 2 );
    canvas.drawNumber( items[item_index].intValue,      items[item_index].xpos,    y_offset );
    canvas.drawString( items[item_index].label.c_str(), items[item_index].xpos+35, y_offset );
    canvas.drawFastHLine( items[item_index].xpos, items[item_index].height-1, items[item_index].width, BUTTON_LIST_BG );
    canvas.pushSprite(items[item_index].truex,items[item_index].truey);vTaskDelay(1);
    canvas.deleteSprite();vTaskDelay(1);
};

void file_browser::show(){
    if( ! is_visible ){ return; }
    TFT_eSPI* display = &tft; // only sprite is supported for this widget right now...
    int16_t current_y = ypos;
    display->setTextFont( DEFAULT_FONT );
    display->setTextSize( 1 );
    display->setTextColor( TFT_DARKGREY );
    int16_t y_offset  = int16_t( ( 30 - display->fontHeight() ) / 2 );
    display->drawString( title.c_str(), xpos, current_y+y_offset );
    display->setTextColor( TFT_LIGHTGREY );
    current_y += 30;
    display->fillRect( xpos, current_y, width, height-30, TFT_BLACK );
    int bg_color, fg_color;
    if( items.size() <= 0 ){
        display->drawString( "No files found", xpos+35, current_y+y_offset );
    } else {
        for( auto& item : items ){
            set_item_colors( item.first, bg_color, fg_color );
            canvas.setTextColor(fg_color);
            item.second.ypos  = 0;
            item.second.xpos  = 0;
            item.second.truex = xpos;
            item.second.truey = current_y;
            y_offset          = int16_t( ( item.second.height - display->fontHeight() ) / 2 );
            display->drawNumber( item.second.intValue,      xpos,    current_y+y_offset );
            display->drawString( item.second.label.c_str(), xpos+35, current_y+y_offset );
            display->drawFastHLine( xpos, current_y+item.second.height-1, item.second.width, BUTTON_LIST_BG );
            current_y += item.second.height;
        }

    }
    ui.reload_page_on_change();
}