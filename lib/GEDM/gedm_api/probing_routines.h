#pragma once

#include "api.h"

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


class G_EDM_PROBE_ROUTINES
{
private:

    //float probing_pulloff;

public:
    G_EDM_PROBE_ROUTINES( void );
    /** Default XYZ probing **/
    void probe_x_negative( void );
    void probe_x_positive( void );
    void probe_y_negative( void );
    void probe_y_positive( void );

    void probe_x( float mm, bool pulloff = true );
    void probe_y( float mm, bool pulloff = true );
    void probe_z( float mm );
    /** 3D Edge finding with Z included **/
    void left_back_edge_3d( void );
    void left_front_edge_3d( void );
    void right_back_edge_3d( void );
    void right_front_edge_3d( void );
    /** 2D edge finding without Z **/
    void left_back_edge_2d( void );
    void left_front_edge_2d( void );
    void right_back_edge_2d( void );
    void right_front_edge_2d( void );
    /** 2D center finder **/
    void center_finder_2d( void );
    void  pulloff_axis( int axis, int direction );
    void  probe_axis( int axis, int direction );
};


