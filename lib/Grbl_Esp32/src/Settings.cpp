#include "Grbl.h"
#include <map>
#include <nvs.h>

bool anyState() {
    return false;
}
bool idleOrJog() {
    return sys.state != State::Idle && sys.state != State::Jog;
}
bool idleOrAlarm() {
    return sys.state != State::Idle && sys.state != State::Alarm;
}
bool notCycleOrHold() {
    return sys.state == State::Cycle && sys.state == State::Hold;
}

Word::Word(type_t type, const char* description, const char* grblName, const char* fullName) :
    _description(description), _grblName(grblName), _fullName(fullName), _type(type) {}

Command* Command::List = NULL;

Command::Command(
    const char* description, type_t type, const char* grblName, const char* fullName, bool (*cmdChecker)()) :
    Word(type, description, grblName, fullName),
    _cmdChecker(cmdChecker) {
    link = List;
    List = this;
}

Setting* Setting::List = NULL;

Setting::Setting(
    const char* description, type_t type, const char* grblName, const char* fullName, bool (*checker)(char*)) :
    Word(type, description, grblName, fullName),
    _checker(checker) {
    link = List;
    List = this;

    // NVS keys are limited to 15 characters, so if the setting name is longer
    // than that, we derive a 15-character name from a hash function
    size_t len = strlen(fullName);
    if (len <= 15) {
        _keyName = _fullName;
    } else {
        // This is Donald Knuth's hash function from Vol 3, chapter 6.4
        char*    hashName = (char*)malloc(16);
        uint32_t hash     = len;
        for (const char* s = fullName; *s; s++) {
            hash = ((hash << 5) ^ (hash >> 27)) ^ (*s);
        }
        sprintf(hashName, "%.7s%08x", fullName, hash);
        _keyName = hashName;
    }
}

Error Setting::check(char* s) {
    if (sys.state != State::Idle && sys.state != State::Alarm) {
        return Error::IdleError;
    }
    if (!_checker) {
        return Error::Ok;
    }
    return _checker(s) ? Error::Ok : Error::InvalidValue;
}

nvs_handle Setting::_handle = 0;

void Setting::init() {
    if (!_handle) {
        if (esp_err_t err = nvs_open("Grbl_ESP32", NVS_READWRITE, &_handle)) {
        }
    }
}

IntSetting::IntSetting(const char*   description,
                       type_t        type,
                       const char*   grblName,
                       const char*   name,
                       int32_t       defVal,
                       int32_t       minVal,
                       int32_t       maxVal,
                       bool (*checker)(char*) = NULL,
                       bool currentIsNvm) :
    Setting(description, type, grblName, name, checker),
    _defaultValue(defVal), _currentValue(defVal), _minValue(minVal), _maxValue(maxVal), _currentIsNvm(currentIsNvm) {
    _storedValue = std::numeric_limits<int32_t>::min();
}

void IntSetting::load() {
    esp_err_t err = nvs_get_i32(_handle, _keyName, &_storedValue);
    if (err) {
        _storedValue  = std::numeric_limits<int32_t>::min();
        _currentValue = _defaultValue;
    } else {
        _currentValue = _storedValue;
    }
}

void IntSetting::setDefault() {
    if (_currentIsNvm) {
        nvs_erase_key(_handle, _keyName);
    } else {
        _currentValue = _defaultValue;
        if (_storedValue != _currentValue) {
            nvs_erase_key(_handle, _keyName);
        }
    }
}

Error IntSetting::setStringValue(char* s) {
    s         = trim(s);
    Error err = check(s);
    if (err != Error::Ok) {
        return err;
    }
    char*   endptr;
    int32_t convertedValue = strtol(s, &endptr, 10);
    if (endptr == s || *endptr != '\0') {
        return Error::BadNumberFormat;
    }
    if (convertedValue < _minValue || convertedValue > _maxValue) {
        return Error::NumberRange;
    }

    // If we don't see the NVM state, we have to make this the live value:
    if (!_currentIsNvm) {
        _currentValue = convertedValue;
    }

    if (_storedValue != convertedValue) {
        if (convertedValue == _defaultValue) {
            nvs_erase_key(_handle, _keyName);
        } else {
            if (nvs_set_i32(_handle, _keyName, convertedValue)) {
                return Error::NvsSetFailed;
            }
            _storedValue = convertedValue;
        }
    }
    check(NULL);
    return Error::Ok;
}

const char* IntSetting::getDefaultString() {
    static char strval[32];
    sprintf(strval, "%d", _defaultValue);
    return strval;
}

