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

#ifndef PHANTOM_GAXIS
#define PHANTOM_GAXIS

#include <stdint.h>


class float_member {
    public:
        float_member(){};
        ~float_member(){};
        float get(){
            return __value;
        };
        bool set( float value ){
            __value = value;
            return true;
        };
    private:
        float __value;
};

class int_member {
    public:
        int_member(){};
        ~int_member(){};
        int get(){
            return __value;
        };
        bool set( int value ){
            __value = value;
            return true;
        };
    private:
        int __value;
};



class GAxis {
    public:
        GAxis( const char * __axis_name) : axis_name(__axis_name ){};
        ~GAxis(){};
        const char*  axis_name;
        int_member   steps_per_mm;
        float_member max_travel_mm;
        float_member home_position;
};


#endif