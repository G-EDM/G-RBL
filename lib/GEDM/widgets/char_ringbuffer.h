#ifndef PHANTOM_UI_RINGBUFFER
#define PHANTOM_UI_RINGBUFFER

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

#include <iostream>
#include <cstring>


static const int total_log_items = 5;
static const int log_item_size   = 256;

class CharRingBuffer {

    public:

        CharRingBuffer( void ){}

        IRAM_ATTR void create_ringbuffer(size_t __capacity){
            capacity    = __capacity;
            index       = 0;
            index_work  = index;
            has_change  = true;
            cringbuffer = new char*[capacity];
            for (size_t i = 0; i < capacity; ++i) {
                cringbuffer[i] = new char[log_item_size];
                memset(cringbuffer[i], 0, log_item_size);
            }
        }
        void delete_ringbuffer(){
            for (size_t i = 0; i < capacity; ++i) {
                delete[] cringbuffer[i];
            }
            //delete[] buffer;
        }
        ~CharRingBuffer() {
            for (size_t i = 0; i < capacity; ++i) {
                delete[] cringbuffer[i];
            }
            delete[] cringbuffer;
        }

        IRAM_ATTR size_t get_buffer_size(){
            return capacity;
        }

        IRAM_ATTR bool has_new_items(){
            return has_change;
        }

        IRAM_ATTR void add_line(const char* line) {
            strncpy(cringbuffer[index], line, log_item_size-1);
            cringbuffer[index][log_item_size-1] = '\0';
            if( ++index == capacity ){
                index = 0;
            }
            has_change = true;
        }

        IRAM_ATTR void reset_work_index(void){
            index_work = index; // latest index points already to the future element
            if( index_work <= 0 ){ // move work index one back to the last written line
                index_work = capacity-1;
            } else {
                --index_work;
            }
            has_change = false;
        }
        IRAM_ATTR const char* pop_line(void){
            int _index = index_work;
            if( index_work == 0 ){
                index_work = capacity-1;
            } else {
                --index_work;
            }
            if( cringbuffer[_index] != nullptr && cringbuffer[_index][0] != '\0' ){
                return cringbuffer[_index];
            }
            return "";
        }

        IRAM_ATTR const char* getLine(int index) const {
            return cringbuffer[index];
        }

    private:
        char** cringbuffer;
        size_t capacity;
        size_t index;
        size_t index_work;
        bool   has_change;

};


#endif