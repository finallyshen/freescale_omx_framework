/*
 * utils for libavcodec
 * Copyright (c) 2001 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * utils.
 */

#include "avcodec.h"
#include "raw.h"
#include "mpeg4audio.h"

const uint16_t ff_mpa_freq_tab[3] = { 44100, 48000, 32000 };

const uint8_t *ff_find_start_code(const uint8_t * restrict p, const uint8_t *end, uint32_t * restrict state){
    int i;

    assert(p<=end);
    if(p>=end)
        return end;

    for(i=0; i<3; i++){
        uint32_t tmp= *state << 8;
        *state= tmp + *(p++);
        if(tmp == 0x100 || p==end)
            return p;
    }

    while(p<end){
        if     (p[-1] > 1      ) p+= 3;
        else if(p[-2]          ) p+= 2;
        else if(p[-3]|(p[-1]-1)) p++;
        else{
            p++;
            break;
        }
    }

    p= FFMIN(p, end)-4;
    *state= AV_RB32(p);

    return p+4;
}

enum PixelFormat ff_find_pix_fmt(const PixelFormatTag *tags, unsigned int fourcc)
{
    while (tags->pix_fmt >= 0) {
        if (tags->fourcc == fourcc)
            return tags->pix_fmt;
        tags++;
    }
    return PIX_FMT_YUV420P;
}


static int parse_config_ALS(GetBitContext *gb, MPEG4AudioConfig *c)
{
    if (get_bits_left(gb) < 112)
        return -1;

    if (get_bits_long(gb, 32) != MKBETAG('A','L','S','\0'))
        return -1;

    // override AudioSpecificConfig channel configuration and sample rate
    // which are buggy in old ALS conformance files
    c->sample_rate = get_bits_long(gb, 32);

    // skip number of samples
    skip_bits_long(gb, 32);

    // read number of channels
    c->chan_config = 0;
    c->channels    = get_bits(gb, 16) + 1;

    return 0;
}

const int ff_mpeg4audio_sample_rates[16] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350
};

const uint8_t ff_mpeg4audio_channels[8] = {
    0, 1, 2, 3, 4, 5, 6, 8
};

static inline int get_object_type(GetBitContext *gb)
{
    int object_type = get_bits(gb, 5);
    if (object_type == AOT_ESCAPE)
        object_type = 32 + get_bits(gb, 6);
    return object_type;
}

static inline int get_sample_rate(GetBitContext *gb, int *index)
{
    *index = get_bits(gb, 4);
    return *index == 0x0f ? (int)get_bits(gb, 24) :
        ff_mpeg4audio_sample_rates[*index];
}

int ff_mpeg4audio_get_config(MPEG4AudioConfig *c, const uint8_t *buf, int buf_size)
{
    GetBitContext gb;
    int specific_config_bitindex;

    init_get_bits(&gb, buf, buf_size*8);
    c->object_type = get_object_type(&gb);
    c->sample_rate = get_sample_rate(&gb, &c->sampling_index);
    c->chan_config = get_bits(&gb, 4);
    if (c->chan_config < (int)FF_ARRAY_ELEMS(ff_mpeg4audio_channels))
        c->channels = ff_mpeg4audio_channels[c->chan_config];
    c->sbr = -1;
    c->ps  = -1;
    if (c->object_type == AOT_SBR || (c->object_type == AOT_PS &&
        // check for W6132 Annex YYYY draft MP3onMP4
        !(show_bits(&gb, 3) & 0x03 && !(show_bits(&gb, 9) & 0x3F)))) {
        if (c->object_type == AOT_PS)
            c->ps = 1;
        c->ext_object_type = AOT_SBR;
        c->sbr = 1;
        c->ext_sample_rate = get_sample_rate(&gb, &c->ext_sampling_index);
        c->object_type = get_object_type(&gb);
        if (c->object_type == AOT_ER_BSAC)
            c->ext_chan_config = get_bits(&gb, 4);
    } else {
        c->ext_object_type = AOT_NULL;
        c->ext_sample_rate = 0;
    }
    specific_config_bitindex = get_bits_count(&gb);

    if (c->object_type == AOT_ALS) {
        skip_bits(&gb, 5);
        if (show_bits_long(&gb, 24) != MKBETAG('\0','A','L','S'))
            skip_bits_long(&gb, 24);

        specific_config_bitindex = get_bits_count(&gb);

        if (parse_config_ALS(&gb, c))
            return -1;
    }

    if (c->ext_object_type != AOT_SBR) {
        while (get_bits_left(&gb) > 15) {
            if (show_bits(&gb, 11) == 0x2b7) { // sync extension
                get_bits(&gb, 11);
                c->ext_object_type = get_object_type(&gb);
                if (c->ext_object_type == AOT_SBR && (c->sbr = get_bits1(&gb)) == 1)
                    c->ext_sample_rate = get_sample_rate(&gb, &c->ext_sampling_index);
                if (get_bits_left(&gb) > 11 && get_bits(&gb, 11) == 0x548)
                    c->ps = get_bits1(&gb);
                break;
            } else
                get_bits1(&gb); // skip 1 bit
        }
    }

    //PS requires SBR
    if (!c->sbr)
        c->ps = 0;
    //Limit implicit PS to the HE-AACv2 Profile
    if ((c->ps == -1 && c->object_type != AOT_AAC_LC) || c->channels & ~0x01)
        c->ps = 0;

    return specific_config_bitindex;
}

#define MAX_NEG_CROP 1024
uint8_t ff_cropTbl[256 + 2 * MAX_NEG_CROP] = {0, };
av_cold void dsputil_static_init(void)
{

}

