/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file OMX_Implement_h
 *  @brief Contains the common definitions for implementing OpenMAX IL
 *  @ingroup OMX_Implement
 */


#ifndef OMX_Implement_h
#define OMX_Implement_h

#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_Audio.h"
#include "OMX_Video.h"


/*copied from DivxDrmExtn.h*/
#ifndef MFW_AVI_SUPPORT_DIVX_DRM
#define MFW_AVI_SUPPORT_DIVX_DRM
#endif

/**< fsl defined buffer flags */
#define OMX_BUFFERFLAG_STARTTRICK 0x10000000
#define OMX_BUFFERFLAG_MAX_FILESIZE 0x20000000 
#define OMX_BUFFERFLAG_MAX_DURATION 0x40000000 

/**< fsl defined error type */
#define FSL_ERRORTYPE(n) ((OMX_ERRORTYPE)(OMX_ErrorVendorStartUnused + n))
#define OMX_ErrorNotComplete FSL_ERRORTYPE(1)

/**< fsl defined Event type */
#define FSL_EVENTTYPE(n) ((OMX_EVENTTYPE)(OMX_EventVendorStartUnused + n))
#define OMX_EventBufferingData FSL_EVENTTYPE(1)
#define OMX_EventBufferingDone FSL_EVENTTYPE(2)
#define OMX_EventBufferingUpdate FSL_EVENTTYPE(3)
#define OMX_EventStreamSkipped FSL_EVENTTYPE(4)

/**< fsl defined media type */
#define   OMX_VIDEO_CodingDIVX    (OMX_VIDEO_CodingVendorStartUnused + 1)  /**< Divx */
#define   OMX_VIDEO_CodingXVID    (OMX_VIDEO_CodingVendorStartUnused + 2)  /**< Xvid */
#define   OMX_VIDEO_CodingDIV3    (OMX_VIDEO_CodingVendorStartUnused + 3)  /**< Divx3 */
#define   OMX_VIDEO_CodingDIV4    (OMX_VIDEO_CodingVendorStartUnused + 4)  /**< Divx4 */
#define   OMX_VIDEO_WMVFormat9a   (OMX_VIDEO_WMFFormatVendorStartUnused + 1) /**< WVC1 */
#define   OMX_VIDEO_CodingWMV9    (OMX_VIDEO_WMFFormatVendorStartUnused + 21) /**< WVC1 */
#define   OMX_VIDEO_WMVFormatWVC1 (OMX_VIDEO_WMFFormatVendorStartUnused + 22) /**< WVC1 */
#define   OMX_VIDEO_SORENSON263   (OMX_VIDEO_WMFFormatVendorStartUnused + 23) /**< SORENSON */
#define   OMX_VIDEO_VP8   (OMX_VIDEO_WMFFormatVendorStartUnused + 24) /**< VP8 */

#define   OMX_AUDIO_CodingFLAC    (OMX_AUDIO_CodingVendorStartUnused + 1)
#define   OMX_AUDIO_CodingAC3     (OMX_AUDIO_CodingVendorStartUnused + 2)

/**< fsl defined index */
#define   FSL_INDEXTYPE(n) ((OMX_INDEXTYPE)(OMX_IndexVendorStartUnused + n))
#define   OMX_IndexParamMediaSeekable FSL_INDEXTYPE(1)
#define   OMX_IndexParamMediaDuration FSL_INDEXTYPE(2)
#define   OMX_IndexParamTrackDuration FSL_INDEXTYPE(3)          /* OMX_TRACK_DURATION */
#define   OMX_IndexConfigParserSendAudioFirst FSL_INDEXTYPE(4)  /* OMX_CONFIG_SENDAUDIOFIRST */
	
#define   OMX_IndexConfigCaptureFrame FSL_INDEXTYPE(5)          /* OMX_CONFIG_CAPTUREFRAME */
#define   OMX_IndexOutputMode FSL_INDEXTYPE(6)                  /* OMX_CONFIG_OUTPUTMODE */
#define   OMX_IndexSysSleep FSL_INDEXTYPE(7)                   /* OMX_CONFIG_SYSSLEEP */

