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

#include "api.h"
#include "Report.h"
#include "GCode.h"
#include "widgets/ui_interface.h"

#include "widgets/char_helpers.h"

namespace api{

    bool buffer_available(){
        //return inputBuffer.available() < 200 ? true : false;
        return inputBuffer.peek() == -1 ? true : false;
    }

    void push_cmd(const char *text, bool block_until_idle ){ 
        portENTER_CRITICAL(&myMutex);
        ui.emit(LOG_LINE,text);
        inputBuffer.push(text); 
        portEXIT_CRITICAL(&myMutex);
        vTaskDelay(10);
        if( block_until_idle ){
            vTaskDelay(50);
            block_while_not_idle();
            while( inputBuffer.peek() != -1 ){
                vTaskDelay(50);
            }
        }
    }


    // get the max possible travel distance in mm for an axis into given direction
    // direction 1 = negative, 0 = positive
    // if will compensate the homing pulloff distance
    // not 100% exact
    // will return a negative value for max negative travel
    // will return a positive value for max positive minus homing pulloff 
    float get_max_travel_possible( int axis, int direction ){ 
        float* current_position = system_get_mpos(); 
        float current[MAX_N_AXIS];
        memcpy( current, current_position, sizeof(current_position[0]) * N_AXIS );
        float travel;
        if( direction ){
            // negative direction
            travel  = glimits.limitsMinPosition( axis ); // -100.0 example for final min pos
            travel -= current[axis];                     // current = -60 = -40 (example with machine at -60 pos)
            travel += 0.1;                               // remove a few steps just in case // -40+0,1 = -39,9
            travel  = MIN(0,travel);
        } else {
            // positive direction
            travel  = current[axis] * -1;
            travel -= DEFAULT_HOMING_PULLOFF;
            travel  = MAX(0,travel);
        }
        return travel;
    }


    int32_t get_step_position( int axis ){
      return sys_position[axis];
    }


