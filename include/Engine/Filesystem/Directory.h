#ifndef DIRECTORY_H
#define DIRECTORY_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>

class Directory {
public:
    static bool          Exists(const char* path);
    static bool          Create(const char* path);
    static bool          GetCurrentWorkingDirectory(char* out, size_t sz);
    static void          GetFiles(vector<char*>* files, const char* path, const char* searchPattern, bool allDirs);
    static vector<char*> GetFiles(const char* path, const char* searchPattern, bool allDirs);
    static void          GetDirectories(vector<char*>* files, const char* path, const char* searchPattern, bool allDirs);
    static vector<char*> GetDirectories(const char* path, const char* searchPattern, bool allDirs);
};

#endif /* DIRECTORY_H */
