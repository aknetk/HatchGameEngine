#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>

class StringUtils {
public:
    static bool WildcardMatch(const char* first, const char* second);
};

#endif /* STRINGUTILS_H */
