//############################################################################################################
//
//  ██████        ███████ ██████  ███    ███  
// ██             ██      ██   ██ ████  ████  
// ██   ███ █████ █████   ██   ██ ██ ████ ██ 
// ██    ██       ██      ██   ██ ██  ██  ██ 
//  ██████        ███████ ██████  ██      ██ 
//
// This is a beta version for testing purposes.
// Only for personal use. Commercial use or redistribution without permission is prohibited. 
// Copyright (c) Roland Lautensack    
//
//############################################################################################################


//############################################################################################################
// If the display does not react to touch input maybe the calibration was messed up. To force a recalibration
// touch the display and restart the ESP. Keep touching the display while the ESP boots up. The display
// should turn black and the recalibration is enabled. Once untouched it will start. Calibration starts
// with the top left corner. If this is not the case a false touch was triggered and the calibration needs
// to be repeated.
// Short: Recalibration can be enforced by touching the display on power up.
//############################################################################################################


//############################################################################################################
//
// Personal todo list
//
// Validate reamer mode
//
//############################################################################################################


#include "ili9341_tft.h"
#include "Motors/Motors.h"
#include "Protocol.h"
#include "gedm_dpm_driver/gedm_dpm_driver.h"
#include "gpo_scope.h"
#include "widgets/ui_interface.h"

#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include <assert.h>
#include <string.h>


TaskHandle_t      drill_task_handle             = NULL;
DRAM_ATTR int64_t last_minimal_ui_update_micros = esp_timer_get_time();

G_EDM_UI_CONTROLLER ui_controller;

G_EDM_UI_CONTROLLER::G_EDM_UI_CONTROLLER() {
    change_sinker_axis_direction(0);
}

void G_EDM_UI_CONTROLLER::start_ui_task(){
  xTaskCreatePinnedToCore(ui_task, "ui_task", UI_TASK_STACK_SIZE, this, TASK_UI_PRIORITY, &ui_task_handle, UI_TASK_CORE);
}


void G_EDM_UI_CONTROLLER::restart_i2s_enforce(){
    int data = 16;
    if( sensor_queue_main != NULL ){ 
        xQueueSend( sensor_queue_main, &data, portMAX_DELAY );
    }
}

/** runs once after the ui is ready **/
void G_EDM_UI_CONTROLLER::run_once_if_ready(){
  for( int i = 0; i < N_AXIS; ++i ){
    //max_feeds[i] = process_speeds_mm_min.RAPID;
  }
  planner.init();
  api::autohome_at_center(); // force machine to homed at center position of each axis on startup
  gcode_core.gc_sync_position();
  api::unlock_machine();
  api::reset_probe_points(); 
  reset_defaults();
  api::push_cmd("G91 G10 L20 P1 X0Y0Z0\r\n"); // reset work coordinates
  setup_spindle(GEDM_A_DIRECTION_PIN, GEDM_A_STEP_PIN);
  planner.position_history_reset();
  planner.update_max_feed();
  api::update_speeds();
  generate_setpoint_min_max_adc();
}

void G_EDM_UI_CONTROLLER::configure(){
    is_ready               = false;
    has_gcode_file         = false;
}


// loader screen
// checking for firmware
// update if
// dpm check
void G_EDM_UI_CONTROLLER::init(){
    start_time = esp_timer_get_time();
    disable_spark_generator();
    ui.build_phantom();
}

void G_EDM_UI_CONTROLLER::alarm_handler(){
    edm_process_is_running = false;
    vTaskDelay(1);
    if (enable_spindle){ stop_spindle(); }
    disable_spark_generator();
    vTaskDelay(200);
    int error_code    = static_cast<int>( sys_last_alarm );
    sys_rt_exec_alarm = ExecAlarm::None;
    sys.abort         = true;
    gproto.protocol_exec_rt_system();
    ui.emit(ALARM,&error_code);
    reset_after_job();
    gproto.protocol_exec_rt_system();
    vTaskDelay(1);
}

void clean_reboot(){
    ui_controller.sensor_end();
    Serial.end();
    ESP.restart();
}


#define MPOS_UPDATE_INTERVAL 200000
DRAM_ATTR int64_t last_mpos_update = 0; // micros
DRAM_ATTR volatile float last_positions[N_AXIS] = {};
IRAM_ATTR bool position_changed(){ // todo: build a flag triggered from the stepper class instead of looping and comparing the position..
    int64_t micros = esp_timer_get_time();
    if( micros - last_mpos_update < MPOS_UPDATE_INTERVAL ){
        return false;
    }
    last_mpos_update = micros;
    bool changed = false;
    float* current_position = system_get_mpos();
    for( int i = 0; i < N_AXIS; ++i ){
        if( last_positions[i] != current_position[i] ){
            changed = true;
            last_positions[i] = current_position[i];
            break;
        }
    }
    return changed;
}



void IRAM_ATTR emit_process_error(){
    std::string text;
    switch ( gconf.process_error ){
        case 1:
            text = "*** Check wire ***";
            break;
        case 2:
            text = "*** Shorted ***";
            break;
        case 3:
            text = "*** Shorted ***";
            break;     
        case 4:
            text = "*** Motionplan timeout ***";
            break; 
        case 5:
            text = "*** Retract limit ***";
            break; 

        default:
            text = "*** Unknown error ***";
            break;
    }
    ui.emit( LOG_LINE, text.c_str() );
    gconf.process_error = 0; // reset
    vTaskDelay(1);
}

/**
 * Starting the UI on core 0
 **/
void IRAM_ATTR ui_task(void *parameter){

    G_EDM_UI_CONTROLLER *ui_controller = (G_EDM_UI_CONTROLLER *)parameter;

    int dummy_item = 0; // used for the emit() function as dummy...

    ui_controller->init();

    //start_time = esp_timer_get_time();
    //disable_spark_generator();
    //ui.build_phantom();

    vTaskDelay(100);

    // wait until ui is ready
    while (!ui_controller->get_is_ready()){
      vTaskDelay(100);
    }
    api::update_speeds();
    ui_controller->set_pwm_pin( GEDM_PWM_PIN );
    ui_controller->setup_pwm_channel();
    ui_controller->change_pwm_frequency( ( int ) pwm_frequency );
    ui_controller->update_duty( pwm_duty );
    ui_controller->disable_spark_generator();

    dpm_driver.setup( Serial );
    dpm_driver.init();

    G_EDM_SENSORS::edm_start_stop();
    ui_controller->setup_events();

    force_redraw       = false;
    int64_t time_since = 0;

    ui.start();

    if( enable_dpm_rxtx ){
        ui.emit(LOG_LINE,"Waiting for DPM connection");
        vTaskDelay(100);
        if(! dpm_driver.power_on_off( false, 0 ) ){
          ui.emit(LOG_LINE,"DPM not available!");
          vTaskDelay(500);
        }
    } else {
        ui.emit(LOG_LINE,"DPM support not enabled");
    }

    int delay = 100;
    ui.emit(LOG_LINE,"//####################");
    ui.emit(LOG_LINE,"// Credits");
    ui.emit(LOG_LINE,"// to the community");
    ui.emit(LOG_LINE,"//####################");vTaskDelay(300);
    ui.emit(LOG_LINE,"@ Tanatara");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ 8kate");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ Ethan Hall");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ Esteban Algar");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ 666");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ td1640");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ MaIzOr");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ gunni");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ DANIEL COLOMBIA");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ charlatanshost");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ tommygun");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ Nikolay");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ renyklever");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ Zenitheus");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ gerritv");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ cnc");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ Shrawan Khatri");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ Alex Treseder");vTaskDelay(delay);
    ui.emit(LOG_LINE,"@ VB");vTaskDelay(1000);



    /*ui.emit(LOG_LINE,"");
    ui.emit(LOG_LINE,"G-EDM");
    ui.emit(LOG_LINE,"Phantom");
    ui.emit(LOG_LINE,"v1.0");
    ui.emit(LOG_LINE,"");

    int page = PAGE_FRONT;
    ui.emit(SHOW_PAGE,&page);*/

    ui_controller->init_adc();

    int data = 0;
    for( int i = 0; i < 3; ++i ){
        ++data;
        xQueueSendFromISR( sensor_queue_main, &data, NULL );
    }

    if( gconf.flag_restart == true ){
        clean_reboot();
    }

    ui_controller->run_once_if_ready();

    position_changed();
    force_redraw = false;
    ui.emit(LOG_LINE,"");
    ui.emit(LOG_LINE,"G-EDM");
    ui.emit(LOG_LINE,"Phantom");
    ui.emit(LOG_LINE,"v1.0");
    ui.emit(LOG_LINE,"");
    int page = PAGE_FRONT;
    ui.emit(SHOW_PAGE,&page);


    for (;;){

      if( gconf.flag_restart == true ){
          clean_reboot();
          vTaskDelay(1);
          continue;
      }

      if( ui_controller->is_probing() ){ // the normal process will never come here since probing is a blocking function inside this task. But let it be.
        vTaskDelay(1000);
        continue;
      }


      if( sys.state == State::Alarm ) {
          ui_controller->alarm_handler();
      }

      if (!edm_process_is_running){
          
          //######################################
          // Not in EDM process
          //######################################

          if( force_redraw ){
              ui.emit(RELOAD,&dummy_item);
              force_redraw = false;
          }

          if( 
            ui_controller->active_page == PAGE_FRONT && // start only if ui shows the frontpage
            ui_controller->ready_to_run()               // start only if everything else is ready too (this is also called in the process where the page is not the frontpage)
          ){

              // everything ready to run
              ui_controller->start_process();
              
          }

          if( position_changed() ){ // todo: check if position request is ongoing and send only if ui is not already informed
              ui.emit(RELOAD_ON_CHANGE,&dummy_item); // little wastefull
          }

          ui.emit(EVENT_TOUCH_PULSE,&dummy_item); // should be at the end; if stuff like factory reset is called that flags the restart variable we want to avoid any more stuff going on

    } else {

          //##########################################
          // In EDM process; slower refresh rates etc.
          //##########################################
          

          if ( // break condition
                !ui_controller->start_edm_process() 
                //|| sys_rt_exec_alarm != ExecAlarm::None 
                || gconf.edm_process_finished // indicates a clean finish // at least it should...
                || api::machine_is_idle()
          ){

              ui.emit(END_SD_CARD_JOB,&dummy_item); // doesn't need any data.. just use the page.. Doesn't matter if it was a gcode job or not. Just send it anyway

              while( gconf.gedm_planner_line_running || !api::machine_is_idle() ){
                if( sys.abort || sys_rt_exec_state.bit.motionCancel || gconf.gedm_stop_process ){
                    break;
                } else { vTaskDelay(100); }
              }

              ui_controller->reset_after_job();

              if( gconf.edm_process_finished && operation_mode==MODE_SINKER ){

                  //ui_controller->move_sinker_axis_to_zero();
                  api::push_cmd("G90 G54 G21\r\n"); 

              } else{ 

                api::push_cmd("G90 G54 G21\r\n"); 

              }

              vTaskDelay(100);
              api::block_while_not_idle(); // very ugly polling, should be replaced with a wait queue #todo
              gconf.edm_process_finished = false;

          } else {

              //########################################################
              // Active process still running
              //########################################################
              if( ui_controller->active_page != PAGE_IN_PROCESS ){ // could be the settings page etc
                  ui.emit(EVENT_TOUCH_PULSE,&dummy_item); // if the keyboard is used it needs this touch monitor
                  vTaskDelay(1);
                  continue;
              }

              if( gconf.edm_pause_motion && gconf.process_error != 0 ){
                  emit_process_error();
              }

              time_since = esp_timer_get_time() - last_minimal_ui_update_micros;
              if( force_redraw ){
                  force_redraw = false;
                  ui.wpos_update(); // little wastefull
                  vTaskDelay(1);
                  continue;
              }

              if( gconf.edm_pause_motion || time_since > 280000 ){ //200ms = 100*1000 = 200000uS
                  ui.process_update();
                  if( position_changed() || gconf.edm_pause_motion ){ // todo: check if position request is ongoing and send only if ui is not already informed
                      ui.wpos_update();
                  }
                  last_minimal_ui_update_micros = esp_timer_get_time();
              }

              if( !gconf.edm_pause_motion ){
                  ui.scope_update();
              }

          }

    }

    vTaskDelay(1);

  }

  vTaskDelete(NULL);

}




