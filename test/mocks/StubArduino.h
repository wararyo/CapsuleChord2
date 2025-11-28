#ifndef STUB_ARDUINO_H
#define STUB_ARDUINO_H

#ifdef NATIVE_TEST

#include <string>
#include <cstdio>
#include <cstdint>
#include <cstdarg>

// Arduino String クラスのスタブ実装
class String {
public:
    std::string _str;

    String() = default;
    String(const char* s) : _str(s ? s : "") {}
    String(const std::string& s) : _str(s) {}
    String(int n) : _str(std::to_string(n)) {}
    String(unsigned int n) : _str(std::to_string(n)) {}
    String(long n) : _str(std::to_string(n)) {}
    String(unsigned long n) : _str(std::to_string(n)) {}
    String(float n, int decimals = 2) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*f", decimals, n);
        _str = buf;
    }
    String(double n, int decimals = 2) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*lf", decimals, n);
        _str = buf;
    }

    const char* c_str() const { return _str.c_str(); }
    size_t length() const { return _str.length(); }

    String& operator=(const String& rhs) { _str = rhs._str; return *this; }
    String& operator=(const char* s) { _str = (s ? s : ""); return *this; }

    String& operator+=(const String& rhs) { _str += rhs._str; return *this; }
    String& operator+=(const char* s) { if(s) _str += s; return *this; }
    String& operator+=(char c) { _str += c; return *this; }

    String operator+(const String& rhs) const { return String(_str + rhs._str); }
    String operator+(const char* s) const { return String(_str + (s ? s : "")); }

    bool operator==(const String& rhs) const { return _str == rhs._str; }
    bool operator==(const char* s) const { return _str == (s ? s : ""); }
    bool operator!=(const String& rhs) const { return _str != rhs._str; }
    bool operator!=(const char* s) const { return _str != (s ? s : ""); }

    char charAt(size_t index) const { return (index < _str.length()) ? _str[index] : 0; }
    char operator[](size_t index) const { return charAt(index); }

    int indexOf(char c, size_t from = 0) const {
        size_t pos = _str.find(c, from);
        return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
    }

    int indexOf(const String& s, size_t from = 0) const {
        size_t pos = _str.find(s._str, from);
        return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
    }

    String substring(size_t from, size_t to) const {
        if (from > _str.length()) return String();
        if (to > _str.length()) to = _str.length();
        return String(_str.substr(from, to - from));
    }

    String substring(size_t from) const {
        return substring(from, _str.length());
    }
};

// グローバル演算子（const char* + String など）
inline String operator+(const char* lhs, const String& rhs) {
    return String(lhs) + rhs;
}

// Serial クラスのスタブ実装
class MockSerial {
public:
    template<typename... Args>
    void printf(const char* fmt, Args... args) {
        std::printf(fmt, args...);
    }

    void print(const char* s) { std::printf("%s", s); }
    void print(const String& s) { std::printf("%s", s.c_str()); }
    void print(int n) { std::printf("%d", n); }
    void print(unsigned int n) { std::printf("%u", n); }
    void print(long n) { std::printf("%ld", n); }
    void print(unsigned long n) { std::printf("%lu", n); }
    void print(float n) { std::printf("%f", n); }
    void print(double n) { std::printf("%lf", n); }

    void println() { std::printf("\n"); }
    void println(const char* s) { std::printf("%s\n", s); }
    void println(const String& s) { std::printf("%s\n", s.c_str()); }
    void println(int n) { std::printf("%d\n", n); }
    void println(unsigned int n) { std::printf("%u\n", n); }
    void println(long n) { std::printf("%ld\n", n); }
    void println(unsigned long n) { std::printf("%lu\n", n); }
    void println(float n) { std::printf("%f\n", n); }
    void println(double n) { std::printf("%lf\n", n); }
};

// グローバルSerialインスタンス（inline変数、C++17）
inline MockSerial Serial;

// Arduino 型定義
typedef uint8_t byte;

#else
// 実機環境では Arduino.h をインクルード
#include <Arduino.h>
#endif

#endif // STUB_ARDUINO_H
