#include "probing_routines.h"


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


G_EDM_PROBE_ROUTINES::G_EDM_PROBE_ROUTINES(){}

void G_EDM_PROBE_ROUTINES::pulloff_axis( int axis, int direction ){
    float* current_position = system_get_mpos();
    char command_buf[40];
    float max_travel =  direction ? 2.0//  MIN( 2.0, get_max_travel_possible( axis, false ) )
                                  : -2.0;//( MIN( 2.0, get_max_travel_possible( axis, true ) ) ) * -1;
    char axis_name = get_axis_name( axis );
    sprintf(command_buf,"G54 G91 G0 %c%.5f\r\n", axis_name, max_travel );
    api::push_cmd( command_buf );
}

void G_EDM_PROBE_ROUTINES::probe_axis( int axis, int direction ){
    probe_touched           = false;
    sys.probe_succeeded     = false;
    
    /** send the probing command and set a max distance to probe **/
    char command_buf[40];
    char axis_name = get_axis_name( axis );
    
    float max_travel = direction ? api::get_max_travel_possible( axis, true )   // probe negative
                                 : api::get_max_travel_possible( axis, false ); // probe positive

    sprintf(command_buf,"G91 G38.2 %c%.5f F40\r\n", axis_name, max_travel );
    api::push_cmd( command_buf );
    api::probe_block();

}
void G_EDM_PROBE_ROUTINES::probe_z( float mm ){
    probe_axis( Z_AXIS, ( mm<0?true:false ) );    
    if(!sys_rt_exec_state.bit.motionCancel && sys.probe_succeeded ){
        sys_probe_position_final[Z_AXIS] = sys_probe_position[Z_AXIS];
        sys_probed_axis[Z_AXIS] = true;    
        api::push_cmd( "G91 G10 P1 L20 Z0\r\n" );
        pulloff_axis( Z_AXIS, true );
    }
    vTaskDelay(100);
    api::block_while_not_idle();
}

void G_EDM_PROBE_ROUTINES::probe_x_negative(){
    probe_x( -1.0, true );
}
void G_EDM_PROBE_ROUTINES::probe_x_positive(){
    probe_x( 1.0, true );
}
void G_EDM_PROBE_ROUTINES::probe_y_negative(){
    probe_y( -1.0, true );
}
void G_EDM_PROBE_ROUTINES::probe_y_positive(){
    probe_y( 1.0, true );
}



// mm is not the distance but indicates if it probes in positive or negative direction
// -1.0 and 1.0
void G_EDM_PROBE_ROUTINES::probe_x( float mm, bool pulloff ){
    probe_axis( X_AXIS, ( mm<0?true:false ) );
    char command_buf[40];
    if(!sys_rt_exec_state.bit.motionCancel && sys.probe_succeeded ){
        sys_probe_position_final[X_AXIS] = sys_probe_position[X_AXIS];
        sys_probed_axis[X_AXIS]          = true;
        float tool_radius                = tool_diameter>0.0?tool_diameter/2.0:0.0;
        sprintf(command_buf,"G91 G10 P1 L20 X%.5f\r\n", ( mm < 0 ? tool_radius : (tool_radius*-1) ) );
        api::push_cmd( command_buf );
        if(pulloff){
            pulloff_axis( X_AXIS, ( mm<0 ? true : false ) );
        }
    }
    vTaskDelay(100);
    api::block_while_not_idle();
}
void G_EDM_PROBE_ROUTINES::probe_y( float mm, bool pulloff ){
    probe_axis( Y_AXIS, ( mm<0?true:false ) );
    char command_buf[40];
    if(!sys_rt_exec_state.bit.motionCancel && sys.probe_succeeded ){
        sys_probe_position_final[Y_AXIS] = sys_probe_position[Y_AXIS];
        sys_probed_axis[Y_AXIS] = true;
        float tool_radius = tool_diameter>0.0?tool_diameter/2.0:0.0;
        sprintf(command_buf,"G91 G10 P1 L20 Y%.5f\r\n", ( mm < 0 ? tool_radius : (tool_radius*-1) ) );
        api::push_cmd( command_buf );
        if(pulloff){
            pulloff_axis( Y_AXIS, ( mm<0 ? true : false ) );
        }
    }
    vTaskDelay(100);
    api::block_while_not_idle();
}

/**
 * 2D probing routines 
 * They are meant for wire mode where Z axis is disabled
 * The center button in 2D mode is a center finder for holes
 **/