//#########################################################
// enables DPM, resets some stuff, pushes break to planner
// loads process page etc.
// shared across all modes before the process starts
//#########################################################
void G_EDM_UI_CONTROLLER::pre_process_defaults(){
    int page                   = PAGE_IN_PROCESS;
    gconf.edm_process_finished = false;
    motion_plan                = 0;
    if( enable_dpm_rxtx ){
        if( !dpm_driver.power_on_off( 1, 3 ) ){
            // problem... unhandled yet
        }// turn on
        vTaskDelay(200);
    }
    planner.push_break_to_position_history();
    api::unlock_machine();
    ui.emit(SHOW_PAGE, &page );    // show the process page
    edm_process_is_running = true; // flag process is running
    generate_reference_voltage();  
}

//##################################################
// Move sinker axis to zero position after job
//##################################################
bool G_EDM_UI_CONTROLLER::move_sinker_axis_to_zero(){
    int sinker_axis = change_sinker_axis_direction( single_axis_mode_direction );
    if( ! sys_probed_axis[sinker_axis] ){ return false; } // if axis is not prbed return, should not happen
    char command_buf[40];
    sprintf( command_buf, "G90 G0 %c%.5f F30\r\n", get_axis_name(sinker_axis), 0.0 ); // to wpos 0
    api::push_cmd(command_buf);
    return true;
}

//##################################################
// Prepare 2D wire specific stuff
//##################################################
void G_EDM_UI_CONTROLLER::wire_gcode_task(){
    ui.emit(LOG_LINE,"Starting 2D wire job");
    motor_manager.set_ignore_disable(true);
    planner.set_cutting_depth_limit( 0.0 ); // normally not needed but reset the z limit anyway
    api::push_cmd("G90 G54\r\n");
    planner.configure();
    api::push_cmd("M9\r"); 
    vTaskDelay(500);
    // set wire speed
    set_speed( wire_spindle_speed );
    if ( enable_spindle && ! simulate_gcode ){
        // spindle should always run in wire edm to pull the wire
        // but for testing it may be useful to allow the spindle to be off
        ui.emit(LOG_LINE,"Spindle bootup");
        start_spindle();
        ui.emit(LOG_LINE,"Spindle bootup done");
    }
    pwm_on();
}


//##################################################
// 1D Drill/Sinker mode
// Setup everything and push the line to the buffer
//##################################################
void G_EDM_UI_CONTROLLER::sinker_drill_single_axis(){

    ui.emit(LOG_LINE,"Starting sinker job");
    
    int    sinker_axis                = change_sinker_axis_direction( single_axis_mode_direction );
    bool   sinker_axis_move_negative  = true;
    float  cutting_depth              = sinker_travel_mm;
    float* current_position           = system_get_mpos();
    float  ratraction_limit[MAX_N_AXIS];

    api::unset_probepositions_for(sinker_axis); // unset the probe position for the sinker axis

    // get the min max positions
    float negative_travel_max = glimits.limitsMinPosition(sinker_axis); //-100
    float positive_travel_max = glimits.limitsMaxPosition(sinker_axis); // 0 homing pulloff not included here
    positive_travel_max      -= DEFAULT_HOMING_PULLOFF;                 // add the homing pulloff to prevent touching the switch at 0
    negative_travel_max      += 0.1;                                    // remove a few steps from the max travel just in case

    memcpy( ratraction_limit, current_position, sizeof(current_position[0]) * N_AXIS );

    motor_manager.set_ignore_disable(true); // more or less deprecated. Motors are enabled all the time without any timer to disable them; was used way back to prevent the motors from getting disabled after some time

    pre_process_defaults();

    if( !simulate_gcode ){
        if (enable_spindle){
            set_speed(wire_spindle_speed);
            start_spindle();
        }
        pwm_on();
        probe_prepare();
        //probe_mode_on();
        generate_reference_voltage();
        switch (single_axis_mode_direction){
            case 0: // SINK_Z_NEG
                ui.emit(LOG_LINE,"Probe Z negative");
                probe_z(-1.0); // z axis down 
                break;
            case 1: // SINK_X_POS
                ui.emit(LOG_LINE,"Probe X positive");
                probe_x(1.0); // x positive
                sinker_axis_move_negative = false;
                break;
            case 2: // SINK_X_NEG
                ui.emit(LOG_LINE,"Probe X negative");
                probe_x(-1.0); // x negative
                break;
            case 3: // SINK_Y_NEG
                ui.emit(LOG_LINE,"Probe Y negative");
                probe_y(-1.0); // y negative
                break;
            case 4: // SINK_Y_POS
                ui.emit(LOG_LINE,"Probe Y positive");
                probe_y(1.0); // y positive
                sinker_axis_move_negative = false;
                break;
        }
        if( !sys_probed_axis[sinker_axis] ){
            // probing failed; fallback to enforce probe positions at current position
            ui.emit(LOG_LINE,"Probe failed. Fallback to current.");
            api::auto_set_probepositions();
        }
        probe_done();
        //probe_mode_off();
    } else {
        ui.emit(LOG_LINE,"Sinker job simulation");
        if( 
             single_axis_mode_direction == 1 // SINK_X_POS
          || single_axis_mode_direction == 4 // SINK_Y_POS
        ){
            sinker_axis_move_negative = false;
        }
        api::auto_set_probepositions();
        generate_reference_voltage();
    }

    vTaskDelay(500);

    float axis_target_relative = cutting_depth;

    if( sinker_axis_move_negative ){

        float travel_left_negative = api::get_max_travel_possible( sinker_axis, 1 );

        axis_target_relative*=-1; // make negative

        if( axis_target_relative < travel_left_negative ){
            axis_target_relative = travel_left_negative;
        }

    } else {

        float travel_left_positive = api::get_max_travel_possible( sinker_axis, 0 );

        if( axis_target_relative > travel_left_positive ){
            axis_target_relative = travel_left_positive;
        }

    }


    planner.set_sinker_direction( sinker_axis_move_negative ? 0 : 1 );
    planner.set_cutting_depth_limit( 0.0 ); // cutting depth in the planner is only needed for floating z
    planner.configure();

    // lock the line history by pushing a break to it; while retracting it can only move back in history until a break stops it
    planner.push_break_to_position_history();

    // the sinker uses a history line and needs somewhere to retract
    // lets push the retration target to the history planner
    // if the axis needs to move back in history it will use this position as target
    ratraction_limit[sinker_axis] = sinker_axis_move_negative ? positive_travel_max : negative_travel_max;

    // planner doesn't use float positions but instead runs on the integer step positions
    // convert the retraction limit position to steps and push it to the history
    int32_t __target[MAX_N_AXIS];
    planner.convert_target_to_steps( ratraction_limit, __target );
    planner.push_to_position_history( __target, false, 0 ); // push retraction max position to the history
    planner.push_current_mpos_to_position_history();        // push current mpos to history
    planner.position_history_force_sync();                  // sync the history index

    vTaskDelay(200);
    if( enable_dpm_rxtx ){
        dpm_driver.power_on_off( 1, 5 ); // turn dpm on
    }
    pwm_on();                    // turn pwm on
    reset_flush_retract_timer(); // reset flushing interval time

    char command_buf1[40];
    char command_buf2[40];

    sprintf( command_buf1, "G90 G0 %c%.5f F30\r\n", get_axis_name(sinker_axis), 0.0 ); // to wpos 0
    sprintf( command_buf2, "G91 G1 %c%.5f F30\r\n", get_axis_name(sinker_axis), axis_target_relative );

    api::push_cmd("G54 G21\r\n"); // select coords and metric
    api::push_cmd("M7\r");        // send M7 to toggle drill/sinker mode this needs to be done after the probing stuff!
    api::push_cmd(command_buf1);  // move to probe pos
    api::push_cmd(command_buf2);  // target position

}

