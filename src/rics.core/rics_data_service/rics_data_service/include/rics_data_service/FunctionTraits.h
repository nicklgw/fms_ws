#ifndef RICS_FUNCTIONTRAITS_H
#define RICS__FUNCTIONTRAITS_H

#include <tuple>
#include <functional>

namespace 
{


template<typename T>
struct function_traits;

/**
 *	偏特化： 普通函数
 */
template<typename Ret, typename... Args>
struct function_traits<Ret(Args...)>
{
public:
    enum { arity = sizeof...(Args) };
    typedef Ret function_type(Args...);
    typedef Ret return_type;
    using stl_function_type = std::function<function_type>;
    typedef Ret(*pointer)(Args...);

    template<size_t I>
    struct args
    {
        static_assert(I < arity, "index is out of range, index must less than sizeof Args");
        using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
    };
};

/**
 *	偏特化： C函数指针
 */
template<typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)> : function_traits<Ret(Args...)>{};

/**
 *	std::function
 */
template <typename Ret, typename... Args>
struct function_traits<std::function<Ret(Args...)>> : function_traits<Ret(Args...)>{};

/**
 *	偏特化： 成员函数
 */
#define FUNCTION_TRAITS(...) \
    template <typename ReturnType, typename ClassType, typename... Args>\
    struct function_traits<ReturnType(ClassType::*)(Args...) __VA_ARGS__> : function_traits<ReturnType(Args...)>{}; \

FUNCTION_TRAITS()
FUNCTION_TRAITS(const)
FUNCTION_TRAITS(volatile)
FUNCTION_TRAITS(const volatile)

/**
 *	偏特化： lambda 表达式
 */
template<typename Callable>
struct function_traits : function_traits<decltype(&Callable::operator())>{};

}
#endif //RICS_OTA_INCLUDE_RICS_UTILITY_FUNCTIONTRAITS_H