void G_EDM_PROBE_ROUTINES::left_front_edge_2d(){
    probe_x(1.0,true);                // x axis probing from left to right and retracting a little after probing
    api::push_cmd("G91 G0 Y-10\r\n"); // move y towards operator
    api::push_cmd("G91 G0 X5\r\n");   // move x to the right
    probe_y(1.0,true);                // y axis probing away from operator
    api::push_cmd("G90 G54 X-2 Y-2\r\n", true); // move xy 2mm away from zero position
}
void G_EDM_PROBE_ROUTINES::left_back_edge_2d(){
    probe_x(1.0,true);               // x axis probing from left to right and retracting a little after probing
    api::push_cmd("G91 G0 Y10\r\n"); // move y away from operator
    api::push_cmd("G91 G0 X5\r\n");  // move x to the right
    probe_y(-1.0,true);              // y axis probing towards operator
    api::push_cmd("G90 G54 X-2 Y2\r\n", true); // move xy 2mm away from zero position
}
void G_EDM_PROBE_ROUTINES::right_back_edge_2d(){
    probe_x(-1.0,true);              // x axis probing from right to left and retracting a little after probing
    api::push_cmd("G91 G0 Y10\r\n"); // move y away from operator
    api::push_cmd("G91 G0 X-5\r\n"); // move x to the left
    probe_y(-1.0,true);              // y axis probing towards operator
    api::push_cmd("G90 G54 X2 Y2\r\n", true); // move xy 2mm away from zero position
}
void G_EDM_PROBE_ROUTINES::right_front_edge_2d(){
    probe_x(-1.0,true);               // x axis probing from right to left and retracting a little after probing
    api::push_cmd("G91 G0 Y-10\r\n"); // move y towards operator
    api::push_cmd("G91 G0 X-5\r\n");  // move x to the left
    probe_y(1.0,true);                // y axis probing away from operator
    api::push_cmd("G90 G54 X2 Y-2\r\n", true); // move xy 2mm away from zero position
}
/** 
  * Electrode needs to be somewhere within the pocket
  * This routine doesn't generate any z motions
  **/
void G_EDM_PROBE_ROUTINES::center_finder_2d(){
    int y_neg, y_pos, x_neg, x_pos, x_target, y_target;
    char command_buf[40];
    float target_mpos;
    probe_x( -1.0, false );
    x_neg       = sys_probe_position_final[X_AXIS];
    api::push_cmd("G90 G0 X1.0\r\n");
    probe_x( 1.0, false );
    x_pos       = sys_probe_position_final[X_AXIS];
    x_target    = ( x_neg - x_pos ) / 2;
    // convert step positions to mpos and move to center of probe points
    target_mpos = system_convert_axis_steps_to_mpos( x_target, X_AXIS);
    sprintf(command_buf,"G90 G0 X%.5f F40\r\n", target_mpos );
    api::push_cmd( command_buf );
    api::block_while_not_idle();
    probe_y( -1.0, false );
    y_neg       = sys_probe_position_final[Y_AXIS];
    api::push_cmd("G90 G0 Y1.0\r\n");
    probe_y( 1.0, false );
    y_pos       = sys_probe_position_final[Y_AXIS];
    y_target    = ( y_neg - y_pos ) / 2;
    // convert step positions to mpos and move to center of probe points
    target_mpos = system_convert_axis_steps_to_mpos( y_target, Y_AXIS);
    sprintf(command_buf,"G90 G0 Y%.5f F40\r\n", target_mpos );
    api::push_cmd( command_buf );
    api::block_while_not_idle();
    // set work zero position
    api::push_cmd("G91 G10 P1 L20 X0 Y0\r\n");
    api::block_while_not_idle();
}





/**
 * 3D probing routines 
 * They are meant for floating or sinker mode where Z axis is enabled
 * The center button in 3D mode is z down probe
 **/
void G_EDM_PROBE_ROUTINES::left_front_edge_3d(){
    probe_z(-1.0); // first probe z
    api::push_cmd("G91 G0 X-10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_x(1.0,true);
    api::push_cmd("G91 G0 Z6\r\n", true);
    api::push_cmd("G91 G0 X5 Y-10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_y(1.0,true);
    api::push_cmd("G91 G0 Z6\r\n");
    api::push_cmd("G90 G54 G0 X0 Y0\r\n", true);
}
void G_EDM_PROBE_ROUTINES::left_back_edge_3d(){
    probe_z(-1.0);
    // probe_mode_off();
    api::push_cmd("G91 G0 X-10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_x(1.0,true);
    api::push_cmd("G91 G0 Z6\r\n", true);
    api::push_cmd("G91 G0 X5 Y10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_y(-1.0,true);
    api::push_cmd("G91 G0 Z6\r\n");
    api::push_cmd("G90 G54 G0 X0 Y0\r\n", true);
}
void G_EDM_PROBE_ROUTINES::right_back_edge_3d(){
    probe_z(-1.0);
    api::push_cmd("G91 G0 X10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_x(-1.0,true);
    api::push_cmd("G91 G0 Z6\r\n", true);
    api::push_cmd("G91 G0 X-5 Y10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_y(-1.0,true);
    api::push_cmd("G91 G0 Z6\r\n");
    api::push_cmd("G90 G54 G0 X0 Y0\r\n", true);
}
void G_EDM_PROBE_ROUTINES::right_front_edge_3d(){
    probe_z(-1.0);
    api::push_cmd("G91 G0 X10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_x(-1.0,true);
    api::push_cmd("G91 G0 Z6\r\n", true);
    api::push_cmd("G91 G0 X-5 Y-10\r\n");
    api::push_cmd("G91 G0 Z-6\r\n", true);
    probe_y(1.0,true);
    api::push_cmd("G91 G0 Z6\r\n");
    api::push_cmd("G90 G54 G0 X0 Y0\r\n", true);
}



 