//######################################################################
// Reamer mode; Z axis will move up and down until stopped or timed out
// This is a task that deleted itself and runs parallel to the main task
// Only for Z axis and starts with a down motion
//######################################################################
DRAM_ATTR volatile bool task_running = false;
void IRAM_ATTR reamer_task_single_axis(void *parameter){

    G_EDM_UI_CONTROLLER *ui_controller = (G_EDM_UI_CONTROLLER *)parameter;

    if( !simulate_gcode ){
        if( enable_spindle ){
            ui_controller->set_speed(wire_spindle_speed);
            ui_controller->start_spindle();
        }
        vTaskDelay(500);
    }

    ui_controller->pwm_on();

    float* current_position = system_get_mpos();
    float  backup_position[MAX_N_AXIS];
    memcpy( backup_position, current_position, sizeof(current_position[0]) * N_AXIS );

    float travel_left_negative = api::get_max_travel_possible( Z_AXIS, 1 );

    if( reamer_travel_mm*-1 < travel_left_negative ){
        reamer_travel_mm = travel_left_negative*-1;
    }


    bool is_moving_down = false;
    char command_buf_down[40];
    char command_buf_up[40];

    sprintf( command_buf_down, "G90 G1 %c%.5f F30\r\n", get_axis_name(Z_AXIS), reamer_travel_mm ); 
    sprintf( command_buf_up,   "G90 G1 %c%.5f F30\r\n", get_axis_name(Z_AXIS), 0.0 ); 

    api::unset_probepositions_for(Z_AXIS); // unset the probe position for the sinker axis
    api::auto_set_probepositions();
    api::push_cmd("G54 G21\r\n");
    task_running = true;

    unsigned long start_time = millis();

    for (;;){

        vTaskDelay(10);

        if( ui_controller->get_pwm_is_off() ){
            ui_controller->pwm_on();
            vTaskDelay(10);
        }

        if (
            sys.abort 
            || !ui_controller->start_edm_process() 
            || ( reamer_duration > 0.0 && float( ( millis() - start_time) ) >= reamer_duration*1000.0 )
        ){ break; }

        if( gconf.gedm_planner_line_running ){

            // axis is moving. Skipping this round
            continue;

        } else {

            if( is_moving_down ){

                api::push_cmd( command_buf_up );
                is_moving_down = false;

            } else {

                api::push_cmd( command_buf_down );
                is_moving_down = true;

            }

            while( !gconf.gedm_planner_line_running ){
              vTaskDelay(10);
            }

        }

    }

    ui_controller->pwm_off();

    gconf.edm_process_finished = true;
    task_running               = false;
    vTaskDelete(NULL);

}



//####################################################################
// If the basic requirements are ok it will start the specific process
// if something is missing it will create an info screen
//####################################################################
void G_EDM_UI_CONTROLLER::start_process(void){

    info_codes msg_id = INFO_NONE;

    int dummy = 1;

    if( operation_mode == MODE_2D_WIRE ){

        //###################################
        // Check requirements for 2D wire
        //###################################
        if( !api::machine_is_fully_homed() ){
            msg_id = NOT_FULLY_HOMED;
        } else if( !has_gcode_file ){
            msg_id = SELECT_GCODE_FILE;
        }

        if( msg_id != INFO_NONE ){

            //###################################
            // Requirements not ok.
            //###################################
            disable_spark_generator();
            ui.emit( LOG_LINE, info_messages[msg_id] );
            //ui.emit(INFOBOX,&msg_id);

        } else {

            //##########################################
            // Requirements for 2D wire ok. Ready to go.
            //##########################################
            if( !sys_probed_axis[X_AXIS] || !sys_probed_axis[Y_AXIS] ){
                // just set the current position of the unprobed axes as the probe position
                // if machine is not probed
                api::auto_set_probepositions();
            }


            pre_process_defaults();
            wire_gcode_task();
            // all set; start sd card line shifting
            ui.emit(RUN_SD_CARD_JOB,&dummy);

        }

    } else if ( operation_mode == MODE_SINKER ){

        //###################################
        // Check requirements for 1D sinker
        //###################################

        /*int  sinker_axis              = change_sinker_axis_direction( single_axis_mode_direction );
        bool sinker_axis_is_homed     = sys_axis_homed[sinker_axis];
        bool sinker_axis_needs_homing = true;

        if( 
             single_axis_mode_direction == 0 // z negative SINK_Z_NEG
          || single_axis_mode_direction == 2 // x negative SINK_X_NEG
          || single_axis_mode_direction == 3 // y negative SINK_Y_NEG
        ){ sinker_axis_needs_homing = false; }

        if( sinker_axis_needs_homing && !sinker_axis_is_homed ){
            msg_id = AXIS_NOT_HOMED;
        }*/

        if( msg_id != INFO_NONE ){

            //###################################
            // Requirements not ok.
            //###################################
            disable_spark_generator(); // disable generator to break the outer loop
            ui.emit( LOG_LINE, info_messages[msg_id] );
            //ui.emit(INFOBOX,&msg_id);

        } else {

            //############################################
            // Requirements for 1D sinker ok. Ready to go.
            //############################################
            sinker_drill_single_axis();

        }

  } else if ( operation_mode == MODE_REAMER ){

      pre_process_defaults();
      xTaskCreatePinnedToCore(reamer_task_single_axis, "reamer_task", 3000, this, 0, &drill_task_handle, 0);
      unsigned long start = millis();
      while( !task_running ){ 
          if( ( millis() - start > 10000 ) || !start_edm_process() ){
            break;
          }
          vTaskDelay(100); 
      }

  } 

}


void IRAM_ATTR G_EDM_UI_CONTROLLER::reset_flush_retract_timer(){
    flush_retract_timer_us = esp_timer_get_time();
}
bool IRAM_ATTR G_EDM_UI_CONTROLLER::check_if_time_to_flush(){
    if (flush_retract_after > 0 && (esp_timer_get_time() - flush_retract_timer_us >= flush_retract_after)){
        reset_flush_retract_timer();
        return true;
    } return false;
}

void G_EDM_UI_CONTROLLER::reset_defaults(){
    sys_probe_state                    = Probe::Off;
    sys_rt_exec_alarm                  = ExecAlarm::None;
    gconf.gedm_retraction_motion       = false;
    gconf.gedm_reprobe_block           = false;
    gconf.gedm_reprobe_motion          = false;
    gconf.gedm_single_axis_drill_task  = false;
    gconf.gedm_floating_z_gcode_task   = false;
    gconf.gedm_wire_gcode_task         = false;
    gconf.edm_pause_motion             = false;
    //sys.edm_pause_motion_probe       = false;
    gconf.gedm_insert_probe            = false;
    sys.probe_succeeded                = true;
    gconf.gedm_reset_protocol          = false;
    gconf.gedm_stop_process            = false;
    gconf.gedm_probe_position_x        = 0.0;
    gconf.gedm_probe_position_y        = 0.0;
    motion_plan                        = 0;
    probe_touched                      = false;
    limit_touched                      = false;
    edm_process_is_running             = false;
    sys_rt_exec_state.bit.motionCancel = false;
    sys_rt_exec_state.bit.reset        = false;
    api::block_while_not_idle();
    sys.abort                          = false;
    force_redraw                       = false;
    planner.reset_planner_state();
    sys.state = State::Idle;
    edm_start_stop();
    int data = 1;
    ui.emit(RESET,&data);
    api::push_cmd("G91 G54 G21\r\n");
}

void G_EDM_UI_CONTROLLER::reset_after_job(){
    if( enable_dpm_rxtx ){
        if( !dpm_driver.power_on_off( 0, 3 ) ){
          // problem... unhandled yet
        }// turn on
        vTaskDelay(200);
    }
    disable_spark_generator();
    stop_spindle();

    int page = PAGE_FRONT;
    gconf.gedm_stop_process            = true;
    sys_rt_exec_state.bit.motionCancel = true;
    api::block_while_not_idle();
    //filehandler->close_file();
    reset_spindle();
    motor_manager.set_ignore_disable(false);
    motor_manager.restore();
    vTaskDelay(100);
    planner.position_history_reset();
    //filehandler->reset_job_finished();
    sys.state                   = State::Idle;
    sys_rt_exec_alarm           = ExecAlarm::None;
    sys_rt_exec_state.bit.reset = false;
    api::block_while_not_idle();
    force_redraw = false;
    api::unlock_machine();
    disable_spark_generator();
    api::block_while_not_idle();
    reset_defaults();
    ui.emit(SHOW_PAGE, &page ); // show the frontpage
}



bool G_EDM_UI_CONTROLLER::ready_to_run(){
    if (
        sys.abort                  // abort
        || !start_edm_process()    // if on/off switch is off
        || !pwm_is_enabled()       // pwm off
        || edm_process_is_running  // edm_process running
        || !I2S_is_ready()         // i2s not ready
        || !api::machine_is_idle() // machine not idle
    ){ return false; }
    vTaskDelay(50);
    return true;
}


bool G_EDM_UI_CONTROLLER::get_is_ready(){
    if (
        !is_ready
        || !protocol_ready 
        || ( esp_timer_get_time() - start_time ) < LOADER_DELAY 
    ){ return false; }
    return true;
}
void G_EDM_UI_CONTROLLER::set_is_ready(){
    vTaskDelay(500);
    is_ready = true;
}


//#######################################
// Set the operation mode. Sinker/wire...
//#######################################
void G_EDM_UI_CONTROLLER::set_operation_mode(int mode){
    operation_mode = mode;
    if( mode > MODE_REAMER ){
        mode = MODE_SINKER; // fallback 
    }
    if( mode == MODE_2D_WIRE ){ // 2D wire
        probe_dimension = 1;
    } else{
        probe_dimension = 0;
    }
}

//###################################
// Check if motion input is blocked
//###################################
bool G_EDM_UI_CONTROLLER::motion_input_is_blocked() {
    if ( edm_process_is_running || sys_rt_exec_state.bit.motionCancel || sys.state == State::CheckMode ){ return true; }
    return false;
}










//###################################
// Change the sinker aixs
//###################################
int G_EDM_UI_CONTROLLER::change_sinker_axis_direction( int direction ){
    single_axis_mode_direction = direction;
    int axis = 0;

    switch (single_axis_mode_direction){
        case 0:
            axis = Z_AXIS;
            //ui_controller.selected_sink_mode = SINK_Z_NEG;
        break;
        case 1:
            axis = X_AXIS;
            //ui_controller.selected_sink_mode = SINK_X_POS;
        break;
        case 2:
            axis = X_AXIS;
            //ui_controller.selected_sink_mode = SINK_X_NEG;
        break;
        case 3:
            axis = Y_AXIS;
            //ui_controller.selected_sink_mode = SINK_Y_NEG;
        break;
        case 4:
            axis = Y_AXIS;
            //ui_controller.selected_sink_mode = SINK_Y_POS;
        break;
    }
    planner.set_sinker_axis( axis );
    return axis;
}











