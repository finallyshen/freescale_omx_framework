#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libavutil/opt.h"
#include "avformat.h"
#include "avcodec.h"


extern AVInputFormat ff_rtsp_demuxer;
extern AVInputFormat ff_applehttp_demuxer;
extern URLProtocol ff_http_protocol;
extern URLProtocol ff_mmsh_protocol;


int test_http(char *url)
{
    int err = 0;
    URLContext *uc = NULL;

    uc = (URLContext*)malloc(sizeof(URLContext) + strlen(url) + 1);
    if(uc == NULL) {
        printf("Allocate for http context failed.\n");
        return 0;
    }

    uc->filename = (char *) &uc[1];
    strcpy(uc->filename, url);
    uc->prot = &ff_http_protocol;
    uc->flags = AVIO_RDONLY;
    uc->is_streamed = 0; /* default = not streamed */
    uc->max_packet_size = 0; /* default: stream file */
    if (uc->prot->priv_data_size) {
        uc->priv_data = malloc(uc->prot->priv_data_size);
        if (uc->prot->priv_data_class) {
            *(const AVClass**)uc->priv_data = uc->prot->priv_data_class;
            av_opt_set_defaults(uc->priv_data);
        }
    }

    err = uc->prot->url_open(uc, uc->filename, uc->flags);
    if (err)
        return err;
    uc->is_connected = 1;

    FILE *fp = fopen("httpstream.dat", "wb");
    if(fp == NULL)
        return 0;

    uint8_t buffer[2048];
    int len = 0;
    while(1) {
        len = uc->prot->url_read(uc, buffer, 2048);
        if (len == 0)
            break;
        fwrite(buffer, 1, len, fp);
        printf("recev %d byte from httpserver.\n", len);
    }

    fclose(fp);
    uc->prot->url_close(uc);
    free(uc);

    return 1;
}

int dump_packet(AVPacket *pkt)
{
    if(pkt->stream_index == 0) {
        FILE *fp = fopen("stream0.dat", "ab");
        if(fp) {
            fwrite(pkt->data, 1, pkt->size, fp);
            fclose(fp);
        }
    }

    if(pkt->stream_index == 1) {
        FILE *fp = fopen("stream1.dat", "ab");
        if(fp) {
            fwrite(pkt->data, 1, pkt->size, fp);
            fclose(fp);
        }
    }

    return 1;
}

int test_httplive(char *url)
{
    int err;
    AVFormatContext *ic = NULL;
    AVInputFormat *fmt = &ff_applehttp_demuxer;
    AVFormatParameters params, *ap = &params;
    AVPacket cur_pkt, *pkt = &cur_pkt;

    memset(ap, 0, sizeof(*ap));

    ic = avformat_alloc_context();
    if(ic == NULL)
        return 0;

    ic->iformat = fmt;
    strcpy(ic->filename, url);

    if (fmt->priv_data_size > 0) {
        ic->priv_data = av_mallocz(fmt->priv_data_size);
        if (!ic->priv_data) {
            return 0;
        }
    } else {
        ic->priv_data = NULL;
    }

    err = ic->iformat->read_header(ic, ap);
    if (err < 0)
        return 0;

    while(1) {
        av_init_packet(pkt);
        //err = ic->iformat->read_packet(ic, pkt);
        err = av_read_packet(ic, pkt);
        if (err < 0)
            return 0;
        printf("stream: %d, len: %d, pts: %lld, flags: %x\n", pkt->stream_index, pkt->size, pkt->pts, pkt->flags);
        dump_packet(pkt);
        if (pkt->destruct) pkt->destruct(pkt);
        pkt->data = NULL; pkt->size = 0;
    }

    return 1;
}

int test_mms(char *url)
{
    int err = 0;
    URLContext *uc = NULL;

    uc = (URLContext*)malloc(sizeof(URLContext) + strlen(url) + 1);
    if(uc == NULL) {
        printf("Allocate for http context failed.\n");
        return 0;
    }

    uc->filename = (char *) &uc[1];
    strcpy(uc->filename, url);
    uc->prot = &ff_mmsh_protocol;
    uc->flags = AVIO_RDONLY;
    uc->is_streamed = 0; /* default = not streamed */
    uc->max_packet_size = 0; /* default: stream file */
    if (uc->prot->priv_data_size) {
        uc->priv_data = malloc(uc->prot->priv_data_size);
        if (uc->prot->priv_data_class) {
            *(const AVClass**)uc->priv_data = uc->prot->priv_data_class;
            av_opt_set_defaults(uc->priv_data);
        }
    }

    err = uc->prot->url_open(uc, uc->filename, uc->flags);
    if (err)
        return err;
    uc->is_connected = 1;

    FILE *fp = fopen("mmsh.dat", "wb");
    if(fp == NULL)
        return 0;

    uint8_t buffer[2048];
    int len = 0;
    while(1) {
        len = uc->prot->url_read(uc, buffer, 2048);
        if (len == 0)
            break;
        fwrite(buffer, 1, len, fp);
        printf("recev %d byte from mmsh server.\n", len);
    }

    fclose(fp);
    uc->prot->url_close(uc);
    free(uc);

    return 1;
}

int test_rtsp(char *url)
{
    int err;
    AVFormatContext *ic = NULL;
    AVInputFormat *fmt = &ff_rtsp_demuxer;
    AVFormatParameters params, *ap = &params;
    AVPacket cur_pkt, *pkt = &cur_pkt;

    memset(ap, 0, sizeof(*ap));

    ic = avformat_alloc_context();
    if(ic == NULL)
        return 0;

    ic->iformat = fmt;
    strcpy(ic->filename, url);

    if (fmt->priv_data_size > 0) {
        ic->priv_data = av_mallocz(fmt->priv_data_size);
        if (!ic->priv_data) {
            return 0;
        }
    } else {
        ic->priv_data = NULL;
    }

    err = ic->iformat->read_header(ic, ap);
    if (err < 0)
        return 0;

    while(1) {
        av_init_packet(pkt);
        //err = ic->iformat->read_packet(ic, pkt);
        err = av_read_packet(ic, pkt);
        if (err < 0)
            return 0;
        printf("stream: %d, len: %d, pts: %lld\n", pkt->stream_index, pkt->size, pkt->pts);
        dump_packet(pkt);
        if (pkt->destruct) pkt->destruct(pkt);
        pkt->data = NULL; pkt->size = 0;
    }

    return 1;
}


int main(int argc, void *argv[])
{
    char *url = (char*)argv[1];

    if(url == NULL)
        return 0;

    av_register_all();

    printf("Loading %s\n", url);
    if(!strncmp(url, "http://", 7))
        test_http(url);

    if(!strncmp(url, "rtsp://", 7))
        test_rtsp(url);

    if(!strncmp(url, "mms://", 6))
        test_mms(url);

    return 1;
}
