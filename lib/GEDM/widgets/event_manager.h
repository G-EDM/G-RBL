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

#ifndef PHANTOM_UI_EVENTMNG
#define PHANTOM_UI_EVENTMNG

#include <vector>
#include <functional>
#include <unordered_map>
#include <type_traits>
#include <esp_attr.h>
//template<typename T>
class EventManager {
    public:
        using Callback = std::function<void(void*)>;

        template<typename EventType>
        IRAM_ATTR EventManager& on(const int event_id, std::function<void(EventType)> callback) {
            callbacks[event_id].emplace_back(
                [callback](void* data) {
                    callback(*static_cast<EventType*>(data));
                }
            );
            return *this;
        }
        template<typename EventType>
        IRAM_ATTR void emit(const int& event_id, EventType* eventData) {
            auto it = callbacks.find(event_id);
            if (it != callbacks.end()) {
                for (const auto& callback : it->second) {
                    //callback(static_cast<void*>(eventData));
                    callback(static_cast<void*>(&eventData));
                }
            }
        }

        void clear_events( void ) {
            callbacks.clear();
        }

        virtual ~EventManager(){
            for( auto& callback : callbacks ){
                callbacks.erase( callback.first );
            }
            callbacks.clear();
        };
        //virtual T get_parent(){ return T(); }

    private:
        std::unordered_map<int, std::vector<Callback>> callbacks;
};

#endif