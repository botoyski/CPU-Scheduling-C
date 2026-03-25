#ifndef UTILS_H
#define UTILS_H

int equals_ignore_case(const char *a, const char *b);
int is_blank_or_comment(const char *line);
char *trim_spaces(char *s);
int parse_int_strict(const char *s, int *out);

#endif
