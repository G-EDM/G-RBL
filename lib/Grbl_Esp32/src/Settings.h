#pragma once

#include <map>
#include <nvs.h>

// Initialize the configuration subsystem
void settings_init();

// Define settings restore bitflags.
enum SettingsRestore {
    Defaults     = bit(0),
    Parameters   = bit(1),
    StartupLines = bit(2),
    // BuildInfo = bit(3), // Obsolete
    Wifi = bit(4),
    All  = 0xff,
};

// Restore subsets of settings to default values
void settings_restore(uint8_t restore_flag);

// Command::List is a linked list of all settings,
// so common code can enumerate them.
class Command;
// extern Command *CommandsList;

// This abstract class defines the generic interface that
// is used to set and get values for all settings independent
// of their underlying data type.  The values are always
// represented as human-readable strings.  This generic
// interface is used for managing settings via the user interface.

// Derived classes implement these generic functions for different
// kinds of data.  Code that accesses settings should use only these
// generic functions and should not use derived classes directly.

enum {
    NO_AXIS = 255,
};
typedef enum : uint8_t {
    GRBL = 1,  // Classic GRBL settings like $100
    EXTENDED,  // Settings added by early versions of Grbl_Esp32
    WEBSET,    // Settings for ESP3D_WebUI, stored in NVS
    GRBLCMD,   // Non-persistent GRBL commands like $H
    WEBCMD,    // ESP3D_WebUI commands that are not directly settings
} type_t;


typedef uint8_t axis_t;

class Word {
protected:
    const char*   _description;
    const char*   _grblName;
    const char*   _fullName;
    type_t        _type;

public:
    Word(type_t type, const char* description, const char* grblName, const char* fullName);
    type_t        getType() { return _type; }
    const char*   getName() { return _fullName; }
    const char*   getGrblName() { return _grblName; }
    const char*   getDescription() { return _description; }
};

class Command : public Word {
protected:
    Command* link;  // linked list of setting objects
    bool (*_cmdChecker)();

public:
    static Command* List;
    Command*        next() { return link; }

    ~Command() {}
    Command(const char* description, type_t type, const char* grblName, const char* fullName, bool (*cmdChecker)());

    virtual Error action(char* value) = 0;
};

class Setting : public Word {
private:
protected:
    // group_t _group;
    axis_t   _axis = NO_AXIS;
    Setting* link;  // linked list of setting objects

    bool (*_checker)(char*);
    const char* _keyName;

public:
    static nvs_handle _handle;
    static void       init();
    static Setting*   List;
    Setting*          next() { return link; }

    Error check(char* s);

    static Error report_nvs_stats(const char* value) {
        nvs_stats_t stats;
        if (esp_err_t err = nvs_get_stats(NULL, &stats)) {
            return Error::NvsGetStatsFailed;
        }
#if 0  // The SDK we use does not have this yet
        nvs_iterator_t it = nvs_entry_find(NULL, NULL, NVS_TYPE_ANY);
        while (it != NULL) {
            nvs_entry_info_t info;
            nvs_entry_info(it, &info);
            it = nvs_entry_next(it);
        }
#endif
        return Error::Ok;
    }

    static Error eraseNVS(const char* value) {
        nvs_erase_all(_handle);
        return Error::Ok;
    }

    ~Setting() {}
    // Setting(const char *description, group_t group, const char * grblName, const char* fullName, bool (*checker)(char *));
    Setting(const char* description, type_t type, const char* grblName, const char* fullName, bool (*checker)(char*));
    axis_t getAxis() { return _axis; }
    void   setAxis(axis_t axis) { _axis = axis; }

    // load() reads the backing store to get the current
    // value of the setting.  This could be slow so it
    // should be done infrequently, typically once at startup.
    virtual void load() {};
    virtual void setDefault() {};
    virtual Error       setStringValue(char* value) = 0;
    Error               setStringValue(String s) { return setStringValue(s.c_str()); }
    virtual const char* getStringValue() = 0;
    virtual const char* getCompatibleValue() { return getStringValue(); }
    virtual const char* getDefaultString() = 0;
};

class IntSetting : public Setting {
private:
    int32_t _defaultValue;
    int32_t _currentValue;
    int32_t _storedValue;
    int32_t _minValue;
    int32_t _maxValue;
    bool    _currentIsNvm;

public:
    IntSetting(const char*   description,
               type_t        type,
               const char*   grblName,
               const char*   name,
               int32_t       defVal,
               int32_t       minVal,
               int32_t       maxVal,
               bool (*checker)(char*),
               bool currentIsNvm = false);

