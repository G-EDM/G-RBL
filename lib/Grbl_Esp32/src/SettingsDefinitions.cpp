#include "Grbl.h"


AxisMaskSetting* homing_dir_mask;
AxisMaskSetting* homing_squared_axes;
FlagSetting*     limit_invert;
FlagSetting*     soft_limits;
AxisMaskSetting* homing_cycle[MAX_N_AXIS];




void make_coordinate(CoordIndex index, const char* name) {
    float coord_data[MAX_N_AXIS] = { 0.0 };
    auto  coord                  = new Coordinates(name);
    coords[index]                = coord;
    if (!coord->load()) {
        coords[index]->setDefault();
    }
}
void make_settings() {
    Setting::init();

    // Propagate old coordinate system data to the new format if necessary.
    // G54 - G59 work coordinate systems, G28, G30 reference positions, etc
    make_coordinate(CoordIndex::G54, "G54");
    make_coordinate(CoordIndex::G55, "G55");
    make_coordinate(CoordIndex::G56, "G56");
    make_coordinate(CoordIndex::G57, "G57");
    make_coordinate(CoordIndex::G58, "G58");
    make_coordinate(CoordIndex::G59, "G59");
    make_coordinate(CoordIndex::G28, "G28");
    make_coordinate(CoordIndex::G30, "G30");


    //###############################################
    // Lesser footprint etc...
    //###############################################
    axis_conf* defaults;
    for( int axis = 0; axis < MAX_N_AXIS; ++axis ){
        defaults     = &axis_configuration[axis];
        g_axis[axis] = new GAxis( defaults->name );
        g_axis[axis]->steps_per_mm.set( defaults->steps_per_mm );
        g_axis[axis]->home_position.set( defaults->home_mpos );
        g_axis[axis]->max_travel_mm.set( defaults->max_travel );
    }


    homing_squared_axes = new AxisMaskSetting(EXTENDED, NULL, "Homing/Squared", DEFAULT_HOMING_SQUARED_AXES);
    homing_dir_mask     = new AxisMaskSetting(GRBL, "23", "Homing/DirInvert", DEFAULT_HOMING_DIR_MASK);
    soft_limits         = new FlagSetting(GRBL, "20", "Limits/Soft", DEFAULT_SOFT_LIMIT_ENABLE, NULL);
   
    // TODO Settings - also need to clear, but not set, soft_limits
    limit_invert       = new FlagSetting(GRBL, "5", "Limits/Invert", DEFAULT_INVERT_LIMIT_PINS);

    homing_cycle[5] = new AxisMaskSetting(EXTENDED, NULL, "Homing/Cycle5", DEFAULT_HOMING_CYCLE_5);
    homing_cycle[4] = new AxisMaskSetting(EXTENDED, NULL, "Homing/Cycle4", DEFAULT_HOMING_CYCLE_4);
    homing_cycle[3] = new AxisMaskSetting(EXTENDED, NULL, "Homing/Cycle3", DEFAULT_HOMING_CYCLE_3);
    homing_cycle[2] = new AxisMaskSetting(EXTENDED, NULL, "Homing/Cycle2", DEFAULT_HOMING_CYCLE_2);
    homing_cycle[1] = new AxisMaskSetting(EXTENDED, NULL, "Homing/Cycle1", DEFAULT_HOMING_CYCLE_1);
    homing_cycle[0] = new AxisMaskSetting(EXTENDED, NULL, "Homing/Cycle0", DEFAULT_HOMING_CYCLE_0);
    
}
