/* ==========================================================================
 * hexdump.c - hexdump.c
 * --------------------------------------------------------------------------
 * Copyright (c) 2013  William Ahern
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ==========================================================================
 */
#include <limits.h> /* INT_MAX */

#include <stdint.h> /* int64_t */
#include <stdio.h>  /* FILE fprintf(3) snprintf(3) */
#include <stdlib.h> /* malloc(3) realloc(3) free(3) */

#include <string.h> /* memset(3) memmove(3) */

#include <errno.h>  /* ERANGE errno */

#include <setjmp.h> /* _setjmp(3) _longjmp(3) */

#include "hexdump.h"


#define SAY_(fmt, ...) fprintf(stderr, fmt "%s", __FILE__, __LINE__, __func__, __VA_ARGS__);
#define SAY(...) SAY_("@@ %s:%d:%s: " __VA_ARGS__, "\n");
#define HAI SAY("HAI")

#define OOPS(...) do { \
	SAY(__VA_ARGS__); \
	__builtin_trap(); \
} while (0)


#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define countof(a) (sizeof (a) / sizeof *(a))

#ifndef NOTUSED
#define NOTUSED __attribute__((unused))
#endif

#ifndef NORETURN
#define NORETURN __attribute__((noreturn))
#endif

#define NARG_(a, b, c, d, e, f, g, h, N,...) N
#define NARG(...) NARG_(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define PASTE(x, y) x##y
#define XPASTE(x, y) PASTE(x, y)

#define STRINGIFY_(s) #s
#define STRINGIFY(s) STRINGIFY_(s)


static unsigned char toprint(unsigned char chr) {
	return (chr > 0x1f && chr < 0x7f)? chr : '.';
} /* toprint() */


static const char *tooctal(char buf[3], unsigned char chr) {
	if (chr > 0x1f && chr < 0x7f) {
		buf[0] = chr;
		buf[1] = '\0';
	} else {
		switch (chr) {
		case '\0':
			buf[0] = '\\';
			buf[1] = '0';
			buf[2] = '\0';

			break;
		case '\a':
			buf[0] = '\\';
			buf[1] = 'a';
			buf[2] = '\0';

			break;
		case '\b':
			buf[0] = '\\';
			buf[1] = 'b';
			buf[2] = '\0';

			break;
		case '\f':
			buf[0] = '\\';
			buf[1] = 'f';
			buf[2] = '\0';

			break;
		case '\n':
			buf[0] = '\\';
			buf[1] = 'n';
			buf[2] = '\0';

			break;
		case '\r':
			buf[0] = '\\';
			buf[1] = 'r';
			buf[2] = '\0';

			break;
		case '\t':
			buf[0] = '\\';
			buf[1] = 't';
			buf[2] = '\0';

			break;
		case '\v':
			buf[0] = '\\';
			buf[1] = 'v';
			buf[2] = '\0';

			break;
		default:
			buf[0] = "01234567"[0x7 & (chr >> 6)];
			buf[1] = "01234567"[0x7 & (chr >> 3)];
			buf[2] = "01234567"[0x7 & (chr >> 0)];

			break;
		}
	}

	return buf;
} /* tooctal() */


static const char *toshort(char buf[3], unsigned char chr) {
	static const char map[][3] = {
		[0x00] = "nul", [0x01] = "soh", [0x02] = "stx", [0x03] = "etx",
		[0x04] = "eot", [0x05] = "enq", [0x06] = "ack", [0x07] = "bel",
		[0x08] = "bs",  [0x09] = "ht",  [0x0a] = "lf",  [0x0b] = "vt",
		[0x0c] = "ff",  [0x0d] = "cr",  [0x0e] = "so",  [0x0f] = "si",
		[0x10] = "dle", [0x11] = "dc1", [0x12] = "dc2", [0x13] = "dc3",
		[0x14] = "dc4", [0x15] = "nak", [0x16] = "syn", [0x17] = "etb",
		[0x18] = "can", [0x19] = "em",  [0x1a] = "sub", [0x1b] = "esc",
		[0x1c] = "fs",  [0x1d] = "gs",  [0x1e] = "rs",  [0x1f] = "us",
		[0x7f] = "del",
	};

	if (chr <= 0x1f || chr == 0x7f) {
		memcpy(buf, map[chr], 3);
	} else if (chr < 0x7f) {
		buf[0] = chr;
		buf[1] = '\0';
	} else {
		buf[0] = "0123456789abcdef"[0x0f & (chr >> 4)];
		buf[1] = "0123456789abcdef"[0x0f & chr];
		buf[2] = '\0';
	}

	return buf;
} /* toshort() */

