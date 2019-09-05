#define C_KEYWORDS(X)   \
    X(_Alignas) \
    X(_Alignof) \
    X(_Atomic) \
    X(_Bool) \
    X(_Complex) \
    X(_Generic) \
    X(_Imaginary) \
    X(_Noreturn) \
    X(_Pragma) \
    X(_Static_assert) \
    X(_Thread_local) \
\
    X(restrict) \
\
    X(__pragma)  \
    X(__int8)  \
    X(__int16)  \
    X(__int32)  \
    X(__int64)  \
    X(__asm)\
    X(__declspec)  \
    X(__restrict) \
    X(__cdecl)\
    X(__fastcall)\
    X(__stdcall)\
\
    X(__attribute__)  \
\
    X(alignas) \
    X(alignof) \
    X(and) \
    X(and_eq) \
    X(asm) \
  /*X(atomic_cancel) \
    X(atomic_commit)*/ \
    X(atomic_noexcept) \
    X(auto) \
    X(bitand) \
    X(bitor) \
    X(bool) \
    X(break) \
    X(case) \
    X(catch) \
    X(char) \
    X(char8_t) \
    X(char16_t) \
    X(char32_t) \
    X(class) \
    X(compl) \
    X(concept) \
    X(const) \
    X(consteval) \
    X(constexpr) \
    X(const_cast) \
    X(continue) \
    X(co_await) \
    X(co_return) \
    X(co_yield) \
    X(decltype) \
    X(default) \
    X(delete) \
    X(do) \
    X(double) \
    X(dynamic_cast) \
    X(else) \
    X(enum) \
    X(explicit) \
    X(export) \
    X(extern) \
    X(false) \
    X(float) \
    X(for) \
    X(friend) \
    X(goto) \
    X(if) \
    X(inline) \
    X(int) \
    X(long) \
    X(mutable) \
    X(namespace) \
    X(new) \
    X(noexcept) \
    X(not) \
    X(not_eq) \
    X(nullptr) \
    X(operator) \
    X(or) \
    X(or_eq) \
    X(private) \
    X(protected) \
    X(public) \
    X(reflexpr) \
    X(register) \
    X(reinterpret_cast) \
    X(requires) \
    X(return) \
    X(short) \
    X(signed) \
    X(sizeof) \
    X(static) \
    X(static_assert) \
    X(static_cast) \
    X(struct) \
    X(switch) \
    X(synchronized) \
    X(template) \
    X(this) \
    X(thread_local) \
    X(throw) \
    X(true) \
    X(try) \
    X(typedef) \
    X(typeid) \
    X(typename) \
    X(union) \
    X(unsigned) \
    X(using) \
    X(virtual) \
    X(void) \
    X(volatile) \
    X(wchar_t) \
    X(while) \
    X(xor) \
    X(xor_eq) \

#define C_TYPE_KEYWORDS(X)    \
    X(__int8)  \
    X(__int16)  \
    X(__int32)  \
    X(__int64)  \
    X(__declspec)  \
    X(__restrict) \
    X(__cdecl)\
    X(__fastcall)\
    X(__stdcall)\
\
    X(auto) \
    X(bool) \
    X(char) \
    X(char8_t) \
    X(char16_t) \
    X(char32_t) \
    X(double) \
    X(float) \
    X(int) \
    X(long) \
    X(register) \
    X(short) \
    X(signed) \
    X(unsigned) \
    X(void) \
    X(wchar_t) \

#define C_TYPE_QUALIFIERS(X) \
    X(__declspec)  \
    X(__restrict) \
    X(__cdecl)\
    X(__fastcall)\
    X(__stdcall)\
\
    X(class) \
    X(const) \
    X(decltype) \
    X(enum) \
    X(extern) \
    X(static) \
    X(struct) \
    X(typename) \
    X(union) \
    X(volatile) \

#define C_TYPES(X) \
    X(int8_t) \
    X(int16_t) \
    X(int32_t) \
    X(int64_t) \
    X(uint8_t) \
    X(uint16_t) \
    X(uint32_t) \
    X(uint64_t) \
    X(int_least8_t) \
    X(int_least16_t) \
    X(int_least32_t) \
    X(int_least64_t) \
    X(uint_least8_t) \
    X(uint_least16_t) \
    X(uint_least32_t) \
    X(uint_least64_t) \
    X(int_fast8_t) \
    X(int_fast16_t) \
    X(int_fast32_t) \
    X(int_fast64_t) \
    X(uint_fast8_t) \
    X(uint_fast16_t) \
    X(uint_fast32_t) \
    X(uint_fast64_t) \
    X(intmax_t) \
    X(uintmax_t) \
    X(size_t) \
    X(ptrdiff_t) \
    X(intptr_t) \