//################################################
// Enter and exit probe mode. 
// It will set the probe PWM/Duty/Voltage/Current
// and resets them after probing 
//################################################
void G_EDM_UI_CONTROLLER::probe_done(){
    if( enable_spindle ){
      stop_spindle();
      set_speed(probe_backup_frequency);
    }  
    probe_mode_off();
    ui.emit(LOG_LINE,"Probe mode off");
}
bool G_EDM_UI_CONTROLLER::probe_prepare(){
    ui.emit(LOG_LINE,"Probe mode on");
    probe_mode_on();
    generate_reference_voltage();
    probe_backup_frequency = get_speed();
    if( enable_spindle && !simulate_gcode ){
      set_speed( wire_spindle_speed_probing );
      start_spindle();
    }
    return true;
}


//###################################
// Stop I2S when changing I2S params
//###################################
void i2s_change_begin(){
    i2sconf.block_write_out = true;
    while( i2sconf.I2S_busy ){ vTaskDelay(10); }
    //stop_i2s();
}
void i2s_change_done(){
    i2sconf.block_write_out   = false;
    ui_controller.restart_i2s_enforce();
}


//###################################
// Raw setter functions 
//###################################
// set the pwm fequency; float value in kHz
void set_pwm_freq( setter_param *item ){
    // frequency provided as float value in kHz
    // convert it to an int value in hz
    int frequency = round( char_to_float( item->second ) * 1000.0 );
    ui_controller.change_pwm_frequency( frequency );
}
// set the pwm duty; float value in %
void set_pwm_duty( setter_param *item ){
    ui_controller.update_duty( char_to_float( item->second ) );
}
// set the operation; int value
void set_edm_mode( setter_param *item ){
    ui_controller.set_operation_mode( char_to_int( item->second ) );
}
// move x
void set_mpos_x( setter_param *item ){
}
// move y
void set_mpos_y( setter_param *item ){
}
// move z
void set_mpos_z( setter_param *item ){
}
// set max feedrate; float value
void set_rapid_speed( setter_param * item ){
    process_speeds_mm_min.RAPID = char_to_float( item->second );
    planner.update_max_feed();
    api::update_speeds();
}
void set_max_feed( setter_param * item ){
    process_speeds_mm_min.EDM = char_to_float( item->second );
    if( process_speeds_mm_min.EDM > process_speeds_mm_min.RAPID ){
        process_speeds_mm_min.EDM = process_speeds_mm_min.RAPID;
    }
    planner.update_max_feed();
    api::update_speeds();
}
// set setpoint min; float value
void set_setpoint_min( setter_param * item ){
    vsense_drop_range_min = char_to_float( item->second );
    ui_controller.generate_setpoint_min_max_adc();
}
// set setpoint max; float value
void set_setpoint_max( setter_param * item ){
    vsense_drop_range_max = char_to_float( item->second );
    ui_controller.generate_setpoint_min_max_adc();
}
// set spindle speed; float value RPM
void set_spindle_speed( setter_param * item ){
    wire_spindle_speed = ui_controller.rpm_to_frequency( char_to_float( item->second ) );
    ui_controller.set_speed(wire_spindle_speed);
}
void set_spindle_speed_probing(  setter_param * item  ){
    wire_spindle_speed_probing = ui_controller.rpm_to_frequency( char_to_float( item->second ) );
}
// set max short circuit duration; int value
void set_short_duration( setter_param * item ){
    adc_critical_conf.short_circuit_max_duration_us = char_to_int( item->second ) * 1000;
}
// set broken wire and max retract distance; float value mm
void set_broken_wire_distance( setter_param * item ){
    retconf.wire_break_distance = char_to_float( item->second );
    planner.set_retraction_steps();
}
// toggle gscope view; bool value
void set_draw_linear( setter_param * item  ){
  gpconf.drawlinear = char_to_bool( item->second );
}
// set rising edge threshold; int value ADC 0-4095
void set_edge_threshold( setter_param * item  ){
    int adc_value = char_to_int( item->second );
    if( adc_value < 1 ){
        adc_value = 1;
    } else if( adc_value > VSENSE_RESOLUTION ){
        adc_value = VSENSE_RESOLUTION;
    }
    adc_critical_conf.edge_treshhold = adc_value;
}
// toggle pwm on/off
void set_pwm_button_state( setter_param *item ){
    i2s_change_begin();
    bool state = char_to_bool( item->second );
    if( state ){
      ui_controller.enable_spark_generator();
    } else {
      ui_controller.disable_spark_generator();
    }
    i2s_change_done();
}
// set i2s use samples; int value
// this is the number of samples used from the buffer; deprecated
void set_i2s_use_samples( setter_param * item  ){
    i2s_change_begin();
    ui_controller.set_avg_i2s( char_to_int( item->second) );
    i2s_change_done();
}
// set i2s sample rate; float value kSps
void set_i2s_rate( setter_param * item  ){
    i2s_change_begin();
    int value = round( char_to_float( item->second ) * 1000.0 );
    if( value > 5000000 ){
        value = 5000000;
    } else if( value < 10000 ){
        value = 10000;
    }
    i2sconf.I2S_sample_rate = value;
    i2s_change_done();
}
// set i2s buffer length; int value
void set_i2s_buffer_length( setter_param * item  ){
    i2s_change_begin();
    int value = char_to_int( item->second );
    if( value > 1000 ){
        value = 1000;
    } else if( value < 10 ){
        value = 10;
    }
    i2sconf.I2S_buff_len = value;
    i2s_change_done();
}
// set i2s buffer count; int value
void set_i2s_buffer_count( setter_param * item  ){
    i2s_change_begin();
    int value = char_to_int( item->second );
    if( value > 128 ){
        value = 128;
    } else if( value < 2 ){
        value = 2;
    }
    i2sconf.I2S_buff_count = value;
    i2s_change_done();
}
// set default average size; int value
void set_average_main( setter_param * item  ){
    int value = char_to_int( item->second );
    if( value < 1 ){
        value = 1;
    } else if( value > vsense_sampler_buffer_size-1 ){
        value = vsense_sampler_buffer_size-1;
    }
    ui_controller.set_avg_default( value );
}
// set rising boost; int value
void set_rising_boost( setter_param * item  ){
    ui_controller.set_avg( true, char_to_int( item->second ) );
}
// set enable early retract exit; bool value
void set_early_retr_exit( setter_param * item ){
    retconf.early_exit = char_to_bool( item->second );
}
// set early retract exit confirmations; int value
void set_retract_confirmations( setter_param * item ){
    int value = char_to_int( item->second );
    if( value < 1 ){ value = 1; }
    adc_critical_conf.retract_confirmations = value;
}
// set max reverse depth; int value number of history lines
void set_max_reverse_depth( setter_param * item ){
    int value = char_to_int( item->second );
    if( value < 1 ){ value = 1; } 
    plconfig.max_reverse_depth = value;
}
// set hard retraction distance; float value mm
void set_retract_hard_mm( setter_param * item ){
    float value = char_to_float( item->second );
    if( value > 5.0 ){ value  = 5.0; }
    retconf.hard_retraction = value;
    planner.set_retraction_steps();
}
// set hard retraction speed; float value mm/min
void set_retract_hard_speed( setter_param * item ){
    float value = char_to_float( item->second );
    if( value > 80.0 ){ value = 80.0; }
    process_speeds_mm_min.WIRE_RETRACT_HARD = value;
    api::update_speeds();
}
// set soft retraction distance; float value mm
void set_retract_soft_mm( setter_param * item ){
    float value = char_to_float( item->second );
    if( value > 5.0 ){ value  = 5.0; }
    retconf.soft_retraction = value;
    planner.set_retraction_steps();
}
// set soft retraction speed; float value mm/min
void set_retract_soft_speed( setter_param * item ){
    float value = char_to_float( item->second );
    if( value > 80.0 ){ value = 80.0; }
    process_speeds_mm_min.WIRE_RETRACT_SOFT = value;
    api::update_speeds();
}
// set sample pool size used for forward confirmation; int value
void set_samples_pool_size( setter_param * item ){
    int value = char_to_int( item->second );
    if( value < 1 ){ value = 1; }
    i2sconf.I2S_pool_size = value;
}
// set use fixed hw timer for i2s readouts; bool value
// hw timer runs at around 20khz
void set_use_hw_timer( setter_param * item ){
    bool value = char_to_bool( item->second );
    adc_hw_timer.use_hw_timer = value;
    if( !adc_hw_timer.use_hw_timer ){
        if( sensor_queue_main != NULL ){
            int data = 11;
            xQueueSend( sensor_queue_main, &data, 50000 );
        }
    } else {
        if( !ui_controller.get_pwm_is_off() ){
            if( sensor_queue_main != NULL ){
                int data = 10;
                xQueueSend( sensor_queue_main, &data, 50000 );
            }
        }
    }
}
// set favg size; int value
void set_favg_size( setter_param * item ){
    int value = char_to_int( item->second );
    if( value < 1 ){
        value = 1;
    } else if( value > vsense_sampler_buffer_size-1 ){
        value = vsense_sampler_buffer_size-1;
    }
    i2sconf.I2S_full_range = value;
    gscope.set_full_range_size( value );
}
// set vdrop threshold; int value ADC
void set_vdrop_thresh( setter_param * item ){
    adc_critical_conf.voltage_feedback_treshhold = MIN( VSENSE_RESOLUTION, char_to_int( item->second ) );
    gscope.set_vdrop_treshhold( adc_critical_conf.voltage_feedback_treshhold );
}
// set number of vchannel drops to block forward motion; int value ADC
void set_vdrops_block( setter_param * item ){
    adc_critical_conf.forward_block_channel_shorts_num = char_to_int( item->second );
}
// set number of vchannel drops to turn pwm off; int value ADC
void set_vdrops_poff( setter_param * item ){
    adc_critical_conf.max_channel_shorts_for_pulse_off = char_to_int( item->second );
}
// set pulse off duration; int value
void set_poff_duration( setter_param * item ){
    adc_critical_conf.pulse_off_duration = char_to_int( item->second );
}
// set early exit on plan
void set_early_x_on( setter_param * item ){
    plconfig.early_exit_on_plan = char_to_int( item->second );
    if( plconfig.early_exit_on_plan > 4 ){
      plconfig.early_exit_on_plan = 4;
    } else if ( plconfig.early_exit_on_plan <= 0 ){
      plconfig.early_exit_on_plan = 1;
    }
}
// set line end confirmations; int value
void set_line_end_confirmations( setter_param * item ){
    plconfig.line_to_line_confirm_counts = char_to_int( item->second );
}
// set reset sense queue before each step; bool value
void set_reset_vsense_queue( setter_param * item ){
    plconfig.reset_sense_queue = char_to_bool( item->second );
}
// set pwm off confirmations; int value
void set_poff_after( setter_param * item ){
    adc_critical_conf.pwm_off_count = char_to_int( item->second );
}
// set use stop/go signals; bool value
void set_use_stopgo_signals( setter_param * item ){
    use_stop_and_go_flags = char_to_bool( item->second );
}
// set zero jitter; int value
void set_zero_jitter( setter_param * item ){
    adc_critical_conf.zeros_jitter = MAX( 0, char_to_int( item->second ) );
}
// set zero threshold; int value adc
void set_zero_threshold( setter_param * item ){
    adc_critical_conf.zero_treshhold = char_to_int( item->second );
}
// set readings low before it moves on; int value
void set_readings_low( setter_param * item ){
    adc_critical_conf.zeros_in_a_row_max = MAX( 1, char_to_int( item->second ) );
}
// set readings high before something happens; int value
void set_readings_high( setter_param * item ){
    adc_critical_conf.highs_in_a_row_max = MAX( 1, char_to_int( item->second ) );
}
// set enable dpm support; bool value
void set_dpm_uart_enabled( setter_param * item ){
    enable_dpm_rxtx = char_to_bool( item->second );
    if( enable_dpm_rxtx ){
        dpm_driver.power_on_off( false, 1 );
    }
}
// set turn dpm on or off; bool value
void set_dpm_power_onoff( setter_param * item ){
    if( !enable_dpm_rxtx ){ return; }
    dpm_driver.power_on_off( char_to_bool( item->second ), 1 );
}
// set dpm voltage; float value
void set_dpm_voltage( setter_param * item ){
    if( !enable_dpm_rxtx ){ return; }
    dpm_driver.set_setting_voltage( char_to_float( item->second ) );
}
// set dpm current; float value
void set_dpm_current( setter_param * item ){
    if( !enable_dpm_rxtx ){ return; }
    dpm_driver.set_setting_current( char_to_float( item->second ) );
}
// set restart esp
void set_esp_restart( setter_param * item ){
    Serial.end();
    sys.abort = true;
    //esp_task_wdt_init(1,true);
    //esp_task_wdt_add(NULL);
    esp_task_wdt_init(1, true);
    esp_task_wdt_add(NULL);
    while(true);  // wait for watchdog timer to be triggered
    ESP.restart();
    //while(true);
    while( true ){ vTaskDelay(10); }
}
// set dpm probe voltage; float value
void set_dpm_probe_voltage( setter_param * item ){
    dpm_voltage_probing = char_to_float( item->second ); 
}
// set dpm probe current; float value
void set_dpm_probe_current( setter_param * item ){
    dpm_current_probing = char_to_float( item->second ); 
}
// set probe frequency; float value khz
void set_probe_frequency( setter_param * item ){
    int value = round( char_to_float( item->second ) * 1000.0 );
    if( value < PWM_FREQUENCY_MIN ){ value = PWM_FREQUENCY_MIN; }
    if( value > PWM_FREQUENCY_MAX ){ value = PWM_FREQUENCY_MAX; }
    pwm_frequency_probing = value;
}
// set probe duty; float value %
void set_probe_duty( setter_param * item ){
    float value = char_to_float( item->second );
    if ( value > PWM_DUTY_MAX ){ value = PWM_DUTY_MAX; }   
    pwm_duty_probing = value;
}
// set probe vfd trigger threshold; int value ADC
void set_probe_vfd_trigger( setter_param * item ){
    int value = char_to_int( item->second );
    if( value < 0 ){ value = 0; } 
    else if( value > VSENSE_RESOLUTION ){ value = VSENSE_RESOLUTION; }
    probe_logic_low_trigger_treshold = value;              
}
// set probe cfd trigger threshold; float value %
void set_probe_cfd_trigger( setter_param * item ){
    float value = char_to_float( item->second );
    if( value <= 1.0 ){ value = 1.0; } 
    else if( value >= 50.0 ){ value = 50.0; }
    probe_trigger_current = value;      
}
// set spindle auto enable; bool value
void set_spindle_enable( setter_param * item ){
    enable_spindle = char_to_bool( item->second );
}
// set turn spindle on/off; bool value
void set_spindle_onoff( setter_param * item ){
    if( char_to_bool( item->second ) ){
      ui_controller.set_speed(wire_spindle_speed);
      ui_controller.start_spindle();
    } else {
      ui_controller.stop_spindle();
    }
    vTaskDelay(100);
}
void set_reamer_distance( setter_param * item ){
    reamer_travel_mm = char_to_float( item->second );
}
void set_reamer_duration( setter_param * item ){
    reamer_duration = char_to_float( item->second );
}
void set_sinker_distance( setter_param * item ){
    sinker_travel_mm = char_to_float( item->second );
}
void set_simulate_gcode( setter_param * item ){
    simulate_gcode = char_to_bool( item->second );
}
// set home axis; int value AXIS or -1 for all
void set_home_axis( setter_param * item ){
    int value = char_to_int( item->second );
    if( value == -1 ){ 
        api::home_machine(); // home all axis: z is excluded if z is disabled;
    } else { api::home_axis( value ); }
}
void set_home_all( setter_param * item ){
    api::home_machine();
}
// set move to origin
void set_move_to_origin( setter_param * item ){
    api::push_cmd("G90 G0 X0 Y0\r\n");
}
// set toggle homing enabled for axis; int value AXIS
void set_homing_enabled( setter_param * item ){
    if( char_to_int( item->second ) == Z_AXIS ){
        z_no_home = !z_no_home;
    }
}
// set axis used for sinker mode
void set_sink_dir( setter_param * item ){
  int direction = ui_controller.single_axis_mode_direction;
  if( ++direction > 4 ){
    direction = 0;
  }  
  ui_controller.change_sinker_axis_direction( direction );
}
// set axis steps per mm; int value
void set_motor_res( setter_param * item ){
    switch( item->first ){
        case PARAM_ID_X_RES:
            g_axis[X_AXIS]->steps_per_mm.set( char_to_int( item->second ) );
        break;
        case PARAM_ID_Y_RES:
            g_axis[Y_AXIS]->steps_per_mm.set( char_to_int( item->second ) );
        break;
        case PARAM_ID_Z_RES:
            g_axis[Z_AXIS]->steps_per_mm.set( char_to_int( item->second ) );
        break;
    }
    api::update_speeds();
}
// set probe position for axis; int value AXIS
void set_probing_pos_action( setter_param * item ){
    api::auto_set_probepositions_for( char_to_int( item->second ) );
}
// set unset probing point for axis; int value AXIS
void set_probing_pos_unset( setter_param * item ){
    api::unset_probepositions_for( char_to_int( item->second ) );
}
void set_flushing_distance( setter_param * item ){
    flush_retract_mm = char_to_float( item->second );
}
void set_flushing_interval( setter_param * item ){
    // user input is in seconds
    // software needs micros
    // seconds to micros
    float seconds = char_to_float( item->second );
    int64_t micros = 0; 
    if( seconds > 0 ){
        micros = int64_t( char_to_float( item->second ) * 1000000.0 );
    }
    flush_retract_after = micros;
}
void set_flushing_offset_steps( setter_param * item ){
    flush_offset_steps = char_to_int( item->second );
}
void set_flushing_disable_spark( setter_param * item ){
    disable_spark_for_flushing = char_to_bool( item->second );
}
void set_gcode_file_has( setter_param * item ){
    ui_controller.has_gcode_file = char_to_bool( item->second );
}
void set_page_current( setter_param * item ){
    ui_controller.active_page = ( uint8_t ) char_to_int( item->second );
}
void set_tooldiameter( setter_param * item ){
    tool_diameter = char_to_float( item->second );
}
void set_jog_axis( float mm_to_travel, uint8_t axis, bool negative = false ){

    #ifdef Z_AXIS_NOT_USED
        if( axis == Z_AXIS ){
            ui.emit( LOG_LINE, info_messages[Z_AXIS_DISABLED] );
            ui.emit( LOG_LINE, "  Comment out #define Z_AXIS_NOT_USED" );
            ui.emit( LOG_LINE, "  in definitions_common.h to enable Z" );
            return;
        }
    #endif

    info_codes msg_id = INFO_NONE;

    if( ui_controller.motion_input_is_blocked() ){

        msg_id = MOTION_NOT_AVAILABLE;

    } else {

        if( negative ){
            mm_to_travel = mm_to_travel*-1;
        }

        if( !api::jog_axis( mm_to_travel, get_axis_name_const( axis ), process_speeds_mm_min.RAPID, axis ) ){
            msg_id = TARGET_OUT_OF_REACH;
        } 

    }

    if( msg_id != INFO_NONE ){
        ui.emit( LOG_LINE, info_messages[msg_id] );
        //ui.emit(INFOBOX,&msg_id); //
    }

}
void set_jog_axis_x_positive( setter_param * item ){
    set_jog_axis( char_to_float( item->second ), X_AXIS );
}
void set_jog_axis_x_negative( setter_param * item ){
    set_jog_axis( char_to_float( item->second ), X_AXIS, true );
}
void set_jog_axis_y_positive( setter_param * item ){
    set_jog_axis( char_to_float( item->second ), Y_AXIS );
}
void set_jog_axis_y_negative( setter_param * item ){
    set_jog_axis( char_to_float( item->second ), Y_AXIS, true );
}
void set_jog_axis_z_positive( setter_param * item ){
    set_jog_axis( char_to_float( item->second ), Z_AXIS );
}
void set_jog_axis_z_negative( setter_param * item ){
    set_jog_axis( char_to_float( item->second ), Z_AXIS, true );
}
void set_stop_edm_process( setter_param * item ){
    int stop_reason = char_to_int( item->second );
    if( stop_reason == 0 ){
        // normal finish?
    } else {

    }
    ui.emit(LOG_LINE,"Something nope");
    //sys_rt_exec_state.bit.motionCancel = true;
    ui_controller.disable_spark_generator();
}
void set_probing_action( setter_param * item ){
    info_codes msg_id = INFO_NONE;
    if( ui_controller.motion_input_is_blocked() ){
        msg_id = MOTION_NOT_AVAILABLE;
    } else if( !api::machine_is_fully_homed() ){
        msg_id = NOT_FULLY_HOMED;
    } else {
        ui.emit(LOG_LINE,"");
        ui.emit(LOG_LINE,"");
        int page = PAGE_PROBING;
        ui.emit(SHOW_PAGE,&page);
        // 0 = left back corner; 1 = back; 2 = right back corner; 3 = left; 4 = center finder; 5 = right; 6 = left front corner; 7 = front; 8 = right front corner;
        int probe_param = char_to_int( item->second );
        //bool probe_2d   = true;// enforce 2d mode ui_controller.probe_dimension==1?true:false;
        std::function<void()> callback              = nullptr;
        static const int      block_size            = 6;
        std::string           str_block[block_size] = {};
        switch( probe_param ){
            case 0:
                str_block[0] = "@ Probing left back edge";
                callback = std::bind(&G_EDM_PROBE_ROUTINES::left_back_edge_2d, ui_controller);
                break;
            case 1:
                str_block[0] = "@ Probing back side";
                callback = std::bind(&G_EDM_PROBE_ROUTINES::probe_y_negative, ui_controller);
                break;
            case 2:
                str_block[0] = "@ Probing right back edge";
                callback = std::bind(&G_EDM_PROBE_ROUTINES::right_back_edge_2d, ui_controller);
                break;
            case 3:
                str_block[0] = "@ Probing left side";
                callback = std::bind(&G_EDM_PROBE_ROUTINES::probe_x_positive, ui_controller);
                break;
            case 4:
                str_block[0] = "@ Center finder";
                callback = std::bind(&G_EDM_PROBE_ROUTINES::center_finder_2d, ui_controller);
                break;
            case 5:
                str_block[0] = "@ Probing right side";
                callback = std::bind(&G_EDM_PROBE_ROUTINES::probe_x_negative, ui_controller);
                break;
            case 6:
                str_block[0] = "@ Probing left front edge";
                callback = std::bind(&G_EDM_PROBE_ROUTINES::left_front_edge_2d, ui_controller);
                break;
            case 7:
                str_block[0] = "@ Probing front side";
                callback = std::bind(&G_EDM_PROBE_ROUTINES::probe_y_positive, ui_controller);
                break;
            case 8:
                str_block[0] = "@ Probing right front edge";
                callback = std::bind(&G_EDM_PROBE_ROUTINES::right_front_edge_2d, ui_controller);
                break;
        }
        str_block[1] = "  Display will not respond";
        str_block[2] = "  until probing is finished";
        str_block[3] = "  The process can be stopped";
        str_block[4] = "  using the motionswitch";
        str_block[5] = "  Tool diameter: ";
        str_block[5] += floatToString( tool_diameter );
        for( int i = 0; i < block_size; ++i ){
          if( str_block[i].length()<=0 ){
            continue;
          }
          ui.emit(LOG_LINE,str_block[i].c_str());
        }
        if( callback != nullptr ){
            vTaskDelay(2000);
            ui_controller.probe_prepare();
            callback();
            ui_controller.probe_done();
            if( sys.probe_succeeded ){
                ui.emit(LOG_LINE,"@ Probe finished");
            } else {
                ui.emit(LOG_LINE,"* Probe failed");
            }
            ui.emit(LOG_LINE,"  Press close to return");
            vTaskDelay(100);
        }

    }
    if( msg_id != INFO_NONE ){
        ui.emit( LOG_LINE, info_messages[msg_id] );
        //ui.emit(INFOBOX,&msg_id);
    }
}
IRAM_ATTR void set_edm_pause( setter_param * item ){
    bool new_state = char_to_bool( item->second );
    bool state_changed = gconf.edm_pause_motion == new_state ? false : true;
    gconf.edm_pause_motion = new_state;
    if( !state_changed ){ return; }
    if( gconf.edm_pause_motion ){
        int rounds = 0;
        while( !planner.get_is_paused() || ++rounds>1000 ){
            gconf.edm_pause_motion = true;
            if( sys.abort || sys_rt_exec_state.bit.motionCancel || gconf.edm_process_finished ){ break; }
            vTaskDelay(1);
        }
      ui.emit(LOG_LINE,"@ Process paused");
    } else {
      ui.emit(LOG_LINE,"@ Process resumed");
    }
}






