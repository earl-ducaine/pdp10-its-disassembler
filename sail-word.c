/* Copyright (C) 2021 Lars Brinkhoff <lars@nocrew.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdio.h>

#include "dis.h"

static int leftover, there_is_some_leftover = 0;

static int sail (int c)
{
  switch (c)
    {
    case 020623: return 0001; //↓
    case 001661: return 0002; //α
    case 001662: return 0003; //β
    case 021047: return 0004; //∧
    case 000254: return 0005; //¬
    case 001665: return 0006; //ε
    case 001700: return 0007; //π
    case 001673: return 0010; //λ
    case 021036: return 0016; //∞
    case 021002: return 0017; //∂
    case 021202: return 0020; //⊂
    case 021203: return 0021; //⊃
    case 021051: return 0022; //∩
    case 021052: return 0023; //∪
    case 021000: return 0024; //∀
    case 021003: return 0025; //∃
    case 021227: return 0026; //⊗
    case 020624: return 0027; //↔
    case 020622: return 0031; //→
    case 021140: return 0033; //≠
    case 021144: return 0034; //≤
    case 021145: return 0035; //≥
    case 021141: return 0036; //≡
    case 021050: return 0037; //∨
    case 020621: return 0136; //↑
    case 020620: return 0137; //←
    default:
      fprintf (stderr, "[illegal character]\n");
      exit (1);
    }
}

static inline int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

static inline word_t
insert (word_t word, unsigned char byte, int *bits)
{
  *bits += 7;
  return (word << 7) | byte;
}

static word_t
get_sail_word (FILE *f)
{
  unsigned char byte;
  word_t word;
  int bits;

  if (feof (f))
    return -1;

  word = 0;
  bits = 0;

  if (there_is_some_leftover)
    {
      word = leftover;
      bits = 7;
      there_is_some_leftover = 0;
    }

  while (bits < 36)
    {
      byte = get_byte (f);
      if (feof (f))
	{
	  if (bits == 0)
	    return -1;
	}

      if (byte == 012)
	{
	  word = insert (word, 015, &bits);
	  word = insert (word, 012, &bits);
	}
      else if (byte == 0137)
	{
	  word = insert (word, 030, &bits);
	}
      else if (byte == 0175)
	{
	  word = insert (word, 0176, &bits);
	}
      else if (byte == 0176)
	{
	  word = insert (word, 032, &bits);
	}
      else if (byte <= 0177)
	{
	  word = insert (word, byte, &bits);
	}
      else if (byte >= 0300 && byte <= 0337)
	{
	  int c = (byte & 037) << 6;
	  c |= get_byte (f) & 077;
	  word = insert (word, sail (c), &bits);
	}
      else if (byte >= 0340 && byte <= 0357)
	{
	  int c = (byte & 017) << 12;
	  c |= (get_byte (f) & 077) << 6;
	  c |= get_byte (f) & 077;
	  word = insert (word, sail (c), &bits);
	}
      else if (byte >= 0360 && byte <= 0367)
	{
	  int c = (byte & 7) << 18;
	  c |= (get_byte (f) & 077) << 12;
	  c |= (get_byte (f) & 077) << 6;
	  c |= get_byte (f) & 077;
	  word = insert (word, sail (c), &bits);
	}
      else
	{
	  fprintf (stderr, "[illegal UTF-8]\n");
	  exit (1);
	}

      if (bits == 35)
	{
	  word <<= 1;
	  bits++;
	}
      else if (bits == 42)
	{
	  leftover = word & 0177;
	  there_is_some_leftover = 1;
	  word >>= 7;
	  word <<= 1;
	}
    }

  if (word > WORDMASK)
    {
      fprintf (stderr, "[error in 36-bit file format (word too large)]\n");
      exit (1);
    }

  return word;
}

static void
rewind_sail_word (FILE *f)
{
  there_is_some_leftover = 0;
  rewind (f);
}

struct word_format sail_word_format = {
  "sail",
  get_sail_word,
  rewind_sail_word,
  NULL,
  NULL
};
