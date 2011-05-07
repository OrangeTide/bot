/* A recursive-descent parser generated by peg 0.1.2 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define YYRULECOUNT 12

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>
#include <complex.h>
#include "xcalc.h"

#define YYSTYPE struct num*
#define YY_INPUT(buf, result, max_size)			\
	{						\
	result = !*instr ? 0 : (*(buf) = *instr++, 1);		\
	}

#define INTEGER_BITS (int)(sizeof(YYSTYPE) * CHAR_BIT)

enum num_type {
	T_COMPLEX,
	T_INTEGER,
};

struct num {
	enum num_type type;
	union {
		long long i;
		double complex c;
	} data;
};

static const char *instr;
static int error_flag;

static long long ipow(long long g, unsigned k)
{
        unsigned long long r0, r1;
        unsigned j = sizeof(k) * CHAR_BIT;
        r0 = 1;
        r1 = g;
        while (j--) {
                if ((k >> j) & 1) {
                        r0 = r0 * r1;
                        r1 = r1 * r1;
                } else {
                        r1 = r0 * r1;
                        r0 = r0 * r0;
                }
        }
        return r0;
}

static inline void num_set_integer(struct num *num, long long i)
{
	num->type = T_INTEGER;
	num->data.i = i;
}

static inline void num_set_complex(struct num *num, double complex c)
{
	num->type = T_COMPLEX;
	num->data.c = c;
}

static struct num *num_integer(long long i)
{
	struct num *ret;
	ret = calloc(1, sizeof(*ret));
	num_set_integer(ret, i);
	return ret;
}

static struct num *num_complex(double complex c)
{
	struct num *ret;
	ret = calloc(1, sizeof(*ret));
	num_set_complex(ret, c);
	return ret;
}

void num_free(struct num *num)
{
	if (num) {
		num->type = -1;
		num->data.i = 0x76543210deadbeefll;
		free(num);
	}
}

const char *num_string(const struct num *num)
{
	static char buf[64];
	if (num)
		switch (num->type) {
		case T_COMPLEX: {
			double i = cimag(num->data.c);
			if (i)
				snprintf(buf, sizeof(buf), "%.10g + %.10g * i",
					creal(num->data.c), i);
			else
				snprintf(buf, sizeof(buf), "%.10g",
					creal(num->data.c));
			break;
		}
		case T_INTEGER:
			snprintf(buf, sizeof(buf), "%llu", num->data.i);
			break;
		}
	else
		snprintf(buf, sizeof(buf), "<NIL>");
	return buf;
}

const char *num_hexstring(const struct num *num)
{
	static char buf[64];
	if (num)
		switch (num->type) {
		case T_COMPLEX: {
			double i = cimag(num->data.c);
			if (i)
				snprintf(buf, sizeof(buf), "%.10a + %.10a * i\n",
					creal(num->data.c), i);
			else
				snprintf(buf, sizeof(buf), "%.10a",
					creal(num->data.c));
			break;
		}
		case T_INTEGER:
			snprintf(buf, sizeof(buf), "%#llx", num->data.i);
			break;
		}
	else
		snprintf(buf, sizeof(buf), "<NIL>");
	return buf;
}

static void num_make_complex(struct num *num)
{
	switch (num->type) {
	case T_COMPLEX:
		break;
	case T_INTEGER:
		num->data.c = num->data.i;
		num->type = T_COMPLEX;
		break;
	}
}

/* make a and b the same type */
static void num_check(struct num *a, struct num *b)
{
	if (a->type != b->type) {
		num_make_complex(a);
		num_make_complex(b);
	}
}

/* does: a += b; free(b); */
static struct num *num_add(struct num *a, struct num *b)
{
	num_check(a, b);
	switch (a->type) {
	case T_COMPLEX:
		a->data.c += b->data.c;
		break;
	case T_INTEGER:
		a->data.i += b->data.i;
		break;
	}
	num_free(b);
	return a;
}

static struct num *num_neg(struct num *num)
{
	switch (num->type) {
	case T_COMPLEX:
		num->data.c = -num->data.c;
		break;
	case T_INTEGER:
		num->data.i = -num->data.i;
		break;
	}
	return num;
}

static inline struct num *num_sub(struct num *a, struct num *b)
{
	return num_add(a, num_neg(b));
}

static struct num *num_pow(struct num *a, struct num *b)
{
	num_check(a, b);
	switch (a->type) {
	case T_COMPLEX:
		a->data.c = cpow(a->data.c, b->data.c);
		break;
	case T_INTEGER:
		a->data.i = ipow(a->data.i, b->data.i);
		break;
	}
	num_free(b);
	return a;
}

static struct num *num_mul(struct num *a, struct num *b)
{
	num_check(a, b);
	switch (a->type) {
	case T_COMPLEX:
		a->data.c *= b->data.c;
		break;
	case T_INTEGER:
		a->data.i *= b->data.i;
		break;
	}
	num_free(b);
	return a;
}

static struct num *num_div(struct num *a, struct num *b)
{
	num_check(a, b);
	if (a->type == T_INTEGER && b->data.i == -1 && a->data.i == LLONG_MIN) {
		num_make_complex(a);
		num_make_complex(b);
	}
	switch (a->type) {
	case T_COMPLEX:
		a->data.c /= b->data.c;
		break;
	case T_INTEGER:
		if (b->data.i)
			a->data.i /= b->data.i;
		else
			num_set_complex(a, NAN);
		break;
	}
	num_free(b);
	return a;
}

static struct num *num_mod(struct num *a, struct num *b)
{
	num_check(a, b);
	if (a->type == T_INTEGER && b->data.i == -1 && a->data.i == LLONG_MIN) {
		num_make_complex(a);
		num_make_complex(b);
	}
	switch (a->type) {
	case T_COMPLEX:
		a->data.c = fmod(creal(a->data.c), creal(b->data.c));
		break;
	case T_INTEGER:
		if (b->data.i)
			a->data.i /= b->data.i;
		else
			num_set_complex(a, NAN);
		break;
	}
	num_free(b);
	return a;
}