static inline unsigned char skipws(const unsigned char **fmt, _Bool nl) {
	static const unsigned char space_nl[] = {
		['\t'] = 1, ['\n'] = 1, ['\v'] = 1, ['\r'] = 1, ['\f'] = 1, [' '] = 1,
	};
	static const unsigned char space_sp[] = {
		['\t'] = 1, ['\v'] = 1, ['\r'] = 1, ['\f'] = 1, [' '] = 1,
	};

	if (nl) {
		while (**fmt < sizeof space_nl && space_nl[**fmt])
			++*fmt;
	} else {
		while (**fmt < sizeof space_sp && space_sp[**fmt])
			++*fmt;
	}

	return **fmt;
} /* skipws() */


static inline int getint(const unsigned char **fmt) {
	static const int limit = ((INT_MAX - (INT_MAX % 10) - 1) / 10);
	int i = -1;

	if (**fmt >= '0' && **fmt <= '9') {
		i = 0;

		do {
			i *= 10;
			i += **fmt - '0';
			++*fmt;
		} while (**fmt >= '0' && **fmt <= '9' && i <= limit);
	}

	return i;
} /* getint() */


#define F_HASH  1
#define F_ZERO  2
#define F_MINUS 4
#define F_SPACE 8
#define F_PLUS 16

#define CHR(x) (STRINGIFY(x)[0])

#define FC2(x, y) ((0xff & (y)) << 8) | (0xff & (x))
#define FC1(x) (0xff & (x))
#define FC(...) XPASTE(FC, NARG(__VA_ARGS__))(__VA_ARGS__)

static inline int getcnv(int *flags, int *width, int *prec, int *bytes, const unsigned char **fmt) {
	int ch;

	*flags = 0;

	for (; (ch = **fmt); ++*fmt) {
		switch (ch) {
		case '#':
			*flags |= F_HASH;
			break;
		case '0':
			*flags |= F_ZERO;
			break;
		case '-':
			*flags |= F_MINUS;
			break;
		case ' ':
			*flags |= F_SPACE;
			break;
		case '+':
			*flags |= F_PLUS;
			break;
		default:
			goto width;
		} /* switch() */
	}

width:
	*width = getint(fmt);
	*prec = (**fmt == '.')? (++*fmt, getint(fmt)) : -1;
	*bytes = 0;

	switch ((ch = **fmt)) {
	case '%':
		break;
	case 'c':
		*bytes = 1;
		break;
	case 'd': case 'i': case 'o': case 'u': case 'X': case 'x':
		*bytes = 4;
		break;
	case 's':
		if (*prec == -1)
			return 0;
		*bytes = *prec;
		break;
	case '_':
		switch (*++*fmt) {
		case 'a':
			switch (*++*fmt) {
			case 'd':
				ch = FC('_', 'd');
				break;
			case 'o':
				ch = FC('_', 'o');
				break;
			case 'x':
				ch = FC('_', 'x');
				break;
			default:
				return 0;
			}
			*bytes = 0;
			break;
		case 'A':
			switch (*++*fmt) {
			case 'd':
				ch = FC('_', 'D');
				break;
			case 'o':
				ch = FC('_', 'O');
				break;
			case 'x':
				ch = FC('_', 'X');
				break;
			default:
				return 0;
			}
			*bytes = 0;

			/* XXX: Not supported yet. */
			return 0;

			break;
		case 'c':
			ch = FC('_', 'c');
			*bytes = 1;
			break;
		case 'p':
			ch = FC('_', 'p');
			*bytes = 1;
			break;
		case 'u':
			ch = FC('_', 'u');
			*bytes = 1;
			break;
		default:
			return 0;
		}

		break;
	} /* switch() */

	++*fmt;

	return ch;
} /* getcnv() */


