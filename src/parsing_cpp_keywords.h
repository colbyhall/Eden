
#if 1
#define CPP_KEYWORDS_2(X) \
X(do) \
X(if) \
X(or) \

#define CPP_KEYWORDS_3(X) \
X(and) \
X(asm) \
X(for) \
X(int) \
X(new) \
X(not) \
X(try) \
X(xor) \

#define CPP_KEYWORDS_4(X) \
X(auto) \
X(bool) \
X(case) \
X(char) \
X(else) \
X(enum) \
X(goto) \
X(long) \
X(this) \
X(true) \
X(void) \

#define CPP_KEYWORDS_5(X) \
X(bitor) \
X(break) \
X(catch) \
X(class) \
X(compl) \
X(const) \
X(false) \
X(float) \
X(or_eq) \
X(short) \
X(throw) \
X(union) \
X(using) \
X(while) \

#define CPP_KEYWORDS_6(X) \
X(and_eq) \
X(bitand) \
X(delete) \
X(double) \
X(export) \
X(extern) \
X(friend) \
X(inline) \
X(not_eq) \
X(public) \
X(return) \
X(signed) \
X(sizeof) \
X(static) \
X(struct) \
X(switch) \
X(typeid) \
X(xor_eq) \

#define CPP_KEYWORDS_7(X) \
X(alignas) \
X(alignof) \
X(char8_t) \
X(concept) \
X(default) \
X(mutable) \
X(nullptr) \
X(private) \
X(typedef) \
X(virtual) \
X(wchar_t) \

#define CPP_KEYWORDS_8(X) \
X(char16_t) \
X(char32_t) \
X(continue) \
X(co_await) \
X(co_yield) \
X(decltype) \
X(explicit) \
X(noexcept) \
X(operator) \
X(reflexpr) \
X(register) \
X(requires) \
X(template) \
X(typename) \
X(unsigned) \
X(volatile) \

#define CPP_KEYWORDS_9(X) \
X(consteval) \
X(constexpr) \
X(constinit) \
X(co_return) \
X(namespace) \
X(protected) \

#define CPP_KEYWORDS_10(X) \
X(const_cast) \

#define CPP_KEYWORDS_11(X) \
X(static_cast) \

#define CPP_KEYWORDS_12(X) \
X(dynamic_cast) \
X(synchronized) \
X(thread_local) \

#define CPP_KEYWORDS_13(X) \
/*X(atomic_cancel)*/ \
/*X(atomic_commit)*/ \
X(static_assert) \

#define CPP_KEYWORDS_15(X) \
X(atomic_noexcept) \

#define CPP_KEYWORDS_17(X) \
X(reinterpret_cast) \

#else

#define CPP_KEYWORDS(X)
    X(alignas         ) \
    X(alignof         ) \
    X(and             ) \
    X(and_eq          ) \
    X(asm             ) \
    X(atomic_cancel   ) \
    X(atomic_commit   ) \
    X(atomic_noexcept ) \
    X(auto            ) \
    X(bitand          ) \
    X(bitor           ) \
    X(bool            ) \
    X(break           ) \
    X(case            ) \
    X(catch           ) \
    X(char            ) \
    X(char8_t         ) \
    X(char16_t        ) \
    X(char32_t        ) \
    X(class           ) \
    X(compl           ) \
    X(concept         ) \
    X(const           ) \
    X(consteval       ) \
    X(constexpr       ) \
    X(constinit       ) \
    X(const_cast      ) \
    X(continue        ) \
    X(co_await        ) \
    X(co_return       ) \
    X(co_yield        ) \
    X(decltype        ) \
    X(default         ) \
    X(delete          ) \
    X(do              ) \
    X(double          ) \
    X(dynamic_cast    ) \
    X(else            ) \
    X(enum            ) \
    X(explicit        ) \
    X(export          ) \
    X(extern          ) \
    X(false           ) \
    X(float           ) \
    X(for             ) \
    X(friend          ) \
    X(goto            ) \
    X(if              ) \
    X(inline          ) \
    X(int             ) \
    X(long            ) \
    X(mutable         ) \
    X(namespace       ) \
    X(new             ) \
    X(noexcept        ) \
    X(not             ) \
    X(not_eq          ) \
    X(nullptr         ) \
    X(operator        ) \
    X(or              ) \
    X(or_eq           ) \
    X(private         ) \
    X(protected       ) \
    X(public          ) \
    X(reflexpr        ) \
    X(register        ) \
    X(reinterpret_cast) \
    X(requires        ) \
    X(return          ) \
    X(short           ) \
    X(signed          ) \
    X(sizeof          ) \
    X(static          ) \
    X(static_assert   ) \
    X(static_cast     ) \
    X(struct          ) \
    X(switch          ) \
    X(synchronized    ) \
    X(template        ) \
    X(this            ) \
    X(thread_local    ) \
    X(throw           ) \
    X(true            ) \
    X(try             ) \
    X(typedef         ) \
    X(typeid          ) \
    X(typename        ) \
    X(union           ) \
    X(unsigned        ) \
    X(using           ) \
    X(virtual         ) \
    X(void            ) \
    X(volatile        ) \
    X(wchar_t         ) \
    X(while           ) \
    X(xor             ) \
    X(xor_eq          ) \

#endif