static struct num *num_and(struct num *a, struct num *b)
{
	if (a->type == T_COMPLEX)
		num_set_integer(a, (long long)creal(a->data.c));
	if (b->type == T_COMPLEX)
		num_set_integer(b, (long long)creal(b->data.c));
	a->data.i &= b->data.i;
	num_free(b);
	return a;
}

static struct num *num_or(struct num *a, struct num *b)
{
	if (a->type == T_COMPLEX)
		num_set_integer(a, (long long)creal(a->data.c));
	if (b->type == T_COMPLEX)
		num_set_integer(b, (long long)creal(b->data.c));
	a->data.i |= b->data.i;
	num_free(b);
	return a;
}

static struct num *num_xor(struct num *a, struct num *b)
{
	if (a->type == T_COMPLEX)
		num_set_integer(a, (long long)creal(a->data.c));
	if (b->type == T_COMPLEX)
		num_set_integer(b, (long long)creal(b->data.c));
	a->data.i ^= b->data.i;
	num_free(b);
	return a;
}

static struct num *num_not(struct num *a)
{
	if (a->type == T_COMPLEX)
		num_set_integer(a, (long long)creal(a->data.c));
	a->data.i = ~a->data.i;
	return a;
}

static struct num *num_shl(struct num *a, struct num *b)
{
	if (b->type == T_COMPLEX)
		num_set_integer(b, (long long)creal(b->data.c));
	switch (a->type) {
	case T_COMPLEX:
		a->data.c = cpow(a->data.c, b->data.i);
		break;
	case T_INTEGER:
		if (b->data.i < 0 && -b->data.i < INTEGER_BITS)
			a->data.i >>= -b->data.i;
		else if (b->data.i >= 0 && b->data.i < INTEGER_BITS)
			a->data.i <<= b->data.i;
		else
			a->data.i = 0;
		break;
	}
	num_free(b);
	return a;
}

static inline struct num *num_shr(struct num *a, struct num *b)
{
	return num_shl(a, num_neg(b));
}

static inline struct num *num_rol(struct num *a, struct num *b)
{
	return num_or(num_shl(a, b), num_shr(a,
		num_sub(num_integer(INTEGER_BITS), b)));
}

static inline struct num *num_ror(struct num *a, struct num *b)
{
	return num_or(num_shr(a, b), num_shl(a,
		num_sub(num_integer(INTEGER_BITS), b)));
}

static struct num *num_eq(struct num *a, struct num *b)
{
	num_check(a, b);
	switch (a->type) {
	case T_COMPLEX:
		num_set_integer(a, a->data.c == b->data.c);
		break;
	case T_INTEGER:
		num_set_integer(a, a->data.i == b->data.i);
		break;
	}
	num_free(b);
	return a;
}

static struct num *num_ne(struct num *a, struct num *b)
{
	num_check(a, b);
	switch (a->type) {
	case T_COMPLEX:
		num_set_integer(a, a->data.c != b->data.c);
		break;
	case T_INTEGER:
		num_set_integer(a, a->data.i != b->data.i);
		break;
	}
	num_free(b);
	return a;
}

static struct num *num_lt(struct num *a, struct num *b)
{
	num_check(a, b);
	switch (a->type) {
	case T_COMPLEX:
		num_set_integer(a, creal(a->data.c) < creal(b->data.c));
		break;
	case T_INTEGER:
		num_set_integer(a, a->data.i < b->data.i);
		break;
	}
	num_free(b);
	return a;
}

static struct num *num_gt(struct num *a, struct num *b)
{
	num_check(a, b);
	switch (a->type) {
	case T_COMPLEX:
		num_set_integer(a, creal(a->data.c) > creal(b->data.c));
		break;
	case T_INTEGER:
		num_set_integer(a, a->data.i > b->data.i);
		break;
	}
	num_free(b);
	return a;
}

static struct num *num_le(struct num *a, struct num *b)
{
	num_check(a, b);
	switch (a->type) {
	case T_COMPLEX:
		num_set_integer(a, creal(a->data.c) <= creal(b->data.c));
		break;
	case T_INTEGER:
		num_set_integer(a, a->data.i <= b->data.i);
		break;
	}
	num_free(b);
	return a;
}

static struct num *num_ge(struct num *a, struct num *b)
{
	num_check(a, b);
	switch (a->type) {
	case T_COMPLEX:
		num_set_integer(a, creal(a->data.c) >= creal(b->data.c));
		break;
	case T_INTEGER:
		num_set_integer(a, a->data.i >= b->data.i);
		break;
	}
	num_free(b);
	return a;
}


#ifndef YY_VARIABLE
#define YY_VARIABLE(T)	static T
#endif
#ifndef YY_LOCAL
#define YY_LOCAL(T)	static T
#endif
#ifndef YY_ACTION
#define YY_ACTION(T)	static T
#endif
#ifndef YY_RULE
#define YY_RULE(T)	static T
#endif
#ifndef YY_PARSE
#define YY_PARSE(T)	T
#endif
#ifndef YYPARSE
#define YYPARSE		yyparse
#endif
#ifndef YYPARSEFROM
#define YYPARSEFROM	yyparsefrom
#endif
#ifndef YY_INPUT
#define YY_INPUT(buf, result, max_size)			\
  {							\
    int yyc= getchar();					\
    result= (EOF == yyc) ? 0 : (*(buf)= yyc, 1);	\
    yyprintf((stderr, "<%c>", yyc));			\
  }
#endif
#ifndef YY_BEGIN
#define YY_BEGIN	( yybegin= yypos, 1)
#endif
#ifndef YY_END
#define YY_END		( yyend= yypos, 1)
#endif
#ifdef YY_DEBUG
# define yyprintf(args)	fprintf args
#else
# define yyprintf(args)
#endif
#ifndef YYSTYPE
#define YYSTYPE	int
#endif

#ifndef YY_PART

typedef void (*yyaction)(char *yytext, int yyleng);
typedef struct _yythunk { int begin, end;  yyaction  action;  struct _yythunk *next; } yythunk;

