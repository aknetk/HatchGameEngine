#if INTERFACE
#include <Engine/Includes/Standard.h>

class StringUtils {
public:
};
#endif

#include <Engine/Utilities/StringUtils.h>

PUBLIC STATIC bool  StringUtils::WildcardMatch(const char* first, const char* second) {
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
PUBLIC STATIC char* StringUtils::StrCaseStr(const char* haystack, const char* needle) {
    if (!needle[0]) return (char*)haystack;

    /* Loop over all possible start positions. */
    for (size_t i = 0; haystack[i]; i++) {
        bool matches = true;
        /* See if the string matches here. */
        for (size_t j = 0; needle[j]; j++) {
            /* If we're out of room in the haystack, give up. */
            if (!haystack[i + j]) return NULL;

            /* If there's a character mismatch, the needle doesn't fit here. */
            if (tolower((unsigned char)needle[j]) !=
                tolower((unsigned char)haystack[i + j])) {
                matches = false;
                break;
            }
        }
        if (matches) return (char *)(haystack + i);
    }
    return NULL;
}
