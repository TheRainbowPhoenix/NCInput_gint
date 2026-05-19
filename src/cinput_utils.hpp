#ifndef CINPUT_UTILS_HPP
#define CINPUT_UTILS_HPP

#include <cstring>
#include <cstdlib>
#include <initializer_list>
#include <new>

namespace cinput {

// =============================================================================
// MINIMAL MOVE
// =============================================================================

template<typename T> struct RemoveReference { typedef T Type; };
template<typename T> struct RemoveReference<T&> { typedef T Type; };
template<typename T> struct RemoveReference<T&&> { typedef T Type; };

template<typename T>
typename RemoveReference<T>::Type&& Move(T&& t) {
    return static_cast<typename RemoveReference<T>::Type&&>(t);
}

// =============================================================================
// MINIMAL STRING
// =============================================================================

class String {
public:
    String() : m_data(nullptr), m_len(0) {
        m_data = (char*)malloc(1);
        m_data[0] = '\0';
    }
    String(const char* s) {
        if (s) {
            m_len = strlen(s);
            m_data = (char*)malloc(m_len + 1);
            strcpy(m_data, s);
        } else {
            m_data = (char*)malloc(1);
            m_data[0] = '\0';
            m_len = 0;
        }
    }
    String(const String& other) {
        m_len = other.m_len;
        m_data = (char*)malloc(m_len + 1);
        strcpy(m_data, other.m_data);
    }
    String(String&& other) noexcept : m_data(other.m_data), m_len(other.m_len) {
        other.m_data = nullptr;
        other.m_len = 0;
    }
    ~String() {
        if (m_data) free(m_data);
    }

    String& operator=(const String& other) {
        if (this != &other) {
            free(m_data);
            m_len = other.m_len;
            m_data = (char*)malloc(m_len + 1);
            strcpy(m_data, other.m_data);
        }
        return *this;
    }

    String& operator=(String&& other) noexcept {
        if (this != &other) {
            free(m_data);
            m_data = other.m_data;
            m_len = other.m_len;
            other.m_data = nullptr;
            other.m_len = 0;
        }
        return *this;
    }

    String& operator=(const char* s) {
        free(m_data);
        if (s) {
            m_len = strlen(s);
            m_data = (char*)malloc(m_len + 1);
            strcpy(m_data, s);
        } else {
            m_data = (char*)malloc(1);
            m_data[0] = '\0';
            m_len = 0;
        }
        return *this;
    }

    String operator+(const String& other) const {
        String res;
        free(res.m_data);
        res.m_len = m_len + other.m_len;
        res.m_data = (char*)malloc(res.m_len + 1);
        strcpy(res.m_data, m_data);
        strcat(res.m_data, other.m_data);
        return res;
    }

    String operator+(const char* other) const {
        String res;
        free(res.m_data);
        size_t other_len = other ? strlen(other) : 0;
        res.m_len = m_len + other_len;
        res.m_data = (char*)malloc(res.m_len + 1);
        strcpy(res.m_data, m_data);
        if (other) strcat(res.m_data, other);
        return res;
    }

    String& operator+=(const String& other) {
        *this = *this + other;
        return *this;
    }

    String& operator+=(const char* other) {
        *this = *this + other;
        return *this;
    }

    String& operator+=(char c) {
        char buf[2] = {c, '\0'};
        *this = *this + buf;
        return *this;
    }

    bool operator==(const String& other) const { return strcmp(m_data, other.m_data) == 0; }
    bool operator==(const char* other) const { return strcmp(m_data, other) == 0; }
    bool operator!=(const String& other) const { return !(*this == other); }

    bool operator<(const String& other) const { return strcmp(m_data, other.m_data) < 0; }

    const char* c_str() const { return m_data; }
    size_t length() const { return m_len; }
    bool empty() const { return m_len == 0; }

    void pop_back() {
        if (m_len > 0) {
            m_len--;
            m_data[m_len] = '\0';
        }
    }

    size_t find(const char* s) const {
        char* p = strstr(m_data, s);
        if (p) return p - m_data;
        return (size_t)-1;
    }