YY_VARIABLE(char *   ) yybuf= 0;
YY_VARIABLE(int	     ) yybuflen= 0;
YY_VARIABLE(int	     ) yypos= 0;
YY_VARIABLE(int	     ) yylimit= 0;
YY_VARIABLE(char *   ) yytext= 0;
YY_VARIABLE(int	     ) yytextlen= 0;
YY_VARIABLE(int	     ) yybegin= 0;
YY_VARIABLE(int	     ) yyend= 0;
YY_VARIABLE(int	     ) yytextmax= 0;
YY_VARIABLE(yythunk *) yythunks= 0;
YY_VARIABLE(int	     ) yythunkslen= 0;
YY_VARIABLE(int      ) yythunkpos= 0;
YY_VARIABLE(YYSTYPE  ) yy;
YY_VARIABLE(YYSTYPE *) yyval= 0;
YY_VARIABLE(YYSTYPE *) yyvals= 0;
YY_VARIABLE(int      ) yyvalslen= 0;

YY_LOCAL(int) yyrefill(void)
{
  int yyn;
  while (yybuflen - yypos < 512)
    {
      yybuflen *= 2;
      yybuf= realloc(yybuf, yybuflen);
    }
  YY_INPUT((yybuf + yypos), yyn, (yybuflen - yypos));
  if (!yyn) return 0;
  yylimit += yyn;
  return 1;
}

YY_LOCAL(int) yymatchDot(void)
{
  if (yypos >= yylimit && !yyrefill()) return 0;
  ++yypos;
  return 1;
}

YY_LOCAL(int) yymatchChar(int c)
{
  if (yypos >= yylimit && !yyrefill()) return 0;
  if (yybuf[yypos] == c)
    {
      ++yypos;
      yyprintf((stderr, "  ok   yymatchChar(%c) @ %s\n", c, yybuf+yypos));
      return 1;
    }
  yyprintf((stderr, "  fail yymatchChar(%c) @ %s\n", c, yybuf+yypos));
  return 0;
}

YY_LOCAL(int) yymatchString(char *s)
{
  int yysav= yypos;
  while (*s)
    {
      if (yypos >= yylimit && !yyrefill()) return 0;
      if (yybuf[yypos] != *s)
        {
          yypos= yysav;
          return 0;
        }
      ++s;
      ++yypos;
    }
  return 1;
}

YY_LOCAL(int) yymatchClass(unsigned char *bits)
{
  int c;
  if (yypos >= yylimit && !yyrefill()) return 0;
  c= yybuf[yypos];
  if (bits[c >> 3] & (1 << (c & 7)))
    {
      ++yypos;
      yyprintf((stderr, "  ok   yymatchClass @ %s\n", yybuf+yypos));
      return 1;
    }
  yyprintf((stderr, "  fail yymatchClass @ %s\n", yybuf+yypos));
  return 0;
}

YY_LOCAL(void) yyDo(yyaction action, int begin, int end)
{
  while (yythunkpos >= yythunkslen)
    {
      yythunkslen *= 2;
      yythunks= realloc(yythunks, sizeof(yythunk) * yythunkslen);
    }
  yythunks[yythunkpos].begin=  begin;
  yythunks[yythunkpos].end=    end;
  yythunks[yythunkpos].action= action;
  ++yythunkpos;
}

YY_LOCAL(int) yyText(int begin, int end)
{
  int yyleng= end - begin;
  if (yyleng <= 0)
    yyleng= 0;
  else
    {
      while (yytextlen < (yyleng - 1))
	{
	  yytextlen *= 2;
	  yytext= realloc(yytext, yytextlen);
	}
      memcpy(yytext, yybuf + begin, yyleng);
    }
  yytext[yyleng]= '\0';
  return yyleng;
}

YY_LOCAL(void) yyDone(void)
{
  int pos;
  for (pos= 0;  pos < yythunkpos;  ++pos)
    {
      yythunk *thunk= &yythunks[pos];
      int yyleng= thunk->end ? yyText(thunk->begin, thunk->end) : thunk->begin;
      yyprintf((stderr, "DO [%d] %p %s\n", pos, thunk->action, yytext));
      thunk->action(yytext, yyleng);
    }
  yythunkpos= 0;
}

YY_LOCAL(void) yyCommit()
{
  if ((yylimit -= yypos))
    {
      memmove(yybuf, yybuf + yypos, yylimit);
    }
  yybegin -= yypos;
  yyend -= yypos;
  yypos= yythunkpos= 0;
}

YY_LOCAL(int) yyAccept(int tp0)
{
  if (tp0)
    {
      fprintf(stderr, "accept denied at %d\n", tp0);
      return 0;
    }
  else
    {
      yyDone();
      yyCommit();
    }
  return 1;
}

YY_LOCAL(void) yyPush(char *text, int count)	{ yyval += count; }
YY_LOCAL(void) yyPop(char *text, int count)	{ yyval -= count; }
YY_LOCAL(void) yySet(char *text, int count)	{ yyval[count]= yy; }

#endif /* YY_PART */

#define	YYACCEPT	yyAccept(yythunkpos0)

YY_RULE(int) yy_NUM(); /* 12 */
YY_RULE(int) yy_factor(); /* 11 */
YY_RULE(int) yy_term5(); /* 10 */
YY_RULE(int) yy_term4(); /* 9 */
YY_RULE(int) yy_term3(); /* 8 */
YY_RULE(int) yy_term2(); /* 7 */
YY_RULE(int) yy_term1(); /* 6 */
YY_RULE(int) yy_term0(); /* 5 */
YY_RULE(int) yy_EOF(); /* 4 */
YY_RULE(int) yy_expr(); /* 3 */
YY_RULE(int) yy_WS(); /* 2 */
YY_RULE(int) yy_stmt(); /* 1 */

