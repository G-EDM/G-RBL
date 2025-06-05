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
// Sinker and Wire EDM firmware 
//
// How to install:
//
// First Install:
//     1. Install visual studio code and open it. Go to extensions and install platform.io
//     2. On windows install the USB to UART driver if needed
//     3. Press open folder and load the project folder. It may take some time until everything is setup. 
//        Restart vstudio once it is finished.
//     4. Connect the ESP via USB. On Linux it may be required to use "chmod 777 /dev/ttyUSB0" 
//        to gain write access to the ESP. ttyUSB0 could be another address too. 
//     5. Press the upload button on the top right and wait until it is finished.
//
// Updating:
//     1. Press the build button. It is at the same location as the upload button. Upload and Build can 
//        can be toggled with the small drop down menu.
//        vStudio will generate a firmware.bin file that should be located within the project folder at
//        /.pio/build/esp32dev/firmware.bin
//     2. Copy the firmware.bin file on the sd card at the top directory. Not into any folder.
//     3. Insert the SD card. Once the sd card is detected it will search for an update. Wait until the update
//        is done and the ESP will restart.
//
//############################################################################################################

//################################################
// Main includes
//################################################
#include <driver/adc.h>
#include "gedm_main.h"
#include "config/definitions.h"

//################################################
// GRBL_ESP32
//################################################
#include "Grbl.h"

TaskHandle_t ui_task_handle  = NULL;
TaskHandle_t protocol_task   = NULL;

DMA_ATTR volatile bool sensor_services_set = false;


void start_services(){
  if( sensor_services_set ){ return; }
  sensor_services_set = true;
  ui_controller.run_sensor_service();
  ui_controller.start_ui_task();
}

void setup() {
    Serial.begin( DPM_RXTX_SERIAL_COMMUNICATION_BAUD_RATE );
    while(!Serial);
    disableCore0WDT();
    disableCore1WDT();
    grbl.grbl_init();
    ui_controller.configure();
    start_services();
    ui_controller.set_is_ready();
    grbl.reset_variables();
    xTaskCreatePinnedToCore( protocol_main_loop, "protocol_task", 15000, &gproto, TASK_PROTOCOL_PRIORITY, &protocol_task, 1 );
    i2sconf.I2S_FORCE_RESTART = true;
}


void loop(){ vTaskDelete(NULL); }

