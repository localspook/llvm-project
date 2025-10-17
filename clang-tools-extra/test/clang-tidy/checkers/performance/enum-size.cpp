// RUN: %check_clang_tidy -std=c++11-or-later %s performance-enum-size %t -- \
// RUN:   -config="{CheckOptions: {performance-enum-size.EnumIgnoreList: '::IgnoredEnum;IgnoredSecondEnum'}}"

// CHECK-FIXES: #include <cstdint>

namespace std
{
using uint8_t = unsigned char;
using int8_t = signed char;
using uint16_t = unsigned short;
using int16_t = signed short;
using uint32_t = unsigned int;
using int32_t = signed int;
}

enum class Value
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: enum 'Value' uses a larger base type ('int', size: 4 bytes) than necessary for its value set, consider using 'std::uint8_t' (1 byte) as the base type to reduce its size [performance-enum-size]
// CHECK-FIXES: enum class Value : std::uint8_t
{
    supported
};

enum class EnumClass : std::int16_t
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: enum 'EnumClass' uses a larger base type ('std::int16_t' (aka 'short'), size: 2 bytes) than necessary for its value set, consider using 'std::uint8_t' (1 byte) as the base type to reduce its size [performance-enum-size]
// CHECK-FIXES: enum class EnumClass : std::uint8_t
{
    supported
};

enum EnumWithType : std::uint16_t
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: enum 'EnumWithType' uses a larger base type ('std::uint16_t' (aka 'unsigned short'), size: 2 bytes) than necessary for its value set, consider using 'std::uint8_t' (1 byte) as the base type to reduce its size [performance-enum-size]
// CHECK-FIXES: enum EnumWithType : std::uint8_t
{
    supported,
    supported2
};

enum EnumWithNegative
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: enum 'EnumWithNegative' uses a larger base type ('int', size: 4 bytes) than necessary for its value set, consider using 'std::int8_t' (1 byte) as the base type to reduce its size [performance-enum-size]
// CHECK-FIXES: enum EnumWithNegative : std::int8_t
{
    s1 = -128,
    s2 = -100,
    s3 = 100,
    s4 = 127
};

enum EnumThatCanBeReducedTo2Bytes
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: enum 'EnumThatCanBeReducedTo2Bytes' uses a larger base type ('int', size: 4 bytes) than necessary for its value set, consider using 'std::int16_t' (2 bytes) as the base type to reduce its size [performance-enum-size]
// CHECK-FIXES: enum EnumThatCanBeReducedTo2Bytes : std::int16_t
{
    a1 = -128,
    a2 = 0x6EEE
};

enum EnumOnlyNegative
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: enum 'EnumOnlyNegative' uses a larger base type ('int', size: 4 bytes) than necessary for its value set, consider using 'std::int8_t' (1 byte) as the base type to reduce its size [performance-enum-size]
// CHECK-FIXES: enum EnumOnlyNegative : std::int8_t
{
    b1 = -125,
    b2 = -50,
    b3 = -10
};

enum CorrectU8 : std::uint8_t {
    c01 = 10,
    c02 = 11
};

enum CorrectU16 : std::uint16_t {
    c11 = 10,
    c12 = 0xFFFF
};

constexpr int getValue() {
    return 256;
}


enum CalculatedDueToUnknown1 : unsigned int
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: enum 'CalculatedDueToUnknown1' uses a larger base type ('unsigned int', size: 4 bytes) than necessary for its value set, consider using 'std::uint16_t' (2 bytes) as the base type to reduce its size [performance-enum-size]
// CHECK-FIXES: enum CalculatedDueToUnknown1 : std::uint16_t
{
    c21 = 10,
    c22 = getValue()
};

enum CalculatedDueToUnknown2 : unsigned int
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: enum 'CalculatedDueToUnknown2' uses a larger base type ('unsigned int', size: 4 bytes) than necessary for its value set, consider using 'std::uint16_t' (2 bytes) as the base type to reduce its size [performance-enum-size]
// CHECK-FIXES: enum CalculatedDueToUnknown2 : std::uint16_t
{
    c31 = 10,
    c32 = c31 + 246
};

enum class IgnoredEnum : std::uint32_t
{
    unused1 = 1,
    unused2 = 2
};

namespace internal
{

enum class IgnoredSecondEnum
{
    unused1 = 1,
    unused2 = 2
};

enum class EnumClassWithoutValues : int {};
enum EnumWithoutValues {};

}

#define ENUM_IN_MACRO enum EnumInMacro

ENUM_IN_MACRO
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: enum 'EnumInMacro' uses a larger base type ('int', size: 4 bytes) than necessary for its value set, consider using 'std::uint8_t' (1 byte) as the base type to reduce its size [performance-enum-size]
// CHECK-FIXES: ENUM_IN_MACRO
{
	v,
};

#define BASE_TYPE_IN_MACRO int

enum class EnumWithBaseTypeInMacro : BASE_TYPE_IN_MACRO
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: enum 'EnumWithBaseTypeInMacro' uses a larger base type ('int', size: 4 bytes) than necessary for its value set, consider using 'std::uint8_t' (1 byte) as the base type to reduce its size [performance-enum-size]
// CHECK-FIXES: enum class EnumWithBaseTypeInMacro : BASE_TYPE_IN_MACRO
{
	v,
};