    IntSetting(type_t        type,
               const char*   grblName,
               const char*   name,
               int32_t       defVal,
               int32_t       minVal,
               int32_t       maxVal,
               bool (*checker)(char*) = NULL,
               bool currentIsNvm      = false) :
        IntSetting(NULL, type, grblName, name, defVal, minVal, maxVal, checker, currentIsNvm) {}

    void        load();
    void        setDefault();
    Error       setStringValue(char* value);
    const char* getStringValue();
    const char* getDefaultString();

    int32_t get() { return _currentValue; }
};

class AxisMaskSetting : public Setting {
private:
    int32_t _defaultValue;
    int32_t _currentValue;
    int32_t _storedValue;

public:
    AxisMaskSetting(const char*   description,
                    type_t        type,
                    const char*   grblName,
                    const char*   name,
                    int32_t       defVal,
                    bool (*checker)(char*));

    AxisMaskSetting(
        type_t type , const char* grblName, const char* name, int32_t defVal, bool (*checker)(char*) = NULL) :
        AxisMaskSetting(NULL, type, grblName, name, defVal, checker) {}

    void        load();
    void        setDefault();
    Error       setStringValue(char* value);
    const char* getCompatibleValue();
    const char* getStringValue();
    const char* getDefaultString();

    int32_t get() { return _currentValue; }
};

class Coordinates {
private:
    float       _currentValue[MAX_N_AXIS];
    const char* _name;

public:
    Coordinates(const char* name) : _name(name) {}

    const char* getName() { return _name; }
    bool        load();
    void        setDefault() {
        float zeros[MAX_N_AXIS] = {
            0.0,
        };
        set(zeros);
    };
    // Copy the value to an array
    void get(float* value) { memcpy(value, _currentValue, sizeof(_currentValue)); }
    // Return a pointer to the array
    const float* get() { return _currentValue; }
    void         set(float* value);
};

extern Coordinates* coords[CoordIndex::End];

class FloatSetting : public Setting {
private:
    float _defaultValue;
    float _currentValue;
    float _storedValue;
    float _minValue;
    float _maxValue;

public:
    FloatSetting(const char*   description,
                 type_t        type,
                 const char*   grblName,
                 const char*   name,
                 float         defVal,
                 float         minVal,
                 float         maxVal,
                 bool (*checker)(char*));

    FloatSetting(type_t        type,
                 const char*   grblName,
                 const char*   name,
                 float         defVal,
                 float         minVal,
                 float         maxVal,
                 bool (*checker)(char*) = NULL) :
        FloatSetting(NULL, type, grblName, name, defVal, minVal, maxVal, checker) {}

    void load();
    void setDefault();
    Error       setStringValue(char* value);
    const char* getStringValue();
    const char* getDefaultString();

    float get() { return _currentValue; }
};




struct cmp_str {
    bool operator()(char const* a, char const* b) const { return strcasecmp(a, b) < 0; }
};



class FlagSetting : public Setting {
private:
    bool   _defaultValue;
    int8_t _storedValue;
    bool   _currentValue;

public:
    FlagSetting(const char*   description,
                type_t        type,
                const char*   grblName,
                const char*   name,
                bool          defVal,
                bool (*checker)(char*));
    FlagSetting(type_t type, const char* grblName, const char* name, bool defVal, bool (*checker)(char*) = NULL) :
        FlagSetting(NULL, type, grblName, name, defVal, checker) {}

    void load();
    void setDefault();
    Error       setStringValue(char* value);
    const char* getCompatibleValue();
    const char* getStringValue();
    const char* getDefaultString();

    bool get() { return _currentValue; }
};



class AxisSettings {
public:
    const char*   name;
    AxisSettings(const char* axisName);
};

extern bool idleOrJog();
extern bool idleOrAlarm();
extern bool anyState();
extern bool notCycleOrHold();

class GrblCommand : public Command {
private:
    Error (*_action)(const char*);

public:
    GrblCommand(const char* grblName,
                const char* name,
                Error (*action)(const char*),
                bool (*cmdChecker)()) :
        Command(NULL, GRBLCMD, grblName, name, cmdChecker),
        _action(action) {}

    Error action(char* value);
};

template <typename T>
class FakeSetting {
private:
    T _value;

public:
    FakeSetting(T value) : _value(value) {}

    T get() { return _value; }
};