#define   INDEX_CONFIG_DIVX_DRM_CALLBACK FSL_INDEXTYPE(8)
#define   INDEX_VC1_EXTRA_DATA 	FSL_INDEXTYPE(9)	
#define   OMX_IndexParamAudioAc3 FSL_INDEXTYPE(10)
#define   OMX_IndexConfigAudioPostProcess FSL_INDEXTYPE(11)
#define   OMX_IndexParamAudioSink FSL_INDEXTYPE(12)
#define   OMX_IndexConfigClock FSL_INDEXTYPE(13)                 /* OMX_CONFIG_CLOCK */
#define   OMX_IndexConfigVideoOutBufPhyAddr FSL_INDEXTYPE(14)
#define   OMX_IndexParamAudioFlac FSL_INDEXTYPE(15)
#define   OMX_IndexParamMemOperator FSL_INDEXTYPE(16)
#define   OMX_IndexConfigAbortBuffering FSL_INDEXTYPE(17)  //OMX_CONFIG_ABORTBUFFERING
#define   OMX_IndexParamVideoCamera FSL_INDEXTYPE(18)
#define   OMX_IndexParamVideoCameraId FSL_INDEXTYPE(19)
#define   OMX_IndexParamVideoSurface FSL_INDEXTYPE(20)
#define   OMX_IndexParamMaxFileDuration FSL_INDEXTYPE(21)
#define   OMX_IndexParamMaxFileSize FSL_INDEXTYPE(22)
#define   OMX_IndexParamInterleaveUs FSL_INDEXTYPE(23)
#define   OMX_IndexParamIsGetMetadata FSL_INDEXTYPE(24)
#define   OMX_IndexParamSurface FSL_INDEXTYPE(25)
#define   OMX_IndexConfigEOS FSL_INDEXTYPE(26)
#define   OMX_IndexParamAudioSource FSL_INDEXTYPE(27)
#define   OMX_IndexConfigMaxAmplitude FSL_INDEXTYPE(28)
#define   OMX_IndexParamDecoderPlayMode FSL_INDEXTYPE(29)
#define   OMX_IndexParamTimeLapseUs FSL_INDEXTYPE(30)
#define   OMX_IndexParamAudioWmaExt FSL_INDEXTYPE(31)
#define   OMX_IndexParamVideoDecChromaAlign FSL_INDEXTYPE(32)
#define   OMX_IndexParamVideoCameraProxy FSL_INDEXTYPE(33)
#define   OMX_IndexParamLongitudex FSL_INDEXTYPE(34)
#define   OMX_IndexParamLatitudex FSL_INDEXTYPE(35)
#define   OMX_IndexParamBufferUsage FSL_INDEXTYPE(36)  //OMX_BUFFER_USAGE

/**< fsl defined macro utils */
#define MAX_NAME_LEN 128
#define MAX_PORT_NUM 8
#define MAX_PORT_BUFFER 32

#define CONTENTPIPE_NAME_LEN 128
#define COMPONENT_NAME_LEN 128
#define FUNCTION_NAME_LEN 128
#define ROLE_NAME_LEN 128
#define MAX_COMPONENTNUM_WITH_SAME_ROLE 8
#define CORE_NAME_LEN 128
#define MAX_VALUE_S64 (OMX_S64)((1ULL<<63)-1)

#define MAX(a,b) ((a)>=(b)?(a):(b))
#define MIN(a,b) ((a)<=(b)?(a):(b))
#define ABS(a) (((a)>0)?(a):-(a))
#ifndef Q16_SHIFT  
#define Q16_SHIFT (0x10000)
#endif
#define MAX_RATE (1.9)
#define MIN_RATE (0.1)
#define MAX_TRICK_MODE_RATE (16)

#define OMX_INIT_STRUCT(ptr, type) \
    do { \
        fsl_osal_memset((ptr), 0x0, sizeof(type)); \
        (ptr)->nSize = sizeof(type); \
        (ptr)->nVersion.s.nVersionMajor = 0x1; \
        (ptr)->nVersion.s.nVersionMinor = 0x1; \
        (ptr)->nVersion.s.nRevision = 0x2; \
        (ptr)->nVersion.s.nStep = 0x0; \
    } while(0);


/**< fsl defined types */
/* Display rotation type */
typedef enum {
    ROTATE_NONE = 0,
    ROTATE_VERT_FLIP,
    ROTATE_HORIZ_FLIP,
    ROTATE_180,
    ROTATE_90_RIGHT,
    ROTATE_90_RIGHT_VFLIP,
    ROTATE_90_RIGHT_HFLIP,
    ROTATE_90_LEFT
}ROTATION;

