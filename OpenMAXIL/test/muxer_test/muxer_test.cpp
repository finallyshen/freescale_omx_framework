/**
 *  Copyright (c) 2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "GMComponent.h"

typedef struct {
    char *in_file;
    char *out_file;
    char *in_role;
    GMComponent Parser;
    GMComponent Muxer;
    OMX_BOOL bHasAudio;
    OMX_BOOL bHasVideo;
    OMX_BOOL bAudioEos;
    OMX_BOOL bVideoEos;
    OMX_BOOL bError;
}TEST;

OMX_ERRORTYPE EventCb(
        GMComponent *pComponent,
        OMX_PTR pAppData,
        OMX_EVENTTYPE eEvent,
        OMX_U32 nData1,
        OMX_U32 nData2,
        OMX_PTR pEventData)
{
    TEST *hTest = (TEST*)pAppData;

    switch(eEvent) {
        case OMX_EventBufferFlag:
            {
                if(nData2 == OMX_BUFFERFLAG_EOS) {
                    if(pComponent == &hTest->Muxer) { 
                        if(hTest->bHasAudio == OMX_TRUE && nData1 == hTest->Muxer.PortPara[AUDIO_PORT_PARA].nStartPortNumber) {
                            printf("Muxer get Audio EOS event.\n");
                            hTest->bAudioEos = OMX_TRUE;
                        }

                        if(hTest->bHasVideo == OMX_TRUE && nData1 == hTest->Muxer.PortPara[VIDEO_PORT_PARA].nStartPortNumber) {
                            printf("Muxer get Video EOS event.\n");
                            hTest->bVideoEos = OMX_TRUE;
                        }
                    }
                }
            }
            break;
        case OMX_EventError:
            hTest->bError = OMX_TRUE;
            break;
        default:
            break;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE LoadParser(TEST *hTest)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = hTest->Parser.Load(hTest->in_role);
    if(ret != OMX_ErrorNone)
        return ret;

    hTest->Parser.RegistSysCallback(EventCb, (OMX_PTR)hTest);

    OMX_U8 buffer[128];
    OMX_PARAM_CONTENTURITYPE *pContent = (OMX_PARAM_CONTENTURITYPE*)buffer;
    fsl_osal_strcpy((fsl_osal_char*)(&pContent->contentURI), hTest->in_file);
    ret = OMX_SetParameter(hTest->Parser.hComponent, OMX_IndexParamContentURI, pContent);
    if(ret != OMX_ErrorNone)
        return ret;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE LoadMuxer(TEST *hTest)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = hTest->Muxer.Load("muxer.mp4");
    if(ret != OMX_ErrorNone)
        return ret;

    hTest->Muxer.RegistSysCallback(EventCb, (OMX_PTR)hTest);

    OMX_U8 buffer[128];
    OMX_PARAM_CONTENTURITYPE *pContent = (OMX_PARAM_CONTENTURITYPE*)buffer;
    fsl_osal_strcpy((fsl_osal_char*)(&pContent->contentURI), hTest->out_file);
    ret = OMX_SetParameter(hTest->Muxer.hComponent, OMX_IndexParamContentURI, pContent);
    if(ret != OMX_ErrorNone)
        return ret;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE StartParser(TEST *hTest)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    OMX_AUDIO_PARAM_PORTFORMATTYPE sAudioPortFmt;
    OMX_INIT_STRUCT(&sAudioPortFmt, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    sAudioPortFmt.nPortIndex = hTest->Parser.PortPara[AUDIO_PORT_PARA].nStartPortNumber;
    sAudioPortFmt.eEncoding = OMX_AUDIO_CodingAutoDetect;
    OMX_SetParameter(hTest->Parser.hComponent, OMX_IndexParamAudioPortFormat, &sAudioPortFmt);

    OMX_VIDEO_PARAM_PORTFORMATTYPE sVideoPortFmt;
    OMX_INIT_STRUCT(&sVideoPortFmt, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    sVideoPortFmt.nPortIndex = hTest->Parser.PortPara[VIDEO_PORT_PARA].nStartPortNumber;
    sVideoPortFmt.eCompressionFormat = OMX_VIDEO_CodingAutoDetect;
    OMX_SetParameter(hTest->Parser.hComponent, OMX_IndexParamVideoPortFormat, &sVideoPortFmt);

    ret = hTest->Parser.StateTransUpWard(OMX_StateIdle);
    if(ret != OMX_ErrorNone)
        return ret;

    ret = hTest->Parser.StateTransUpWard(OMX_StateExecuting);
    if(ret != OMX_ErrorNone)
        return ret;

    while(1) {
        OMX_GetParameter(hTest->Parser.hComponent, OMX_IndexParamAudioPortFormat, &sAudioPortFmt);
        OMX_GetParameter(hTest->Parser.hComponent, OMX_IndexParamVideoPortFormat, &sVideoPortFmt);
        if(sAudioPortFmt.eEncoding != OMX_AUDIO_CodingAutoDetect
                && sVideoPortFmt.eCompressionFormat != OMX_VIDEO_CodingAutoDetect)
            break;

        if(hTest->bError == OMX_TRUE)
            return OMX_ErrorFormatNotDetected;

        fsl_osal_sleep(20000);
    }

    OMX_PARAM_U32TYPE u32Type;
    OMX_INIT_STRUCT(&u32Type, OMX_PARAM_U32TYPE);

    if(sAudioPortFmt.eEncoding == OMX_AUDIO_CodingMP3
            || sAudioPortFmt.eEncoding == OMX_AUDIO_CodingAMR) {
        u32Type.nPortIndex = hTest->Parser.PortPara[AUDIO_PORT_PARA].nStartPortNumber;
        OMX_GetParameter(hTest->Parser.hComponent, OMX_IndexParamNumAvailableStreams, &u32Type);
        if(u32Type.nU32 > 0) {
            u32Type.nU32 = 0;
            OMX_SetParameter(hTest->Parser.hComponent, OMX_IndexParamActiveStream, &u32Type);
            hTest->bHasAudio = OMX_TRUE;
        }
        else
            hTest->Parser.PortDisable(hTest->Parser.PortPara[AUDIO_PORT_PARA].nStartPortNumber);
    }
    else
        hTest->Parser.PortDisable(hTest->Parser.PortPara[AUDIO_PORT_PARA].nStartPortNumber);


    if(sVideoPortFmt.eCompressionFormat == OMX_VIDEO_CodingAVC
            || sVideoPortFmt.eCompressionFormat == OMX_VIDEO_CodingMPEG4) {
        u32Type.nPortIndex = hTest->Parser.PortPara[VIDEO_PORT_PARA].nStartPortNumber;
        OMX_GetParameter(hTest->Parser.hComponent, OMX_IndexParamNumAvailableStreams, &u32Type);
        if(u32Type.nU32> 0) {
            u32Type.nU32 = 0;
            OMX_SetParameter(hTest->Parser.hComponent, OMX_IndexParamActiveStream, &u32Type);
            hTest->bHasVideo = OMX_TRUE;
        }
        else
            hTest->Parser.PortDisable(hTest->Parser.PortPara[VIDEO_PORT_PARA].nStartPortNumber);
    }
    else
        hTest->Parser.PortDisable(hTest->Parser.PortPara[VIDEO_PORT_PARA].nStartPortNumber);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ConnectComponent(
        GMComponent *pOutComp,
        OMX_U32 nOutPortIndex,
        GMComponent *pInComp,
        OMX_U32 nInPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = pOutComp->Link(nOutPortIndex,pInComp,nInPortIndex);
    if(ret != OMX_ErrorNone) {
        printf("Connect component[%s:%d] with component[%s:%d] failed.\n",
                pOutComp->name, nOutPortIndex, pInComp->name, nInPortIndex);
        return ret;
    }

    ret = pInComp->Link(nInPortIndex, pOutComp, nOutPortIndex);
    if(ret != OMX_ErrorNone) {
        printf("Connect component[%s:%d] with component[%s:%d] failed.\n",
                pInComp->name, nInPortIndex, pOutComp->name, nOutPortIndex);
        pOutComp->Link(nOutPortIndex, NULL, 0);
        return ret;
    }

    LOG_DEBUG("Connect component[%s:%d] with component[%s:%d] success.\n",
            pOutComp->name, nOutPortIndex, pInComp->name, nInPortIndex);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Connect(TEST *hTest)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GMComponent *Parser = &hTest->Parser;
    GMComponent *Muxer = &hTest->Muxer;

    if(hTest->bHasAudio) {
        OMX_AUDIO_PARAM_PORTFORMATTYPE sAudioFmt;
        OMX_INIT_STRUCT(&sAudioFmt, OMX_AUDIO_PARAM_PORTFORMATTYPE);
        sAudioFmt.nPortIndex = Parser->PortPara[AUDIO_PORT_PARA].nStartPortNumber;
        OMX_GetParameter(Parser->hComponent, OMX_IndexParamAudioPortFormat, &sAudioFmt);
        sAudioFmt.nPortIndex = Muxer->PortPara[AUDIO_PORT_PARA].nStartPortNumber;
        OMX_SetParameter(Muxer->hComponent, OMX_IndexParamAudioPortFormat, &sAudioFmt);

        Parser->PortDisable(Parser->PortPara[AUDIO_PORT_PARA].nStartPortNumber);
        ret = ConnectComponent(Parser, Parser->PortPara[AUDIO_PORT_PARA].nStartPortNumber, 
                Muxer, Muxer->PortPara[AUDIO_PORT_PARA].nStartPortNumber);
        if(ret != OMX_ErrorNone)
            return ret;
        Parser->PortEnable(Parser->PortPara[AUDIO_PORT_PARA].nStartPortNumber);
        Muxer->PortEnable(Muxer->PortPara[AUDIO_PORT_PARA].nStartPortNumber);
    }

    if(hTest->bHasVideo) {
        OMX_VIDEO_PARAM_PORTFORMATTYPE sVideoFmt;
        OMX_INIT_STRUCT(&sVideoFmt, OMX_VIDEO_PARAM_PORTFORMATTYPE);
        sVideoFmt.nPortIndex = Parser->PortPara[VIDEO_PORT_PARA].nStartPortNumber;
        OMX_GetParameter(Parser->hComponent, OMX_IndexParamVideoPortFormat, &sVideoFmt);
        sVideoFmt.nPortIndex = Muxer->PortPara[VIDEO_PORT_PARA].nStartPortNumber;
        OMX_SetParameter(Muxer->hComponent, OMX_IndexParamVideoPortFormat, &sVideoFmt);

        Parser->PortDisable(Parser->PortPara[VIDEO_PORT_PARA].nStartPortNumber);
        ret = ConnectComponent(Parser, Parser->PortPara[VIDEO_PORT_PARA].nStartPortNumber, 
                Muxer, Muxer->PortPara[VIDEO_PORT_PARA].nStartPortNumber);
        if(ret != OMX_ErrorNone)
            return ret;
        Parser->PortEnable(Parser->PortPara[VIDEO_PORT_PARA].nStartPortNumber);
        Muxer->PortEnable(Muxer->PortPara[VIDEO_PORT_PARA].nStartPortNumber);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE StartProcess(TEST *hTest)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GMComponent *Parser = &hTest->Parser;
    GMComponent *Muxer = &hTest->Muxer;

    ret = Muxer->StateTransUpWard(OMX_StateIdle);
    if(ret != OMX_ErrorNone)
        return ret;
    ret = Muxer->StateTransUpWard(OMX_StateExecuting);
    if(ret != OMX_ErrorNone)
        return ret;
    ret = Parser->StartProcess();
    if(ret != OMX_ErrorNone)
        return ret;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE WaitingEos(TEST *hTest)
{
    while(1) {
        printf("audio: %d, video: %d, audio eos: %d, video eos: %d\n",
                hTest->bHasAudio, hTest->bHasVideo, hTest->bAudioEos, hTest->bVideoEos);
        if((hTest->bHasAudio && hTest->bAudioEos || !hTest->bHasAudio)
                && (hTest->bHasVideo && hTest->bVideoEos || !hTest->bHasVideo))
            break;
        fsl_osal_sleep(500000);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE UnLoadParser(TEST *hTest)
{
    hTest->Parser.StateTransDownWard(OMX_StateIdle);
    hTest->Parser.StateTransDownWard(OMX_StateLoaded);
    hTest->Parser.UnLoad();
    return OMX_ErrorNone;
}

OMX_ERRORTYPE UnLoadMuxer(TEST *hTest)
{
    hTest->Muxer.StateTransDownWard(OMX_StateIdle);
    hTest->Muxer.StateTransDownWard(OMX_StateLoaded);
    hTest->Muxer.UnLoad();
    return OMX_ErrorNone;
}

int main(int argc, char *argv[])
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    if(argc < 4) {
        printf("Unit test of mp4 muxer component.\n");
        printf("This test convert in_file to mp4 format.\n");
        printf("Usage: ./bin <in_file> <in_role> <out_file>\n");
        return 0;
    }

    OMX_Init();

    TEST hTest;
    fsl_osal_memset(&hTest, 0, sizeof(TEST));
    hTest.in_file = argv[1];
    hTest.in_role = argv[2];
    hTest.out_file = argv[3];

    ret = LoadParser(&hTest);
    if(ret != OMX_ErrorNone) {
        printf("Load parser failed.\n");
        return 0;
    }

    ret = LoadMuxer(&hTest);
    if(ret != OMX_ErrorNone) {
        printf("Load mp4 muxer failed.\n");
        return 0;
    }

    ret = StartParser(&hTest);
    if(ret != OMX_ErrorNone) {
        printf("Start parser failed.\n");
        return 0;
    }

    ret = Connect(&hTest);
    if(ret != OMX_ErrorNone) {
        printf("connect failed.\n");
        return 0;
    }

    ret = StartProcess(&hTest);
    if(ret != OMX_ErrorNone) {
        printf("start Muxer failed.\n");
        return 0;
    }

    WaitingEos(&hTest);

    UnLoadParser(&hTest);
    UnLoadMuxer(&hTest);

    OMX_Deinit();

    return 1;
}

/* File EOF */
