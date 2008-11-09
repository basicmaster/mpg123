/*
	synth.c: The functions for synthesizing samples, at the end of decoding.

	copyright 1995-2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp, heavily dissected and rearranged by Thomas Orgis
*/

#include "mpg123lib_intern.h"
#include "debug.h"

/*
	Part 1: All synth functions that produce signed short.
	That is:
		- synth_1to1 with cpu-specific variants (synth_1to1_i386, synth_1to1_i586 ...)
		- synth_1to1_mono and synth_1to1_mono2stereo; which use opt_synth_1to1(fr).
	Nearly every decoder variant has it's own synth_1to1, while the mono conversion is shared.
*/

#define SAMPLE_T short
#define WRITE_SAMPLE(samples,sum,clip) WRITE_SHORT_SAMPLE(samples,sum,clip)

/* Part 1a: All straight 1to1 decoding functions */
#define BLOCK 0x40 /* One decoding block is 64 samples. */

#define SYNTH_NAME synth_1to1
#include "synth.h"
#undef SYNTH_NAME

/* Mono-related synths; they wrap over _some_ synth_1to1. */
#define SYNTH_NAME       opt_synth_1to1(fr)
#define MONO_NAME        synth_1to1_mono
#define MONO2STEREO_NAME synth_1to1_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME


/* Now we have possibly some special synth_1to1 ...
   ... they produce signed short; the mono functions defined above work on the special synths, too. */

#ifdef OPT_X86
/* The i386-specific C code, here as short variant, later 8bit and float. */
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_1to1_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

#undef BLOCK /* Following functions are so special that they don't need this. */

#ifdef OPT_I586
/* This is defined in assembler. */
int synth_1to1_i586_asm(real *bandPtr, int channel, unsigned char *out, unsigned char *buffs, int *bo, real *decwin);
/* This is just a hull to use the mpg123 handle. */
int synth_1to1_i586(real *bandPtr, int channel, mpg123_handle *fr, int final)
{
	int ret;
	if(fr->have_eq_settings) do_equalizer(bandPtr,channel,fr->equalizer);

	ret = synth_1to1_i586_asm(bandPtr, channel, fr->buffer.data+fr->buffer.fill, fr->rawbuffs, fr->bo, fr->decwin);
	if(final) fr->buffer.fill += 128;
	return ret;
}
#endif

#ifdef OPT_I586_DITHER
/* This is defined in assembler. */
int synth_1to1_i586_asm_dither(real *bandPtr, int channel, unsigned char *out, unsigned char *buffs, int *bo, real *decwin);
/* This is just a hull to use the mpg123 handle. */
int synth_1to1_i586_dither(real *bandPtr, int channel, mpg123_handle *fr, int final)
{
	int ret;
	if(fr->have_eq_settings) do_equalizer(bandPtr,channel,fr->equalizer);

	ret = synth_1to1_i586_asm_dither(bandPtr, channel, fr->buffer.data+fr->buffer.fill, fr->rawbuffs, fr->bo, fr->decwin);
	if(final) fr->buffer.fill += 128;
	return ret;
}
#endif

#ifdef OPT_3DNOW
/* Those are defined in assembler. */
void do_equalizer_3dnow(real *bandPtr,int channel, real equalizer[2][32]);
int synth_1to1_3dnow_asm(real *bandPtr, int channel, unsigned char *out, unsigned char *buffs, int *bo, real *decwin);
/* This is just a hull to use the mpg123 handle. */
int synth_1to1_3dnow(real *bandPtr, int channel, mpg123_handle *fr, int final)
{
	int ret;

	if(fr->have_eq_settings) do_equalizer_3dnow(bandPtr,channel,fr->equalizer);

	/* this is in asm, can be dither or not */
	/* uh, is this return from pointer correct? */ 
	ret = (int) synth_1to1_3dnow_asm(bandPtr, channel, fr->buffer.data+fr->buffer.fill, fr->rawbuffs, fr->bo, fr->decwin);
	if(final) fr->buffer.fill += 128;
	return ret;
}
#endif

#ifdef OPT_MMX
/* This is defined in assembler. */
int synth_1to1_MMX(real *bandPtr, int channel, short *out, short *buffs, int *bo, float *decwins);
/* This is just a hull to use the mpg123 handle. */
int synth_1to1_mmx(real *bandPtr, int channel, mpg123_handle *fr, int final)
{
	if(fr->have_eq_settings) do_equalizer(bandPtr,channel,fr->equalizer);

	/* in asm */
	synth_1to1_MMX(bandPtr, channel, (short*) (fr->buffer.data+fr->buffer.fill), (short *) fr->rawbuffs, fr->bo, fr->decwins);
	if(final) fr->buffer.fill += 128;
	return 0;
}
#endif