void get_max_feed( getter_param * item ){
    item->floatValue = process_speeds_mm_min.EDM;
}
void get_flushing_disable_spark( getter_param * item ){
    item->boolValue = disable_spark_for_flushing;
}
void get_flushing_offset_steps( getter_param * item ){
    item->intValue = flush_offset_steps;
}
void get_flushing_interval( getter_param * item ){
    // user sees seconds
    // software uses micros
    float seconds = 0.0;
    if( flush_retract_after > 0 ){
      seconds = float( flush_retract_after ) / 1000000.0;
    }
    item->floatValue = seconds;
}
void get_flushing_distance( getter_param * item ){
    item->floatValue = flush_retract_mm;
}
void get_pwm_freq( getter_param * item ){
    item->floatValue = float( ui_controller.get_freq() ) / 1000.0;
}
void get_pwm_duty( getter_param * item ){
    item->floatValue = ui_controller.get_duty_percent();
}
void get_mpos_x( getter_param * item ){
    //size_t free_heap = esp_get_free_heap_size();
    //item->floatValue = float(free_heap);
    item->floatValue = system_get_mpos_for_axis( X_AXIS );
}
void get_mpos_y( getter_param * item ){
    //size_t min_free_heap = esp_get_minimum_free_heap_size();
    //item->floatValue = float(min_free_heap);
    item->floatValue = system_get_mpos_for_axis( Y_AXIS );
}
void get_mpos_z( getter_param * item ){
    item->floatValue = system_get_mpos_for_axis( Z_AXIS );
}
void get_pwm_button_state( getter_param * item ){
    item->boolValue = ui_controller.pwm_is_enabled();
    //item->boolValue = !item->boolValue;vTaskDelay(1000);
}
void get_rapid_speed( getter_param * item ){
    item->floatValue = process_speeds_mm_min.RAPID; // hacky. Needs a review. Also in the planner.
}
void get_setpoint_min( getter_param * item ){
    item->floatValue = vsense_drop_range_min;
}
void get_setpoint_max( getter_param * item ){
    item->floatValue = vsense_drop_range_max;
}
void get_spindle_speed_probing(  getter_param * item  ){
    item->floatValue = ui_controller.frequency_to_rpm( (float)wire_spindle_speed_probing );
}
void get_spindle_speed( getter_param * item ){
    item->floatValue = ui_controller.frequency_to_rpm( (float)wire_spindle_speed );
}
void get_short_duration( getter_param * item ){
    item->intValue = round( adc_critical_conf.short_circuit_max_duration_us / 1000 );
}
void get_broken_wire_distance( getter_param * item ){
    item->floatValue = retconf.wire_break_distance;
}
void get_draw_linear( getter_param * item  ){
    item->boolValue = gpconf.drawlinear;
}
void get_edge_threshold( getter_param * item  ){
    item->intValue = adc_critical_conf.edge_treshhold;
}
void get_i2s_use_samples( getter_param * item  ){
    item->intValue = ui_controller.get_avg_i2s();
}
void get_i2s_rate( getter_param * item  ){
    item->floatValue = float(i2sconf.I2S_sample_rate)/1000.0;
}
void get_i2s_buffer_length( getter_param * item  ){
    item->intValue = i2sconf.I2S_buff_len;//round(i2sconf.I2S_sample_rate/1000);
}
void get_i2s_buffer_count( getter_param * item  ){
    item->intValue = i2sconf.I2S_buff_count;//round(i2sconf.I2S_sample_rate/1000);
}
void get_average_main( getter_param * item  ){
    item->intValue = ui_controller.get_avg_default();
}
void get_rising_boost( getter_param * item  ){
    item->intValue = ui_controller.get_avg( true );
}
void get_early_retr_exit( getter_param * item ){
    item->boolValue = retconf.early_exit;
}
void get_retract_confirmations( getter_param * item ){
    item->intValue = adc_critical_conf.retract_confirmations;
}
void get_max_reverse_depth( getter_param * item ){
    item->intValue = plconfig.max_reverse_depth;
}
void get_retract_hard_mm( getter_param * item ){
    item->floatValue = retconf.hard_retraction;
}
void get_retract_hard_speed( getter_param * item ){
    item->floatValue = process_speeds_mm_min.WIRE_RETRACT_HARD;
}
void get_retract_soft_mm( getter_param * item ){
    item->floatValue = retconf.soft_retraction;
}
void get_retract_soft_speed( getter_param * item ){
    item->floatValue = process_speeds_mm_min.WIRE_RETRACT_SOFT;
}
void get_samples_pool_size( getter_param * item ){
    item->intValue = i2sconf.I2S_pool_size;
}
void get_use_hw_timer( getter_param * item ){
    item->boolValue = adc_hw_timer.use_hw_timer;
}
void get_favg_size( getter_param * item ){
    item->intValue = i2sconf.I2S_full_range;
}
void get_vdrop_thresh( getter_param * item ){
    item->intValue = adc_critical_conf.voltage_feedback_treshhold;
}
void get_vdrops_block( getter_param * item ){
    item->intValue = adc_critical_conf.forward_block_channel_shorts_num;
}
void get_vdrops_poff( getter_param * item ){
    item->intValue = adc_critical_conf.max_channel_shorts_for_pulse_off;
}
void get_poff_duration( getter_param * item ){
    item->intValue = adc_critical_conf.pulse_off_duration;
}
void get_early_x_on( getter_param * item ){
    item->intValue = plconfig.early_exit_on_plan;
}
void get_line_end_confirmations( getter_param * item ){
    item->intValue = plconfig.line_to_line_confirm_counts;
}
void get_reset_vsense_queue( getter_param * item ){
    item->boolValue = plconfig.reset_sense_queue;
}
void get_poff_after( getter_param * item ){
    item->intValue = adc_critical_conf.pwm_off_count;
}
void get_use_stopgo_signals( getter_param * item ){
    item->boolValue = use_stop_and_go_flags;
}
void get_zero_jitter( getter_param * item ){
    item->intValue = adc_critical_conf.zeros_jitter;
}
void get_zero_threshold( getter_param * item ){
    item->intValue = adc_critical_conf.zero_treshhold;
}
void get_readings_low( getter_param * item ){
    item->intValue = adc_critical_conf.zeros_in_a_row_max;
}
void get_readings_high( getter_param * item ){
    item->intValue = adc_critical_conf.highs_in_a_row_max;
}
void get_dpm_uart_enabled( getter_param * item ){
    item->boolValue = enable_dpm_rxtx;
}
void get_dpm_power_onoff( getter_param * item ){
    if( !enable_dpm_rxtx ){ 
      item->boolValue = false;
      return; 
    }
    item->boolValue = dpm_driver.get_power_is_on();
}
void get_dpm_voltage( getter_param * item ){
    if( !enable_dpm_rxtx ){ 
      item->floatValue = 0.0;
      return; 
    }
    int reading;
    dpm_driver.get_setting_voltage( reading ); // return values are in mV/mA
    float d = float( reading );
    if( reading > -1 ){ d /= 1000.0; }
    item->floatValue = d;
}
void get_dpm_current( getter_param * item ){
    if( !enable_dpm_rxtx ){ 
      item->floatValue = 0.0;
      return; 
    }
    int reading;
    dpm_driver.get_setting_current( reading ); // return values are in mV/mA
    float d = float( reading );
    if( reading > -1 ){ d /= 1000.0; }
    item->floatValue = d;
}
void get_motor_res( getter_param * item ){
    switch( item->param_id ){
        case PARAM_ID_X_RES:
            item->intValue = g_axis[X_AXIS]->steps_per_mm.get();
        break;
        case PARAM_ID_Y_RES:
            item->intValue = g_axis[Y_AXIS]->steps_per_mm.get();
        break;
        case PARAM_ID_Z_RES:
            item->intValue = g_axis[Z_AXIS]->steps_per_mm.get();
        break;
    }
}
void get_dpm_probe_voltage( getter_param * item ){
    item->floatValue = dpm_voltage_probing; 
}
void get_dpm_probe_current( getter_param * item ){
    item->floatValue = dpm_current_probing; 
}
void get_probe_frequency( getter_param * item ){
    item->intValue   = pwm_frequency_probing;
    item->floatValue = float( pwm_frequency_probing ) / 1000.0;
}
void get_probe_duty( getter_param * item ){
    item->floatValue = pwm_duty_probing; 
}
void get_probe_vfd_trigger( getter_param * item ){
    item->intValue = probe_logic_low_trigger_treshold;
}
void get_probe_cfd_trigger( getter_param * item ){
    item->floatValue = probe_trigger_current;
}
void get_spindle_enable( getter_param * item ){
    item->boolValue = enable_spindle;
}
void get_spindle_onoff( getter_param * item ){
    item->boolValue = ui_controller.spindle_is_running();
}
void get_reamer_distance( getter_param * item ){
    item->floatValue = reamer_travel_mm;
}
void get_reamer_duration( getter_param * item ){
    item->floatValue = reamer_duration;
}
void get_sinker_distance( getter_param * item ){
    item->floatValue = sinker_travel_mm;
}
void get_simulate_gcode( getter_param * item ){
    item->boolValue = simulate_gcode;
}
void get_sink_dir( getter_param * item ){
    item->intValue = ui_controller.single_axis_mode_direction;
}
void get_probing_action( getter_param * item ){
}
void get_probe_pos( getter_param * item ){
    item->floatValue = api::probe_pos_to_mpos(item->intValue);
    item->boolValue  = sys_probed_axis[item->intValue];
}
void get_edm_mode( getter_param * item ){
    item->intValue = operation_mode;
}
void get_home_axis( getter_param * item ){
    item->boolValue = sys_axis_homed[item->intValue];
}
void get_homing_enabled( getter_param * item ){
    if( item->intValue == Z_AXIS && z_no_home ){ 
        item->boolValue = false; 
        return;
    }
    item->boolValue = true;
}
void get_motion_state( getter_param * item ){
    item->boolValue = ui_controller.motion_input_is_blocked() ? false : true;
}
void get_tooldiameter( getter_param * item ){
    item->floatValue = tool_diameter;
}
IRAM_ATTR void get_edm_pause( getter_param * item ){
    item->boolValue = gconf.edm_pause_motion ? true : false;
}
IRAM_ATTR void get_wpos_x( getter_param * item ){
    float *wpos = api::get_wpos();
    item->floatValue = wpos[X_AXIS];
}
IRAM_ATTR void get_wpos_y( getter_param * item ){
    float *wpos = api::get_wpos();
    item->floatValue = wpos[Y_AXIS];
}
IRAM_ATTR void get_wpos_z( getter_param * item ){
    float *wpos = api::get_wpos();
    item->floatValue = wpos[Z_AXIS];
}















