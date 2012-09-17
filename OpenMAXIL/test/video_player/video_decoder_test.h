/**
 *  Copyright (c) 2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

 
/*! video_decoder_test.h
*   The video_decoder_test header file contains the declarations used by the sample IL Client
*   that uses an Video decoder component.
*/

#ifndef VIDEO_DECODER_TEST_h
#define VIDEO_DECODER_TEST_h

#include "Queue.h"
#include "OMX_Component.h"
#include "OMX_Core.h"
#include "OMX_ContentPipe.h"
#include "OMX_Audio.h"
#include "OMX_Implement.h"


#define VIDEOD_COMP_NAME_LENGTH     128
#define VIDEOD_INPUT_BUFFER_SIZE    750
#define VIDEOD_FRAME_SIZE           576   /* No of stereo samples outputed per
                                            call to the decode frame */

#define VIDEOD_READ_LENGTH          1500
#define MAX_OMXQUEUE_SIZE           128

#define MPEG4                       1
#define H264                        2


typedef struct {
    OMX_COMMANDTYPE Cmd;
    OMX_U32 nParam1;
    OMX_PTR pCmdData;
}CMDINFO;

typedef struct
{
    OMX_HANDLETYPE EventQueue;
    OMX_HANDLETYPE EmptyBufQueue;
    OMX_HANDLETYPE FilledBufQueue;
    OMX_STATETYPE eState;
    OMX_ERRORTYPE eError;
}AppHandler;


typedef struct VDT_CLIENT_PARAMS
{
    OMX_HANDLETYPE          hDecoder;
    OMX_HANDLETYPE          hRender;
    OMX_HANDLETYPE          hParser;

    CP_PIPETYPE             *pVideoDirPipe;
    OMX_HANDLETYPE          hContentPipe;
    OMX_STRING              role;

    OMX_VIDEO_CODINGTYPE    VideoFmt;

    OMX_U8                  *pVESBuf[MAX_PORT_BUFFER]; 
    OMX_U32                 nVESBufSize;
    OMX_U32                 nVESBufCnt;

    OMX_U8                  *pYUVBuf[MAX_PORT_BUFFER]; 
    OMX_U32                 nYUVBufSize;
    OMX_U32                 nYUVBufCnt;

    OMX_U8                  * pInfilename;

    OMX_BUFFERHEADERTYPE    *pParserOutBufHdr[MAX_PORT_BUFFER];
    OMX_BUFFERHEADERTYPE    *pDecoderInBufHdr[MAX_PORT_BUFFER];

    OMX_BUFFERHEADERTYPE    *pDecoderOutBufHdr[MAX_PORT_BUFFER];
    OMX_BUFFERHEADERTYPE    *pRenderInBufHdr[MAX_PORT_BUFFER];

    AppHandler              sAppHandler;

    OMX_BOOL                bEOS;

    OMX_BOOL                decOutPortDisabled;

    OMX_BOOL                bsetting_changed;

    OMX_U8                  nParserAudioPortIdx;
    OMX_U8                  nParserVideoPortIdx;
    OMX_U32                 nFrames;

}VDT_CLIENT_PARAMS;

typedef enum
{
    DataBuffer,
    Eventype,
} Message_type;

typedef enum
{
    EmptyBuffer,
    FilledBuffer,
}Buffer_type;


typedef enum
{
    Statetransition,     /* 0 */
    EmptyBufferDone,
    FillBufferDone,
    EndOfStream,
    PortSettingsChanged,
    PortDisabled,        /* 5 */
    PortEnabled,
    PortFlushed,
    BufferMarked,
    MarkEventGenerated,
    PortFormatDetected,  /* 10 */
    Error
}Event_type;

typedef struct
{
    Buffer_type type;
    OMX_BUFFERHEADERTYPE* pBuffer;
}Data_type;


/**
    Message is used for passing information through queue.
    MsgType - identifies the type of message. The value
    in the union depends on MsgType.
*/
typedef struct
{
    Message_type    MsgType;
    Data_type       data;
    Event_type      event;
    OMX_U32         nData1;
    OMX_U32         nData2;
    OMX_HANDLETYPE  hComponent;
}Message;


#endif
/* File EOF */