typedef enum
{
    DIVX_DRM_ERROR_NONE = 0,     /*!< No drm error happens */
    DIVX_DRM_OUTPUT_PROTECTION_COMMIT,
    DIVX_DRM_ERR_NEED_RENTAL_CONFIRMATION, /*!< No drm error happens but this is a rental file.
                                            Need user's confirmation to play it or not.
                                            GUI shall display how many views left for this file, 
                                            and let user decide whether to play the file. */
    DIVX_DRM_ERROR_NOT_AUTH_USER,        /*!< Not an authorized user. 
                                        The user shall activate the player if not, 
                                        or this encrypted file is for another user.*/
    DIVX_DRM_ERROR_RENTAL_EXPIRED,       /*!< This is a rental file and is expired. So the user can no longer play it. */
    DIVX_DRM_ERROR_OTHERS,       /*!< Other error, reserved.*/
} DIVX_DRM_ERR_CODE;


typedef struct {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TICKS sTrackDuration;
}OMX_TRACK_DURATION;

typedef struct {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bSendAudioFrameFirst;
}OMX_CONFIG_SENDAUDIOFIRST;

/** FLAC params */
typedef struct OMX_AUDIO_PARAM_FLACTYPE {
    OMX_U32 nSize;                 
    OMX_VERSIONTYPE nVersion;      
    OMX_U32 nPortIndex;            
    OMX_U32 nChannels;             
    OMX_U32 nBitRate;              
    OMX_U32 nSampleRate;           
    OMX_U32 nBitPerSample;       
    OMX_U32 nTotalSample;       
    OMX_U32 nBlockSize;       
} OMX_AUDIO_PARAM_FLACTYPE;

/** AC3 params */
typedef struct OMX_AUDIO_PARAM_AC3TYPE {
    OMX_U32 nSize;                 
    OMX_VERSIONTYPE nVersion;      
    OMX_U32 nPortIndex;            
    OMX_U32 nChannels;             
    OMX_U32 nBitRate;              
    OMX_U32 nSampleRate;           
    OMX_U32 nAudioBandWidth;       
    OMX_AUDIO_CHANNELMODETYPE eChannelMode;   
} OMX_AUDIO_PARAM_AC3TYPE;

typedef struct OMX_AUDIO_CONFIG_POSTPROCESSTYPE {
    OMX_U32 nSize;             /**< size of the structure in bytes */
    OMX_VERSIONTYPE nVersion;  /**< OMX specification version information */
    OMX_U32 nPortIndex;        /**< port that this structure applies to */
    OMX_BOOL bEnable;          /**< Enable/disable for post process */
    OMX_BU32 sDelay;           /**< average delay in milliseconds */
} OMX_AUDIO_CONFIG_POSTPROCESSTYPE;

typedef struct
{
    OMX_STRING  sInFilePath;
    OMX_S32     nEncoder;
    OMX_S32     nWidth;
    OMX_S32     nHeight;
    OMX_BOOL    bMPEG4FrameParsing;   //Needs to be enabled for frame decoding
    OMX_BOOL    bH264FrameParsing;    //Needs to be enabled for frame decoding
    OMX_BOOL    bWmvDecParsing;     //Enabled if source parses for frame size
    OMX_S32     RealVideo;
    OMX_BOOL    bAVCEnc;
}VIDSRC_CONFIG;

typedef struct {
    OMX_U32 nPortIdx;
    OMX_U32 size;
    OMX_U8  data[128];
}EXTRA_DATA;

typedef enum {
    CAP_NONE,
    CAP_SNAPSHOT,
    CAP_THUMBNAL
}CAPTURETYPE;

typedef struct {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    CAPTURETYPE eType;
    OMX_BOOL bDone;
    OMX_U8* pBuffer;
    OMX_U32 nFilledLen;
}OMX_CONFIG_CAPTUREFRAME;

typedef enum {
    MODE_NONE,
    MODE_PAL,
    MODE_NTSC,
    MODE_720P
}TV_MODE;

typedef enum {
    LAYER_NONE,
    LAYER0,
    LAYER1,
    LAYER2,
}FB_LAYER;

