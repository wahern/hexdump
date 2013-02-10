/* ==========================================================================
 * hexdump.h - hexdump.c
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
#ifndef HEXDUMP_H
#define HEXDUMP_H


/*
 * H E X D U M P  V E R S I O N  I N T E R F A C E S
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define HXD_VERSION HXD_V_REL
#define HXD_VENDOR "william@25thandClement.com"

#define HXD_V_REL 0x20130209
#define HXD_V_ABI 0x20130208
#define HXD_V_API 0x20130208

int hxd_version(void);
const char *hxd_vendor(void);

int hxd_v_rel(void);
int hxd_v_abi(void);
int hxd_v_api(void);


/*
 * H E X D U M P  E R R O R  I N T E R F A C E S
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define HXD_EBASE -(('D' << 24) | ('U' << 16) | ('M' << 8) | 'P')
#define HXD_ERROR(error) ((error) >= XD_EBASE && (error) < XD_ELAST)


enum hxd_errors {
	HXD_EFORMAT = HXD_EBASE,
	HXD_EDRAINED,
	HXD_ENOTSUPP,
	HXD_EOOPS,
	HXD_ELAST
}; /* enum hxd_errors */

const char *hxd_strerror(int);


/*
 * H E X D U M P  C O R E  I N T E R F A C E S
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct hexdump;

struct hexdump *hxd_open(int *);

void hxd_close(struct hexdump *);

void hxd_reset(struct hexdump *);

int hxd_compile(struct hexdump *, const char *, int);

const char *hxd_help(struct hexdump *);

size_t hxd_blocksize(struct hexdump *);

int hxd_write(struct hexdump *, const void *, size_t);

int hxd_flush(struct hexdump *);

size_t hxd_read(struct hexdump *, void *, size_t);


/*
 * H E X D U M P  L U A  I N T E R F A C E S
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int luaopen_hexdump(/* pointer to lua_State */);


#endif /* HEXDUMP_H */
