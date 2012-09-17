/*
 * Register all the formats and protocols
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
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
#include "avformat.h"
#include "rtp.h"
#include "rdt.h"
#include "url.h"

#define REGISTER_MUXER(X,x) { \
    extern AVOutputFormat ff_##x##_muxer; \
    if(CONFIG_##X##_MUXER) av_register_output_format(&ff_##x##_muxer); }

#define REGISTER_DEMUXER(X,x) { \
    extern AVInputFormat ff_##x##_demuxer; \
    if(CONFIG_##X##_DEMUXER) av_register_input_format(&ff_##x##_demuxer); }

#define REGISTER_MUXDEMUX(X,x)  REGISTER_MUXER(X,x); REGISTER_DEMUXER(X,x)

#define REGISTER_PROTOCOL(X,x) { \
    extern URLProtocol ff_##x##_protocol; \
    if(CONFIG_##X##_PROTOCOL) ffurl_register_protocol(&ff_##x##_protocol, sizeof(ff_##x##_protocol)); }

void av_register_all(void)
{
    static int initialized;

    if (initialized)
        return;
    initialized = 1;

    //avcodec_register_all();

    /* (de)muxers */
    REGISTER_DEMUXER  (MPEGTS, mpegts);

    REGISTER_DEMUXER  (APPLEHTTP, applehttp);
    REGISTER_MUXDEMUX (RTP, rtp);
    REGISTER_MUXDEMUX (RTSP, rtsp);
    REGISTER_MUXDEMUX (SAP, sap);
    REGISTER_DEMUXER  (SDP, sdp);
#if CONFIG_RTPDEC
    av_register_rtp_dynamic_payload_handlers();
    av_register_rdt_dynamic_payload_handlers();
#endif

    /* protocols */
    REGISTER_PROTOCOL (APPLEHTTP, applehttp);
    REGISTER_PROTOCOL (HTTP, http);
    REGISTER_PROTOCOL (MMSH, mmsh);
    REGISTER_PROTOCOL (MMST, mmst);
    REGISTER_PROTOCOL (RTMP, rtmp);
    REGISTER_PROTOCOL (RTP, rtp);
    REGISTER_PROTOCOL (TCP, tcp);
    REGISTER_PROTOCOL (UDP, udp);
}
