#if INTERFACE

#include <Engine/Includes/Standard.h>

class StringUtils {
private:
};
#endif

#include <Engine/Utilities/StringUtils.h>

PUBLIC STATIC bool StringUtils::WildcardMatch(const char* first, const char* second) {
    if (*first == 0 && *second == 0)
        return true;
    if (*first == 0 && *second == '*' && *(second + 1) != 0)
        return false;
    if (*first == *second || *second == '?')
        return StringUtils::WildcardMatch(first + 1, second + 1);
    if (*second == '*')
        return StringUtils::WildcardMatch(first, second + 1) || StringUtils::WildcardMatch(first + 1, second);
    return false;
}
