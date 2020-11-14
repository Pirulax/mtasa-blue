#pragma once

#include <type_traits>
#include <array>
#include <utility>

/**
    is_Nspecialization

    These structs allow testing whether a type is a specialization of a
    class template

    Note: Take care of optional parameters. For example std::unordered_map
    has 5 template arguments, thus is_5specialization needs to be used, rather
    than is_2specialization (Key, Value).

    Usage can be the following:
    if constexpr(is_2specialization<std::vector, T>::value)
    {
        // T is a vector!
        using param_t = typename is_2specialization<std::vector, T>::param_t
        // param_t is the type of the content of the vector
    }

    For each version of is_Nspecialization we have two class templates: 
        - The first one (inheriting from std::false_type) is used to provide a 
          default case, where something isn't a match. It does not impose restrictions
          on anything apart from the Test parameter (which is required to take N template 
          parameters). Thus it matches anything.

        - The second one (std::true_type) is used to perform the actual match
          by specializting the template for Test<Arg, Arg2>
**/

template <typename Test, template <typename> class Ref>
struct is_specialization : std::false_type
{
};

template <template <typename> class Ref, typename Args>
struct is_specialization<Ref<Args>, Ref> : std::true_type
{
    using param_t = Args;
};

template <typename Test, template <typename, typename> class Ref>
struct is_2specialization : std::false_type
{
};

template <template <typename, typename> class Ref, typename Arg1, typename Arg2>
struct is_2specialization<Ref<Arg1, Arg2>, Ref> : std::true_type
{
    using param1_t = Arg1;
    using param2_t = Arg2;
};

template <typename Test, template <typename, typename, typename, typename, typename> class Ref>
struct is_5specialization : std::false_type
{
};

template <template <typename, typename, typename, typename, typename> class Ref, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
struct is_5specialization<Ref<Arg1, Arg2, Arg3, Arg4, Arg5>, Ref> : std::true_type
{
    using param1_t = Arg1;
    using param2_t = Arg2;
    using param3_t = Arg3;
    using param4_t = Arg4;
    using param5_t = Arg5;
};

// is_variant
//  Returns whether a T is a variant
//  If T is a variant, it also allows accessing the individual types
//  of the variant via param1_t (first type) and rest_t (which is a 
//  variant of the remaining types).
template <typename Test>
struct is_variant : std::false_type
{
};

template <typename Arg1, typename... Args>
struct is_variant<std::variant<Arg1, Args...>> : std::true_type
{
    using param1_t = Arg1;
    using rest_t = std::variant<Args...>;
    static constexpr auto count = sizeof...(Args) + 1;
};

/**
    nth_element

    Returns the nth element of a parameter pack by recursively 
    discarding and counting down until the index is zero, at which
    point the type is returned
**/

// Recursive case, I is larger than 1, therefore
// we need to look at the I-1 case by discarding the
// first type. 
template <std::size_t I, typename T, typename... Ts>
struct nth_element_impl
{
    using type = typename nth_element_impl<I - 1, Ts...>::type;
};

// Base case: If we ask for the 0th element, the current element
// is the one we're looking for
template <typename T, typename... Ts>
struct nth_element_impl<0, T, Ts...>
{
    using type = T;
};

template <std::size_t I, typename... Ts>
using nth_element_t = typename nth_element_impl<I, Ts...>::type;


// common_variant
// common_variant builds a single variant from types or variants
//  bool, bool -> variant<bool>
//  int, bool -> variant<int, bool>
//  variant<int, bool>, bool -> variant<int, bool>
//  variant<int, bool>, float -> variant<int, bool, float>
//  variant<int, bool>, variant<bool, float> -> variant<int, bool, float>
//  variant<int, bool>, variant<bool, float, double> -> variant<int, bool, float, double>

// Two different types
//  int, bool -> variant<int, bool>
template <typename T, typename U>
struct common_variant
{
    using type = std::variant<T, U>;
};

// Identical types
//  int, int -> variant<int>
template <typename T>
struct common_variant<T, T>
{
    using type = std::variant<T>;
};