YY_ACTION(void) yy_7_NUM(char *yytext, int yyleng)
{
  yyprintf((stderr, "do yy_7_NUM\n"));
   yy = num_integer(atoll(yytext)); ;
}
YY_ACTION(void) yy_6_NUM(char *yytext, int yyleng)
{
  yyprintf((stderr, "do yy_6_NUM\n"));
   yy = num_complex(atof(yytext)); ;
}
YY_ACTION(void) yy_5_NUM(char *yytext, int yyleng)
{
  yyprintf((stderr, "do yy_5_NUM\n"));
   yy = num_complex(atof(yytext)); ;
}
YY_ACTION(void) yy_4_NUM(char *yytext, int yyleng)
{
  yyprintf((stderr, "do yy_4_NUM\n"));
   yy = num_complex(I); ;
}
YY_ACTION(void) yy_3_NUM(char *yytext, int yyleng)
{
  yyprintf((stderr, "do yy_3_NUM\n"));
   yy = num_integer(strtoll(yytext, 0, 8)); ;
}
YY_ACTION(void) yy_2_NUM(char *yytext, int yyleng)
{
  yyprintf((stderr, "do yy_2_NUM\n"));
   yy = num_integer(strtoll(yytext, 0, 2)); ;
}
YY_ACTION(void) yy_1_NUM(char *yytext, int yyleng)
{
  yyprintf((stderr, "do yy_1_NUM\n"));
   yy = num_integer(strtoll(yytext, 0, 16)); ;
}
YY_ACTION(void) yy_1_factor(char *yytext, int yyleng)
{
#define i yyval[-1]
  yyprintf((stderr, "do yy_1_factor\n"));
   yy = i; ;
#undef i
}
YY_ACTION(void) yy_2_term5(char *yytext, int yyleng)
{
#define l yyval[-1]
  yyprintf((stderr, "do yy_2_term5\n"));
   yy = num_not(l); ;
#undef l
}
YY_ACTION(void) yy_1_term5(char *yytext, int yyleng)
{
#define l yyval[-1]
  yyprintf((stderr, "do yy_1_term5\n"));
   yy = l; ;
#undef l
}
YY_ACTION(void) yy_2_term4(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_2_term4\n"));
   yy = l; ;
#undef r
#undef l
}
YY_ACTION(void) yy_1_term4(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_1_term4\n"));
   l = num_pow(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_4_term3(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_4_term3\n"));
   yy = l; ;
#undef r
#undef l
}
YY_ACTION(void) yy_3_term3(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_3_term3\n"));
   l = num_mod(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_2_term3(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_2_term3\n"));
   l = num_div(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_1_term3(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_1_term3\n"));
   l = num_mul(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_3_term2(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_3_term2\n"));
   yy = l; ;
#undef r
#undef l
}
YY_ACTION(void) yy_2_term2(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_2_term2\n"));
   l = num_sub(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_1_term2(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_1_term2\n"));
   l = num_add(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_5_term1(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_5_term1\n"));
   yy = l; ;
#undef r
#undef l
}
YY_ACTION(void) yy_4_term1(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_4_term1\n"));
   l = num_rol(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_3_term1(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_3_term1\n"));
   l = num_ror(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_2_term1(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_2_term1\n"));
   l = num_shr(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_1_term1(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_1_term1\n"));
   l = num_shl(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_7_term0(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_7_term0\n"));
   yy = l; ;
#undef r
#undef l
}
YY_ACTION(void) yy_6_term0(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_6_term0\n"));
   l = num_ne(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_5_term0(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_5_term0\n"));
   l = num_eq(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_4_term0(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_4_term0\n"));
   l = num_le(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_3_term0(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_3_term0\n"));
   l = num_ge(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_2_term0(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_2_term0\n"));
   l = num_gt(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_1_term0(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_1_term0\n"));
   l = num_lt(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_4_expr(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_4_expr\n"));
   yy = l; ;
#undef r
#undef l
}
YY_ACTION(void) yy_3_expr(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_3_expr\n"));
   l = num_xor(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_2_expr(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_2_expr\n"));
   l = num_or(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_1_expr(char *yytext, int yyleng)
{
#define r yyval[-1]
#define l yyval[-2]
  yyprintf((stderr, "do yy_1_expr\n"));
   l = num_and(l, r); ;
#undef r
#undef l
}
YY_ACTION(void) yy_2_stmt(char *yytext, int yyleng)
{
#define i yyval[-1]
  yyprintf((stderr, "do yy_2_stmt\n"));
   error_flag = 1; ;
#undef i
}
YY_ACTION(void) yy_1_stmt(char *yytext, int yyleng)
{
#define i yyval[-1]
  yyprintf((stderr, "do yy_1_stmt\n"));
   yy = i; ;
#undef i
}

YY_RULE(int) yy_NUM()
{  int yypos0= yypos, yythunkpos0= yythunkpos;
  yyprintf((stderr, "%s\n", "NUM"));
  {  int yypos2= yypos, yythunkpos2= yythunkpos;  if (!yymatchString("0x")) goto l3;  yyText(yybegin, yyend);  if (!(YY_BEGIN)) goto l3;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\176\000\000\000\176\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l3;
  l4:;	
  {  int yypos5= yypos, yythunkpos5= yythunkpos;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\176\000\000\000\176\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l5;  goto l4;
  l5:;	  yypos= yypos5; yythunkpos= yythunkpos5;
  }  yyText(yybegin, yyend);  if (!(YY_END)) goto l3;  if (!yy_WS()) goto l3;  yyDo(yy_1_NUM, yybegin, yyend);  goto l2;
  l3:;	  yypos= yypos2; yythunkpos= yythunkpos2;  if (!yymatchString("0b")) goto l6;  yyText(yybegin, yyend);  if (!(YY_BEGIN)) goto l6;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\176\000\000\000\176\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l6;
  l7:;	
  {  int yypos8= yypos, yythunkpos8= yythunkpos;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\176\000\000\000\176\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l8;  goto l7;
  l8:;	  yypos= yypos8; yythunkpos= yythunkpos8;
  }  yyText(yybegin, yyend);  if (!(YY_END)) goto l6;  if (!yy_WS()) goto l6;  yyDo(yy_2_NUM, yybegin, yyend);  goto l2;
  l6:;	  yypos= yypos2; yythunkpos= yythunkpos2;  if (!yymatchChar('0')) goto l9;  yyText(yybegin, yyend);  if (!(YY_BEGIN)) goto l9;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\176\000\000\000\176\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l9;
  l10:;	
  {  int yypos11= yypos, yythunkpos11= yythunkpos;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\176\000\000\000\176\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l11;  goto l10;
  l11:;	  yypos= yypos11; yythunkpos= yythunkpos11;
  }  yyText(yybegin, yyend);  if (!(YY_END)) goto l9;  if (!yy_WS()) goto l9;  yyDo(yy_3_NUM, yybegin, yyend);  goto l2;
  l9:;	  yypos= yypos2; yythunkpos= yythunkpos2;  if (!yymatchChar('i')) goto l12;  if (!yy_WS()) goto l12;  yyDo(yy_4_NUM, yybegin, yyend);  goto l2;
  l12:;	  yypos= yypos2; yythunkpos= yythunkpos2;  yyText(yybegin, yyend);  if (!(YY_BEGIN)) goto l13;
  {  int yypos14= yypos, yythunkpos14= yythunkpos;
  {  int yypos16= yypos, yythunkpos16= yythunkpos;  if (!yymatchChar('+')) goto l17;  goto l16;
  l17:;	  yypos= yypos16; yythunkpos= yythunkpos16;  if (!yymatchChar('-')) goto l14;
  }
  l16:;	  goto l15;
  l14:;	  yypos= yypos14; yythunkpos= yythunkpos14;
  }
  l15:;	
  l18:;	
  {  int yypos19= yypos, yythunkpos19= yythunkpos;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l19;  goto l18;
  l19:;	  yypos= yypos19; yythunkpos= yythunkpos19;
  }  if (!yymatchChar('.')) goto l13;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l13;
  l20:;	
  {  int yypos21= yypos, yythunkpos21= yythunkpos;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l21;  goto l20;
  l21:;	  yypos= yypos21; yythunkpos= yythunkpos21;
  }
  {  int yypos22= yypos, yythunkpos22= yythunkpos;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\000\000\040\000\000\000\040\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l22;
  {  int yypos24= yypos, yythunkpos24= yythunkpos;
  {  int yypos26= yypos, yythunkpos26= yythunkpos;  if (!yymatchChar('+')) goto l27;  goto l26;
  l27:;	  yypos= yypos26; yythunkpos= yythunkpos26;  if (!yymatchChar('-')) goto l24;
  }
  l26:;	  goto l25;
  l24:;	  yypos= yypos24; yythunkpos= yythunkpos24;
  }
  l25:;	  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l22;
  l28:;	
  {  int yypos29= yypos, yythunkpos29= yythunkpos;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l29;  goto l28;
  l29:;	  yypos= yypos29; yythunkpos= yythunkpos29;
  }  goto l23;
  l22:;	  yypos= yypos22; yythunkpos= yythunkpos22;
  }
  l23:;	  yyText(yybegin, yyend);  if (!(YY_END)) goto l13;  if (!yy_WS()) goto l13;  yyDo(yy_5_NUM, yybegin, yyend);  goto l2;
  l13:;	  yypos= yypos2; yythunkpos= yythunkpos2;  yyText(yybegin, yyend);  if (!(YY_BEGIN)) goto l30;
  {  int yypos31= yypos, yythunkpos31= yythunkpos;
  {  int yypos33= yypos, yythunkpos33= yythunkpos;  if (!yymatchChar('+')) goto l34;  goto l33;
  l34:;	  yypos= yypos33; yythunkpos= yythunkpos33;  if (!yymatchChar('-')) goto l31;
  }
  l33:;	  goto l32;
  l31:;	  yypos= yypos31; yythunkpos= yythunkpos31;
  }
  l32:;	  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l30;
  l35:;	
  {  int yypos36= yypos, yythunkpos36= yythunkpos;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l36;  goto l35;
  l36:;	  yypos= yypos36; yythunkpos= yythunkpos36;
  }  if (!yymatchChar('.')) goto l30;
  l37:;	
  {  int yypos38= yypos, yythunkpos38= yythunkpos;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l38;  goto l37;
  l38:;	  yypos= yypos38; yythunkpos= yythunkpos38;
  }
  {  int yypos39= yypos, yythunkpos39= yythunkpos;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\000\000\040\000\000\000\040\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l39;
  {  int yypos41= yypos, yythunkpos41= yythunkpos;
  {  int yypos43= yypos, yythunkpos43= yythunkpos;  if (!yymatchChar('+')) goto l44;  goto l43;
  l44:;	  yypos= yypos43; yythunkpos= yythunkpos43;  if (!yymatchChar('-')) goto l41;
  }
  l43:;	  goto l42;
  l41:;	  yypos= yypos41; yythunkpos= yythunkpos41;
  }
  l42:;	  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l39;
  l45:;	
  {  int yypos46= yypos, yythunkpos46= yythunkpos;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l46;  goto l45;
  l46:;	  yypos= yypos46; yythunkpos= yythunkpos46;
  }  goto l40;
  l39:;	  yypos= yypos39; yythunkpos= yythunkpos39;
  }
  l40:;	  yyText(yybegin, yyend);  if (!(YY_END)) goto l30;  if (!yy_WS()) goto l30;  yyDo(yy_6_NUM, yybegin, yyend);  goto l2;
  l30:;	  yypos= yypos2; yythunkpos= yythunkpos2;  yyText(yybegin, yyend);  if (!(YY_BEGIN)) goto l1;
  {  int yypos47= yypos, yythunkpos47= yythunkpos;
  {  int yypos49= yypos, yythunkpos49= yythunkpos;  if (!yymatchChar('+')) goto l50;  goto l49;
  l50:;	  yypos= yypos49; yythunkpos= yythunkpos49;  if (!yymatchChar('-')) goto l47;
  }
  l49:;	  goto l48;
  l47:;	  yypos= yypos47; yythunkpos= yythunkpos47;
  }
  l48:;	  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l1;
  l51:;	
  {  int yypos52= yypos, yythunkpos52= yythunkpos;  if (!yymatchClass((unsigned char *)"\000\000\000\000\000\000\377\003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l52;  goto l51;
  l52:;	  yypos= yypos52; yythunkpos= yythunkpos52;
  }  yyText(yybegin, yyend);  if (!(YY_END)) goto l1;  if (!yy_WS()) goto l1;  yyDo(yy_7_NUM, yybegin, yyend);
  }
  l2:;	
  yyprintf((stderr, "  ok   %s @ %s\n", "NUM", yybuf+yypos));
  return 1;
  l1:;	  yypos= yypos0; yythunkpos= yythunkpos0;
  yyprintf((stderr, "  fail %s @ %s\n", "NUM", yybuf+yypos));
  return 0;
}
YY_RULE(int) yy_factor()
{  int yypos0= yypos, yythunkpos0= yythunkpos;  yyDo(yyPush, 1, 0);
  yyprintf((stderr, "%s\n", "factor"));
  {  int yypos54= yypos, yythunkpos54= yythunkpos;  if (!yy_NUM()) goto l55;  yyDo(yySet, -1, 0);  goto l54;
  l55:;	  yypos= yypos54; yythunkpos= yythunkpos54;  if (!yymatchChar('(')) goto l53;  if (!yy_WS()) goto l53;  if (!yy_expr()) goto l53;  yyDo(yySet, -1, 0);  if (!yymatchChar(')')) goto l53;  if (!yy_WS()) goto l53;
  }
  l54:;	  yyDo(yy_1_factor, yybegin, yyend);
  yyprintf((stderr, "  ok   %s @ %s\n", "factor", yybuf+yypos));  yyDo(yyPop, 1, 0);
  return 1;
  l53:;	  yypos= yypos0; yythunkpos= yythunkpos0;
  yyprintf((stderr, "  fail %s @ %s\n", "factor", yybuf+yypos));
  return 0;
}
YY_RULE(int) yy_term5()
{  int yypos0= yypos, yythunkpos0= yythunkpos;  yyDo(yyPush, 1, 0);
  yyprintf((stderr, "%s\n", "term5"));
  {  int yypos57= yypos, yythunkpos57= yythunkpos;  if (!yy_factor()) goto l58;  yyDo(yySet, -1, 0);  yyDo(yy_1_term5, yybegin, yyend);  goto l57;
  l58:;	  yypos= yypos57; yythunkpos= yythunkpos57;  if (!yymatchChar('~')) goto l56;  if (!yy_WS()) goto l56;  if (!yy_factor()) goto l56;  yyDo(yySet, -1, 0);  yyDo(yy_2_term5, yybegin, yyend);
  }
  l57:;	
  yyprintf((stderr, "  ok   %s @ %s\n", "term5", yybuf+yypos));  yyDo(yyPop, 1, 0);
  return 1;
  l56:;	  yypos= yypos0; yythunkpos= yythunkpos0;
  yyprintf((stderr, "  fail %s @ %s\n", "term5", yybuf+yypos));
  return 0;
}
YY_RULE(int) yy_term4()
{  int yypos0= yypos, yythunkpos0= yythunkpos;  yyDo(yyPush, 2, 0);
  yyprintf((stderr, "%s\n", "term4"));  if (!yy_term5()) goto l59;  yyDo(yySet, -2, 0);
  l60:;	
  {  int yypos61= yypos, yythunkpos61= yythunkpos;  if (!yymatchString("**")) goto l61;  if (!yy_WS()) goto l61;  if (!yy_term4()) goto l61;  yyDo(yySet, -1, 0);  yyDo(yy_1_term4, yybegin, yyend);  goto l60;
  l61:;	  yypos= yypos61; yythunkpos= yythunkpos61;
  }  yyDo(yy_2_term4, yybegin, yyend);
  yyprintf((stderr, "  ok   %s @ %s\n", "term4", yybuf+yypos));  yyDo(yyPop, 2, 0);
  return 1;
  l59:;	  yypos= yypos0; yythunkpos= yythunkpos0;
  yyprintf((stderr, "  fail %s @ %s\n", "term4", yybuf+yypos));
  return 0;
}
YY_RULE(int) yy_term3()
{  int yypos0= yypos, yythunkpos0= yythunkpos;  yyDo(yyPush, 2, 0);
  yyprintf((stderr, "%s\n", "term3"));  if (!yy_term4()) goto l62;  yyDo(yySet, -2, 0);
  l63:;	
  {  int yypos64= yypos, yythunkpos64= yythunkpos;
  {  int yypos65= yypos, yythunkpos65= yythunkpos;  if (!yymatchChar('*')) goto l66;  if (!yy_WS()) goto l66;  if (!yy_term4()) goto l66;  yyDo(yySet, -1, 0);  yyDo(yy_1_term3, yybegin, yyend);  goto l65;
  l66:;	  yypos= yypos65; yythunkpos= yythunkpos65;  if (!yymatchChar('/')) goto l67;  if (!yy_WS()) goto l67;  if (!yy_term4()) goto l67;  yyDo(yySet, -1, 0);  yyDo(yy_2_term3, yybegin, yyend);  goto l65;
  l67:;	  yypos= yypos65; yythunkpos= yythunkpos65;  if (!yymatchChar('%')) goto l64;  if (!yy_WS()) goto l64;  if (!yy_term4()) goto l64;  yyDo(yySet, -1, 0);  yyDo(yy_3_term3, yybegin, yyend);
  }
  l65:;	  goto l63;
  l64:;	  yypos= yypos64; yythunkpos= yythunkpos64;
  }  yyDo(yy_4_term3, yybegin, yyend);
  yyprintf((stderr, "  ok   %s @ %s\n", "term3", yybuf+yypos));  yyDo(yyPop, 2, 0);
  return 1;
  l62:;	  yypos= yypos0; yythunkpos= yythunkpos0;
  yyprintf((stderr, "  fail %s @ %s\n", "term3", yybuf+yypos));
  return 0;
}
YY_RULE(int) yy_term2()
{  int yypos0= yypos, yythunkpos0= yythunkpos;  yyDo(yyPush, 2, 0);
  yyprintf((stderr, "%s\n", "term2"));  if (!yy_term3()) goto l68;  yyDo(yySet, -2, 0);
  l69:;	
  {  int yypos70= yypos, yythunkpos70= yythunkpos;
  {  int yypos71= yypos, yythunkpos71= yythunkpos;  if (!yymatchChar('+')) goto l72;  if (!yy_WS()) goto l72;  if (!yy_term3()) goto l72;  yyDo(yySet, -1, 0);  yyDo(yy_1_term2, yybegin, yyend);  goto l71;
  l72:;	  yypos= yypos71; yythunkpos= yythunkpos71;  if (!yymatchChar('-')) goto l70;  if (!yy_WS()) goto l70;  if (!yy_term3()) goto l70;  yyDo(yySet, -1, 0);  yyDo(yy_2_term2, yybegin, yyend);
  }
  l71:;	  goto l69;
  l70:;	  yypos= yypos70; yythunkpos= yythunkpos70;
  }  yyDo(yy_3_term2, yybegin, yyend);
  yyprintf((stderr, "  ok   %s @ %s\n", "term2", yybuf+yypos));  yyDo(yyPop, 2, 0);
  return 1;
  l68:;	  yypos= yypos0; yythunkpos= yythunkpos0;
  yyprintf((stderr, "  fail %s @ %s\n", "term2", yybuf+yypos));
  return 0;
}
YY_RULE(int) yy_term1()
{  int yypos0= yypos, yythunkpos0= yythunkpos;  yyDo(yyPush, 2, 0);
  yyprintf((stderr, "%s\n", "term1"));  if (!yy_term2()) goto l73;  yyDo(yySet, -2, 0);
  l74:;	
  {  int yypos75= yypos, yythunkpos75= yythunkpos;
  {  int yypos76= yypos, yythunkpos76= yythunkpos;  if (!yymatchString("<<")) goto l77;  if (!yy_WS()) goto l77;  if (!yy_term2()) goto l77;  yyDo(yySet, -1, 0);  yyDo(yy_1_term1, yybegin, yyend);  goto l76;
  l77:;	  yypos= yypos76; yythunkpos= yythunkpos76;  if (!yymatchString(">>")) goto l78;  if (!yy_WS()) goto l78;  if (!yy_term2()) goto l78;  yyDo(yySet, -1, 0);  yyDo(yy_2_term1, yybegin, yyend);  goto l76;
  l78:;	  yypos= yypos76; yythunkpos= yythunkpos76;  if (!yymatchString(">>>")) goto l79;  if (!yy_WS()) goto l79;  if (!yy_term2()) goto l79;  yyDo(yySet, -1, 0);  yyDo(yy_3_term1, yybegin, yyend);  goto l76;
  l79:;	  yypos= yypos76; yythunkpos= yythunkpos76;  if (!yymatchString("<<<")) goto l75;  if (!yy_WS()) goto l75;  if (!yy_term2()) goto l75;  yyDo(yySet, -1, 0);  yyDo(yy_4_term1, yybegin, yyend);
  }
  l76:;	  goto l74;
  l75:;	  yypos= yypos75; yythunkpos= yythunkpos75;
  }  yyDo(yy_5_term1, yybegin, yyend);
  yyprintf((stderr, "  ok   %s @ %s\n", "term1", yybuf+yypos));  yyDo(yyPop, 2, 0);
  return 1;
  l73:;	  yypos= yypos0; yythunkpos= yythunkpos0;
  yyprintf((stderr, "  fail %s @ %s\n", "term1", yybuf+yypos));
  return 0;
}
YY_RULE(int) yy_term0()
{  int yypos0= yypos, yythunkpos0= yythunkpos;  yyDo(yyPush, 2, 0);
  yyprintf((stderr, "%s\n", "term0"));  if (!yy_term1()) goto l80;  yyDo(yySet, -2, 0);
  l81:;	
  {  int yypos82= yypos, yythunkpos82= yythunkpos;
  {  int yypos83= yypos, yythunkpos83= yythunkpos;  if (!yymatchChar('<')) goto l84;  if (!yy_WS()) goto l84;  if (!yy_term1()) goto l84;  yyDo(yySet, -1, 0);  yyDo(yy_1_term0, yybegin, yyend);  goto l83;
  l84:;	  yypos= yypos83; yythunkpos= yythunkpos83;  if (!yymatchChar('>')) goto l85;  if (!yy_WS()) goto l85;  if (!yy_term1()) goto l85;  yyDo(yySet, -1, 0);  yyDo(yy_2_term0, yybegin, yyend);  goto l83;
  l85:;	  yypos= yypos83; yythunkpos= yythunkpos83;  if (!yymatchString(">=")) goto l86;  if (!yy_WS()) goto l86;  if (!yy_term1()) goto l86;  yyDo(yySet, -1, 0);  yyDo(yy_3_term0, yybegin, yyend);  goto l83;
  l86:;	  yypos= yypos83; yythunkpos= yythunkpos83;  if (!yymatchString("<=")) goto l87;  if (!yy_WS()) goto l87;  if (!yy_term1()) goto l87;  yyDo(yySet, -1, 0);  yyDo(yy_4_term0, yybegin, yyend);  goto l83;
  l87:;	  yypos= yypos83; yythunkpos= yythunkpos83;  if (!yymatchString("==")) goto l88;  if (!yy_WS()) goto l88;  if (!yy_term1()) goto l88;  yyDo(yySet, -1, 0);  yyDo(yy_5_term0, yybegin, yyend);  goto l83;
  l88:;	  yypos= yypos83; yythunkpos= yythunkpos83;  if (!yymatchString("!=")) goto l82;  if (!yy_WS()) goto l82;  if (!yy_term1()) goto l82;  yyDo(yySet, -1, 0);  yyDo(yy_6_term0, yybegin, yyend);
  }
  l83:;	  goto l81;
  l82:;	  yypos= yypos82; yythunkpos= yythunkpos82;
  }  yyDo(yy_7_term0, yybegin, yyend);
  yyprintf((stderr, "  ok   %s @ %s\n", "term0", yybuf+yypos));  yyDo(yyPop, 2, 0);
  return 1;
  l80:;	  yypos= yypos0; yythunkpos= yythunkpos0;
  yyprintf((stderr, "  fail %s @ %s\n", "term0", yybuf+yypos));
  return 0;
}
YY_RULE(int) yy_EOF()
{  int yypos0= yypos, yythunkpos0= yythunkpos;
  yyprintf((stderr, "%s\n", "EOF"));
  {  int yypos90= yypos, yythunkpos90= yythunkpos;  if (!yymatchDot()) goto l90;  goto l89;
  l90:;	  yypos= yypos90; yythunkpos= yythunkpos90;
  }
  yyprintf((stderr, "  ok   %s @ %s\n", "EOF", yybuf+yypos));
  return 1;
  l89:;	  yypos= yypos0; yythunkpos= yythunkpos0;
  yyprintf((stderr, "  fail %s @ %s\n", "EOF", yybuf+yypos));
  return 0;
}
YY_RULE(int) yy_expr()
{  int yypos0= yypos, yythunkpos0= yythunkpos;  yyDo(yyPush, 2, 0);
  yyprintf((stderr, "%s\n", "expr"));  if (!yy_term0()) goto l91;  yyDo(yySet, -2, 0);
  l92:;	
  {  int yypos93= yypos, yythunkpos93= yythunkpos;
  {  int yypos94= yypos, yythunkpos94= yythunkpos;  if (!yymatchChar('&')) goto l95;  if (!yy_WS()) goto l95;  if (!yy_term0()) goto l95;  yyDo(yySet, -1, 0);  yyDo(yy_1_expr, yybegin, yyend);  goto l94;
  l95:;	  yypos= yypos94; yythunkpos= yythunkpos94;  if (!yymatchChar('|')) goto l96;  if (!yy_WS()) goto l96;  if (!yy_term0()) goto l96;  yyDo(yySet, -1, 0);  yyDo(yy_2_expr, yybegin, yyend);  goto l94;
  l96:;	  yypos= yypos94; yythunkpos= yythunkpos94;  if (!yymatchChar('^')) goto l93;  if (!yy_WS()) goto l93;  if (!yy_term0()) goto l93;  yyDo(yySet, -1, 0);  yyDo(yy_3_expr, yybegin, yyend);
  }
  l94:;	  goto l92;
  l93:;	  yypos= yypos93; yythunkpos= yythunkpos93;
  }  yyDo(yy_4_expr, yybegin, yyend);
  yyprintf((stderr, "  ok   %s @ %s\n", "expr", yybuf+yypos));  yyDo(yyPop, 2, 0);
  return 1;
  l91:;	  yypos= yypos0; yythunkpos= yythunkpos0;
  yyprintf((stderr, "  fail %s @ %s\n", "expr", yybuf+yypos));
  return 0;
}
YY_RULE(int) yy_WS()
{
  yyprintf((stderr, "%s\n", "WS"));
  l98:;	
  {  int yypos99= yypos, yythunkpos99= yythunkpos;  if (!yymatchClass((unsigned char *)"\000\046\000\000\001\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l99;  goto l98;
  l99:;	  yypos= yypos99; yythunkpos= yythunkpos99;
  }
  yyprintf((stderr, "  ok   %s @ %s\n", "WS", yybuf+yypos));
  return 1;
}
YY_RULE(int) yy_stmt()
{  int yypos0= yypos, yythunkpos0= yythunkpos;  yyDo(yyPush, 1, 0);
  yyprintf((stderr, "%s\n", "stmt"));
  {  int yypos101= yypos, yythunkpos101= yythunkpos;  if (!yy_WS()) goto l102;  if (!yy_expr()) goto l102;  yyDo(yySet, -1, 0);  if (!yy_EOF()) goto l102;  yyDo(yy_1_stmt, yybegin, yyend);  goto l101;
  l102:;	  yypos= yypos101; yythunkpos= yythunkpos101;  if (!yymatchDot()) goto l100;  yyDo(yy_2_stmt, yybegin, yyend);
  }
  l101:;	
  yyprintf((stderr, "  ok   %s @ %s\n", "stmt", yybuf+yypos));  yyDo(yyPop, 1, 0);
  return 1;
  l100:;	  yypos= yypos0; yythunkpos= yythunkpos0;
  yyprintf((stderr, "  fail %s @ %s\n", "stmt", yybuf+yypos));
  return 0;
}

#ifndef YY_PART

typedef int (*yyrule)();

YY_PARSE(int) YYPARSEFROM(yyrule yystart)
{
  int yyok;
  if (!yybuflen)
    {
      yybuflen= 1024;
      yybuf= malloc(yybuflen);
      yytextlen= 1024;
      yytext= malloc(yytextlen);
      yythunkslen= 32;
      yythunks= malloc(sizeof(yythunk) * yythunkslen);
      yyvalslen= 32;
      yyvals= malloc(sizeof(YYSTYPE) * yyvalslen);
      yybegin= yyend= yypos= yylimit= yythunkpos= 0;
    }
  yybegin= yyend= yypos;
  yythunkpos= 0;
  yyval= yyvals;
  yyok= yystart();
  if (yyok) yyDone();
  yyCommit();
  return yyok;
  (void)yyrefill;
  (void)yymatchDot;
  (void)yymatchChar;
  (void)yymatchString;
  (void)yymatchClass;
  (void)yyDo;
  (void)yyText;
  (void)yyDone;
  (void)yyCommit;
  (void)yyAccept;
  (void)yyPush;
  (void)yyPop;
  (void)yySet;
  (void)yytextmax;
}

YY_PARSE(int) YYPARSE(void)
{
  return YYPARSEFROM(yy_stmt);
}

#endif

struct num *xcalc(const char *s)
{
	error_flag = 0;
	instr = s;
	yy = NULL;
	while (yyparse()) ;
	if (error_flag) {
		num_free(yy);
		yy = NULL;
	}
	return yy;
}

