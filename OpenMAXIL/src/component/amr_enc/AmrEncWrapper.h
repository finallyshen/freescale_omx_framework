/**
 *  Copyright (c) 2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */


#ifndef AMR_ENC_WRAPPER
#define AMR_ENC_WRAPPER

#include "wbamr_enc_interface.h"
#include "nb_amr_enc_api.h"

class AmrEncWrapper{

public:
    AmrEncWrapper(){
        };
    virtual ~AmrEncWrapper(){
        };
    
    virtual OMX_ERRORTYPE InstanceInit() = 0;
    virtual OMX_ERRORTYPE CodecInit(OMX_AUDIO_PARAM_AMRTYPE *pAmrType) = 0;
    virtual OMX_ERRORTYPE InstanceDeInit() = 0;
    virtual OMX_U32 getInputBufferPushSize() = 0;
    virtual OMX_ERRORTYPE encodeFrame(
                    OMX_U8 *pOutBuffer, 
                    OMX_U32 *pOutputDataSize,
                    OMX_U8 *pInputBuffer, 
                    OMX_U32 *pInputDataSize) = 0;
    virtual OMX_TICKS getTsPerFrame(OMX_U32 sampleRate) = 0;
};

#define WB_INPUT_BUF_PUSH_SIZE (WBAMR_L_FRAME * sizeof(WBAMR_S16))
    
class WbAmrEncWrapper: public AmrEncWrapper{
private:
    WBAMRE_Encoder_Config *pAmrEncConfig;
	WBAMR_U16 nOutputSize;
	WBAMR_S16 mode_in;

public:
    WbAmrEncWrapper(){
        pAmrEncConfig = NULL;
        };
    ~WbAmrEncWrapper(){
        };
    
    OMX_ERRORTYPE InstanceInit();
    OMX_ERRORTYPE CodecInit(OMX_AUDIO_PARAM_AMRTYPE *pAmrType);
    OMX_ERRORTYPE InstanceDeInit();
    OMX_U32 getInputBufferPushSize();
    OMX_ERRORTYPE encodeFrame(
                    OMX_U8 *pOutBuffer, 
                    OMX_U32 *pOutputDataSize,
                    OMX_U8 *pInputBuffer, 
                    OMX_U32 *pInputDataSize);
    OMX_TICKS getTsPerFrame(OMX_U32 sampleRate);

};

#define ENCORD_FRAME (5)
#define NB_INPUT_BUF_PUSH_SIZE ((ENCORD_FRAME * L_FRAME) * sizeof (NBAMR_S16))


class NbAmrEncWrapper: public AmrEncWrapper{
private:
	sAMREEncoderConfigType *pAmrEncConfig;

public:
    NbAmrEncWrapper(){
        pAmrEncConfig = NULL;
        };
    ~NbAmrEncWrapper(){
        };
    OMX_ERRORTYPE InstanceInit();
    OMX_ERRORTYPE CodecInit(OMX_AUDIO_PARAM_AMRTYPE *pAmrType);
    OMX_ERRORTYPE InstanceDeInit();
    OMX_U32 getInputBufferPushSize();
    OMX_ERRORTYPE encodeFrame(
                    OMX_U8 *pOutBuffer, 
                    OMX_U32 *pOutputDataSize,
                    OMX_U8 *pInputBuffer, 
                    OMX_U32 *pInputDataSize);
    OMX_TICKS getTsPerFrame(OMX_U32 sampleRate);

};

#endif