// Type + Variant which starts with the Type
//  int, variant<int> -> variant<int>
template <typename T, typename... Ts>
struct common_variant<T, std::variant<T, Ts...>>
{
    using type = std::variant<T, Ts...>;
};

// Variant + Empty = Variant
template <typename... Ts>
struct common_variant<std::variant<Ts...>, std::variant<>>
{
    using type = std::variant<Ts...>;
};

// Empty + Variant = Variant
template <typename... Ts>
struct common_variant<std::variant<>, std::variant<Ts...>>
{
    using type = std::variant<Ts...>;
};

// T + Variant = Variant or a new variant consisting of T + Variant
// This is done by checking if T is convertible to the variant
template <typename T, typename... Ts>
struct common_variant<T, std::variant<Ts...>>
{
    using type = std::conditional_t<std::is_convertible_v<T, std::variant<Ts...>>, std::variant<Ts...>, std::variant<T, Ts...>>;
};

// Variant + T = Variant or a new variant consisting of T + Variant
// Simply calls the above case
template <typename T, typename... Ts>
struct common_variant<std::variant<Ts...>, T>
{
    using type = typename common_variant<T, std::variant<Ts...>>::type;
};


// Variant + Variant = Combined Variant
// This recursively calls itself and deconstructs the first variant, while 
// adding the first type in the first variant to the second variant (via the T + variant overload)
template <typename T, typename... Ts, typename... Us>
struct common_variant<std::variant<T, Ts...>, std::variant<Us...>>
{
    using type = typename common_variant<std::variant<Ts...>, typename common_variant<T, std::variant<Us...>>::type>::type;
};

// dummy_type
// generic dummy type
struct dummy_type
{
};


// n_tuple: Constructs a tuple of size N (with dummy_type as parameter types)
//  n_tuple<2>::type == std::tuple<dummy_type, dummy_type>
template <std::size_t, bool HasEnough = false, typename... Args>
struct n_tuple;

// Second parameter is true -> We have reached N types in Args
template <std::size_t N, typename... Args>
struct n_tuple<N, true, Args...>
{
    using type = std::tuple<Args...>;
};

// Second parameter is false -> Add a dummy type and 
// decide if sizeof...(Args) + 1 is enough
template <std::size_t N, typename... Args>
struct n_tuple<N, false, Args...>
{
    using type = typename n_tuple<N, sizeof...(Args) + 1 >= N, Args..., dummy_type>::type;
};

// pad_func_with_func
// pads Func with as many dummy_type arguments as needed to 
// have the same number of arguments as FuncB has
template <auto* Func, typename T>
struct pad_func_with_func_impl
{
};

// pad_func_with_func_impl takes a tuple of additional arguments, which are then 
// taken as parameter. This allows us to discard exactly sizeof...(Ts) function
// arguments. Effectively this means Call is a function that is identical in behavior
// to Func, but takes additional sizeof...(Ts) dummy arguments that are discarded.
template <typename... Ts, typename Ret, typename... Args, auto (*Func)(Args...)->Ret>
struct pad_func_with_func_impl<Func, std::tuple<Ts...>>
{
    static inline Ret Call(Args... arg, Ts... args) { return Func(arg...); }
};

template <auto*, auto*>
struct pad_func_with_func
{
};

// pad_func_with_func initially needs to figure out the maximum number of parameters for the two functions
// It then determines how many parameters need to be added to Func, in order to have parity in argument count
// By using n_tuple, we build a Tuple of exactly the amout of parameters that need to be added to Func, which 
// is then applied via the impl function above
template <typename Ret, typename... Args, auto (*Func)(Args...)->Ret, typename RetB, typename... ArgsB, auto (*FuncB)(ArgsB...)->RetB>
struct pad_func_with_func<Func, FuncB> : pad_func_with_func_impl<Func, typename n_tuple<std::max(sizeof...(Args), sizeof...(ArgsB)) - sizeof...(Args),
                                                                                        std::max(sizeof...(Args), sizeof...(ArgsB)) - sizeof...(Args) == 0>::type>
{
};

