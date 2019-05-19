#pragma once

#include "types.h"

struct String;
struct Buffer;

bool is_eof(u32 c);
bool is_eol(u32 c);
bool is_whitespace(u32 c);
bool is_letter(u32 c);   
bool is_oct_digit(u32 c);
bool is_hex_digit(u32 c);
bool is_digit(u32 c);
bool is_symbol(u32 c);
bool is_lowercase(u32 c);
bool is_uppercase(u32 c);
u32 to_lowercase(u32 c);
u32 to_uppercase(u32 c);

#include "array.h"

enum Syntax_Highlight_Type {
    SHT_IDENT,
    SHT_COMMENT,
    SHT_KEYWORD, 
    SHT_OPERATOR,
    SHT_TYPE,
    SHT_FUNCTION,
    SHT_GENERIC_TYPE,
    SHT_GENERIC_FUNCTION,
    SHT_MACRO,
    SHT_NUMERIC_LITERAL,
    SHT_STRING_LITERAL,
    SHT_DIRECTIVE,
    SHT_ANNOTATION,
    SHT_PARAMETER,
};
#define DFA_PARSER 0
struct Syntax_Highlight {
    //size_t type : 4;
    //size_t where : (sizeof(size_t) * 8 - 4);
    Syntax_Highlight_Type type = {};
    const u32* where = 0;
};

void parse_syntax(Buffer* buffer);
struct Buf_String {
    size_t i;
    size_t size;
};