enum vm_opcode {
	OP_HALT,  /* 0/0 */
	OP_NOOP,  /* 0/0 */
	OP_TRAP,  /* 0/0 */
	OP_PC,    /* 0/1 | push program counter */
	OP_TRUE,  /* 0/1 | push true */
	OP_FALSE, /* 0/1 | push false */
	OP_ZERO,  /* 0/1 | push 0 */
	OP_ONE,   /* 0/1 | push 1 */
	OP_TWO,   /* 0/1 | push 2 */
	OP_I8,    /* 0/1 | load 8-bit unsigned int from code */
	OP_I16,   /* 0/1 | load 16-bit unsigned int from code */
	OP_I32,   /* 0/1 | load 32-bit unsigned int from code */
	OP_NEG,   /* 1/1 | arithmetic negative */
	OP_SUB,   /* 2/1 | S(-2) - S(-1) */
	OP_ADD,   /* 2/1 | S(-2) + S(-1) */
	OP_NOT,   /* 1/1 | logical not */
	OP_POP,   /* 1/0 | pop top of stack */
	OP_DUP,   /* 1/2 | dup top of stack */
	OP_SWAP,  /* 2/2 | swap values at top of stack */
	OP_READ,  /* 1/1 | read bytes from input buffer */
	OP_COUNT, /* 0/1 | count of bytes in input buffer */
	OP_PUTC,  /* 0/0 | copy char directly to output buffer */
	OP_CONV,  /* 5/0 | write conversion to output buffer */
	OP_TRIM,  /* 0/0 | trim trailing white space from output buffer */
	OP_JMP,   /* 2/0 | conditional jump to address */
	OP_RESET, /* 0/0 | reset input buffer position */
}; /* enum vm_opcode */


static const char *vm_strop(enum vm_opcode op) {
	static const char *txt[] = {
		[OP_HALT]  = "HALT",
		[OP_NOOP]  = "NOOP",
		[OP_TRAP]  = "TRAP",
		[OP_PC]    = "PC",
		[OP_TRUE]  = "TRUE",
		[OP_FALSE] = "FALSE",
		[OP_ZERO]  = "ZERO",
		[OP_ONE]   = "ONE",
		[OP_TWO]   = "TWO",
		[OP_I8]    = "I8",
		[OP_I16]   = "I16",
		[OP_I32]   = "I32",
		[OP_NEG]   = "NEG",
		[OP_SUB]   = "SUB",
		[OP_ADD]   = "ADD",
		[OP_NOT]   = "NOT",
		[OP_POP]   = "POP",
		[OP_DUP]   = "DUP",
		[OP_SWAP]  = "SWAP",
		[OP_READ]  = "READ",
		[OP_COUNT] = "COUNT",
		[OP_PUTC]  = "PUTC",
		[OP_CONV]  = "CONV",
		[OP_TRIM]  = "TRIM",
		[OP_JMP]   = "JMP",
		[OP_RESET] = "RESET",
	};

	if ((int)op >= 0 && op < (int)countof(txt) && txt[op])
		return txt[op];
	else
		return "-";
} /* vm_strop() */


struct vm_state {
	jmp_buf trap;

	size_t blocksize;

	int64_t stack[8];
	int sp;

	unsigned char code[4096];
	int pc;

	struct {
		unsigned char *base, *p, *pe;
		size_t address;
		_Bool eof;
	} i;

	struct {
		unsigned char *base, *p, *pe;
	} o;
}; /* struct vm_state */


NOTUSED static void op_dump(struct vm_state *M, int *pc, FILE *fp) {
	enum vm_opcode op = M->code[*pc];
	unsigned n;

	fprintf(fp, "%d: ", *pc);

	switch (op) {
	case OP_I8:
		fprintf(fp, "%s %u\n", vm_strop(op), (unsigned)M->code[++*pc]);

		break;
	case OP_I16:
		n = M->code[++*pc] << 8;
		n |= M->code[++*pc];

		fprintf(fp, "%s %u\n", vm_strop(op), n);

		break;
	case OP_I32:
		n = M->code[++*pc] << 24;
		n |= M->code[++*pc] << 16;
		n |= M->code[++*pc] << 8;
		n |= M->code[++*pc] << 0;

		fprintf(fp, "%s %u\n", vm_strop(op), n);

		break;
	case OP_PUTC: {
		const char *txt = vm_strop(op);
		int chr = M->code[++*pc];

		switch (chr) {
		case '\n':
			fprintf(fp, "%s \\n (0x0a)\n", txt);

			break;
		case '\r':
			fprintf(fp, "%s \\r (0x0d)\n", txt);

			break;
		case '\t':
			fprintf(fp, "%s \\t (0x09)\n", txt);

			break;
		default:
			if (chr > 31 && chr < 127)
				fprintf(fp, "%s %c (0x%.2x)\n", txt, chr, chr);
			else
				fprintf(fp, "%s . (0x%.2x)\n", txt, chr);

			break;
		} /* switch() */

		break;
	}
	default:
		fprintf(fp, "%s\n", vm_strop(op));

		break;
	} /* switch() */

	++*pc;
} /* op_dump() */


NOTUSED static void vm_dump(struct vm_state *M, FILE *fp) {
	enum vm_opcode op;
	int pc = 0;

	fprintf(fp, "-- blocksize: %zu\n", M->blocksize);

	do {
		op = M->code[pc];
		op_dump(M, &pc, fp);
	} while (op != OP_HALT);
} /* vm_dump() */


