/* $Id:$ */

/** @file macros.h */

#ifndef MACROS_H
#define MACROS_H


/* Compile time assertions */
# define assert_compile(expr) extern "C" void __ct_assert__(int a[1 - 2 * !(expr)])

assert_compile(sizeof(uint32) == 4);
assert_compile(sizeof(uint16) == 2);
assert_compile(sizeof(uint8)  == 1);

#define lengthof(array) (sizeof(array)/sizeof(array[0]))
#define endof(array)    (&array[lengthof(array)])
#define lastof(array)   (&array[lengthof(array) - 1])

#define cpp_offsetof(s,m)   (((size_t)&reinterpret_cast<const volatile char&>((((s*)(char*)8)->m))) - 8)

#define containing_record(addr, type, field) ((type*)((byte*)(addr) - (size_t)(&((type*)0)->field)))



#endif /* MACROS_H */
