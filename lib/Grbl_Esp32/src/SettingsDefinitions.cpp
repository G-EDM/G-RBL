#include "Grbl.h"


AxisMaskSetting* step_invert_mask;
AxisMaskSetting* dir_invert_mask;
AxisMaskSetting* homing_dir_mask;
AxisMaskSetting* homing_squared_axes;
FlagSetting*     step_enable_invert;
FlagSetting*     limit_invert;
FlagSetting*     probe_invert;
FlagSetting*     report_inches;
FlagSetting*     soft_limits;
FloatSetting*    arc_tolerance;
FloatSetting*    homing_debounce;
FloatSetting*    homing_pulloff;
AxisMaskSetting* homing_cycle[MAX_N_AXIS];



// Construct e.g. X_MAX_RATE from axisName "X" and tail "_MAX_RATE"
// in dynamically allocated memory that will not be freed.

static const char* makename(const char* axisName, const char* tail) {
    char* retval = (char*)malloc(strlen(axisName) + strlen(tail) + 2);

    strcpy(retval, axisName);
    strcat(retval, "/");
    return strcat(retval, tail);
}

static bool checkStartupLine(char* value) {
    if (!value) {  // No POST functionality
        return true;
    }
    if (sys.state != State::Idle) {
        return false;
    }
    return gcode_core.gc_execute_line(value) == Error::Ok;
}

static bool postMotorSetting(char* value) {
    if (!value) {
        motor_manager.motors_read_settings();
    }
    return true;
}

// Generates a string like "122" from axisNum 2 and base 120
static const char* makeGrblName(int axisNum, int base) {
    // To omit A,B,C axes:
    // if (axisNum > 2) return NULL;
    char buf[4];
    snprintf(buf, 4, "%d", axisNum + base);
    char* retval = (char*)malloc(strlen(buf));
    return strcpy(retval, buf);
}

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



    homing_debounce     = new FloatSetting(GRBL, "26", "Homing/Debounce", DEFAULT_HOMING_DEBOUNCE_DELAY, 0, 10000);
    homing_squared_axes = new AxisMaskSetting(EXTENDED, NULL, "Homing/Squared", DEFAULT_HOMING_SQUARED_AXES);
    homing_dir_mask     = new AxisMaskSetting(GRBL, "23", "Homing/DirInvert", DEFAULT_HOMING_DIR_MASK);
    soft_limits         = new FlagSetting(GRBL, "20", "Limits/Soft", DEFAULT_SOFT_LIMIT_ENABLE, NULL);
    report_inches       = new FlagSetting(GRBL, "13", "Report/Inches", DEFAULT_REPORT_INCHES);
   
    // TODO Settings - also need to clear, but not set, soft_limits
    arc_tolerance      = new FloatSetting(GRBL, "12", "GCode/ArcTolerance", DEFAULT_ARC_TOLERANCE, 0, 1);
    probe_invert       = new FlagSetting(GRBL, "6", "Probe/Invert", DEFAULT_INVERT_PROBE_PIN);
    limit_invert       = new FlagSetting(GRBL, "5", "Limits/Invert", DEFAULT_INVERT_LIMIT_PINS);
    step_enable_invert = new FlagSetting(GRBL, "4", "Stepper/EnableInvert", DEFAULT_INVERT_ST_ENABLE);
    dir_invert_mask    = new AxisMaskSetting(GRBL, "3", "Stepper/DirInvert", DEFAULT_DIRECTION_INVERT_MASK, postMotorSetting);
    step_invert_mask   = new AxisMaskSetting(GRBL, "2", "Stepper/StepInvert", DEFAULT_STEPPING_INVERT_MASK, postMotorSetting);

    homing_cycle[5] = new AxisMaskSetting(EXTENDED, NULL, "Homing/Cycle5", DEFAULT_HOMING_CYCLE_5);
    homing_cycle[4] = new AxisMaskSetting(EXTENDED, NULL, "Homing/Cycle4", DEFAULT_HOMING_CYCLE_4);
    homing_cycle[3] = new AxisMaskSetting(EXTENDED, NULL, "Homing/Cycle3", DEFAULT_HOMING_CYCLE_3);
    homing_cycle[2] = new AxisMaskSetting(EXTENDED, NULL, "Homing/Cycle2", DEFAULT_HOMING_CYCLE_2);
    homing_cycle[1] = new AxisMaskSetting(EXTENDED, NULL, "Homing/Cycle1", DEFAULT_HOMING_CYCLE_1);
    homing_cycle[0] = new AxisMaskSetting(EXTENDED, NULL, "Homing/Cycle0", DEFAULT_HOMING_CYCLE_0);
    
}
