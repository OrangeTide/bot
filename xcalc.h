#ifndef XCALC_H
#define XCALC_H
struct num *xcalc(const char *s);
const char *num_string(const struct num *num);
const char *num_hexstring(const struct num *num);
void num_free(struct num *num);
#endif
