#pragma once

#include <cstddef>

namespace lw
{

struct IndexSentinel
{
};

struct IndexRange
{
    size_t start;
    size_t step;
    size_t count;

    class Iterator
    {
    public:
        constexpr Iterator(size_t value, size_t step, size_t remaining) : _value(value), _step(step), _remaining(remaining) {}

        constexpr size_t operator*() const { return _value; }

        constexpr Iterator& operator++()
        {
            _value += _step;
            --_remaining;
            return *this;
        }

        friend constexpr bool operator==(const Iterator& it, IndexSentinel) { return it._remaining == 0; }
        friend constexpr bool operator!=(const Iterator& it, IndexSentinel s) { return !(it == s); }

    private:
        size_t _value;
        size_t _step;
        size_t _remaining;
    };

    constexpr Iterator begin() const { return Iterator(start, step, count); }
    constexpr IndexSentinel end() const { return {}; }
};

} // namespace lw