const char* IntSetting::getStringValue() {
    static char strval[32];

    int currentSettingValue;
    if (_currentIsNvm) {
        if (std::numeric_limits<int32_t>::min() == _storedValue) {
            currentSettingValue = _defaultValue;
        } else {
            currentSettingValue = _storedValue;
        }
    } else {
        currentSettingValue = get();
    }

    sprintf(strval, "%d", currentSettingValue);
    return strval;
}

AxisMaskSetting::AxisMaskSetting(const char*   description,
                                 type_t        type,
                                 const char*   grblName,
                                 const char*   name,
                                 int32_t       defVal,
                                 bool (*checker)(char*) = NULL) :
    Setting(description, type, grblName, name, checker),
    _defaultValue(defVal), _currentValue(defVal) {}

void AxisMaskSetting::load() {
    esp_err_t err = nvs_get_i32(_handle, _keyName, &_storedValue);
    if (err) {
        _storedValue  = -1;
        _currentValue = _defaultValue;
    } else {
        _currentValue = _storedValue;
    }
}

void AxisMaskSetting::setDefault() {
    _currentValue = _defaultValue;
    if (_storedValue != _currentValue) {
        nvs_erase_key(_handle, _keyName);
    }
}

Error AxisMaskSetting::setStringValue(char* s) {
    s         = trim(s);
    Error err = check(s);
    if (err != Error::Ok) {
        return err;
    }
    int32_t convertedValue;
    char*   endptr;
    if (*s == '\0') {
        convertedValue = 0;
    } else {
        convertedValue = strtol(s, &endptr, 10);
        if (endptr == s || *endptr != '\0') {
            // Try to convert as an axis list
            convertedValue = 0;
            auto axisNames = String("XYZABC");
            while (*s) {
                int index = axisNames.indexOf(toupper(*s++));
                if (index < 0) {
                    return Error::BadNumberFormat;
                }
                convertedValue |= bit(index);
            }
        }
    }
    _currentValue = convertedValue;
    if (_storedValue != _currentValue) {
        if (_currentValue == _defaultValue) {
            nvs_erase_key(_handle, _keyName);
        } else {
            if (nvs_set_i32(_handle, _keyName, _currentValue)) {
                return Error::NvsSetFailed;
            }
            _storedValue = _currentValue;
        }
    }
    check(NULL);
    return Error::Ok;
}

const char* AxisMaskSetting::getCompatibleValue() {
    static char strval[32];
    sprintf(strval, "%d", get());
    return strval;
}

static char* maskToString(uint32_t mask, char* strval) {
    char* s = strval;
    for (int i = 0; i < MAX_N_AXIS; i++) {
        if (mask & bit(i)) {
            *s++ = "XYZABC"[i];
        }
    }
    *s = '\0';
    return strval;
}

const char* AxisMaskSetting::getDefaultString() {
    static char strval[32];
    return maskToString(_defaultValue, strval);
}

const char* AxisMaskSetting::getStringValue() {
    static char strval[32];
    return maskToString(get(), strval);
}

FloatSetting::FloatSetting(const char*   description,
                           type_t        type,
                           const char*   grblName,
                           const char*   name,
                           float         defVal,
                           float         minVal,
                           float         maxVal,
                           bool (*checker)(char*) = NULL) :
    Setting(description, type, grblName, name, checker),
    _defaultValue(defVal), _currentValue(defVal), _minValue(minVal), _maxValue(maxVal) {}

void FloatSetting::load() {
    union {
        int32_t ival;
        float   fval;
    } v;
    if (nvs_get_i32(_handle, _keyName, &v.ival)) {
        _currentValue = _defaultValue;
    } else {
        _currentValue = v.fval;
    }
}

void FloatSetting::setDefault() {
    _currentValue = _defaultValue;
    if (_storedValue != _currentValue) {
        nvs_erase_key(_handle, _keyName);
    }
}

Error FloatSetting::setStringValue(char* s) {
    s         = trim(s);
    Error err = check(s);
    if (err != Error::Ok) {
        return err;
    }

    float   convertedValue;
    uint8_t len    = strlen(s);
    uint8_t retlen = 0;
    if (!read_float(s, &retlen, &convertedValue) || retlen != len) {
        return Error::BadNumberFormat;
    }
    if (convertedValue < _minValue || convertedValue > _maxValue) {
        return Error::NumberRange;
    }
    _currentValue = convertedValue;
    if (_storedValue != _currentValue) {
        if (_currentValue == _defaultValue) {
            nvs_erase_key(_handle, _keyName);
        } else {
            union {
                int32_t ival;
                float   fval;
            } v;
            v.fval = _currentValue;
            if (nvs_set_i32(_handle, _keyName, v.ival)) {
                return Error::NvsSetFailed;
            }
            _storedValue = _currentValue;
        }
    }
    check(NULL);
    return Error::Ok;
}