typedef enum {
    DEC_STREAM_MODE,
    DEC_FILE_MODE,
}OMX_DECODE_MODE;

typedef struct {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bTv;
    FB_LAYER eFbLayer;
    TV_MODE eTvMode; 
    OMX_CONFIG_RECTTYPE sRectIn;
    OMX_CONFIG_RECTTYPE sRectOut;
    ROTATION eRotation;
    OMX_BOOL bSetupDone;
}OMX_CONFIG_OUTPUTMODE;

typedef struct {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bSleep;
}OMX_CONFIG_SYSSLEEP;


typedef struct _DivxDrmInfo
{
    int drm_code;   /* divx drm error code */
    int  use_limit;     /* Maximum views of a rental file. 0 means this is NOT a rental file. */
    int  use_count;     /* Views already played by the user for a rental file. Only valid for a rental file.
                        No greater than the "use_limit".If its equal to the "use_limit", this rental file is expired. */
    int rental_confirmed;   /* Only valid for a rental file,
                            whether user confirms to play the rental file. */
    OMX_U8 cgmsaSignal;
    OMX_U8 acptbSignal;
    OMX_U8 digitalProtectionSignal;  								
	OMX_BOOL bDivxDrmPresent;
}DivxDrmInfo;

typedef void (*pf_cb_drm)(void* context, OMX_PTR drm_info);


typedef struct OMX_CONFIG_CAPABILITY {
    OMX_U32 nSize;            /**< size of the structure in bytes */
    OMX_VERSIONTYPE nVersion; /**< OMX specification version information */
    OMX_U32  nPortIndex;      /**< Port index*/
    OMX_BOOL bCapability;     /**< Flag to indicate if this media syncable */
} OMX_PARAM_CAPABILITY;

typedef struct {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_PTR hClock;
} OMX_CONFIG_CLOCK;

 /** 
 * Video out port buffer physical address
 *
 * STRUCT MEMBERS:
 *  nSize         : Size of the structure in bytes                    
 *  nVersion      : OMX specification version information      
 *  nBufferIndex  : Which buffer to query
 *  nPhysicalAddr : Physical address to return
 */
typedef struct OMX_CONFIG_VIDEO_OUTBUFTYPE {
    OMX_U32 nSize; 
    OMX_VERSIONTYPE nVersion;
    OMX_BUFFERHEADERTYPE  * pBufferHdr;
    OMX_PTR nPhysicalAddr;
} OMX_CONFIG_VIDEO_OUTBUFTYPE;

/* memory operator functions */
typedef struct 
{
    int nSize;                        /*!requested memory size */
    unsigned long nPhyAddr; /*!physical memory address allocated */
    unsigned long nCpuAddr; /*!cpu addr for system free usage */
    unsigned long nVirtAddr; /*!virtual user space address */	
}OMX_MEM_DESC;
typedef OMX_BOOL (*pf_mem_malloc)(OMX_MEM_DESC* pOutMemDesc);
typedef OMX_BOOL (*pf_mem_free)(OMX_MEM_DESC* pOutMemDesc);
typedef struct {
    pf_mem_malloc pfMalloc;
    pf_mem_free pfFree;
} OMX_PARAM_MEM_OPERATOR;

typedef struct {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL bAbort;
} OMX_CONFIG_ABORTBUFFERING;

typedef struct {
    OMX_U32 nSize; 
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL bGetMetadata;
} OMX_PARAM_IS_GET_METADATA;

/** WMA params externtion */
typedef struct OMX_AUDIO_PARAM_WMATYPE_EXT {
    OMX_U32 nSize;            /**< size of the structure in bytes */
    OMX_VERSIONTYPE nVersion; /**< OMX specification version information */
    OMX_U32 nPortIndex;       /**< port that this structure applies to */
	/**< Extensions for WMA codec config */
    OMX_U32 nBitsPerSample;   
} OMX_AUDIO_PARAM_WMATYPE_EXT;

typedef enum {
    BUFFER_SW_READ_NEVER  = 0x1,
    BUFFER_SW_READ_OFTEN  = 0x2,
    BUFFER_SW_WRITE_NEVER = 0x4,
    BUFFER_SW_WRITE_OFTEN = 0x8,
    BUFFER_PHY_CONTINIOUS = 0x10
} OMX_BUFFER_USAGE;


#endif

/* File EOF */
