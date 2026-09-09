#include <iostream>

template<typename Map, typename Key>
constexpr size_t index_of(const Map& map, const Key& key)
{
    auto it = map.find(key);

    if (it == map.end())
        return static_cast<std::size_t>(-1);

    return it - map.begin();
}