const char* FloatSetting::getDefaultString() {
    static char strval[32];
    (void)sprintf(strval, "%.3f", _defaultValue);
    return strval;
}

const char* FloatSetting::getStringValue() {
    static char strval[32];
    (void)sprintf(strval, "%.3f", get());
#if 0
    // With the goal of representing both large and small floating point
    // numbers compactly while showing clearly that the are floating point,
    // remove trailing zeros leaving at least one post-decimal digit.
    // The loop is guaranteed to terminate because the string contains
    // a decimal point which is not a '0'.
    for (char *p = strval + strlen(strval) - 1; *p == '0'; --p) {
        if (*(p-1) != '.' && *(p-1) != ',') {
            *p = '\0';
        }
    }
#endif
    return strval;
}




FlagSetting::FlagSetting(const char*   description,
                         type_t        type,
                         const char*   grblName,
                         const char*   name,
                         bool          defVal,
                         bool (*checker)(char*) = NULL) :
    Setting(description, type, grblName, name, checker),
    _defaultValue(defVal) {}

void FlagSetting::load() {
    esp_err_t err = nvs_get_i8(_handle, _keyName, &_storedValue);
    if (err) {
        _storedValue  = -1;  // Neither well-formed false (0) nor true (1)
        _currentValue = _defaultValue;
    } else {
        _currentValue = !!_storedValue;
    }
}
void FlagSetting::setDefault() {
    _currentValue = _defaultValue;
    if (_storedValue != _currentValue) {
        nvs_erase_key(_handle, _keyName);
    }
}

Error FlagSetting::setStringValue(char* s) {
    s         = trim(s);
    Error err = check(s);
    if (err != Error::Ok) {
        return err;
    }
    _currentValue = (strcasecmp(s, "on") == 0) || (strcasecmp(s, "true") == 0) || (strcasecmp(s, "enabled") == 0) ||
                    (strcasecmp(s, "yes") == 0) || (strcasecmp(s, "1") == 0);
    // _storedValue is -1, 0, or 1
    // _currentValue is 0 or 1
    if (_storedValue != (int8_t)_currentValue) {
        if (_currentValue == _defaultValue) {
            nvs_erase_key(_handle, _keyName);
        } else {
            if (nvs_set_i8(_handle, _keyName, _currentValue)) {
                return Error::NvsSetFailed;
            }
            _storedValue = _currentValue;
        }
    }
    check(NULL);
    return Error::Ok;
}
const char* FlagSetting::getDefaultString() {
    return _defaultValue ? "On" : "Off";
}
const char* FlagSetting::getStringValue() {
    return get() ? "On" : "Off";
}
const char* FlagSetting::getCompatibleValue() {
    return get() ? "1" : "0";
}


AxisSettings::AxisSettings(const char* axisName) : name(axisName) {}

Error GrblCommand::action(char* value) {
    if (_cmdChecker && _cmdChecker()) {
        return Error::IdleError;
    }
    return _action((const char*)value);
};
Coordinates* coords[CoordIndex::End];

bool Coordinates::load() {
    size_t len;
    switch (nvs_get_blob(Setting::_handle, _name, _currentValue, &len)) {
        case ESP_OK:
            return true;
        case ESP_ERR_NVS_INVALID_LENGTH:
            // This could happen if the stored value is longer than the buffer.
            // That is highly unlikely since we always store MAX_N_AXIS coordinates.
            // It would indicate that we have decreased MAX_N_AXIS since the
            // value was stored.  We don't flag it as an error, but rather
            // accept the initial coordinates and ignore the residue.
            // We could issue a warning message if we were so inclined.
            return true;
        case ESP_ERR_NVS_INVALID_NAME:
        case ESP_ERR_NVS_INVALID_HANDLE:
        default:
            return false;
    }
};

void Coordinates::set(float value[MAX_N_AXIS]) {
    memcpy(&_currentValue, value, sizeof(_currentValue));
    nvs_set_blob(Setting::_handle, _name, _currentValue, sizeof(_currentValue));
}