    size_t find(const String& s) const { return find(s.c_str()); }

private:
    char* m_data;
    size_t m_len;
};

inline String operator+(const char* lhs, const String& rhs) {
    return String(lhs) + rhs;
}

// =============================================================================
// MINIMAL VECTOR
// =============================================================================

template<typename T>
class Vector {
public:
    Vector() : m_data(nullptr), m_size(0), m_cap(0) {}
    Vector(const Vector& other) : m_data(nullptr), m_size(0), m_cap(0) {
        reserve(other.m_size);
        for (size_t i = 0; i < other.m_size; ++i) {
            new(&m_data[i]) T(other.m_data[i]);
        }
        m_size = other.m_size;
    }
    Vector(Vector&& other) noexcept : m_data(other.m_data), m_size(other.m_size), m_cap(other.m_cap) {
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_cap = 0;
    }

    Vector(std::initializer_list<T> list) : m_data(nullptr), m_size(0), m_cap(0) {
        reserve(list.size());
        for (const auto& item : list) {
            push_back(item);
        }
    }

    ~Vector() {
        clear();
        free(m_data);
    }

    Vector& operator=(const Vector& other) {
        if (this != &other) {
            clear();
            reserve(other.m_size);
            for (size_t i = 0; i < other.m_size; ++i) {
                new(&m_data[i]) T(other.m_data[i]);
            }
            m_size = other.m_size;
        }
        return *this;
    }

    Vector& operator=(Vector&& other) noexcept {
        if (this != &other) {
            clear();
            free(m_data);
            m_data = other.m_data;
            m_size = other.m_size;
            m_cap = other.m_cap;
            other.m_data = nullptr;
            other.m_size = 0;
            other.m_cap = 0;
        }
        return *this;
    }

    void reserve(size_t new_cap) {
        if (new_cap > m_cap) {
            T* new_data = (T*)malloc(new_cap * sizeof(T));
            for (size_t i = 0; i < m_size; ++i) {
                new(&new_data[i]) T(Move(m_data[i]));
                m_data[i].~T();
            }
            free(m_data);
            m_data = new_data;
            m_cap = new_cap;
        }
    }

    void push_back(const T& val) {
        if (m_size == m_cap) {
            reserve(m_cap == 0 ? 1 : m_cap * 2);
        }
        new(&m_data[m_size++]) T(val);
    }

    void push_back(T&& val) {
        if (m_size == m_cap) {
            reserve(m_cap == 0 ? 1 : m_cap * 2);
        }
        new(&m_data[m_size++]) T(Move(val));
    }

    void clear() {
        for (size_t i = 0; i < m_size; ++i) {
            m_data[i].~T();
        }
        m_size = 0;
    }

    T& operator[](size_t i) { return m_data[i]; }
    const T& operator[](size_t i) const { return m_data[i]; }

    size_t size() const { return m_size; }
    bool empty() const { return m_size == 0; }

    T* begin() { return m_data; }
    const T* begin() const { return m_data; }
    T* end() { return m_data + m_size; }
    const T* end() const { return m_data + m_size; }

    void erase(T* it) {
        size_t idx = it - m_data;
        if (idx < m_size) {
            m_data[idx].~T();
            for (size_t i = idx; i < m_size - 1; ++i) {
                new(&m_data[i]) T(Move(m_data[i+1]));
                m_data[i+1].~T();
            }
            m_size--;
        }
    }

private:
    T* m_data;
    size_t m_size;
    size_t m_cap;
};

// =============================================================================
// MINIMAL UNIQUE_PTR
// =============================================================================

template<typename T>
class UniquePtr {
public:
    UniquePtr() : m_ptr(nullptr) {}
    explicit UniquePtr(T* p) : m_ptr(p) {}
    ~UniquePtr() { if (m_ptr) delete m_ptr; }

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) noexcept : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            if (m_ptr) delete m_ptr;
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    T* operator->() const { return m_ptr; }
    T& operator*() const { return *m_ptr; }
    T* get() const { return m_ptr; }
    explicit operator bool() const { return m_ptr != nullptr; }

private:
    T* m_ptr;
};

} // namespace cinput

#endif // CINPUT_UTILS_HPP
