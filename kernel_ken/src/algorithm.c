#include "algorithm.h"

uint8_t between(uint32_t a, uint32_t b, uint32_t c)
{
    return a <= b && b <= c;
}

char *itoa(int32_t value, char *result, int base)
{
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

char *utoa(uint32_t value, char *result, int base)
{
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

void memcpy(void *dest, const void *src, uint32_t len)
{
    const uint8_t *sp = (const uint8_t*)src;
    uint8_t *dp = (uint8_t *)dest;
    for(; len != 0; len--) *dp++ = *sp++;
}

void memset(void *dest, uint8_t val, uint32_t len)
{
    uint8_t *temp = (uint8_t*)dest;
    for ( ; len != 0; len--) *temp++ = val;
}

int strcmp(char *s1, char *s2)
{
    while(*s1 && (*s1 == *s2)){
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char *strcpy(char *dest, const char *src)
{
    char *ret = dest;
    while((*dest++ = *src++));
    return ret;
}

char *strcat(char *dest, const char *src)
{
    char *ret = dest;
    while (*dest) {
        dest++;
    }

    while ((*dest++ = *src++));

    return ret;
}

int strlen(const char *s)
{
    int len = 0;
    while(*s++){
        len++;
    }
    return len;
}

uint32_t difference(uint32_t a, uint32_t b)
{
    return (a < b) ? (b - a) : (a - b);
}