//#########################################################################
// Default function to get single parameters
//#########################################################################
void (*getter_table[TOTAL_PARAM_ITEMS])( getter_param *item ) = { nullptr };
//#########################################################################
// Default function to set single parameters
//#########################################################################
void (*setter_table[TOTAL_PARAM_ITEMS])( setter_param *item ) = { nullptr };
//#########################################################################
// Setter function for commands with axis included like move axis x
//#########################################################################
//void (*setter_table_axis[TOTAL_AXIS_PARAM_ITEMS])( setter_param_axis *item ) = { nullptr };

void G_EDM_UI_CONTROLLER::setup_events( void ){

    //#######################################################################
    // Getter function table
    //#######################################################################
    getter_table[PARAM_ID_FREQ]                  = get_pwm_freq;
    getter_table[PARAM_ID_DUTY]                  = get_pwm_duty;
    getter_table[PARAM_ID_MODE]                  = get_edm_mode;
    getter_table[PARAM_ID_MPOSX]                 = get_mpos_x;
    getter_table[PARAM_ID_MPOSY]                 = get_mpos_y;
    getter_table[PARAM_ID_MPOSZ]                 = get_mpos_z;
    getter_table[PARAM_ID_PWM_STATE]             = get_pwm_button_state;
    getter_table[PARAM_ID_SPEED_RAPID]           = get_rapid_speed;
    getter_table[PARAM_ID_MAX_FEED]              = get_max_feed;
    getter_table[PARAM_ID_SETMIN]                = get_setpoint_min;
    getter_table[PARAM_ID_SETMAX]                = get_setpoint_max;
    getter_table[PARAM_ID_SPINDLE_SPEED]         = get_spindle_speed;
    getter_table[PARAM_ID_SPINDLE_ENABLE]        = get_spindle_enable;
    getter_table[PARAM_ID_SHORT_DURATION]        = get_short_duration;
    getter_table[PARAM_ID_BROKEN_WIRE_MM]        = get_broken_wire_distance;
    getter_table[PARAM_ID_DRAW_LINEAR]           = get_draw_linear;
    getter_table[PARAM_ID_EDGE_THRESH]           = get_edge_threshold;
    getter_table[PARAM_ID_I2S_SAMPLES]           = get_i2s_use_samples;
    getter_table[PARAM_ID_I2S_RATE]              = get_i2s_rate;
    getter_table[PARAM_ID_I2S_BUFFER_L]          = get_i2s_buffer_length;
    getter_table[PARAM_ID_I2S_BUFFER_C]          = get_i2s_buffer_count;
    getter_table[PARAM_ID_MAIN_AVG]              = get_average_main;
    getter_table[PARAM_ID_RISE_BOOST]            = get_rising_boost;
    getter_table[PARAM_ID_EARLY_RETR]            = get_early_retr_exit;
    getter_table[PARAM_ID_RETRACT_CONF]          = get_retract_confirmations;
    getter_table[PARAM_ID_MAX_REVERSE]           = get_max_reverse_depth;
    getter_table[PARAM_ID_RETRACT_H_MM]          = get_retract_hard_mm;
    getter_table[PARAM_ID_RETRACT_H_F]           = get_retract_hard_speed;
    getter_table[PARAM_ID_RETRACT_S_MM]          = get_retract_soft_mm;
    getter_table[PARAM_ID_RETRACT_S_F]           = get_retract_soft_speed;
    getter_table[PARAM_ID_BEST_OF]               = get_samples_pool_size;
    getter_table[PARAM_ID_USEHWTIMTER]           = get_use_hw_timer;
    getter_table[PARAM_ID_FAVG_SIZE]             = get_favg_size;
    getter_table[PARAM_ID_VDROP_THRESH]          = get_vdrop_thresh;
    getter_table[PARAM_ID_VFD_DROPS_BLOCK]       = get_vdrops_block;
    getter_table[PARAM_ID_VFD_SHORTS_POFF]       = get_vdrops_poff;
    getter_table[PARAM_ID_POFF_DURATION]         = get_poff_duration;
    getter_table[PARAM_ID_EARLY_X_ON]            = get_early_x_on;
    getter_table[PARAM_ID_LINE_ENDS]             = get_line_end_confirmations;
    getter_table[PARAM_ID_RESET_QUEUE]           = get_reset_vsense_queue;
    getter_table[PARAM_ID_POFF_AFTER]            = get_poff_after;
    getter_table[PARAM_ID_USE_STOPGO]            = get_use_stopgo_signals;
    getter_table[PARAM_ID_ZERO_JITTER]           = get_zero_jitter;
    getter_table[PARAM_ID_READINGS_H]            = get_readings_high;
    getter_table[PARAM_ID_READINGS_L]            = get_readings_low;
    getter_table[PARAM_ID_ZERO_THRESH]           = get_zero_threshold;
    getter_table[PARAM_ID_DPM_UART]              = get_dpm_uart_enabled;
    getter_table[PARAM_ID_DPM_ONOFF]             = get_dpm_power_onoff;
    getter_table[PARAM_ID_DPM_VOLTAGE]           = get_dpm_voltage;
    getter_table[PARAM_ID_DPM_CURRENT]           = get_dpm_current;
    getter_table[PARAM_ID_X_RES]                 = get_motor_res;
    getter_table[PARAM_ID_Y_RES]                 = get_motor_res;
    getter_table[PARAM_ID_Z_RES]                 = get_motor_res;
    getter_table[PARAM_ID_DPM_PROBE_V]           = get_dpm_probe_voltage;
    getter_table[PARAM_ID_DPM_PROBE_C]           = get_dpm_probe_current;
    getter_table[PARAM_ID_PROBE_FREQ]            = get_probe_frequency;
    getter_table[PARAM_ID_PROBE_DUTY]            = get_probe_duty;
    getter_table[PARAM_ID_PROBE_TR_V]            = get_probe_vfd_trigger;
    getter_table[PARAM_ID_PROBE_TR_C]            = get_probe_cfd_trigger;
    getter_table[PARAM_ID_SPINDLE_ONOFF]         = get_spindle_onoff;
    getter_table[PARAM_ID_REAMER_DIST]           = get_reamer_distance;
    getter_table[PARAM_ID_REAMER_DURA]           = get_reamer_duration;
    getter_table[PARAM_ID_SINK_DIST]             = get_sinker_distance;
    getter_table[PARAM_ID_SIMULATION]            = get_simulate_gcode;
    getter_table[PARAM_ID_SINK_DIR]              = get_sink_dir;
    getter_table[PARAM_ID_PROBE_ACTION]          = get_probing_action;
    getter_table[PARAM_ID_GET_PROBE_POS]         = get_probe_pos;
    getter_table[PARAM_ID_HOME_AXIS]             = get_home_axis;
    getter_table[PARAM_ID_HOME_ENA]              = get_homing_enabled;
    getter_table[PARAM_ID_HOME_SEQ]              = get_homing_enabled;
    getter_table[PARAM_ID_MOTION_STATE]          = get_motion_state;
    getter_table[PARAM_ID_FLUSHING_INTERVAL]     = get_flushing_interval;
    getter_table[PARAM_ID_FLUSHING_DISTANCE]     = get_flushing_distance;
    getter_table[PARAM_ID_FLUSHING_FLUSH_OFFSTP] = get_flushing_offset_steps;
    getter_table[PARAM_ID_FLUSHING_FLUSH_NOSPRK] = get_flushing_disable_spark;
    getter_table[PARAM_ID_WPOSX]                 = get_wpos_x;
    getter_table[PARAM_ID_WPOSY]                 = get_wpos_y;
    getter_table[PARAM_ID_WPOSZ]                 = get_wpos_z;
    getter_table[PARAM_ID_EDM_PAUSE]             = get_edm_pause;
    getter_table[PARAM_ID_TOOLDIAMETER]          = get_tooldiameter;
    getter_table[PARAM_ID_SPINDLE_SPEED_PROBING] = get_spindle_speed_probing;

    //#######################################################################
    // Setter function table
    //#######################################################################
    setter_table[PARAM_ID_FREQ]                  = set_pwm_freq;
    setter_table[PARAM_ID_DUTY]                  = set_pwm_duty;
    setter_table[PARAM_ID_MODE]                  = set_edm_mode;
    setter_table[PARAM_ID_PWM_STATE]             = set_pwm_button_state;
    setter_table[PARAM_ID_SPEED_RAPID]           = set_rapid_speed;
    setter_table[PARAM_ID_MAX_FEED]              = set_max_feed;
    setter_table[PARAM_ID_SETMIN]                = set_setpoint_min;
    setter_table[PARAM_ID_SETMAX]                = set_setpoint_max;
    setter_table[PARAM_ID_SPINDLE_SPEED]         = set_spindle_speed;
    setter_table[PARAM_ID_SPINDLE_ENABLE]        = set_spindle_enable;
    setter_table[PARAM_ID_SHORT_DURATION]        = set_short_duration;
    setter_table[PARAM_ID_BROKEN_WIRE_MM]        = set_broken_wire_distance;
    setter_table[PARAM_ID_DRAW_LINEAR]           = set_draw_linear;
    setter_table[PARAM_ID_EDGE_THRESH]           = set_edge_threshold;
    setter_table[PARAM_ID_I2S_SAMPLES]           = set_i2s_use_samples;
    setter_table[PARAM_ID_I2S_RATE]              = set_i2s_rate;
    setter_table[PARAM_ID_I2S_BUFFER_L]          = set_i2s_buffer_length;
    setter_table[PARAM_ID_I2S_BUFFER_C]          = set_i2s_buffer_count;
    setter_table[PARAM_ID_MAIN_AVG]              = set_average_main;
    setter_table[PARAM_ID_RISE_BOOST]            = set_rising_boost;
    setter_table[PARAM_ID_EARLY_RETR]            = set_early_retr_exit;
    setter_table[PARAM_ID_RETRACT_CONF]          = set_retract_confirmations;
    setter_table[PARAM_ID_MAX_REVERSE]           = set_max_reverse_depth;
    setter_table[PARAM_ID_RETRACT_H_MM]          = set_retract_hard_mm;
    setter_table[PARAM_ID_RETRACT_H_F]           = set_retract_hard_speed;
    setter_table[PARAM_ID_RETRACT_S_MM]          = set_retract_soft_mm;
    setter_table[PARAM_ID_RETRACT_S_F]           = set_retract_soft_speed;
    setter_table[PARAM_ID_BEST_OF]               = set_samples_pool_size;
    setter_table[PARAM_ID_USEHWTIMTER]           = set_use_hw_timer;
    setter_table[PARAM_ID_FAVG_SIZE]             = set_favg_size;
    setter_table[PARAM_ID_VDROP_THRESH]          = set_vdrop_thresh;
    setter_table[PARAM_ID_VFD_DROPS_BLOCK]       = set_vdrops_block;
    setter_table[PARAM_ID_VFD_SHORTS_POFF]       = set_vdrops_poff;
    setter_table[PARAM_ID_POFF_DURATION]         = set_poff_duration;
    setter_table[PARAM_ID_EARLY_X_ON]            = set_early_x_on;
    setter_table[PARAM_ID_LINE_ENDS]             = set_line_end_confirmations;
    setter_table[PARAM_ID_RESET_QUEUE]           = set_reset_vsense_queue;
    setter_table[PARAM_ID_POFF_AFTER]            = set_poff_after;
    setter_table[PARAM_ID_USE_STOPGO]            = set_use_stopgo_signals;
    setter_table[PARAM_ID_ZERO_JITTER]           = set_zero_jitter;
    setter_table[PARAM_ID_READINGS_H]            = set_readings_high;
    setter_table[PARAM_ID_READINGS_L]            = set_readings_low;
    setter_table[PARAM_ID_ZERO_THRESH]           = set_zero_threshold;
    setter_table[PARAM_ID_DPM_UART]              = set_dpm_uart_enabled;
    setter_table[PARAM_ID_DPM_ONOFF]             = set_dpm_power_onoff;
    setter_table[PARAM_ID_DPM_VOLTAGE]           = set_dpm_voltage;
    setter_table[PARAM_ID_DPM_CURRENT]           = set_dpm_current;
    setter_table[PARAM_ID_X_RES]                 = set_motor_res;
    setter_table[PARAM_ID_Y_RES]                 = set_motor_res;
    setter_table[PARAM_ID_Z_RES]                 = set_motor_res;
    setter_table[PARAM_ID_ESP_RESTART]           = set_esp_restart;
    setter_table[PARAM_ID_DPM_PROBE_V]           = set_dpm_probe_voltage;
    setter_table[PARAM_ID_DPM_PROBE_C]           = set_dpm_probe_current;
    setter_table[PARAM_ID_PROBE_FREQ]            = set_probe_frequency;
    setter_table[PARAM_ID_PROBE_DUTY]            = set_probe_duty;
    setter_table[PARAM_ID_PROBE_TR_V]            = set_probe_vfd_trigger;
    setter_table[PARAM_ID_PROBE_TR_C]            = set_probe_cfd_trigger;
    setter_table[PARAM_ID_SPINDLE_ONOFF]         = set_spindle_onoff;
    setter_table[PARAM_ID_REAMER_DIST]           = set_reamer_distance;
    setter_table[PARAM_ID_REAMER_DURA]           = set_reamer_duration;
    setter_table[PARAM_ID_SINK_DIST]             = set_sinker_distance;
    setter_table[PARAM_ID_SIMULATION]            = set_simulate_gcode;
    setter_table[PARAM_ID_SINK_DIR]              = set_sink_dir;
    setter_table[PARAM_ID_PROBE_ACTION]          = set_probing_action;
    setter_table[PARAM_ID_SET_PROBE_POS]         = set_probing_pos_action;
    setter_table[PARAM_ID_UNSET_PROBE_POS]       = set_probing_pos_unset;
    setter_table[PARAM_ID_HOME_ALL]              = set_home_all;
    setter_table[PARAM_ID_HOME_AXIS]             = set_home_axis;
    setter_table[PARAM_ID_HOME_ENA]              = set_homing_enabled;
    setter_table[PARAM_ID_HOME_SEQ]              = set_home_axis;
    setter_table[PARAM_ID_MOVE_TO_ORIGIN]        = set_move_to_origin;
    setter_table[PARAM_ID_FLUSHING_INTERVAL]     = set_flushing_interval;
    setter_table[PARAM_ID_FLUSHING_DISTANCE]     = set_flushing_distance;
    setter_table[PARAM_ID_FLUSHING_FLUSH_OFFSTP] = set_flushing_offset_steps;
    setter_table[PARAM_ID_FLUSHING_FLUSH_NOSPRK] = set_flushing_disable_spark;
    // this could be merged into a single function 
    setter_table[PARAM_ID_XRIGHT]                = set_jog_axis_x_positive;
    setter_table[PARAM_ID_XLEFT]                 = set_jog_axis_x_negative;
    setter_table[PARAM_ID_YUP]                   = set_jog_axis_y_positive;
    setter_table[PARAM_ID_YDOWN]                 = set_jog_axis_y_negative;
    setter_table[PARAM_ID_ZUP]                   = set_jog_axis_z_positive;
    setter_table[PARAM_ID_ZDOWN]                 = set_jog_axis_z_negative;
    setter_table[PARAM_ID_SD_FILE_SET]           = set_gcode_file_has;
    setter_table[PARAM_ID_SET_PAGE]              = set_page_current;
    setter_table[PARAM_ID_SET_STOP_EDM]          = set_stop_edm_process;
    setter_table[PARAM_ID_EDM_PAUSE]             = set_edm_pause;
    setter_table[PARAM_ID_TOOLDIAMETER]          = set_tooldiameter;
    setter_table[PARAM_ID_SPINDLE_SPEED_PROBING] = set_spindle_speed_probing;

    //#######################################################################
    // Getter callback
    //#######################################################################
    const std::function<void(getter_param *item)> get_callback = [](getter_param *item){
        if( item->param_id < 0 || item->param_id >= TOTAL_PARAM_ITEMS ){
          return;
        }
        if( getter_table[item->param_id] != nullptr ){
            getter_table[item->param_id]( item );
        }
    };

    //#######################################################################
    // Setter callback
    //#######################################################################
    const std::function<void(setter_param *item)> set_callback = [](setter_param *item){
        if( item->first < 0 || item->first >= TOTAL_PARAM_ITEMS ){
          return;
        }
        if( setter_table[item->first] != nullptr ){
            setter_table[item->first]( item );
        }
    };

    //#######################################################################
    // Bind callbacks
    //#######################################################################
    ui_controller.on(PARAM_GET,get_callback);
    ui_controller.on(PARAM_SET,set_callback);

}



