#include "algorithm.h"
#include "print.h"
#include "monitor.h"
#include "syscall.h"
/*
                                                 __----~~~~~~~~~~~------___
                                      .  .   ~~//====......          __--~ ~~
                      -.            \_|//     |||\\  ~~~~~~::::... /~
                   ___-==_       _-~o~  \/    |||  \\            _/~~-
           __---~~~.==~||\=_    -_--~/_-~|-   |\\   \\        _/~
       _-~~     .=~    |  \\-_    '-~7  /-   /  ||    \      /
     .~       .~       |   \\ -_    /  /-   /   ||      \   /
    /  ____  /         |     \\ ~-_/  /|- _/   .||       \ /
    |~~    ~~|--~~~~--_ \     ~==-/   | \~--===~~        .\
             '         ~-|      /|    |-~\~~       __--~~
                         |-~~-_/ |    |   ~\_   _-~            /\
                              /  \     \__   \/~                \__
                          _--~ _/ | .-~~____--~-/                  ~~==.
                         ((->/~   '.|||' -_|    ~~-/ ,              . _||
                                    -_     ~\      ~~---l__i__i__i--~~_/
                                    _-~-__   ~)  \--______________--~~
                                  //.-~~~-~_--~- |-------~~~~~~~~
                                         //.-~~~--\

                                ~~~ HERE BE DRAGONS ~~~
*/



// See https://stackoverflow.com/questions/16647278/minimal-implementation-of-sprintf-or-printf


// Sadly we don't have access to <stdarg.h> or anything else that might make this half ways sane.
// So, we have to resort to black magic to get this to work.
//
// The arguments SHOULD be located on the stack as follows:
//      [ Return Address, ... , Last Named Variable, First Vargs Item, Second Vargs Item, ...   ]
//     0xc000  <---- (Example addresses because 'high' and 'low' are confusing for stacks) ----> 0xc02F
//
// This is because the GCC convention is to push parameters to the stack in reverse order
//
// Example:
//
//          EXAMPLE function declaration: my_func(uint32_t a, ...)
//          EXAMPLE function call       : my_func(10, 20, 30, 40)
//
// So we can do something like this:
//          uint32_t* vargs_start = (&a) + 1;      // <--- Start of the '...' part
//
// thus:
//          print(a)
//          print(vargs_start[0]
//          print(vargs_start[1]
//          print(vargs_start[2]
//
// would print out:
//          10, 20, 30, 40

// Also, for some reason, I can't get varargs to work with a char* as the last named argument, hence the
// absolutely absurd macros.

static void ftoa_fixed(char *buffer, double value);
static void ftoa_sci(char *buffer, double value);


//
// These read contents off the stack in a way that doesn't completely suck
// They more or less replace va_arg from <stdarg.h>, but a lot more janky
//

int varg_next_int(void **arg)
{
    int *cast = *arg;

    uint32_t addr = (uint32_t)*arg;
    addr += sizeof(uint32_t);
    *arg = (void*)addr;

    return *cast;
}

uint32_t varg_next_uint32_t(void **arg)
{
    uint32_t *cast = *arg;

    uint32_t addr = (uint32_t)*arg;
    addr += sizeof(uint32_t);
    *arg = (void*)addr;

    return *cast;
}

char *varg_next_charptr(void **arg)
{
    char **cast = *arg;

    uint32_t addr = (uint32_t)*arg;
    addr += sizeof(char *);
    *arg = (void*)addr;

    return *cast;
}

double varg_next_double(void **arg)
{
    double *cast = *arg;

    uint32_t addr = (uint32_t)*arg;
    addr += sizeof(double);
    *arg = (void*)addr;

    return *cast;
}

// The rest of the implementation ...

