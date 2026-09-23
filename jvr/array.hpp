// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#ifndef JVR_ARRAY_H
#define JVR_ARRAY_H
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#ifndef VK_NULL_HANDLE
#define VK_NULL_HANDLE nullptr
#endif // !VK_NULL_HANDLE

namespace jvr
{
template <class T> struct array {
    T *data;
    uint32_t capacity = 0;
    uint32_t len = 0;
    bool keep = false;

    static uint32_t round_up(uint32_t elem)
    {
        if (elem <= 4)
            return 4;
        uint32_t size = 4;
        while (size < elem) {
            size *= 2;
        }
        return size;
    }

    array()
    {
        capacity = 0;
        len = 0;
        data = VK_NULL_HANDLE;
    }

    array(uint32_t count)
    {
        uint32_t new_count = round_up(count);
        capacity = new_count;
        data = new T[new_count];
        len = count;
    }

    array(std::initializer_list<T> elems)
    {
        capacity = round_up(elems.size());
        len = elems.size();
        data = new T[capacity];
        uint32_t i = 0;
        for (T el : elems) {
            data[i] = el;
            i++;
        }
    }

    void push(T elem)
    {
        if (len == capacity) {
            capacity *= 2;
            data = reinterpret_cast<T *>(realloc(data, sizeof(T) * capacity));
        }
        data[len] = elem;
        len++;
    }
    uint32_t size() const
    {
        return len;
    }

    T get(uint32_t index) const
    {
        return data[index];
    }
    T operator[](uint32_t index) const
    {
        return data[index];
    }
    T &operator[](uint32_t index)
    {
        return data[index];
    }

    void capacity_from_len()
    {
        uint32_t new_count = round_up(len);
        capacity = new_count;
        data = new T[new_count];
    }

    void operator=(uint32_t count)
    {
        uint32_t new_count = round_up(count);
        capacity = new_count;
        data = new T[new_count];
        len = count;
    }

    struct iterator {
        T *data;
        uint32_t len;
        uint32_t pos;
        iterator(T *m_data, uint32_t m_len, uint32_t m_pos = 0)
        {
            data = m_data;
            len = m_len;
            pos = m_pos;
        }
        iterator &operator++()
        {
            if (pos != len)
                pos++;
            return *this;
        }
        bool operator==(iterator other) const
        {
            return pos == other.pos;
        }
        bool operator!=(iterator other) const
        {
            return !(*this == other);
        }
        T operator*() const
        {
            return data[pos];
        }
    };

    iterator begin() const
    {
        return iterator(data, len, 0);
    }
    iterator end() const
    {
        return iterator(data, len, len);
    }

    iterator begin()
    {
        return iterator(data, len, 0);
    }
    iterator end()
    {
        return iterator(data, len, len);
    }

    ~array()
    {
        delete[] data;
    }
};
}
#endif