#ifdef OPT_SSE
/* This is defined in assembler. */
void synth_1to1_sse_asm(real *bandPtr, int channel, short *samples, short *buffs, int *bo, real *decwin);
/* This is just a hull to use the mpg123 handle. */
int synth_1to1_sse(real *bandPtr, int channel, mpg123_handle *fr, int final)
{
	if(fr->have_eq_settings) do_equalizer(bandPtr,channel,fr->equalizer);

	synth_1to1_sse_asm(bandPtr, channel, (short*) (fr->buffer.data+fr->buffer.fill), (short *) fr->rawbuffs, fr->bo, fr->decwins);
	if(final) fr->buffer.fill += 128;
	return 0;
}
#endif

#ifdef OPT_3DNOWEXT
/* This is defined in assembler. */
void synth_1to1_3dnowext_asm(real *bandPtr, int channel, short *samples, short *buffs, int *bo, real *decwin);
/* This is just a hull to use the mpg123 handle. */
int synth_1to1_3dnowext(real *bandPtr, int channel, mpg123_handle *fr, int final)
{
	if(fr->have_eq_settings) do_equalizer(bandPtr,channel,fr->equalizer);

	synth_1to1_3dnowext_asm(bandPtr, channel, (short*) (fr->buffer.data+fr->buffer.fill), (short *) fr->rawbuffs, fr->bo, fr->decwins);
	if(final) fr->buffer.fill += 128;
	return 0;
}
#endif

/*
	Part 1b: 2to1 synth.
	Only generic and i386 functions this time.
*/
#define BLOCK 0x20 /* One decoding block is 32 samples. */

#define SYNTH_NAME synth_2to1
#include "synth.h"
#undef SYNTH_NAME

#define SYNTH_NAME       opt_synth_2to1(fr) /* This is just for the _i386 one... gotta check if it is really useful... */
#define MONO_NAME        synth_2to1_mono
#define MONO2STEREO_NAME synth_2to1_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#ifdef OPT_X86
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_2to1_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

#undef BLOCK

/*
	Part 1c: 4to1 synth.
	Same procedure as above...
*/
#define BLOCK 0x10 /* One decoding block is 16 samples. */

#define SYNTH_NAME synth_4to1
#include "synth.h"
#undef SYNTH_NAME

#define SYNTH_NAME       opt_synth_4to1(fr) /* This is just for the _i386 one... gotta check if it is really useful... */
#define MONO_NAME        synth_4to1_mono
#define MONO2STEREO_NAME synth_4to1_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#ifdef OPT_X86
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_4to1_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

#undef BLOCK

/*
	Part 1d: ntom synth.
	Same procedure as above... Just no extra play anymore, straight synth that uses the plain dct64.
*/

/* These are all in one header, there's no flexibility to gain. */
#define SYNTH_NAME       synth_ntom
#define MONO_NAME        synth_ntom_mono
#define MONO2STEREO_NAME synth_ntom_mono2stereo
#include "synth_ntom.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

/* Done with short output. */
#undef SAMPLE_T
#undef WRITE_SAMPLE

/* 
	Part 2: All synth functions that produce 8bit output.
	What we need is just a special WRITE_SAMPLE. For the generic and i386 functions, that is.
	For the rather optimized synth_1to1, we will need the postprocessing 8bit converters from synth_8bit.h .
*/

#define SAMPLE_T unsigned char
#define WRITE_SAMPLE(samples,sum,clip) WRITE_8BIT_SAMPLE(samples,sum,clip)

/* Part 2a: All straight 1to1 decoding functions */
#define BLOCK 0x40 /* One decoding block is 64 samples. */

#define SYNTH_NAME synth_1to1_8bit
#include "synth.h"
#undef SYNTH_NAME

/* Mono-related synths; they wrap over _some_ synth_1to1_8bit (could be generic, could be i386). */
#define SYNTH_NAME       opt_synth_1to1_8bit(fr)
#define MONO_NAME        synth_1to1_8bit_mono
#define MONO2STEREO_NAME synth_1to1_8bit_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#ifdef OPT_X86
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_1to1_8bit_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

/* But now, we need functions that take the 16bit output of optimized synth_1to1 and convert it.
   I suppose that is still faster than dropping the optimization altogether! */

#define BASE_SYNTH_NAME  opt_synth_1to1(fr)
#define SYNTH_NAME       synth_1to1_8bit_wrap
#define MONO_NAME        synth_1to1_8bit_wrap_mono
#define MONO2STEREO_NAME synth_1to1_8bit_wrap_mono2stereo
#include "synth_8bit.h"
#undef BASE_SYNTH_NAME
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#undef BLOCK