#define vm_enter(M) _setjmp((M)->trap)

NORETURN static void vm_throw(struct vm_state *M, int error) {
	_longjmp(M->trap, error);
} /* vm_throw() */


static void vm_putc(struct vm_state *M, unsigned char ch) {
	unsigned char *tmp;
	size_t size, p;

	if (!(M->o.p < M->o.pe)) {
		size = MAX(M->o.pe - M->o.base, 64);
		p = M->o.p - M->o.base;

		if (~size < size)
			vm_throw(M, ENOMEM);

		size *= 2;

		if (!(tmp = realloc(M->o.base, size)))
			vm_throw(M, errno);

		M->o.base = tmp;
		M->o.p = &tmp[p];
		M->o.pe = &tmp[size];
	}

	*M->o.p++ = ch;
} /* vm_putc() */


static void vm_push(struct vm_state *M, int64_t v) {
	M->stack[M->sp++] = v;
} /* vm_push() */


static int64_t vm_pop(struct vm_state *M) {
	return M->stack[--M->sp];
} /* vm_pop() */


NOTUSED static int64_t vm_peek(struct vm_state *M, int i) {
	return (i < 0)? M->stack[M->sp + i] : M->stack[i];
} /* vm_peek() */


static void vm_conv(struct vm_state *M, int flags, int width, int prec, int fc, int64_t word) {
	char fmt[32], *fp, buf[256], label[3];
	const char *s;
	int i, len;

	switch (fc) {
	case FC('_', 'c'):
		s = tooctal(label, word);
		prec = (prec > 0)? MIN(prec, 3) : 3;
		fc = 's';

		break;
	case FC('_', 'p'):
		word = toprint(word);
		fc = 'c';

		break;
	case FC('_', 'u'):
		s = toshort(label, word);
		prec = (prec > 0)? MIN(prec, 3) : 3;
		fc = 's';

		break;
	case FC('_', 'd'):
		word = M->i.address + (M->i.p - M->i.base);
		fc = 'd';

		break;
	case FC('_', 'o'):
		word = M->i.address + (M->i.p - M->i.base);
		fc = 'o';

		break;
	case FC('_', 'x'):
		word = M->i.address + (M->i.p - M->i.base);
		fc = 'x';

		break;
	case FC('_', 'D'):
		/* FALL THROUGH */
	case FC('_', 'O'):
		/* FALL THROUGH */
	case FC('_', 'X'):
		fc = 'x';

		vm_throw(M, HXD_ENOTSUPP);

		break;
	case FC('s'):
		s = (const char *)M->i.p;

		if (prec <= 0 || prec > M->i.pe - M->i.p)
			prec = M->i.pe - M->i.p;

		break;
	case FC('c'):
		/* FALL THROUGH */
	case FC('d'): case FC('i'): case FC('o'):
	case FC('u'): case FC('X'): case FC('x'):
		break;
	default:
		vm_throw(M, HXD_ENOTSUPP);

		break;
	} /* switch() */

	fp = fmt;

	*fp++ = '%';

	if (flags & F_HASH)
		*fp++ = '#';
	if (flags & F_ZERO)
		*fp++ = '0';
	if (flags & F_MINUS)
		*fp++ = '-';
	if (flags & F_PLUS)
		*fp++ = '+';

	*fp++ = '*';
	*fp++ = '.';
	*fp++ = '*';
	*fp++ = fc;
	*fp = '\0';
//SAY("fmt:%s prec:%d s:%s", fmt, prec, s);

	switch (fc) {
	case 's':
		len = snprintf(buf, sizeof buf, fmt, (int)MAX(width, 0), (int)MAX(prec, 0), s);

		break;
	case 'u':
		len = snprintf(buf, sizeof buf, fmt, (int)MAX(width, 0), (int)MAX(prec, 0), (unsigned)word);

		break;
	default:
		len = snprintf(buf, sizeof buf, fmt, (int)MAX(width, 0), (int)MAX(prec, 0), (int)word);

		break;
	}

	if (-1 == len)
		vm_throw(M, errno);

	if (len >= (int)sizeof buf)
		vm_throw(M, ENOMEM);

	for (i = 0; i < len; i++)
		vm_putc(M, buf[i]);
} /* vm_conv() */


