#if INTERFACE
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
};
#endif

#include <Engine/TextFormats/INI.h>
#include <Engine/Diagnostics/Log.h>
#include <Engine/Application.h>

char* strgets(char* buf, int n, char** stream) {
    char* readerStart = *stream;
    char* reader = readerStart;
    memset(buf, 0, n);
    while (*reader) {
        switch (*reader) {
            case '\r':
                memcpy(buf, readerStart, (reader - readerStart) > n ? n : (reader - readerStart));
                *stream = reader + 1;
                if (reader[1] == '\n')
                    *stream = reader + 2;
                return buf;
            case '\n':
                memcpy(buf, readerStart, (reader - readerStart) > n ? n : (reader - readerStart));
                *stream = reader + 1;
                return buf;
            default:
                reader++;
                break;
        }
    }
    if (readerStart == reader)
        return NULL;

    memcpy(buf, readerStart, (reader - readerStart) > n ? n : (reader - readerStart));
    return buf;
}

PUBLIC STATIC INI* INI::Load(const char* filename) {
    INI* ini = new INI;

    char buf[120];
    char section[60];
    bool hasSection = false;
    char key[60];
    char value[60];

    ini->count = 0;

    // FILE* f = fopen(filename, "rb");
	// if (!f) {
	// 	Log::Print(Log::LOG_ERROR, "Couldn't open file '%s'!", filename);
    //     delete ini;
	// 	return NULL;
	// }
    //
	// while (fgets(buf, sizeof(buf), f) != NULL) {
    //     if (buf[0] == '#') continue;
    //
    //     if (sscanf(buf, "[%[^][]]", section) == 1) {
	// 		hasSection = true;
	// 	}
	// 	else if (sscanf(buf, "%[^ =]= %s", key, value) == 2 ||
    //         sscanf(buf, "%[^ =]=%s", key, value) == 2 ||
    //         sscanf(buf, "%[^ =] = %s", key, value) == 2 ||
    //         sscanf(buf, "%[^ =] =%s", key, value) == 2) {
    //         if (hasSection) {
    //             strcpy(ini->item[ini->count].section, section);
    //         }
	// 		strcpy(ini->item[ini->count].key, key);
	// 		strcpy(ini->item[ini->count].value, value);
    //         ini->item[ini->count].hasSection = hasSection;
    //         ini->count++;
	// 	}
	// }
    // fclose(f);

    SDL_RWops* rw = SDL_RWFromFile(filename, "rb");
    if (!rw) {
        Log::Print(Log::LOG_ERROR, "Couldn't open file '%s'!", filename);
        delete ini;
		return NULL;
    }

    Sint64 rwSize = SDL_RWsize(rw);
    if (rwSize < 0) {
        Log::Print(Log::LOG_ERROR, "Could not get size of file \"%s\": %s", filename, SDL_GetError());
        delete ini;
		return NULL;
    }

    char* data = (char*)malloc(rwSize + 1);
    SDL_RWread(rw, data, rwSize, 1);
    data[rwSize] = 0;
    SDL_RWclose(rw);

    char* dataStart = data;
    while (strgets(buf, sizeof(buf), &dataStart) != NULL) {
        if (buf[0] == '#') continue;
        if (strlen(buf) < 2) continue;

        if (sscanf(buf, "[%[^][]]", section) == 1) {
			hasSection = true;
		}
		else if (sscanf(buf, "%[^ =]= %s", key, value) == 2 ||
            sscanf(buf, "%[^ =]=%s", key, value) == 2 ||
            sscanf(buf, "%[^ =] = %s", key, value) == 2 ||
            sscanf(buf, "%[^ =] =%s", key, value) == 2) {
            if (hasSection) {
                strcpy(ini->item[ini->count].section, section);
            }
			strcpy(ini->item[ini->count].key, key);
			strcpy(ini->item[ini->count].value, value);
            ini->item[ini->count].hasSection = hasSection;
            ini->count++;
		}
	}

    free(data);

    return ini;
}

PUBLIC bool INI::GetString(const char* section, const char* key, char* dest) {
	if (count == 0)
		return 0;

	for (int x = 0; x < count; x++) {
        if (!strcmp(section, item[x].section)) {
    		if (!strcmp(key, item[x].key)) {
    			strcpy(dest, item[x].value);
    			return 1;
    		}
        }
	}

	return 0;
}
PUBLIC bool INI::GetInteger(const char* section, const char* key, int* dest) {
	if (count == 0)
		return 0;

	for (int x = 0; x < count; x++) {
        if (!strcmp(section, item[x].section)) {
    		if (!strcmp(key, item[x].key)) {
    			*dest = atoi(item[x].value);
    			return 1;
    		}
        }
	}

	return 0;
}
PUBLIC bool INI::GetBool(const char* section, const char* key, bool* dest) {
	if (count == 0)
		return 0;

	for (int x = 0; x < count; x++) {
        if (!strcmp(section, item[x].section)) {
    		if (!strcmp(key, item[x].key)) {
                *dest = !strcmp(item[x].value, "true") || !strcmp(item[x].value, "1");
    			return 1;
    		}
        }
	}

	return 0;
}

PUBLIC bool INI::SetString(const char* section, const char* key, const char* value) {
    int where = -1;
	for (int x = 0; x < count; x++) {
		if (strcmp(section, item[x].section) == 0) {
    		if (strcmp(key, item[x].key) == 0) {
    			where = x;
                break;
    		}
        }
	}
    if (where < 0)
        where = count++;

    strcpy(item[where].section, section);
    strcpy(item[where].key, key);
    strcpy(item[where].value, value);
	return 1;
}
PUBLIC bool INI::SetInteger(const char* section, const char* key, int value) {
    int where = -1;
	for (int x = 0; x < count; x++) {
		if (strcmp(section, item[x].section) == 0) {
    		if (strcmp(key, item[x].key) == 0) {
    			where = x;
                break;
    		}
        }
	}
    if (where < 0)
        where = count++;

    strcpy(item[where].section, section);
    strcpy(item[where].key, key);
    sprintf(item[where].value, "%d", value);
	return 1;
}
PUBLIC bool INI::SetBool(const char* section, const char* key, bool value) {
    int where = -1;
	for (int x = 0; x < count; x++) {
		if (strcmp(section, item[x].section) == 0) {
    		if (strcmp(key, item[x].key) == 0) {
    			where = x;
                break;
    		}
        }
	}
    if (where < 0)
        where = count++;

    strcpy(item[where].section, section);
    strcpy(item[where].key, key);
    sprintf(item[where].value, "%d", value);
	return 1;
}

PUBLIC void INI::Dispose() {

}
