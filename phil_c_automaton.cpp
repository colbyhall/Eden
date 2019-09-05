void parse_pda() {
    enum {
        IDENT = 0, // ident
        LPAREN = 1, // (
        LSQUARE = 2, // [
        LCURLY = 3, // {
        LTRI = 4, // <
        RPAREN = 5, // )
        RSQUARE = 6, // ]
        RCURLY = 7, // }
        RTRI = 8, // >
        REFPTR = 9, // * &
        EQU = 10, // =
        SEMI = 11, // ;
        COMMA = 12, // ,
        MISCOP = 13, // digit ' " ~ ! % ^ - + \ | : . / ?
        POUND = 14, // #

        NUM_CHAR_TYPES
    };
    static const unsigned char char_type[256] { /*
[  0     -  -  -     8  ]  t  n  v  f  r  [  1  4     -  -  -  -  -  -  -  -  -  -     3  1  ]*/
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,/*
   !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /  0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >  ?*/
0,13,13,14, 0,13, 9,13, 1, 5, 9,13,12,13,13,13,13,13,13,13,13,13,13,13,13,13,13,11, 4,10, 8,13,/*
@  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z  [  \  ]  ^  _*/
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,13, 6,13, 0,/*
`  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y  z  {  |  }  ~  \127*/
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,13, 7,13, 0,
// 128 and greater: all identifier
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    };
    enum State {
        STMT, // Could be a decl or expr: not sure.
        DECL, // Definitely decl.
        EXPR, // Definitely expr.

        NUM_STATES
    };
    struct {
        State state : 2;
        Syntax_Highlight_Type type : 4;
    } stack[4096], *stack_ptr = stack;
    State state = STMT;
    Syntax_Highlight_Type ident_color = SHT_IDENT;
#define COLOR(x) push(x) // @Temporary: confusing names.
#define PUSH(x) { stack_ptr->state = state; stack_ptr->color = ident_color; }
    while (p < buf_end) {
        if (*p <= ' ') { do p++; while (*p <= ' '); }
        if (*(u16*)p == '//') { push(SHT_COMMENT); p += 2; do p++; while (*p != '\n' || p[-1] == '\\'); continue; }
        if (*(u16*)p == '*/') { push(SHT_COMMENT); do p++; while (*(u16*)(p - 2) != '/*'); continue; }

        int ch = SHT_IDENT;
        if (*p < 256) ch = char_type[*p];
        //State new_state = (State)state_table[state][char_type[*p]];

        Syntax_Highlight* decl_type = nullptr;

        Syntax_Highlight* s = push(SHT_OPERATOR); // @Speed: initialize this differently

        switch (ch) {
        case IDENT: {
            s->type = SHT_IDENT; // @Speed @Temporary
            C_Ident i;
            i.p = p;
            while (char_type[*p] == IDENT) p++;
            i.n = p - i.p;
            switch (get_chunk0(i)) {
#define CHUNK(x) case KW_CHUNK0(#x)
            CHUNK(do): {
                s->type = SHT_KEYWORD;
                continue;
            }
            CHUNK(class)   :
            CHUNK(const)   :
            CHUNK(decltype):
            CHUNK(enum)    :
            CHUNK(extern)  :
            CHUNK(static)  :
            CHUNK(struct)  :
            CHUNK(typename):
            CHUNK(union)   :
            CHUNK(volatile): {
                continue;
            }
            CHUNK(auto)    :
            CHUNK(bool)    :
            CHUNK(char)    :
            CHUNK(char8_t) :
            CHUNK(char16_t):
            CHUNK(char32_t):
            CHUNK(double)  :
            CHUNK(float)   :
            CHUNK(int)     :
            CHUNK(long)    :
            CHUNK(register):
            CHUNK(short)   :
            CHUNK(signed)  :
            CHUNK(unsigned):
            CHUNK(void)    :
            CHUNK(wchar_t) : {
                s->type = SHT_KEYWORD;
                decl_type = s;
                continue;
            }
            default: break;
            }
            p++;
            break;
        }
        case LPAREN:
        case LSQUARE:
        case LCURLY:
        case LTRI:
        case RPAREN:
        case RSQUARE:
        case RCURLY:
        case RTRI:
        case REFPTR:
        case EQU:
        case SEMI:
        case COMMA: {
            s->type = SHT_OPERATOR; // @Speed @Temporary
            p++;
            break;
        }
        case POUND: {
            s->type = SHT_DIRECTIVE; // @Speed @Temporary
            while (*p != '\n' || p[-1] == '\\') p++;
        }
        case MISCOP:
            switch (*p) {
            case '"':
            case '\'': {
                s->type = SHT_STRING_LITERAL; // @Speed @Temporary
                u8 end = *p;
            string_literal_scan:
                if (*p != end) {
                    p++;
                    if (*p == '\\') p += 2;
                    goto string_literal_scan;
                }
                break;
            }
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9': {
                s->type = SHT_NUMERIC_LITERAL; // @Speed @Temporary
            numeric_literal_scan:
                if (c_is_digit(*p)) {
                    p++;
                    if (*p == '\'' || *p == '.') p++;
                    goto numeric_literal_scan;
                }
                break;
            }
            default: {
                p++;
                break;
            }
            }
        default: {
            p++;
            break;
        }
        }
        if (state == STMT) {
        } else if (state == DECL) {
        } else if (state == EXPR) {
        }
    }
}
// STMT !type @ ident     : color(TYPE);
// STMT !type @ ( [ < >   : color(OP), push EXPR;
// STMT !type @ { ;       : color(OP);
// STMT !type @   ) ] } ; : color(OP), pop;
// STMT !type @ * &       : color(OP), push EXPR;
// STMT !type @ =         : color(OP), push EXPR;
// STMT !type @ ,         : color(OP);
// STMT  type @ ident     : color(cur_col);
// 
