#pragma once

#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>

namespace di {

// Forward declaration
class Injector;

namespace detail {

// Primary template for detecting constructors with N arguments
template<typename T, typename... Args>
struct is_constructible_with : std::is_constructible<T, Args...> {};

// Helpers to detect constructor arguments
template<typename T, typename = void>
struct ConstructorArguments {
    using type = std::tuple<>;
    static constexpr size_t count = 0;

    template<typename Injector>
    static auto resolveArgs(Injector&) {
        return std::tuple<>();
    }
};

// Specialization for default constructor
template<typename T>
struct ConstructorArguments<T, 
    std::enable_if_t<std::is_default_constructible_v<T>>> {
    using type = std::tuple<>;
    static constexpr size_t count = 0;

    template<typename Injector>
    static auto resolveArgs(Injector&) {
        return std::tuple<>();
    }
};

// Detector for 1-arg constructor
template<typename T, typename Arg1, typename = void>
struct has_1arg_ctor : std::false_type {};

template<typename T, typename Arg1>
struct has_1arg_ctor<T, Arg1, std::void_t<decltype(T(std::declval<Arg1>()))>> 
    : std::true_type {};

// Detector for 2-arg constructor
template<typename T, typename Arg1, typename Arg2, typename = void>
struct has_2arg_ctor : std::false_type {};

template<typename T, typename Arg1, typename Arg2>
struct has_2arg_ctor<T, Arg1, Arg2, std::void_t<decltype(T(std::declval<Arg1>(), std::declval<Arg2>()))>> 
    : std::true_type {};

// Detector for 3-arg constructor
template<typename T, typename Arg1, typename Arg2, typename Arg3, typename = void>
struct has_3arg_ctor : std::false_type {};

template<typename T, typename Arg1, typename Arg2, typename Arg3>
struct has_3arg_ctor<T, Arg1, Arg2, Arg3, std::void_t<decltype(T(std::declval<Arg1>(), std::declval<Arg2>(), std::declval<Arg3>()))>> 
    : std::true_type {};

// Specialization for 1-argument constructors
// Here we need to handle all possible interface types, but for simplicity we'll
// focus on common patterns
template<typename T, typename Arg>
struct ConstructorArguments<T,
    std::enable_if_t<has_1arg_ctor<T, std::shared_ptr<Arg>>::value>> {
    
    using type = std::tuple<std::shared_ptr<Arg>>;
    static constexpr size_t count = 1;

    template<typename Injector>
    static auto resolveArgs(Injector& injector) {
        return std::tuple<std::shared_ptr<Arg>>(injector.template resolve<Arg>());
    }
};

// Specialization for 2-argument constructors
template<typename T, typename Arg1, typename Arg2>
struct ConstructorArguments<T, 
    std::enable_if_t<has_2arg_ctor<T, std::shared_ptr<Arg1>, std::shared_ptr<Arg2>>::value>> {

    using type = std::tuple<std::shared_ptr<Arg1>, std::shared_ptr<Arg2>>;
    static constexpr size_t count = 2;

    template<typename Injector>
    static auto resolveArgs(Injector& injector) {
        return std::tuple<std::shared_ptr<Arg1>, std::shared_ptr<Arg2>>(
            injector.template resolve<Arg1>(),
            injector.template resolve<Arg2>()
        );
    }
};

// Specialization for 3-argument constructors
template<typename T, typename Arg1, typename Arg2, typename Arg3>
struct ConstructorArguments<T,
    std::enable_if_t<has_3arg_ctor<T, std::shared_ptr<Arg1>, std::shared_ptr<Arg2>, std::shared_ptr<Arg3>>::value>> {

    using type = std::tuple<std::shared_ptr<Arg1>, std::shared_ptr<Arg2>, std::shared_ptr<Arg3>>;
    static constexpr size_t count = 3;

    template<typename Injector>
    static auto resolveArgs(Injector& injector) {
        return std::tuple<std::shared_ptr<Arg1>, std::shared_ptr<Arg2>, std::shared_ptr<Arg3>>(
            injector.template resolve<Arg1>(),
            injector.template resolve<Arg2>(),
            injector.template resolve<Arg3>()
        );
    }
};

// Function to create an object with tuple of arguments
template<typename T, typename Tuple, std::size_t... I>
std::shared_ptr<T> createFromTuple(Tuple&& args, std::index_sequence<I...>) {
    return std::make_shared<T>(std::get<I>(std::forward<Tuple>(args))...);
}

// Simple wrapper to call the above with the right index sequence
template<typename T, typename Tuple>
std::shared_ptr<T> createWithTuple(Tuple&& args) {
    return createFromTuple<T>(
        std::forward<Tuple>(args),
        std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{}
    );
}

} // namespace detail

// Public interface for resolving constructor arguments
template<typename T>
struct TypeInfo {
    using ArgsTuple = typename detail::ConstructorArguments<T>::type;
    static constexpr size_t ArgumentCount = detail::ConstructorArguments<T>::count;

    template<typename Injector>
    static std::shared_ptr<T> create(Injector& injector) {
        auto args = detail::ConstructorArguments<T>::resolveArgs(injector);
        return detail::createWithTuple<T>(args);
    }
};

} // namespace di 