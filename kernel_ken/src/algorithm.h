#ifndef ALGORITHM_H
#define ALGORITHM_H

//
// Implementations of some nice C functions
//
// Most are based on posts from stackoverflow or pages on wikipedia.
// Some are modified to work for us though, especially because we have no stdlib.h
//
//

#include "common.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


/// Returns TRUE if a < b < c
/// \returns TRUE for (a < b) && (a < c), otherwise FALSE
uint8_t between(uint32_t a, uint32_t b, uint32_t c);

/// Converts a s32int to a null terminated char*
/// \param [in] value the s32int to convert to a char*
/// \param [out] result a buffer sufficiently large to hold the resulting string. Note that 32 bytes is safe
/// due to the bounds on an integer size (or even a lower size is probably fine)
/// \param [in] base the base (from 2 to 36) to convert to.
/// \returns the provided result* (which has the string)
char *itoa (int32_t value, char *result, int base);

/// Converts a uint32_t to a null terminated char*
/// \param [in] value the uint32_t to convert to a char*
/// \param [out] result a buffer sufficiently large to hold the resulting string. Note that 32 bytes is safe
/// due to the bounds on an integer size (or even a lower size is probably fine)
/// \param [in] base the base (from 2 to 36) to convert to.
/// \returns the provided result* (which has the string)
char *utoa(uint32_t value, char *result, int base);


/// Write the specified value to len bytes following dest
/// \param [in] dest the location to start writing
/// \param [in] val the value to write to each byte
/// \param [in] len how many bytes to write
void memset(void *dest, uint8_t val, uint32_t len);

/// Copy len bytes from src to dest
/// \param [in] dest the location to write to
/// \param [in] src the location to read from
/// \param [in] len the number of bytes to write
void memcpy(void *dest, const void *src, uint32_t len);

/// Compare two strings based on a lexicographic comparison.
/// \param [in] s1 first string to compare
/// \param [in] s2 second string to compare
/// \returns <0 if s1 < s2; 0 if s1 = s2, else >0 for s1 > s2
int strcmp(char *s1, char *s2);

/// Copy a null terminated string from src to dest
/// \param [in] dest a copy of src will be written to this buffer including null terminator
/// \param [in] src the string to make a copy of
/// \return a pointer to the dest buffer passed into the function (containing the new string copy)
char *strcpy(char *dest, const char *src);

/// Concatenate src onto the end of dest
/// \param [in] dest a null terminated string (plus additional space at the end of the buffer because src and
/// a null terminator will be written to the end)
/// \param [in] src a null terminated string whose contents will be written to the end of dest
/// \return a pointer to dest, containing the newly concatenated string
char *strcat(char *dest, const char *src);

/// Get the length of a null terminated string
/// \param s a null terminated string
/// \return the length of a null terminated string (not including the terminator, as is convention)
int strlen(const char *s);

/// Gets the absolute difference between two values. Note that this is safe from numeric underflow in most cases
/// \param [in] a a uint32_t
/// \param [in] b a uint32_t
/// \returns the difference between a and b
uint32_t difference(uint32_t a, uint32_t b);


#endif
