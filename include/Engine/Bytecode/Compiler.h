#ifndef ENGINE_BYTECODE_COMPILER_H
#define ENGINE_BYTECODE_COMPILER_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED

class Compiler;

#include <Engine/Bytecode/Types.h>
#include <Engine/Bytecode/CompilerEnums.h>

class Compiler {
public:
    static Parser               parser;
    static Scanner              scanner;
    static ParseRule*           Rules;
    static vector<ObjFunction*> Functions;
    static HashMap<Token>*      TokenMap;
    static const char*          Magic;
    static bool                 PrettyPrint;
    Compiler* Enclosing = NULL;
    ObjFunction*    Function = NULL;
    int             Type = 0;
    Local           Locals[0x100];
    int             LocalCount = 0;
    int             ScopeDepth = 0;
    int             WithDepth = 0;
    vector<Uint32>  ClassHashList;
    vector<Uint32>  ClassExtendedList;

    Token         MakeToken(int type);
    Token         MakeTokenRaw(int type, const char* message);
    Token         ErrorToken(const char* message);
    bool          IsEOF();
    bool          IsDigit(char c);
    bool          IsHexDigit(char c);
    bool          IsAlpha(char c);
    bool          IsIdentifierStart(char c);
    bool          IsIdentifierBody(char c);
    bool          MatchChar(char expected);
    char          AdvanceChar();
    char          PrevChar();
    char          PeekChar();
    char          PeekNextChar();
    virtual void  SkipWhitespace();
    int           CheckKeyword(int start, int length, const char* rest, int type);
    virtual int   GetKeywordType();
    Token         StringToken();
    Token         NumberToken();
    Token         IdentifierToken();
    virtual Token ScanToken();
    void          AdvanceToken();
    Token         NextToken();
    Token         PeekToken();
    Token         PrevToken();
    bool          MatchToken(int expectedType);
    bool          MatchAssignmentToken();
    bool          CheckToken(int expectedType);
    void          ConsumeToken(int type, const char* message);
    void          SynchronizeToken();
    bool          ReportError(int line, const char* string, ...);
    bool          ReportErrorPos(int line, int pos, const char* string, ...);
    void          ErrorAt(Token* token, const char* message);
    void          Error(const char* message);
    void          ErrorAtCurrent(const char* message);
    bool          IsValueType(char* str);
    bool          IsReferenceType(char* str);
    void  ParseVariable(const char* errorMessage);
    Uint8 IdentifierConstant(Token* name);
    bool  IdentifiersEqual(Token* a, Token* b);
    void  MarkInitialized();
    void  DefineVariableToken(Token global);
    void  DeclareVariable();
    void  EmitSetOperation(Uint8 setOp, int arg, Token name);
    void  EmitGetOperation(Uint8 getOp, int arg, Token name);
    void  EmitAssignmentToken(Token assignmentToken);
    void  EmitCopy(Uint8 count);
    void  NamedVariable(Token name, bool canAssign);
    void  ScopeBegin();
    void  ScopeEnd();
    void  ClearToScope(int depth);
    void  PopToScope(int depth);
    void  AddLocal(Token name);
    int   ResolveLocal(Token* name);
    Uint8 GetArgumentList();
    void  AddEnum(Token name);
    int   ResolveEnum(Token* name);
    void  DeclareEnum();
    void  GetThis(bool canAssign);
    void  GetSuper(bool canAssign);
    void  GetDot(bool canAssign);
    void  GetElement(bool canAssign);
    void GetGrouping(bool canAssign);
    void GetLiteral(bool canAssign);
    void GetInteger(bool canAssign);
    void GetDecimal(bool canAssign);
    void GetString(bool canAssign);
    void GetArray(bool canAssign);
    void GetMap(bool canAssign);
    void GetConstant(bool canAssign);
    void GetVariable(bool canAssign);
    void GetLogicalAND(bool canAssign);
    void GetLogicalOR(bool canAssign);
    void GetConditional(bool canAssign);
    void GetUnary(bool canAssign);
    void GetBinary(bool canAssign);
    void GetSuffix(bool canAssign);
    void GetCall(bool canAssign);
    void GetExpression();
    void GetPrintStatement();
    void GetExpressionStatement();
    void GetContinueStatement();
    void GetDoWhileStatement();
    void GetReturnStatement();
    void GetRepeatStatement();
    void GetSwitchStatement();
    void GetCaseStatement();
    void GetDefaultStatement();
    void GetWhileStatement();
    void GetBreakStatement();
    void GetBlockStatement();
    void GetWithStatement();
    void GetForStatement();
    void GetIfStatement();
    void GetStatement();
    int  GetFunction(int type);
    void GetMethod();
    void GetVariableDeclaration();
    void GetClassDeclaration();
    void GetEventDeclaration();
    void GetEnumDeclaration();
    void GetDeclaration();
    static void   MakeRules();
    ParseRule*    GetRule(int type);
    void          ParsePrecedence(Precedence precedence);
    Uint32        GetHash(char* string);
    Uint32        GetHash(Token token);
    Chunk*        CurrentChunk();
    int           CodePointer();
    void          EmitByte(Uint8 byte);
    void          EmitBytes(Uint8 byte1, Uint8 byte2);
    void          EmitUint16(Uint16 value);
    void          EmitUint32(Uint32 value);
    void          EmitConstant(VMValue value);
    void          EmitLoop(int loopStart);
    int           GetJump(int offset);
    int           GetPosition();
    int           EmitJump(Uint8 instruction);
    int           EmitJump(Uint8 instruction, int jump);
    void          PatchJump(int offset);
    void          EmitStringHash(char* string);
    void          EmitStringHash(Token token);
    void          EmitReturn();
    void          StartBreakJumpList();
    void          EndBreakJumpList();
    void          StartContinueJumpList();
    void          EndContinueJumpList();
    void          StartSwitchJumpList();
    void          EndSwitchJumpList();
    int           FindConstant(VMValue value);
    int           MakeConstant(VMValue value);
    static void   PrintValue(VMValue value);
    static void   PrintValue(char** buffer, int* buf_start, VMValue value);
    static void   PrintValue(char** buffer, int* buf_start, VMValue value, int indent);
    static void   PrintObject(char** buffer, int* buf_start, VMValue value, int indent);
    static int    HashInstruction(const char* name, Chunk* chunk, int offset);
    static int    ConstantInstruction(const char* name, Chunk* chunk, int offset);
    static int    ConstantInstructionN(const char* name, int n, Chunk* chunk, int offset);
    static int    SimpleInstruction(const char* name, int offset);
    static int    SimpleInstructionN(const char* name, int n, int offset);
    static int    ByteInstruction(const char* name, Chunk* chunk, int offset);
    static int    LocalInstruction(const char* name, Chunk* chunk, int offset);
    static int    InvokeInstruction(const char* name, Chunk* chunk, int offset);
    static int    JumpInstruction(const char* name, int sign, Chunk* chunk, int offset);
    static int    WithInstruction(const char* name, Chunk* chunk, int offset);
    static int    DebugInstruction(Chunk* chunk, int offset);
    static void   DebugChunk(Chunk* chunk, const char* name, int arity);
    static void   HTML5ConvertChunk(Chunk* chunk, const char* name, int arity);
    static void   Init();
    void          Initialize(Compiler* enclosing, int scope, int type);
    bool          Compile(const char* filename, const char* source, const char* output);
    ObjFunction*  FinishCompiler();
    virtual       ~Compiler();
    static void   Dispose(bool freeTokens);
};

#endif /* ENGINE_BYTECODE_COMPILER_H */
