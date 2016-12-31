/*
 * bitstream.c
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of a52dec, a free ATSC A-52 stream decoder.
 * See http://liba52.sourceforge.net/ for updates.
 *
 * a52dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * a52dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <inttypes.h>

#include "a52.h"
#include "a52_internal.h"
#include "bitstream.h"

#define BUFFER_SIZE 4096

void a52_bitstream_set_ptr (a52_state_t * state, uint8_t * buf)
{
    int align;

    align = (long)buf & 3;
    state->buffer_start = (uint32_t *) (buf - align);
    state->bits_left = 0;
    bitstream_get (state, align * 8);
}

static inline void bitstream_fill_current (a52_state_t * state)
{
    uint32_t tmp;

    tmp = *(state->buffer_start++);
    state->current_word = swab32 (tmp);
}

/*
 * The fast paths for _get is in the
 * bitstream.h header file so it can be inlined.
 *
 * The "bottom half" of this routine is suffixed _bh
 *
 * -ah
 */

uint32_t a52_bitstream_get_bh (a52_state_t * state, uint32_t num_bits)
{
    uint32_t result;

    num_bits -= state->bits_left;
    result = ((state->current_word << (32 - state->bits_left)) >>
	      (32 - state->bits_left));

    bitstream_fill_current (state);

    if (num_bits != 0)
	result = (result << num_bits) | (state->current_word >> (32 - num_bits));

    state->bits_left = 32 - num_bits;

    return result;
}

int32_t a52_bitstream_get_bh_2 (a52_state_t * state, uint32_t num_bits)
{
    int32_t result;

    num_bits -= state->bits_left;
    result = ((((int32_t)state->current_word) << (32 - state->bits_left)) >>
	      (32 - state->bits_left));

    bitstream_fill_current(state);

    if (num_bits != 0)
	result = (result << num_bits) | (state->current_word >> (32 - num_bits));
	
    state->bits_left = 32 - num_bits;

    return result;
}