static void vm_exec(struct vm_state *M) {
	enum vm_opcode op;
	int64_t v;

exec:
	op = M->code[M->pc];

	switch (op) {
	case OP_HALT:
		return /* void */;
	case OP_NOOP:
		break;
	case OP_TRAP:
		vm_throw(M, HXD_EOOPS);

		break;
	case OP_PC:
		vm_push(M, M->pc);

		break;
	case OP_TRUE:
		vm_push(M, 1);

		break;
	case OP_FALSE:
		vm_push(M, 0);

		break;
	case OP_ZERO:
		vm_push(M, 0);

		break;
	case OP_ONE:
		vm_push(M, 1);

		break;
	case OP_TWO:
		vm_push(M, 2);

		break;
	case OP_I8:
		vm_push(M, M->code[++M->pc]);

		break;
	case OP_I16:
		v = M->code[++M->pc] << 8;
		v |= M->code[++M->pc];

		vm_push(M, v);

		break;
	case OP_I32:
		v = M->code[++M->pc] << 24;
		v = M->code[++M->pc] << 16;
		v = M->code[++M->pc] << 8;
		v |= M->code[++M->pc];

		vm_push(M, v);

		break;
	case OP_NEG:
		vm_push(M, -vm_pop(M));

		break;
	case OP_SUB: {
		int64_t b = vm_pop(M);
		int64_t a = vm_pop(M);

		vm_push(M, a - b);

		break;
	}
	case OP_ADD: {
		int64_t b = vm_pop(M);
		int64_t a = vm_pop(M);

		vm_push(M, a + b);

		break;
	}
	case OP_NOT:
		vm_push(M, !vm_pop(M));

		break;
	case OP_POP:
		vm_pop(M);

		break;
	case OP_DUP: {
		int64_t v = vm_pop(M);

		vm_push(M, v);
		vm_push(M, v);

		break;
	}
	case OP_SWAP: {
		int64_t x = vm_pop(M);
		int64_t y = vm_pop(M);

		vm_push(M, x);
		vm_push(M, y);

		break;
	}
	case OP_READ: {
		int64_t i, n, v;

		n = vm_pop(M);
		v = 0;

		for (i = 0; i < n && M->i.p < M->i.pe; i++) {
			v <<= 8;
			v |= *M->i.p++;
		}

		vm_push(M, v);

		break;
	}
	case OP_COUNT:
		vm_push(M, M->i.pe - M->i.p);

		break;
	case OP_PUTC: {
		vm_putc(M, M->code[++M->pc]);

		break;
	}
	case OP_CONV: {
		int fc = vm_pop(M);
		int prec = vm_pop(M);
		int width = vm_pop(M);
		int flags = vm_pop(M);
		int64_t word = vm_pop(M);

		vm_conv(M, flags, width, prec, fc, word);

//		fprintf(stdout, "(spec:%d width:%d prec:%d flags:%d words:0x%.8x)", spec, width, prec, flags, (int)word);

		break;
	}
	case OP_TRIM:
		while (M->o.p > M->o.base && (M->o.p[-1] == ' ' || M->o.p[-1] == '\t'))
			--M->o.p;

		break;
	case OP_JMP: {
		int64_t pc = vm_pop(M);

		if (vm_pop(M)) {
			M->pc = pc;
			goto exec;
		}

		break;
	}
	case OP_RESET:
		M->i.p = M->i.base;

		break;
	} /* switch() */

	++M->pc;

	goto exec;
} /* vm_exec() */


static void emit_op(struct vm_state *M, unsigned char code) {
	if (M->pc >= (int)sizeof M->code - 1)
		vm_throw(M, ENOMEM);
	M->code[M->pc++] = code;
} /* emit_op() */


static void emit_int(struct vm_state *M, int64_t i) {
	_Bool isneg;

	if ((isneg = (i < 0)))
		i *= -1;

	if (i > ((1LL << 32) - 1)) {
		vm_throw(M, ERANGE);
	} else if (i > ((1LL << 16) - 1)) {
		emit_op(M, OP_I32);
		emit_op(M, 0xff & (i >> 24));
		emit_op(M, 0xff & (i >> 16));
		emit_op(M, 0xff & (i >> 8));
		emit_op(M, 0xff & (i >> 0));
	} else if (i > ((1LL << 8) - 1)) {
		emit_op(M, OP_I16);
		emit_op(M, 0xff & (i >> 8));
		emit_op(M, 0xff & (i >> 0));
	} else {
		switch (i) {
		case 0:
			emit_op(M, OP_ZERO);
			break;
		case 1:
			emit_op(M, OP_ONE);
			break;
		case 2:
			emit_op(M, OP_TWO);
			break;
		default:
			emit_op(M, OP_I8);
			emit_op(M, 0xff & i);
			break;
		}
	}

	if (isneg) {
		emit_op(M, OP_NEG);
	}
} /* emit_int() */


