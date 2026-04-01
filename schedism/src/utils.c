#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

// 
int equals_ignore_case(const char *a, const char *b)
{
    while (*a && *b)
    {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

int is_blank_or_comment(const char *line)
{
    while (*line)
    {
        if (*line == '#')
            return 1;
        if (!isspace((unsigned char)*line))
            return 0;
        line++;
    }
    return 1;
}

char *trim_spaces(char *s)
{
    while (*s && isspace((unsigned char)*s))
        s++;

    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1]))
        end--;
    *end = '\0';

    return s;
}

int parse_int_strict(const char *s, int *out)
{
    if (s == NULL || *s == '\0')
        return 0;

    char *end = NULL;
    long value = strtol(s, &end, 10);
    if (end == s || *end != '\0')
        return 0;

    if (value < -2147483648L || value > 2147483647L)
        return 0;

    *out = (int)value;
    return 1;
}