/*
	Part 2b: 2to1 synth. Only generic and i386.
*/
#define BLOCK 0x20 /* One decoding block is 32 samples. */

#define SYNTH_NAME synth_2to1_8bit
#include "synth.h"
#undef SYNTH_NAME

/* Mono-related synths; they wrap over _some_ synth_2to1_8bit (could be generic, could be i386). */
#define SYNTH_NAME       opt_synth_2to1_8bit(fr)
#define MONO_NAME        synth_2to1_8bit_mono
#define MONO2STEREO_NAME synth_2to1_8bit_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#ifdef OPT_X86
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_2to1_8bit_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

#undef BLOCK

/*
	Part 2c: 4to1 synth. Only generic and i386.
*/
#define BLOCK 0x10 /* One decoding block is 16 samples. */

#define SYNTH_NAME synth_4to1_8bit
#include "synth.h"
#undef SYNTH_NAME

/* Mono-related synths; they wrap over _some_ synth_4to1_8bit (could be generic, could be i386). */
#define SYNTH_NAME       opt_synth_4to1_8bit(fr)
#define MONO_NAME        synth_4to1_8bit_mono
#define MONO2STEREO_NAME synth_4to1_8bit_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#ifdef OPT_X86
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_4to1_8bit_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

#undef BLOCK

/*
	Part 2d: ntom synth.
	Same procedure as above... Just no extra play anymore, straight synth that may use an optimized dct64.
*/

/* These are all in one header, there's no flexibility to gain. */
#define SYNTH_NAME       synth_ntom_8bit
#define MONO_NAME        synth_ntom_8bit_mono
#define MONO2STEREO_NAME synth_ntom_8bit_mono2stereo
#include "synth_ntom.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#undef SAMPLE_T
#undef WRITE_SAMPLE

#ifndef REAL_IS_FIXED /* Returning 32bit integer won't make much sense, since the precision with fixed math is lower. */
/* 
	Part 3: All synth functions that produce float output.
	What we need is just a special WRITE_SAMPLE. For the generic and i386 functions, that is.
	The optimized synths would need to be changed internally to support float output.
*/

#define SAMPLE_T real
#define WRITE_SAMPLE(samples,sum,clip) WRITE_REAL_SAMPLE(samples,sum,clip)

/* Part 3a: All straight 1to1 decoding functions */
#define BLOCK 0x40 /* One decoding block is 64 samples. */

#define SYNTH_NAME synth_1to1_real
#include "synth.h"
#undef SYNTH_NAME

/* Mono-related synths; they wrap over _some_ synth_1to1_real (could be generic, could be i386). */
#define SYNTH_NAME       opt_synth_1to1_real(fr)
#define MONO_NAME        synth_1to1_real_mono
#define MONO2STEREO_NAME synth_1to1_real_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#ifdef OPT_X86
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_1to1_real_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

#undef BLOCK

/*
	Part 3b: 2to1 synth. Only generic and i386.
*/
#define BLOCK 0x20 /* One decoding block is 32 samples. */

#define SYNTH_NAME synth_2to1_real
#include "synth.h"
#undef SYNTH_NAME

/* Mono-related synths; they wrap over _some_ synth_2to1_real (could be generic, could be i386). */
#define SYNTH_NAME       opt_synth_2to1_real(fr)
#define MONO_NAME        synth_2to1_real_mono
#define MONO2STEREO_NAME synth_2to1_real_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#ifdef OPT_X86
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_2to1_real_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

#undef BLOCK

/*
	Part 3c: 4to1 synth. Only generic and i386.
*/
#define BLOCK 0x10 /* One decoding block is 16 samples. */

#define SYNTH_NAME synth_4to1_real
#include "synth.h"
#undef SYNTH_NAME

/* Mono-related synths; they wrap over _some_ synth_4to1_real (could be generic, could be i386). */
#define SYNTH_NAME       opt_synth_4to1_real(fr)
#define MONO_NAME        synth_4to1_real_mono
#define MONO2STEREO_NAME synth_4to1_real_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#ifdef OPT_X86
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_4to1_real_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

#undef BLOCK

/*
	Part 3d: ntom synth.
	Same procedure as above... Just no extra play anymore, straight synth that may use an optimized dct64.
*/

/* These are all in one header, there's no flexibility to gain. */
#define SYNTH_NAME       synth_ntom_real
#define MONO_NAME        synth_ntom_real_mono
#define MONO2STEREO_NAME synth_ntom_real_mono2stereo
#include "synth_ntom.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#undef SAMPLE_T
#undef WRITE_SAMPLE

#endif /* non-fixed type */