static void emit_putc(struct vm_state *M, unsigned char chr) {
	emit_op(M, OP_PUTC);
	emit_op(M, chr);
} /* emit_putc() */


static void emit_jmp(struct vm_state *M, int *from) {
	*from = M->pc;
	emit_op(M, OP_TRAP);
	emit_op(M, OP_TRAP);
	emit_op(M, OP_TRAP);
	emit_op(M, OP_TRAP);
	emit_op(M, OP_TRAP);
	emit_op(M, OP_TRAP);
} /* emit_jmp() */


static void emit_link(struct vm_state *M, int from, int to) {
	int pc = M->pc;

	M->pc = from;

	emit_op(M, OP_PC);

	if (to < from) {
		if (from - to > 65535)
			vm_throw(M, ERANGE);

		emit_op(M, OP_I16);
		M->code[M->pc++] = 0xff & ((from - to) >> 8);
		M->code[M->pc++] = 0xff & ((from - to) >> 0);
		emit_op(M, OP_SUB);
	} else {
		if (to - from > 65535)
			vm_throw(M, ERANGE);

		emit_op(M, OP_I16);
		M->code[M->pc++] = 0xff & ((to - from) >> 8);
		M->code[M->pc++] = 0xff & ((to - from) >> 0);
		emit_op(M, OP_ADD);
	}

	emit_op(M, OP_JMP);

	M->pc = pc;
} /* emit_link() */


static void emit_unit(struct vm_state *M, int loop, int limit, size_t *blocksize, const unsigned char **fmt) {
	_Bool quoted = 0, escaped = 0;
	int consumes = 0;
	int L1, L2, from, ch;

	loop = (loop < 0)? 1 : loop;

	/* loop counter */
	emit_int(M, 0);

	/* top of loop */
	L1 = M->pc;
	emit_op(M, OP_DUP); /* dup counter */
	emit_int(M, loop);  /* push loop count */
	emit_op(M, OP_SWAP);
	emit_op(M, OP_SUB); /* loop - counter */
	emit_op(M, OP_NOT);
	emit_jmp(M, &L2);

	emit_int(M, 1);
	emit_op(M, OP_ADD);

	while ((ch = **fmt)) {
		switch (ch) {
		case '%': {
			int fc, flags, width, prec, bytes;
			int from;

			if (escaped)
				goto copyout;

			++*fmt;

			if (!(fc = getcnv(&flags, &width, &prec, &bytes, fmt)))
				vm_throw(M, HXD_EFORMAT);

			--*fmt;

			if (fc == '%') {
				ch = '%';
				goto copyout;
			}

			if (limit >= 0 && bytes > 0) {
				bytes = MIN(limit - consumes, bytes);

				if (!bytes) /* FIXME: define better error */
					vm_throw(M, HXD_EDRAINED);
			}

			consumes += bytes;

			if (bytes > 0) {
				emit_op(M, OP_COUNT);
				emit_op(M, OP_NOT);
				emit_jmp(M, &from);
			}

			emit_int(M, (fc == 's')? 0 : bytes);
			emit_op(M, OP_READ);
			emit_int(M, flags);
			emit_int(M, MAX(0, width));
			emit_int(M, MAX(0, prec));
			emit_int(M, fc);
			emit_op(M, OP_CONV);

			if (bytes > 0)
				emit_link(M, from, M->pc);

			break;
		}
		case ' ': case '\t': case '\n':
			if (quoted || escaped)
				goto copyout;

			goto epilog;
		case '"':
			if (escaped)
				goto copyout;

			quoted = !quoted;

			break;
		case '\\':
			if (escaped)
				goto copyout;

			escaped = 1;

			break;
		case '0':
			if (escaped)
				ch = '\0';

			goto copyout;
		case 'a':
			if (escaped)
				ch = '\a';

			goto copyout;
		case 'b':
			if (escaped)
				ch = '\b';

			goto copyout;
		case 'f':
			if (escaped)
				ch = '\f';

			goto copyout;
		case 'n':
			if (escaped)
				ch = '\n';

			goto copyout;
		case 'r':
			if (escaped)
				ch = '\r';

			goto copyout;
		case 't':
			if (escaped)
				ch = '\t';

			goto copyout;
		case 'v':
			if (escaped)
				ch = '\v';

			goto copyout;
		default:
copyout:
			emit_putc(M, ch);

			escaped = 0;
		}

		++*fmt;
	}

epilog:
	if (loop > 0 && consumes < limit) {
		emit_int(M, limit - consumes);
		emit_op(M, OP_READ);
		emit_op(M, OP_POP);

		consumes = limit;
	}

	emit_op(M, OP_TRUE);
	emit_jmp(M, &from);
	emit_link(M, from, L1);

	emit_link(M, L2, M->pc);
	emit_op(M, OP_POP); /* pop loop counter */

	if (loop > 1)
		emit_op(M, OP_TRIM);

	*blocksize += (size_t)(consumes * loop);

	return /* void */;
} /* emit_unit() */


