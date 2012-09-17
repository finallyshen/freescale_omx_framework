/**
 *  Copyright (c) 2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */


#ifndef AMR_DEC_WRAPPER
#define AMR_DEC_WRAPPER

#include "wbamr_dec_interface.h"
#include "nb_amr_dec_api.h"

class AmrDecWrapper{

public:
    AmrDecWrapper(){
        };
    virtual ~AmrDecWrapper(){
        };
    
    virtual OMX_ERRORTYPE InstanceInit() = 0;
    virtual OMX_ERRORTYPE CodecInit(OMX_AUDIO_PARAM_AMRTYPE *pAmrType) = 0;
    virtual OMX_ERRORTYPE InstanceDeInit() = 0;
    virtual OMX_U32 getInputBufferPushSize() = 0;
    virtual OMX_ERRORTYPE decodeFrame(
                    OMX_U8 *pInputBuffer, 
                    OMX_U8 *pOutBuffer, 
                    OMX_AUDIO_PARAM_AMRTYPE * pAmrType,
                    OMX_U32 inputDataSize,
                    OMX_U32 *pConsumedSize) = 0;
    virtual OMX_U32 getFrameOutputSize() = 0;
    virtual OMX_U32 getBitRate(OMX_U32 InputframeSize) = 0;
    virtual OMX_TICKS getTsPerFrame(OMX_U32 sampleRate) = 0;
    

protected:
    OMX_U8 *pInBuf;
        
    
    
};

#define WB_INPUT_BUF_PUSH_SIZE WBAMR_SERIAL_FRAMESIZE
    
class WbAmrDecWrapper: public AmrDecWrapper{
private:
    /* size of packed frame for each mode */
    static OMX_U32 const MMSPackedCodedSize[16];
    static OMX_U32 const IF1PackedCodedSize[16];
    static OMX_U32 const IF2PackedCodedSize[16];

    WBAMRD_Decoder_Config *pWbAmrDecConfig;
    
public:
    WbAmrDecWrapper(){
        pWbAmrDecConfig = NULL;
        };
    ~WbAmrDecWrapper(){
        };
    
    OMX_ERRORTYPE InstanceInit();
    OMX_ERRORTYPE CodecInit(OMX_AUDIO_PARAM_AMRTYPE *pAmrType);
    OMX_ERRORTYPE InstanceDeInit();
    OMX_U32 getInputBufferPushSize();
    OMX_ERRORTYPE decodeFrame(
                    OMX_U8 *pInputBuffer, 
                    OMX_U8 *pOutBuffer, 
                    OMX_AUDIO_PARAM_AMRTYPE * pAmrType,
                    OMX_U32 inputDataSize,
                    OMX_U32 *pConsumedSize);
    OMX_U32 getFrameOutputSize();
    OMX_U32 getBitRate(OMX_U32 InputframeSize);
    OMX_TICKS getTsPerFrame(OMX_U32 sampleRate);

};

#define FRAMESPERLOOP 5
#define NB_INPUT_BUF_PUSH_SIZE (SERIAL_FRAMESIZE*2*FRAMESPERLOOP)


class NbAmrDecWrapper: public AmrDecWrapper{
private:
    /* size of packed frame for each mode */
    static OMX_U32 const MMSPackedCodedSize[16];
    static OMX_U32 const IF1PackedCodedSize[16];
    static OMX_U32 const IF2PackedCodedSize[16];

    sAMRDDecoderConfigType *pNbAmrDecConfig;
    
public:
    NbAmrDecWrapper(){
        pNbAmrDecConfig = NULL;
        };
    ~NbAmrDecWrapper(){
        };
    OMX_ERRORTYPE InstanceInit();
    OMX_ERRORTYPE CodecInit(OMX_AUDIO_PARAM_AMRTYPE *pAmrType);
    OMX_ERRORTYPE InstanceDeInit();
    OMX_U32 getInputBufferPushSize();
    OMX_ERRORTYPE decodeFrame(
                    OMX_U8 *pInputBuffer, 
                    OMX_U8 *pOutBuffer, 
                    OMX_AUDIO_PARAM_AMRTYPE * pAmrType,
                    OMX_U32 inputDataSize,
                    OMX_U32 *pConsumedSize);
    OMX_U32 getFrameOutputSize();
    OMX_U32 getBitRate(OMX_U32 InputframeSize);
    OMX_TICKS getTsPerFrame(OMX_U32 sampleRate);

};

#endif

