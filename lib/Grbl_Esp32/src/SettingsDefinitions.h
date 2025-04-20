#pragma once

extern AxisMaskSetting* step_invert_mask;
extern AxisMaskSetting* dir_invert_mask;
extern AxisMaskSetting* homing_dir_mask;
extern AxisMaskSetting* homing_squared_axes;
extern AxisMaskSetting* homing_cycle[MAX_N_AXIS];
extern FlagSetting* step_enable_invert;
extern FlagSetting* limit_invert;
extern FlagSetting* probe_invert;
extern FlagSetting* report_inches;
extern FlagSetting* soft_limits;
extern FloatSetting* arc_tolerance;
extern FloatSetting* homing_debounce;
extern FloatSetting* homing_pulloff;