struct hexdump {
	struct vm_state vm;

	char help[64];
}; /* struct hexdump */


static void hxd_init(struct hexdump *X) {
	memset(X, 0, sizeof *X);
} /* hxd_init() */


struct hexdump *hxd_open(int *error) {
	struct hexdump *X;

	if (!(X = malloc(sizeof *X)))
		goto syerr;

	hxd_init(X);

	return X;	
syerr:
	*error = errno;

	hxd_close(X);

	return NULL;
} /* hxd_open() */


static void hxd_destroy(struct hexdump *X) {
	free(X->vm.i.base);
	free(X->vm.o.base);
} /* hxd_destroy() */


void hxd_close(struct hexdump *X) {
	if (!X)
		return /* void */;

	hxd_destroy(X);
	free(X);
} /* hxd_close() */


void hxd_reset(struct hexdump *X) {
	X->vm.i.address = 0;
	X->vm.i.p = X->vm.i.base;
	X->vm.o.p = X->vm.o.base;
	X->vm.pc = 0;
} /* hxd_reset() */


int hxd_compile(struct hexdump *X, const const char *_fmt, int flags) {
	const unsigned char *fmt = (const unsigned char *)_fmt;
	unsigned char *tmp;
	int error;

	hxd_reset(X);

	if ((error = vm_enter(&X->vm)))
		goto error;

	while (skipws(&fmt, 1)) {
		int lc, loop, limit;
		size_t blocksize = 0;

		emit_op(&X->vm, OP_RESET);

		do {
			loop = getint(&fmt);

			if ('/' == skipws(&fmt, 0)) {
				fmt++;
				limit = getint(&fmt);
			} else {
				limit = -1;
			}

			skipws(&fmt, 0);
			emit_unit(&X->vm, loop, limit, &blocksize, &fmt);
		} while ((lc = skipws(&fmt, 0)) && lc != '\n');

		if (blocksize > X->vm.blocksize)
			X->vm.blocksize = blocksize;
	}

	emit_op(&X->vm, OP_HALT);
	memset(&X->vm.code[X->vm.pc], OP_TRAP, sizeof X->vm.code - X->vm.pc);

	if (!(tmp = realloc(X->vm.i.base, X->vm.blocksize)))
		goto syerr;

	X->vm.i.base = tmp;
	X->vm.i.p = tmp;
	X->vm.i.pe = &tmp[X->vm.blocksize];

	return 0;
syerr:
	error = errno;
error:
	hxd_reset(X);
	memset(X->vm.code, 0, sizeof X->vm.code);

	return error;
} /* hxd_compile() */


const char *hxd_help(struct hexdump *X) {
	return "helps";
} /* hxd_help() */


int hxd_write(struct hexdump *X, const void *src, size_t len) {
	const unsigned char *p, *pe;
	size_t n;
	int error;

	if ((error = vm_enter(&X->vm)))
		goto error;

	if (X->vm.i.pe == X->vm.i.base)
		vm_throw(&X->vm, HXD_EOOPS);

	p = src;
	pe = p + len;

	while (p < pe) {
		n = MIN(pe - p, X->vm.i.pe - X->vm.i.p);
		memcpy(X->vm.i.p, p, n);
		X->vm.i.p += n;
		p += n;

		if (X->vm.i.p < X->vm.i.pe)
			break;

		X->vm.i.p = X->vm.i.base;
		X->vm.pc = 0;
		vm_exec(&X->vm);
		X->vm.i.p = X->vm.i.base;
		X->vm.i.address += X->vm.blocksize;
	}

	return 0;
error:
	return error;
} /* hxd_write() */


int hxd_flush(struct hexdump *X) {
	unsigned char *pe;
	int error;

	if ((error = vm_enter(&X->vm)))
		goto error;

	if (X->vm.i.p > X->vm.i.base) {
		pe = X->vm.i.pe;
		X->vm.i.pe = X->vm.i.p;
		X->vm.i.p = X->vm.i.base;
		X->vm.pc = 0;
		vm_exec(&X->vm);
		X->vm.i.p = X->vm.i.base;
		X->vm.i.pe = pe;
	}

	return 0;
error:
	return error;
} /* hxd_write() */