/*
* Switch case like function jump table generator
* useful for example in CRPCFunctions where you need to
* translate an enum to a function run time.
*
* The 'normal' way would be to use a big switch case, but
* who wants to do that? Its really boresome! Here, I've spent
* 6 hours to make this work, so you better use it..
*
* Note: I could've used a recursive templte, yes, but GCC and MSVC
* are unable to optimize it into a jmptbl. clang is able to, and
* usually makes a jumptable out of anything (you'd be suprised)
*
*/
namespace detail
{
    template<typename Enum_t, bool checkIsValueValid, size_t begin, typename Callable_t, size_t... EnumValues>
    auto EnumToFunctionDispatchImpl(size_t rtvalue, Callable_t&& getFunctionPtr, std::index_sequence<EnumValues...> seq)
    {
        /*
        * Get common return types. This produces the function pointer
        * This is a check as well, because if one of the function calls
        * returns something that doesnt match the other function calls return value
        * it'll error (well, it'll blow up, and throw like 300 syntax errors, but you got the point)
        */
        using Pfn_t = std::common_type_t<
            std::invoke_result_t<Callable_t,
            std::integral_constant<Enum_t, Enum_t(begin + EnumValues)>>...
            >;

        if (checkIsValueValid && (rtvalue < begin || (rtvalue - begin) >= seq.size()))
            return Pfn_t{ nullptr };

        /*
        * An inline function pointer table obtained with calling
        * getFunctionPtr with the whole index_sequence
        */
        return std::array<Pfn_t, seq.size()>{ // Initalize a value -> function pointer table
            std::invoke(std::forward<Callable_t>(getFunctionPtr),
                std::integral_constant<Enum_t, Enum_t(begin + EnumValues)>{})... // Invoke function ptr getter with the index sequence
        } [size_t(rtvalue)] ; // Actually map the given runtime value to a function pointer
    }
};

/*
*  Use it like: EnumToFunction(<runtime enum value>, [](const enumconst) { return EnumTemplatedFunction<enumconst()> })(Templated function args...);
*  See CRPCFunctions for actual usage
*
*  rtvalue - runtime enum value
*  getFunctionPtr - its first argument is an integral_constant<Enum_t, <Enum_t value>. You can use this
*                   as the template parameter for your templated function (I mean it's value)
*
*  FUCKING IMPORTNAT NOTE(prevents brain cancer)::: Enum_t MUST HAVE ::BEGIN AND ::END!!!
*  OTHERWISE THE FUCKING TEMPLATE DEDUCTION FAILS AND YOU'LL GET BRAIN CANCER LIKE ME
*/
template<bool checkIsValueValid = true, class Enum_t, Enum_t eBegin = Enum_t::BEGIN, Enum_t eEnd = Enum_t::END, class Callable_t>
auto EnumToFunction(Enum_t eRTValue, Callable_t&& getFunctionPtr)
{
    constexpr auto len = size_t(eEnd) - size_t(eBegin);
    return detail::EnumToFunctionDispatchImpl<Enum_t, checkIsValueValid, size_t(eBegin)>(
        size_t(eRTValue), std::forward<Callable_t>(getFunctionPtr), std::make_index_sequence<len>{});
}

/*
* Same as above, but calls the function with the given args, and returns the called functions return value
*/
template<class Enum_t, bool checkIsValueValid = true, Enum_t eBegin = Enum_t::BEGIN, Enum_t eEnd = Enum_t::END, class Callable_t, typename... Args_t>
auto EnumToFunctionDispatch(Enum_t eRTValue, Callable_t&& getFunctionPtr, Args_t&&... args)
{
    const auto pfn = EnumToFunction<checkIsValueValid, Enum_t, eBegin, eEnd>(eRTValue, std::forward<Callable_t>(getFunctionPtr));
    using Ret_t = std::invoke_result_t<decltype(pfn), Args_t...>;

    return pfn ? pfn(std::forward<Args_t>(args)...) : Ret_t{};
}
