#ifndef ENGINE_UTILITIES_STRINGUTILS_H
#define ENGINE_UTILITIES_STRINGUTILS_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED


#include <Engine/Includes/Standard.h>

class StringUtils {
public:
    static bool WildcardMatch(const char* first, const char* second);
};

#endif /* ENGINE_UTILITIES_STRINGUTILS_H */
