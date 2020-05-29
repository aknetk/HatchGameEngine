#ifndef ENGINE_TEXTFORMATS_XML_XMLPARSER_H
#define ENGINE_TEXTFORMATS_XML_XMLPARSER_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED


#include <Engine/Includes/Standard.h>
#include <Engine/Bytecode/Enums.h>
#include <Engine/IO/Stream.h>
#include <Engine/Includes/HashMap.h>
#include <Engine/TextFormats/XML/XMLNode.h>

class XMLParser {
public:
    static bool     MatchToken(Token tok, const char* string);
    static float    TokenToNumber(Token tok);
    static XMLNode* Parse();
    static XMLNode* ParseFromResource(const char* filename);
    static void     Free(XMLNode* root);
};

#endif /* ENGINE_TEXTFORMATS_XML_XMLPARSER_H */
