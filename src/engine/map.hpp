// dict.hpp
#pragma once

#include <iostream>     // std::cout
#include <map>          // std::map
#include <string>       // std::string
#include <type_traits>  // std::is_same_v, std::is_arithmetic_v
#include <sstream>      // std::ostringstream
#include <iterator>     // std::next
#include <utility>      // std::declval

// Detect if T supports `operator<<(std::ostream&, const T&)`
template <typename T, typename = void>
struct dict_is_streamable : std::false_type {};

template <typename T>
struct dict_is_streamable<T, std::void_t<
    decltype( std::declval<std::ostream&>() << std::declval<const T&>() )
>> : std::true_type {};

template <typename K, typename V>
class Dict
{
public:
    // Auto-creates a default-constructed value when key is missing
    V& operator[](const K& key)
    {
        return data_[key];
    }

    bool contains(const K& key) const
    {
        return data_.find(key) != data_.end();
    }

    void erase(const K& key)
    {
        data_.erase(key);
    }

    void print() const
    {
        std::cout << "{ ";
        for (auto it = data_.begin(); it != data_.end(); ++it)
        {
            std::cout << keyToString(it->first) << ": " << valueToString(it->second);
            if (std::next(it) != data_.end()) std::cout << ", ";
        }
        std::cout << " }\n";
    }

private:
    std::map<K, V> data_; // ordered by key

    // --- minimal, safe stringification helpers ---

    static std::string quote(const std::string& s) { return "\"" + s + "\""; }

    template <typename T>
    static std::string stringify(const T& x)
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            return quote(x);
        }
        else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>)
        {
            return quote(std::string{x});
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            return x ? "true" : "false";
        }
        else if constexpr (std::is_arithmetic_v<T>)
        {
            return std::to_string(x);
        }
        else if constexpr (dict_is_streamable<T>::value)
        {
            std::ostringstream oss;
            oss << x;  // uses operator<< if available
            return oss.str();
        }
        else
        {
            return "<unprintable>";
        }
    }

    template <typename T>
    static std::string keyToString(const T& k) { return stringify(k); }

    template <typename T>
    static std::string valueToString(const T& v) { return stringify(v); }
};
