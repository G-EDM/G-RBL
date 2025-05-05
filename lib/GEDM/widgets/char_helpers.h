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

#ifndef PHANTOM_UI_CHARHELPER
#define PHANTOM_UI_CHARHELPER

#include <esp_attr.h>
//#include <cstring>
//#include <vector>
//#include <cstdlib>
//#include <iomanip>
//#include <cctype>
//#include <cstdio>

IRAM_ATTR std::string floatToString( float value );
IRAM_ATTR std::string intToString( int value );
IRAM_ATTR std::string float_to_string( float value, int precision );
// Warning: use delete[] after done with the result or it will leak memory
IRAM_ATTR const char* int_to_char( int value );
IRAM_ATTR const char* float_to_char( float value );
IRAM_ATTR bool        char_to_bool( const char* value );
IRAM_ATTR float       char_to_float( const char* value );
IRAM_ATTR int         char_to_int( const char* value );

bool path_has_extension( const char* path, const char* extension );
std::string remove_extension( const std::string& filePath );


std::pair<int, std::string> split_key_value( char* input );


#endif