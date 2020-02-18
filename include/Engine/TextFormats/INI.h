#ifndef INI_H
#define INI_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>

class INI {
public:
    struct ConfigItems {
    char section[60];
    bool hasSection;
    char key[60];
    char value[60];
    }; 
    ConfigItems item[80];
    int count = 0;

    static INI* Load(const char* filename);
    bool GetString(const char* section, const char* key, char* dest);
    bool GetInteger(const char* section, const char* key, int* dest);
    bool GetBool(const char* section, const char* key, bool* dest);
    bool SetString(const char* section, const char* key, const char* value);
    bool SetInteger(const char* section, const char* key, int value);
    bool SetBool(const char* section, const char* key, bool value);
    void Dispose();
};

#endif /* INI_H */