size_t hxd_read(struct hexdump *X, void *dst, size_t lim) {
	unsigned char *p, *pe, *op;
	size_t n;

	p = dst;
	pe = p + lim;
	op = X->vm.o.base;

	while (p < pe && op < X->vm.o.p) {
		n = MIN(pe - p, X->vm.o.p - op);
		memcpy(p, op, n);
		p += n;
		op += n;
	}

	n = X->vm.o.p - op;
	memmove(X->vm.o.base, op, n);
	X->vm.o.p = &X->vm.o.base[n];

	return p - (unsigned char *)dst;
} /* hxd_read() */


const char *hxd_strerror(int error) {
	static const char *txt[] = {
		[HXD_EFORMAT - HXD_EBASE] = "invalid format",
		[HXD_EDRAINED - HXD_EBASE] = "unit drains buffer",
		[HXD_ENOTSUPP - HXD_EBASE] = "unsupported conversion sequence",
		[HXD_EOOPS - HXD_EBASE] = "machine traps",
	};

	if (error >= 0)
		return strerror(error);

	if (error >= HXD_EBASE && error < HXD_ELAST) {
		error -= HXD_EBASE;

		if (error < (int)countof(txt) && txt[error])
			return txt[error];
	}

	return "unknown error (hexdump)";
} /* hxd_strerror() */


#if HEXDUMP_MAIN

#include <stdio.h>  /* FILE stdout stderr stdin fprintf(3) */

#include <string.h> /* strcmp(3) */

#include <unistd.h> /* getopt(3) */

#include <err.h>


static void run(struct hexdump *X, FILE *fp, _Bool flush) {
	char buf[256];
	size_t len;
	int error;

	while ((len = fread(buf, 1, sizeof buf, fp))) {
		if ((error = hxd_write(X, buf, len)))
			errx(EXIT_FAILURE, "%s", hxd_strerror(error));

		while ((len = hxd_read(X, buf, sizeof buf)))
			fwrite(buf, 1, len, stdout);
	}

	if (flush) {
		if ((error = hxd_flush(X)))
			errx(EXIT_FAILURE, "%s", hxd_strerror(error));

		while ((len = hxd_read(X, buf, sizeof buf)))
			fwrite(buf, 1, len, stdout);
	}
} /* run() */


int main(int argc, char **argv) {
	extern char *optarg;
	extern int optind;
	int opt;
	_Bool dump = 0;
	struct hexdump *X;
	char buf[256], *fmt = "16/1 %.2x", fmtbuf[512];
	size_t len;
	int error;

	while (-1 != (opt = getopt(argc, argv, "e:f:Dh"))) {
		switch (opt) {
		case 'e':
			fmt = optarg;

			break;
		case 'f': {
			FILE *fp = (!strcmp(optarg, "-"))? stdin : fopen(optarg, "r");

			if (!fp)
				err(EXIT_FAILURE, "%s", optarg);

			len = fread(fmtbuf, 1, sizeof fmtbuf - 1, fp);
			fmtbuf[len] = '\0';

			if (fp != stdin)
				fclose(fp);

			fmt = fmtbuf;

			break;
		}
		case 'D':
			dump = 1;

			break;
		default: {
			FILE *fp = (opt == 'h')? stdout : stderr;

			fprintf(fp,
				"hexdump [-e:f:Dh] [file ...]\n" \
				"  -e FMT   hexdump string format\n" \
				"  -f PATH  path to hexdump format file\n" \
				"  -D       dump the compiled machine\n" \
				"  -h       print usage help\n" \
				"\n" \
				"Report bugs to <william@25thandClement.com>\n"
			);

			return (opt == 'h')? 0 : EXIT_FAILURE;
		}
		} /* switch() */
	}

	argc -= optind;
	argv += optind;

	if (!(X = hxd_open(&error)))
		OOPS("open: %s", hxd_strerror(error));

	if ((error = hxd_compile(X, fmt, 0)))
		OOPS("%s: %s", fmt, hxd_strerror(error));

	if (dump) {
		vm_dump(&X->vm, stdout);

		goto exit;
	}

	if (!optind) {
		run(X, stdin, 1);
	} else {
		int i;

		for (i = 0; i < argc; i++) {
			FILE *fp;

			if (!(fp = fopen(argv[i], "r")))
				err(EXIT_FAILURE, "%s", argv[i]);

			run(X, fp, !argv[i + 1]);

			fclose(fp);
		}
	}
exit:
	hxd_close(X);

	return 0;
} /* main() */

#endif /* HEXDUMP_MAIN */
