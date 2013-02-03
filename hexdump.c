
#include <limits.h> /* INT_MAX */
#include <errno.h>  /* EOVERFLOW */


#define XD_EBASE -(('D' << 24) | ('U' << 16) | ('M' << 8) | 'P')
#define XD_ERROR(error) ((error) >= XD_EBASE && (error) < XD_ELAST)

enum xd_errors {
	XD_EFORMAT = XD_EBASE,
	XD_ELAST
}; /* enum xd_errors */


typedef struct hexdump {
	struct {
		unsigned short iterations;
		unsigned short bytes;
	} fmt[8][8];
} XD;


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


#define I(op)  (0x0f & (op))
#define A0(op) (0x03 & ((op) >> 4))
#define A1(op) (0x03 & ((op) >> 6))
#define DST(op) AO(op)
#define SRC(op) A1(op)
#define VAL(op) A1(op)

#define M_I(M) I((M)->code[(M)->pc])
#define M_DST(M) ((M)->r[DST(M_I(M))])
#define M_SRC(M) ((M)->r[SRC(M_I(M))])
#define M_VAL(M) ((M)->r[VAL(M_I(M))])

enum {
	I_HALT,
	I_NOOP,
	I_PC,
	I_LOAD,
	I_LT,
	I_NEG,
	I_ADD,
	I_JMP,
	I_REWIND,
	I_BUFFER,
	I_PUTL,
};


struct machine {
	int pc;
	int64_t r[4];

	unsigned char code[4096];

	struct {
		unsigned char *top, *p, *pe;
		_Bool eof;
	} i;

	struct {
		unsigned char *top, *p, *pe;
	} o;
}; /* struct machine */


static int exec(struct machine *M) {
	unsigned op;
	int64_t *r;

exec:
	op = M->code[M->pc];

	switch (I(op)) {
	case I_HALT:
		return 0;
	case I_NOOP:
		break;
	case I_PC:
		M->r[DST(op)] = M->pc;

		break;
	case I_LOAD:
		r = &M->r[DST(op)];
		*r = 0;

		switch (VAL(op)) {
		case 3:
			*r |= (0xffU & M->code[++M->pc]);
			*r <<= 8;
			*r |= (0xffU & M->code[++M->pc]);
			*r <<= 8;
			*r |= (0xffU & M->code[++M->pc]);
			*r <<= 8;
			*r |= (0xffU & M->code[++M->pc]);
			*r <<= 8;
		case 2:
			*r |= (0xffU & M->code[++M->pc]);
			*r <<= 8;
			*r |= (0xffU & M->code[++M->pc]);
			*r <<= 8;
		case 1:
			*r |= (0xffU & M->code[++M->pc]);
			*r <<= 8;
		case 0:
			*r |= (0xffU & M->code[++M->pc]);
		}

		break;
	case I_LT:
		M_DST(M) = (M_DST(M) < M_SRC(M));

		break;
	case I_NEG:
		M_DST(M) = -M_DST(M);

		break;
	case I_ADD:
		M_DST(M) = M_DST(M) + M_SRC(M);

		break;
	case I_JMP:
		if (!M->r[A0(op)])
			break;

		M->pc = M->r[A1(op)];

		goto exec;
	case I_REWIND:
		M->p = M->block;

		break;
	case I_BUFFER:
		if ((M->i.pe - M->i.top) < M->r[SRC(op)] && !M->i.eof)
			return EAGAIN;

		break;
	case I_PUTL:
		*M->o.p++ = M->code[++M->pc];

		break;
	} /* switch() */

	++M->pc;

	goto exec;

	return 0;
} /* exec() */






static inline size_t getfmt(unsigned char *code, size_t lim, const unsigned char **fmt) {
	_Bool quoted = 0, escaped = 0;
	unsigned char *cp, *pe;
	int ch;

	cp = code;
	pe = &code[lim];

	for (; (ch = **fmt); ++*fmt) {
		switch (ch) {
		case '%':
			if (escaped)
				goto copyout;

			for (++*fmt; (ch = **fmt)) {

			break;
		case ' ': case '\t':
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
			if (pe - cp < 2)
				goto epilog;

			*cp++ = I_PUTL;
			*cp++ = ch;

			escaped = 0;
		}
	}
epilog:
	return cp - code;
} /* getfmt() */


static int parse(const unsigned char *fmt) {
	int iterations, bytes;

	while (skipws(&fmt, 1)) {
		iterations = getint(&fmt);

		if ('/' == skipws(&fmt, 0)) {
			fmt++;
			bytes = getint(&fmt);
		} else {
			bytes = -1;
		}

		skipws(&fmt, 0);
	}

	return 0;
badfmt:
	return XD_EFORMAT;
} /* parse() */



int main(int argc, char **argv) {
	parse((void *)argv[1]);

	return 0;
} /* main() */