static int __internal__vfprintf(char* output, int size, char const *fmt, void *arg)
{
    if(size == 0)
        return 0; // Well, we can't really write anything. May as well exit early.

    size--;

    int output_index = 0;

    int int_temp;
    char char_temp;
    char *string_temp;
    double double_temp;

    char ch;
    int length = 0;
    char buffer[512];

    while (output_index < size && (ch = *fmt++)) {
        if ( '%' == ch ) {
            switch (ch = *fmt++) {
                /* %% - print out a single %    */
                case '%':
                    output[output_index++] = '%';
                    length++;
                    break;

                    /* %c: print out a character    */
                case 'c':
                    char_temp = varg_next_int(&arg);
                    output[output_index++] = char_temp;
                    length++;
                    break;

                    /* %s: print out a string       */
                case 's':
                    string_temp = varg_next_charptr(&arg);
                    for(int i = 0; i < strlen(string_temp) && output_index < size; i++){
                        output[output_index++] = string_temp[i];
                    }
                    length += strlen(string_temp);
                    break;

                    /* %d: print out an int         */
                case 'u':
                    int_temp = varg_next_int(&arg);
                    utoa(int_temp, buffer, 10);
                    for(int i = 0; i < strlen(buffer) && output_index < size; i++){
                        output[output_index++] = buffer[i];
                    }
                    length += strlen(buffer);
                    break;

                case 'd':
                    int_temp = varg_next_int(&arg);
                    itoa(int_temp, buffer, 10);
                    for(int i = 0; i < strlen(buffer) && output_index < size; i++){
                        output[output_index++] = buffer[i];
                    }
                    length += strlen(buffer);
                    break;

                    /* %x: print out an int in hex  */
                case 'x':
                    output[output_index++] = '0';
                    output[output_index++] = 'x';
                    int_temp = varg_next_int(&arg);
                    utoa(int_temp, buffer, 16);
                    for(int i = 0; i < strlen(buffer) && output_index < size; i++){
                        output[output_index++] = buffer[i];
                    }
                    length += strlen(buffer);
                    break;

                case 'f':
                    double_temp =  varg_next_double(&arg);
                    ftoa_fixed(buffer, double_temp);
                    for(int i = 0; i < strlen(buffer) && output_index < size; i++){
                        output[output_index++] = buffer[i];
                    }
                    length += strlen(buffer);
                    break;

                case 'e':
                    double_temp = varg_next_double(&arg);
                    ftoa_sci(buffer, double_temp);
                    for(int i = 0; i < strlen(buffer) && output_index < size; i++){
                        output[output_index++] = buffer[i];
                    }
                    length += strlen(buffer);
                    break;
            }
        }
        else {
            output[output_index++] = (ch);
            length++;
        }
    }
    output[output_index] = '\0';
    return length;
}

static int normalize(double *val) {
    int exponent = 0;
    double value = *val;

    while (value >= 1.0) {
        value /= 10.0;
        ++exponent;
    }

    while (value < 0.1) {
        value *= 10.0;
        --exponent;
    }
    *val = value;
    return exponent;
}

static void ftoa_fixed(char *buffer, double value) {
    /* carry out a fixed conversion of a double value to a string, with a precision of 5 decimal digits.
     * Values with absolute values less than 0.000001 are rounded to 0.0
     * Note: this blindly assumes that the buffer will be large enough to hold the largest possible result.
     * The largest value we expect is an IEEE 754 double precision real, with maximum magnitude of approximately
     * e+308. The C standard requires an implementation to allow a single conversion to produce up to 512
     * characters, so that's what we really expect as the buffer size.
     */

    int exponent = 0;
    int places = 0;
    static const int width = 15;

    if (value == 0.0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    if (value < 0.0) {
        *buffer++ = '-';
        value = -value;
    }

    exponent = normalize(&value);

    while (exponent > 0) {
        int digit = value * 10;
        *buffer++ = digit + '0';
        value = value * 10 - digit;
        ++places;
        --exponent;
    }

    if (places == 0)
        *buffer++ = '0';

    *buffer++ = '.';

    while (exponent < 0 && places < width) {
        *buffer++ = '0';
        --exponent;
        ++places;
    }

    while (places < width) {
        int digit = value * 10.0;
        *buffer++ = digit + '0';
        value = value * 10.0 - digit;
        ++places;
    }
    *buffer = '\0';
}

void ftoa_sci(char *buffer, double value) {
    int exponent = 0;
//    int places = 0;
    static const int width = 15;

    if (value == 0.0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    if (value < 0.0) {
        *buffer++ = '-';
        value = -value;
    }

    exponent = normalize(&value);

    int digit = value * 10.0;
    *buffer++ = digit + '0';
    value = value * 10.0 - digit;
    --exponent;

    *buffer++ = '.';

    for (int i = 0; i < width; i++) {
        int digit = value * 10.0;
        *buffer++ = digit + '0';
        value = value * 10.0 - digit;
    }

    *buffer++ = 'e';
    itoa(exponent, buffer, 10);
}

int __kernel__printf__internal(uint32_t fmt,...)
{
    void *arg_start = (void*)((uint32_t)&fmt + sizeof(fmt));

    int length;
    char buffer[512] = "";
    length = __internal__vfprintf(buffer, 512, (char*)fmt, arg_start);

    for(int i = 0; i < strlen(buffer); i++){
        monitor_put(buffer[i]);
    }

    return length;
}

int __user__printf__internal(uint32_t fmt,...)
{
    void *arg_start = (void*)((uint32_t)&fmt + sizeof(fmt));

    int length;
    char buffer[512] = "";
    length = __internal__vfprintf(buffer, 512, (char*)fmt, arg_start);

    syscall_monitor_write(buffer);

    return length;
}

int __snprintf__internal(uint32_t buf, uint32_t size, uint32_t fmt,...)
{
    void *arg_start = (void*)((uint32_t)&fmt + sizeof(fmt));

    int length;
    char *buffer = (char*)buf;
    length = __internal__vfprintf(buffer, size, (char*)fmt, arg_start);

    return length;
}