    void update_speeds(){
        int slowest_axis                 = motor_manager.get_slowest_axis();
        process_speeds.RAPID             = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.RAPID );  
        process_speeds.EDM               = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.EDM );
        process_speeds.PROBING           = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.PROBING );
        process_speeds.REPROBE           = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.REPROBE );
        process_speeds.RETRACT           = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.RETRACT );
        process_speeds.HOMING_FEED       = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.HOMING_FEED );
        process_speeds.HOMING_SEEK       = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.HOMING_SEEK );
        process_speeds.MICROFEED         = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.MICROFEED );
        process_speeds.WIRE_RETRACT_HARD = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.WIRE_RETRACT_HARD );
        process_speeds.WIRE_RETRACT_SOFT = motor_manager.get_motor( slowest_axis )->get_step_delay_for_speed( process_speeds_mm_min.WIRE_RETRACT_SOFT );
    }

    void autohome_at_center(){
        push_cmd("$HCT\r");

    }

    void unlock_machine(){
        /** enable steppers and unlock machine state**/
        push_cmd("$X\r");
        vTaskDelay(50);
    }
    bool machine_is_idle(){
        return ( ( sys.state == State::Alarm || sys.state == State::Idle ) && ! gconf.gedm_planner_line_running) ? true : false;
    }
    void block_while_not_idle(){
        int count = 0;
        while( ! machine_is_idle() ){
            if( sys.abort || gconf.gedm_stop_process || sys_rt_exec_state.bit.motionCancel ){
                if( ++count > 20 ){ break; }
            }
            vTaskDelay(50);
        }
    }
    void probe_block(){
        idle_timer = esp_timer_get_time();
        while( ! sys.probe_succeeded || sys_probe_state == Probe::Active ){
            if( gconf.gedm_stop_process || sys.abort || sys_rt_exec_state.bit.motionCancel || sys.state == State::Alarm ){ break; }
            vTaskDelay(500);
        }
        idle_timer = esp_timer_get_time();
    }
    void block_while_homing( int axis_index ){
        while( ! sys_axis_homed[axis_index] || gconf.gedm_planner_line_running ){
            if( sys.abort || sys_rt_exec_state.bit.motionCancel ){ break; }
            vTaskDelay(10);
        }
        if( machine_is_fully_homed() ){
            //push_cmd("G90 G54 X-30 Y-30\r\n");
            push_cmd("G28.1\r\n");                 // set home position
        }
        block_while_not_idle();
    }
    void cancel_running_job(){
      sys_rt_exec_state.bit.motionCancel = true;
      //filehandler.close_file();
    }
    void show_positions(){
      float* current_position = system_get_mpos();
      mpos_to_wpos(current_position);
    }

    void machine_to_work_pos( float *position ){
        mpos_to_wpos( position );
    }

    IRAM_ATTR float* get_wpos(){
        float* current_position = system_get_mpos();
        mpos_to_wpos(current_position);
        return current_position;
    }
    bool machine_is_fully_homed(){
        bool fully_homed = true;
        for( int i = 0; i < N_AXIS; ++i ){
            if( i == Z_AXIS && z_no_home ){
                // skip Z if homing is not wanted. Electrode wears and if it starts to get shorter homing may be a problem.
                continue;
            }
            if( ! sys_axis_homed[i] ){
                fully_homed = false;
            }
        }
        return fully_homed;
    }

    void home_axis( int axis ){
        switch (axis){
            case X_AXIS:
                home_x();
                break;
            case Y_AXIS:
                home_y();
                break;
            case Z_AXIS:
                if( ! z_no_home ){ home_z(); } 
                break;
        }
        block_while_not_idle();
    }

    void home_machine(){
        if( ! z_no_home ){ home_z(); } 
        home_x();
        home_y();
        block_while_not_idle();
    }
    void home_x(){
        if( sys_rt_exec_state.bit.motionCancel ){ return; }
        sys_axis_homed[X_AXIS] = false;
        push_cmd( "$HX\r" );
        block_while_homing( X_AXIS );
        vTaskDelay(500);
    }
    void home_y(){
        if( sys_rt_exec_state.bit.motionCancel ){ return; }
        sys_axis_homed[Y_AXIS] = false;
        push_cmd( "$HY\r" );
        block_while_homing( Y_AXIS );
        vTaskDelay(500);
    }
    void home_z(){
        if( sys_rt_exec_state.bit.motionCancel ){ return; }
        #ifdef Z_AXIS_NOT_USED
            sys_axis_homed[Z_AXIS] = true;
        #else
            sys_axis_homed[Z_AXIS] = false;
            push_cmd( "$HZ\r" );
            block_while_homing( Z_AXIS );
            vTaskDelay(500);
        #endif
    }

    void force_home( int axis ){ // don't use!
        if( sys_position[axis] <= (DEFAULT_HOMING_PULLOFF*-1) * g_axis[axis]->steps_per_mm.get() ){
        //if( sys_position[axis] <= (DEFAULT_HOMING_PULLOFF*-1) * axis_settings[ axis ]->steps_per_mm->get() ){
                return;
        }
        sys_position[axis] = (DEFAULT_HOMING_PULLOFF*-1) * g_axis[axis]->steps_per_mm.get(); 
        //sys_position[axis] = (DEFAULT_HOMING_PULLOFF*-1) * axis_settings[ axis ]->steps_per_mm->get(); 
        gcode_core.gc_sync_position();
    }

    void reset_probe_points(){
        for( int i = 0; i < N_AXIS; ++i ){
            sys_probed_axis[i] = false;
        }
    }
    void set_current_position_as_probe_position(){
        sys.probe_succeeded = true;
        memcpy(sys_probe_position, sys_position, sizeof(sys_position));
        for( int i = 0; i < N_AXIS; ++i ){
            sys_probed_axis[i]          = true;
            sys_probe_position_final[i] = sys_probe_position[i];
        }
        push_cmd("G91 G10 P1 L20 X0 Y0 Z0\r\n");
        vTaskDelay(500);
    }
    bool machine_is_fully_probed(){
        bool is_fully_probed = true;
        for( int i = 0; i < N_AXIS; ++i ){
            if( ! sys_probed_axis[i] ){
                is_fully_probed = false;
            }
        }
        return is_fully_probed;
    }

    float probe_pos_to_mpos( int axis ){
        static float probe_position[MAX_N_AXIS];
        system_convert_array_steps_to_mpos( probe_position, sys_probe_position_final );
        return probe_position[axis];
    }


    void unset_probepositions_for( int axis ){
        sys_probed_axis[axis]          = false;
        sys_probe_position_final[axis] = 0;
    };

    void auto_set_probepositions_for( int axis ){
        char command_buf[40];
        sprintf(command_buf,"G91 G10 P1 L20 %c0\r\n", get_axis_name(axis));  
        sys_probed_axis[axis]          = true;
        sys_probe_position_final[axis] = sys_position[axis];
        push_cmd(command_buf,true);
        vTaskDelay(100);
    };

    // set the current position as probe position for each axis
    // IF the axis is not probed
    void auto_set_probepositions(){
      for (int i = 0; i < N_AXIS; ++i)
      {
        if (!sys_probed_axis[i])
        {

          if (i == X_AXIS)
          {
            push_cmd("G91 G10 P1 L20 X0\r\n"); 
          }
          else if (i == Y_AXIS)
          {
            push_cmd("G91 G10 P1 L20 Y0\r\n");
          }
          else if (i == Z_AXIS)
          {
            push_cmd("G91 G10 P1 L20 Z0\r\n");
          }
          sys_probed_axis[i]          = true;
          sys_probe_position_final[i] = sys_position[i];
        }
      }
      push_cmd("G90 G54 G21\r\n",true);
      vTaskDelay(1000);
    }

    void set_reprobe_points(){
        float* current_position = system_get_mpos();
        float position_copy[MAX_N_AXIS];
        memcpy(position_copy, current_position, sizeof(current_position[0]) * N_AXIS);
        set_reprobe_point_x( position_copy[ X_AXIS ] );
        set_reprobe_point_y( position_copy[ Y_AXIS ] );
    }
    void set_reprobe_point_x( float x ){
        gconf.gedm_probe_position_x = x;
    }
    void set_reprobe_point_y( float y ){
        gconf.gedm_probe_position_y = y;
    }

    bool jog_axis( float mm, const char *axis_name, int speed, int axis ){ // todo: remove axis name requirenment
        if(  gconf.gedm_planner_line_running ){
          sys_rt_exec_state.bit.motionCancel = true;
          while( gconf.gedm_planner_line_running ){
            vTaskDelay(1);
            sys_rt_exec_state.bit.motionCancel = true;
          }
        }
        block_while_not_idle();
        sys_rt_exec_state.bit.motionCancel = false;
        float* current_position = system_get_mpos();
        float position_copy[MAX_N_AXIS];
        memcpy(position_copy, current_position, sizeof(current_position[0]) * N_AXIS);
        position_copy[axis] += mm;

        if( glimits.limitsCheckTravel( position_copy ) ){
            return false;
        }
        // cancel current jog motion if there is any
        char command_buf[40];
        sprintf(command_buf,"$J=G91 G21 %s%.5f F%d\r\n", axis_name, mm, speed);    
        // push new jog command to input buffer
        push_cmd( command_buf );
        return true;
    }
    bool jog_up( float mm ){
        #ifdef Z_AXIS_NOT_USED
            return true;
        #endif
        return jog_axis( mm, "Z", Z_AXIS_JOG_SPEED, Z_AXIS );
    }
    bool jog_down( float mm ){
        #ifdef Z_AXIS_NOT_USED
            return true;
        #endif
        return jog_axis( (mm*-1), "Z", Z_AXIS_JOG_SPEED, Z_AXIS );
    }
    bool jog_forward( float mm ){
        return jog_axis( mm, "Y", X_AXIS_JOG_SPEED, Y_AXIS );
    }
    bool jog_back( float mm ){
        return jog_axis( (mm*-1), "Y", X_AXIS_JOG_SPEED, Y_AXIS );
    }
    bool jog_left( float mm ){
        return jog_axis( mm, "X", X_AXIS_JOG_SPEED, X_AXIS );
    }
    bool jog_right( float mm ){
        return jog_axis( (mm*-1), "X", X_AXIS_JOG_SPEED, X_AXIS );
    }



};