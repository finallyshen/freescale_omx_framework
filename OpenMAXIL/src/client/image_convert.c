/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <stdio.h>
#include <stdint.h>                                                               
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev.h>

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "fsl_osal.h"
#include "OMX_ImageConvert.h"
#include "jpeg_enc_interface.h"

#if 0
#ifdef MX37
#include "mxc_ipu_imp_lib.h"
#endif 
#ifdef MX51
#include "mxc_ipu_hl_lib.h"
#endif
#endif

#include "Mem.h"
#include "Log.h"

#include "cc16Wrapper.h"  // for color convert and resize

#define CHECK_IMAGE_FORMAT(in,out) \
    do {\
        if(in->eColorFormat == OMX_COLOR_FormatUnused) {\
            printf("input image format is not specified.\n");\
            return OMX_FALSE;\
        }\
        \
        if(out->eColorFormat == OMX_COLOR_FormatUnused) {\
            printf("output image format is not specified.\n");\
            return OMX_FALSE;\
        }\
    } while(0)



typedef struct _OMX_ImageConvertPrivateData {
        FILE *fp_out;
        OMX_U8 *out_buf;
        Resize_mode resize_mode;
}OMX_ImageConvertPrivateData;

typedef struct jpeg_enc_context{
	JPEG_ENC_UINT8 * pBuf;
	JPEG_ENC_INT32  nSize;
	JPEG_ENC_INT32 nOffset;
	FILE *fpOut;
}jpeg_enc_context;


//method function
static OMX_BOOL ic_delete(OMX_ImageConvert *h);
static OMX_BOOL
ic_resize(OMX_ImageConvert *h, 
          OMX_IMAGE_PORTDEFINITIONTYPE *in_format, OMX_CONFIG_RECTTYPE *pCropRect, OMX_U8 *in,
          OMX_IMAGE_PORTDEFINITIONTYPE *out_format, OMX_U8 *out);
static OMX_BOOL
ic_jpeg_enc(OMX_ImageConvert *h, OMX_IMAGE_PORTDEFINITIONTYPE *format, OMX_U8 *buf);
static OMX_BOOL
ic_raw_data(OMX_ImageConvert *h, OMX_IMAGE_PORTDEFINITIONTYPE *format, OMX_U8 *buf);
static OMX_BOOL
ic_set_property(OMX_ImageConvert *h, IC_PROPERTY property, OMX_PTR value);


//local function
static OMX_U32 pxlfmt2bpp(OMX_COLOR_FORMATTYPE omx_pxlfmt);
static int omx2ipu_pxlfmt(OMX_COLOR_FORMATTYPE omx_pxlfmt);
static void fill_exif_params(jpeg_enc_exif_parameters *params);

JPEG_ENC_UINT8  jpeg_enc_output_callback(JPEG_ENC_UINT8 ** out_buf_ptrptr,JPEG_ENC_UINT32 *out_buf_len_ptr,
                                         JPEG_ENC_UINT8 flush, void * context, JPEG_ENC_MODE enc_mode);
JPEG_ENC_YUV_FORMAT jpeg_enc_color_format(OMX_COLOR_FORMATTYPE eColorFormat);

#define USING_CONTEXT
//global variable
#ifndef USING_CONTEXT
jpeg_enc_context gJpegEncCtx;
#endif

OMX_ImageConvert* OMX_ImageConvertCreate()
{
    OMX_ImageConvert *ic = NULL;
    OMX_ImageConvertPrivateData* data=NULL;

    data = (OMX_ImageConvertPrivateData *)FSL_MALLOC(sizeof(OMX_ImageConvertPrivateData));
    if (data == NULL)
        return NULL;
    fsl_osal_memset(data, 0, sizeof(OMX_ImageConvertPrivateData));
    /* allocate memory for image convert object */
    ic = (OMX_ImageConvert *)FSL_MALLOC(sizeof(OMX_ImageConvert));
    if (ic == NULL)
        return NULL;
	
    fsl_osal_memset(ic, 0, sizeof(OMX_ImageConvert));

    ic->resize = ic_resize;
    ic->jpeg_enc = ic_jpeg_enc;
    ic->raw_data = ic_raw_data;
    ic->set_property = ic_set_property;
    ic->delete_it = ic_delete;
	data->resize_mode = KEEP_DEST_RESOLUTION;
    ic->pData = data;

    return ic;
}

static OMX_BOOL ic_delete(OMX_ImageConvert *h)
{
    OMX_ImageConvertPrivateData* data=(OMX_ImageConvertPrivateData*)h->pData;

    FSL_FREE(data);
    FSL_FREE(h);

    return OMX_TRUE;
}

static OMX_BOOL
ic_resize(OMX_ImageConvert *h, 
          OMX_IMAGE_PORTDEFINITIONTYPE *in_format, OMX_CONFIG_RECTTYPE *pCropRect, OMX_U8 *in,
          OMX_IMAGE_PORTDEFINITIONTYPE *out_format, OMX_U8 *out)
{
    if(out_format->eColorFormat != OMX_COLOR_Format16bitRGB565)
    {
	LOG_ERROR("re_resize output format error!!! %d\n", out_format->eColorFormat);
	return OMX_FALSE;
    }

    void *pColorConvert16 = cc16WrapperCreate();
    if(!pColorConvert16 ||
       !cc16WrapperInit(pColorConvert16, ((in_format->nFrameWidth)&(~1)), ((in_format->nFrameHeight)&(~1)), ((in_format->nFrameWidth)&(~1)), pCropRect, in_format->eColorFormat,
                                                        out_format->nFrameWidth, ((out_format->nFrameHeight)&(~1)), ((out_format->nFrameWidth)&(~1)), 0) ||
        !cc16WrapperSetMode(pColorConvert16 ,1) ||
        !cc16WrapperConvert(pColorConvert16, (OMX_U8*)in, (OMX_U8*)out)) 
    {
        LOG_ERROR("failed to do color conversion!!!\n");
        cc16WrapperDelete(&pColorConvert16);
        return OMX_FALSE;
    }
	
    cc16WrapperDelete(&pColorConvert16);
    return OMX_TRUE;


#if 0

    OMX_ImageConvertPrivateData* data=(OMX_ImageConvertPrivateData*)h->pData;
    OMX_U32 in_size, out_size;

    ipu_lib_handle_t ipu_handle;
    ipu_lib_input_param_t sInParam;
    ipu_lib_output_param_t sOutParam;
    int mode, ret;

    CHECK_IMAGE_FORMAT(in_format, out_format);

    if(in_format->nFrameWidth == out_format->nFrameWidth
         && in_format->nFrameHeight == out_format->nFrameHeight
         && in_format->eColorFormat == out_format->eColorFormat) 
    {
        fsl_osal_memcpy(out, in, 
                in_format->nFrameWidth*in_format->nFrameHeight*pxlfmt2bpp(in_format->eColorFormat)/8);
        return OMX_TRUE;
    }

    fsl_osal_memset(&sInParam, 0, sizeof(ipu_lib_input_param_t));
    sInParam.width = in_format->nFrameWidth;
    sInParam.height = in_format->nFrameHeight;
    sInParam.fmt = omx2ipu_pxlfmt(in_format->eColorFormat);
    if(data->resize_mode == KEEP_DEST_RESOLUTION) {
        if(pCropRect->nWidth*out_format->nFrameHeight
           >= pCropRect->nHeight*out_format->nFrameWidth) {
            /* need crop left and right side */
            OMX_U32 width = pCropRect->nHeight*out_format->nFrameWidth/out_format->nFrameHeight;
            width = (width+7)/8*8;
            sInParam.input_crop_win.pos.x = (((pCropRect->nWidth - width)/2 + pCropRect->nLeft)+7)/8*8;
            sInParam.input_crop_win.pos.y = pCropRect->nTop;
            sInParam.input_crop_win.win_w = width;
            sInParam.input_crop_win.win_h = pCropRect->nHeight;
        }

        if(pCropRect->nWidth*out_format->nFrameHeight
            < pCropRect->nHeight*out_format->nFrameWidth) {
            /* need crop top and bottom side */
            OMX_U32 height = pCropRect->nWidth*out_format->nFrameHeight/out_format->nFrameWidth;
            sInParam.input_crop_win.pos.x = (pCropRect->nLeft+7)/8*8;
            sInParam.input_crop_win.pos.y = (((pCropRect->nHeight - height)/2 + pCropRect->nTop)+1)/2*2;
            sInParam.input_crop_win.win_w = (pCropRect->nWidth+7)/8*8;
            sInParam.input_crop_win.win_h = height;
        }
    }

    fsl_osal_memset(&sOutParam, 0, sizeof(ipu_lib_output_param_t));
    sOutParam.fmt = omx2ipu_pxlfmt(out_format->eColorFormat);
    if(data->resize_mode == KEEP_ORG_RESOLUTION) {
        if(in_format->nFrameWidth*out_format->nFrameHeight
                >= in_format->nFrameHeight*out_format->nFrameWidth) {
            /* keep target width*/
            OMX_U32 height = out_format->nFrameWidth*in_format->nFrameHeight/in_format->nFrameWidth;
            sOutParam.output_win.win_w = out_format->nFrameWidth;
            sOutParam.output_win.win_h = height;
            sOutParam.output_win.pos.x = 0;
            sOutParam.output_win.pos.y = (out_format->nFrameHeight - height)/2;
            sOutParam.width = out_format->nFrameWidth;
            sOutParam.height = out_format->nFrameHeight;
        }

        if(in_format->nFrameWidth*out_format->nFrameHeight
                < in_format->nFrameHeight*out_format->nFrameWidth) {
            /* keep target height*/
            OMX_U32 width = out_format->nFrameHeight*in_format->nFrameWidth/in_format->nFrameHeight;
            sOutParam.output_win.win_w = width;
            sOutParam.output_win.win_h = out_format->nFrameHeight;
            sOutParam.output_win.pos.x = ((out_format->nFrameWidth - width)/2 + 7)/8*8;
            sOutParam.output_win.pos.y = 0;
            sOutParam.width = out_format->nFrameWidth;
            sOutParam.height = out_format->nFrameHeight;
        }
    }
    else {
        sOutParam.width = out_format->nFrameWidth;
        sOutParam.height = out_format->nFrameHeight;
    }

    LOG_DEBUG("sInParam: width: %d, height: %d, win_w: %d, win_h: %d, x: %d, y: %d\n",
            sInParam.width, sInParam.height, 
            sInParam.input_crop_win.win_w, sInParam.input_crop_win.win_h,
            sInParam.input_crop_win.pos.x, sInParam.input_crop_win.pos.y);

    LOG_DEBUG("sOutParam: width: %d, height: %d, win_w: %d, win_h: %d, x: %d, y: %d\n",
            sOutParam.width, sOutParam.height,
            sOutParam.output_win.win_w, sOutParam.output_win.win_h,
            sOutParam.output_win.pos.x, sOutParam.output_win.pos.y);

    mode = OP_NORMAL_MODE;
    fsl_osal_memset(&ipu_handle, 0, sizeof(ipu_lib_handle_t));
#ifdef MX37 
    ret = mxc_ipu_lib_task_init(&sInParam, &sOutParam, NULL, mode, &ipu_handle);
#else 
    ret = mxc_ipu_lib_task_init(&sInParam, NULL, &sOutParam, mode, &ipu_handle);
#endif
    if(ret < 0) {
        printf("mxc_ipu_lib_task_init failed!\n");
        return OMX_FALSE;
    }

    out_format->nFrameWidth = sOutParam.width;
    out_format->nFrameHeight = sOutParam.height;

    /* copy input to ipu allocated input buffer */
    fsl_osal_memcpy(ipu_handle.inbuf_start[0], in, ipu_handle.ifr_size);

    /* call ipu task */
#ifdef MX37 
    ret = mxc_ipu_lib_task_buf_update(&ipu_handle, 0, 0, 0, 0);
#else 
    ret = mxc_ipu_lib_task_buf_update(&ipu_handle,sInParam.user_def_paddr[0],0,0,0,0);
#endif
    if(ret < 0) {
        printf("mxc_ipu_lib_task_buf_update failed.\n");
        return OMX_FALSE;
    }

    /* copy ipu output to output buffer */
    fsl_osal_memcpy(out, ipu_handle.outbuf_start[0], ipu_handle.ofr_size);

    mxc_ipu_lib_task_uninit(&ipu_handle);

    return OMX_TRUE;
#endif

}

static OMX_BOOL
ic_jpeg_enc(OMX_ImageConvert *h, OMX_IMAGE_PORTDEFINITIONTYPE *format, OMX_U8 *buf)
{
    OMX_ImageConvertPrivateData* data=(OMX_ImageConvertPrivateData*)h->pData;

    jpeg_enc_object *obj_ptr;
    jpeg_enc_parameters *params;
    JPEG_ENC_UINT8 number_mem_info;
    jpeg_enc_memory_info *mem_info;
    JPEG_ENC_RET_TYPE return_val;	
    JPEG_ENC_UINT8 * i_buff;
    JPEG_ENC_UINT8 * y_buff;
    JPEG_ENC_UINT8 * u_buff;
    JPEG_ENC_UINT8 * v_buff;
    jpeg_enc_context *  jpeg_enc_ctx;
    int i;

    /* --------------------------------------------
     * Allocate memory for Encoder Object
     * -------------------------------------------*/
    obj_ptr = (jpeg_enc_object *)FSL_MALLOC(sizeof(jpeg_enc_object));
    if(obj_ptr ==NULL)
        return OMX_FALSE;
    fsl_osal_memset(obj_ptr,0,sizeof(jpeg_enc_object));

    /* Assign the function for streaming output */
    obj_ptr->jpeg_enc_push_output = jpeg_enc_output_callback;

#ifdef USING_CONTEXT
    jpeg_enc_ctx = (jpeg_enc_context *)FSL_MALLOC(sizeof (jpeg_enc_context));
#else
    jpeg_enc_ctx = &gJpegEncCtx;
#endif    
    if(jpeg_enc_ctx == NULL){
	  printf("Malloc jpeg_enc_context failed\n");
        return OMX_FALSE;
    }
    jpeg_enc_ctx->nSize = 1024;
    jpeg_enc_ctx->nOffset = 0;
    jpeg_enc_ctx->fpOut = data->fp_out;

    jpeg_enc_ctx->pBuf = (JPEG_ENC_UINT8 *)FSL_MALLOC(jpeg_enc_ctx->nSize);
    if(jpeg_enc_ctx->pBuf==NULL)	{
        printf("Malloc failed\n");
        return OMX_FALSE;
    }
    fsl_osal_memset(jpeg_enc_ctx->pBuf,0,jpeg_enc_ctx->nSize);
    obj_ptr->context = jpeg_enc_ctx;
    
    /* --------------------------------------------
     * Fill up the parameter structure of JPEG Encoder
     * -------------------------------------------*/
    params = &(obj_ptr->parameters);
    /*Default parameters*/
    params->compression_method = JPEG_ENC_SEQUENTIAL;
    params->mode = JPEG_ENC_MAIN_ONLY;
    params->quality = 75;
    params->restart_markers = 0;
    //params->y_width = 0;
    //params->y_height = 0;
    //params->u_width = 0;
    //params->u_height = 0;
    //params->v_width = 0;
    //params->v_height = 0;
    params->primary_image_height = 640;
    params->primary_image_width = 480;
    params->exif_flag = 0;

    params->yuv_format = jpeg_enc_color_format(format->eColorFormat);

    if (params->exif_flag)
    {
        fill_exif_params(&(params->exif_params));
    }
    else
    {
        /* Pixel size is unknown by default */
        params->jfif_params.density_unit = 0;
        /* Pixel aspect ratio is square by default */
        params->jfif_params.X_density = 1;
        params->jfif_params.Y_density = 1;
    }

    /* --------------------------------------------
     * Allocate memory for Input and Output Buffers
     * -------------------------------------------*/

    if( params->yuv_format == JPEG_ENC_YUV_420_NONINTERLEAVED) {
        params->y_width = format->nFrameWidth;
        params->y_height = format->nFrameHeight;
        params->u_width = params->v_width= 
                          (format->nFrameWidth & 0x1) ? (format->nFrameWidth>>1)+1:format->nFrameWidth>>1;
        params->u_height = params->v_height= 
                          (format->nFrameHeight & 0x1) ? (format->nFrameHeight>>1)+1:format->nFrameHeight>>1;
#if 1
	params->y_left = params->u_left = params->v_left = 0;
	params->y_top = params->u_top = params->v_top = 0 ;
	params->y_total_width = params->y_width;
	params->y_total_height = params->y_height;
	params->u_total_width = params->u_width;
	params->u_total_height = params->u_height;
	params->v_total_width = params->v_width;
	params->v_total_height = params->v_height;
	params->raw_dat_flag= 0;	
#endif		
        OMX_S32 framesize = format->nFrameWidth * format->nFrameHeight;
        y_buff = buf;
        u_buff = buf+ framesize;
        v_buff = u_buff + (framesize>>2);
        i_buff = NULL;
    }
    else    {
        // other format not support now
        printf("jpeg_enc: not support color format\n ");
        return OMX_FALSE;
    } 

    /* --------------------------------------------
     * QUERY MEMORY REQUIREMENTS
     * -------------------------------------------*/
    return_val = jpeg_enc_query_mem_req(obj_ptr);

    if(return_val != JPEG_ENC_ERR_NO_ERROR)
    {
        printf("JPEG encoder returned an error when jpeg_enc_query_mem_req was called \n");
        printf("Return Val %d\n",return_val);
        return OMX_FALSE;
    }

    /* --------------------------------------------
     * ALLOCATE MEMORY REQUESTED BY CODEC
     * -------------------------------------------*/

    number_mem_info = obj_ptr->mem_infos.no_entries;

    for(i = 0; i < number_mem_info; i++)
    {
        /* This example code ignores the 'alignment' and
         * 'memory_type', but some other applications might want
         * to allocate memory based on them */
        mem_info = &(obj_ptr->mem_infos.mem_info[i]);
    	mem_info->memptr = FSL_MALLOC(mem_info->size);
        if(mem_info->memptr==NULL)	{
            printf("Malloc error after query\n");
            return OMX_FALSE;
        }
        fsl_osal_memset(mem_info->memptr,0,sizeof(mem_info->memptr));
    }

    /* --------------------------------------------
     * Call encoder Init routine
     * -------------------------------------------*/
    return_val = jpeg_enc_init(obj_ptr);

    if(return_val != JPEG_ENC_ERR_NO_ERROR)
    {
        printf("JPEG encoder returned an error when jpeg_enc_init was called \n");
        printf("Return Val %d\n",return_val);
        return OMX_FALSE;
    }

    /* Temporary and later this will be removed once streaming output
     * is implemented */
    /* obj_ptr->out_buff = out_buff_ref; */
    /* --------------------------------------------
     * CALL JPEG ENCODER : Frame Level API
     * -------------------------------------------*/     
    return_val = jpeg_enc_encodeframe(obj_ptr, i_buff,
            y_buff, u_buff, v_buff);
    if(return_val == JPEG_ENC_ERR_ENCODINGCOMPLETE) {
        printf("Encoding of Image completed\n");
    }
    else {
        printf("JPEG encoder returned an error in jpeg_enc_encodeframe \n");
        printf("Return Val %d\n",return_val);
        return OMX_FALSE;
    }

    /* --------------------------------------------
     * FREE MEMORY REQUESTED BY CODEC
     * -------------------------------------------*/
    number_mem_info = obj_ptr->mem_infos.no_entries;
    for(i = 0; i < number_mem_info; i++)
    {
        mem_info = &(obj_ptr->mem_infos.mem_info[i]);
        FSL_FREE(mem_info->memptr);
    }

    FSL_FREE(jpeg_enc_ctx->pBuf);

#ifdef USING_CONTEXT
    if(jpeg_enc_ctx)
    {
        FSL_FREE(jpeg_enc_ctx);
        jpeg_enc_ctx = NULL;
    }
#endif
    FSL_FREE(obj_ptr);

    return OMX_TRUE;
}

static OMX_BOOL
ic_raw_data(OMX_ImageConvert *h, OMX_IMAGE_PORTDEFINITIONTYPE *format, OMX_U8 *buf)
{
    OMX_ImageConvertPrivateData* data=(OMX_ImageConvertPrivateData*)h->pData;
    OMX_U32 size;

    if(data->out_buf == NULL)
        return OMX_FALSE;

    size = format->nFrameWidth * format->nFrameHeight
           * pxlfmt2bpp(format->eColorFormat)/8;

    fsl_osal_memcpy(data->out_buf, buf, size);

    return OMX_TRUE;
}

static OMX_BOOL
ic_set_property(OMX_ImageConvert *h, IC_PROPERTY property, OMX_PTR value)
{
    OMX_ImageConvertPrivateData* data=(OMX_ImageConvertPrivateData*)h->pData;

    switch(property) {
        case PROPERTY_RESIZE_MODE:
            data->resize_mode = *((Resize_mode*)value);
            break;
        case PROPERTY_OUT_FILE:
            data->fp_out = (FILE*)value;
            break;
        case PROPERTY_OUT_BUFFER:
            data->out_buf = (OMX_U8*)value;
            break;
        default:
            return OMX_FALSE;
    }

    return OMX_TRUE;
}

static OMX_U32 pxlfmt2bpp(OMX_COLOR_FORMATTYPE omx_pxlfmt)
{
	OMX_U32 bpp; // bit per pixel

	switch(omx_pxlfmt) {
	case OMX_COLOR_FormatMonochrome:
	  bpp = 1;
	  break;
	case OMX_COLOR_FormatL2:
	  bpp = 2;
	case OMX_COLOR_FormatL4:
	  bpp = 4;
	  break;
	case OMX_COLOR_FormatL8:
	case OMX_COLOR_Format8bitRGB332:
	case OMX_COLOR_FormatRawBayer8bit:
	case OMX_COLOR_FormatRawBayer8bitcompressed:
	  bpp = 8;
	  break;
	case OMX_COLOR_FormatRawBayer10bit:
	  bpp = 10;
	  break;
	case OMX_COLOR_FormatYUV411Planar:
	case OMX_COLOR_FormatYUV411PackedPlanar:
	case OMX_COLOR_Format12bitRGB444:
	case OMX_COLOR_FormatYUV420Planar:
	case OMX_COLOR_FormatYUV420PackedPlanar:
	case OMX_COLOR_FormatYUV420SemiPlanar:
	case OMX_COLOR_FormatYUV444Interleaved:
	  bpp = 12;
	  break;
	case OMX_COLOR_FormatL16:
	case OMX_COLOR_Format16bitARGB4444:
	case OMX_COLOR_Format16bitARGB1555:
	case OMX_COLOR_Format16bitRGB565:
	case OMX_COLOR_Format16bitBGR565:
	case OMX_COLOR_FormatYUV422Planar:
	case OMX_COLOR_FormatYUV422PackedPlanar:
	case OMX_COLOR_FormatYUV422SemiPlanar:
	case OMX_COLOR_FormatYCbYCr:
	case OMX_COLOR_FormatYCrYCb:
	case OMX_COLOR_FormatCbYCrY:
	case OMX_COLOR_FormatCrYCbY:
	  bpp = 16;
	  break;
	case OMX_COLOR_Format18bitRGB666:
	case OMX_COLOR_Format18bitARGB1665:
	  bpp = 18;
	  break;
	case OMX_COLOR_Format19bitARGB1666:
	  bpp = 19;
	  break;
	case OMX_COLOR_FormatL24:
	case OMX_COLOR_Format24bitRGB888:
	case OMX_COLOR_Format24bitBGR888:
	case OMX_COLOR_Format24bitARGB1887:
	  bpp = 24;
	  break;
	case OMX_COLOR_Format25bitARGB1888:
	  bpp = 25;
	  break;
	case OMX_COLOR_FormatL32:
	case OMX_COLOR_Format32bitBGRA8888:
	case OMX_COLOR_Format32bitARGB8888:
	  bpp = 32;
	  break;
	default:
	  bpp = 0;
	  break;
	}
	return bpp;
}

static int omx2ipu_pxlfmt(OMX_COLOR_FORMATTYPE omx_pxlfmt)
{
	int ipu_pxlfmt; // bit per pixel

    switch(omx_pxlfmt) {
        case  OMX_COLOR_Format16bitRGB565:
            ipu_pxlfmt = v4l2_fourcc('R', 'G', 'B', 'P');
            break;
        case  OMX_COLOR_Format24bitRGB888:
            ipu_pxlfmt = v4l2_fourcc('R', 'G', 'B', '3');
            break;
        case  OMX_COLOR_Format24bitBGR888:
            ipu_pxlfmt = v4l2_fourcc('B', 'G', 'R', '3');
            break;
        case  OMX_COLOR_Format32bitBGRA8888:
            ipu_pxlfmt = v4l2_fourcc('B', 'G', 'R', 'A');
            break;
        case  OMX_COLOR_Format32bitARGB8888:
            ipu_pxlfmt = v4l2_fourcc('R', 'G', 'B', 'A');
            break;
        case  OMX_COLOR_FormatYUV420Planar:
            ipu_pxlfmt = v4l2_fourcc('I', '4', '2', '0');
            break;
        case  OMX_COLOR_FormatYUV420SemiPlanar:
            ipu_pxlfmt = v4l2_fourcc('N', 'V', '1', '2');
            break;
        case  OMX_COLOR_FormatYUV422Planar:
            ipu_pxlfmt = v4l2_fourcc('4', '2', '2','P');
            break;
        case  OMX_COLOR_FormatYUV422SemiPlanar:
            ipu_pxlfmt = v4l2_fourcc('N', 'V', '1', '6');
            break;
        case OMX_COLOR_FormatUnused:
        case  OMX_COLOR_FormatMonochrome:
        case  OMX_COLOR_Format8bitRGB332:
        case  OMX_COLOR_Format12bitRGB444:
        case  OMX_COLOR_Format16bitARGB4444:
        case  OMX_COLOR_Format16bitARGB1555:
        case  OMX_COLOR_Format16bitBGR565:
        case  OMX_COLOR_Format18bitRGB666:
        case  OMX_COLOR_Format18bitARGB1665:
        case  OMX_COLOR_Format19bitARGB1666:
        case  OMX_COLOR_Format24bitARGB1887:
        case  OMX_COLOR_Format25bitARGB1888:
        case  OMX_COLOR_FormatYUV411Planar:
        case  OMX_COLOR_FormatYUV411PackedPlanar:
        case  OMX_COLOR_FormatYUV420PackedPlanar:
        //case  OMX_COLOR_FormatYUV422Planar:
        case  OMX_COLOR_FormatYUV422PackedPlanar:
        //case  OMX_COLOR_FormatYUV422SemiPlanar:
        case  OMX_COLOR_FormatYCbYCr:
        case  OMX_COLOR_FormatYCrYCb:
        case  OMX_COLOR_FormatCbYCrY:
        case  OMX_COLOR_FormatCrYCbY:
        case  OMX_COLOR_FormatYUV444Interleaved:
        case  OMX_COLOR_FormatRawBayer8bit:
        case  OMX_COLOR_FormatRawBayer10bit:
        case  OMX_COLOR_FormatRawBayer8bitcompressed:
        case  OMX_COLOR_FormatL2:
        case  OMX_COLOR_FormatL4:
        case  OMX_COLOR_FormatL8:
        case  OMX_COLOR_FormatL16:
        case  OMX_COLOR_FormatL24:
        case  OMX_COLOR_FormatL32:
        case  OMX_COLOR_FormatYUV420PackedSemiPlanar:
        case  OMX_COLOR_FormatYUV422PackedSemiPlanar:
        case  OMX_COLOR_Format18BitBGR666:
        case  OMX_COLOR_Format24BitARGB6666:
        case  OMX_COLOR_Format24BitABGR6666:
        default:
            ipu_pxlfmt = 0;
            break;
    }
	
	return ipu_pxlfmt;	
}

static void fill_exif_params(jpeg_enc_exif_parameters *params)
{
    /* tags values required from application */
    const JPEG_ENC_UINT32 XResolution[2] = { 72, 1};
    const JPEG_ENC_UINT32 YResolution[2] = { 72, 1};
    const JPEG_ENC_UINT16 ResolutionUnit = 2;
    const JPEG_ENC_UINT16 YCbCrPositioning = 1;

    /* IFD0 params */
    params->IFD0_info.x_resolution[0] = XResolution[0];
    params->IFD0_info.x_resolution[1] = XResolution[1];
    params->IFD0_info.y_resolution[0] = YResolution[0];
    params->IFD0_info.y_resolution[1] = YResolution[1];
    params->IFD0_info.resolution_unit = ResolutionUnit;
    params->IFD0_info.ycbcr_positioning = YCbCrPositioning;

    /* IFD1 params */
    params->IFD1_info.x_resolution[0] = XResolution[0];
    params->IFD1_info.x_resolution[1] = XResolution[1];
    params->IFD1_info.y_resolution[0] = YResolution[0];
    params->IFD1_info.y_resolution[1] = YResolution[1];
    params->IFD1_info.resolution_unit = ResolutionUnit;

}



//std::map<void *,jpeg_enc_context *> MapCallBack;
//typedef std::map<void *,jpeg_enc_context *>::value_type CallBack_Instance_Pair_t;

JPEG_ENC_UINT8  jpeg_enc_output_callback(JPEG_ENC_UINT8 ** out_buf_ptrptr,JPEG_ENC_UINT32 *out_buf_len_ptr,
    JPEG_ENC_UINT8 flush,  void * context, JPEG_ENC_MODE enc_mode)
{
    JPEG_ENC_UINT32 i;
#ifdef USING_CONTEXT
    jpeg_enc_context * pjpeg_enc_ctx = (jpeg_enc_context *)context;
#else
    jpeg_enc_context * pjpeg_enc_ctx =&gJpegEncCtx;
#endif    
    FILE * fp_out= pjpeg_enc_ctx->fpOut;
	
    if(*out_buf_ptrptr == NULL)    {
        /* This function is called for the 1'st time from the codec */
        *out_buf_ptrptr = pjpeg_enc_ctx->pBuf;
        *out_buf_len_ptr = pjpeg_enc_ctx->nSize;
    }
    else if(flush == 1)    {
        /* Flush the buffer*/     
	fwrite(*out_buf_ptrptr,1,*out_buf_len_ptr,fp_out);	
    }
    else
    {
    	fwrite(*out_buf_ptrptr,1,pjpeg_enc_ctx->nSize,fp_out);
        /* Now provide a new buffer */
        *out_buf_ptrptr = pjpeg_enc_ctx->pBuf;
        *out_buf_len_ptr = pjpeg_enc_ctx->nSize;
    }

    return(1); /* Success */
}

JPEG_ENC_YUV_FORMAT jpeg_enc_color_format(OMX_COLOR_FORMATTYPE eColorFormat)
{
	JPEG_ENC_YUV_FORMAT ret_format = JPEG_ENC_YUV_420_NONINTERLEAVED;
	switch(eColorFormat){			
		case OMX_COLOR_FormatYUV420Planar:
			ret_format = JPEG_ENC_YUV_420_NONINTERLEAVED; break;	
    		case OMX_COLOR_FormatYUV422Planar:
			ret_format = JPEG_ENC_YU_YV_422_INTERLEAVED; break;
    		case OMX_COLOR_FormatYCbYCr:
			ret_format = JPEG_ENC_YU_YV_422_INTERLEAVED; break;	
		case OMX_COLOR_FormatYCrYCb:
			ret_format = JPEG_ENC_YV_YU_422_INTERLEAVED; break;	
		case OMX_COLOR_FormatCbYCrY:
			ret_format = JPEG_ENC_UY_VY_422_INTERLEAVED; break;	
		case OMX_COLOR_FormatCrYCbY:
			ret_format = JPEG_ENC_VY_UY_422_INTERLEAVED; break;	
		case OMX_COLOR_FormatYUV444Interleaved:
			ret_format = JPEG_ENC_YUV_444_NONINTERLEAVED; break;	 // TODO: no 444 noninterleaved in omx
		case OMX_COLOR_FormatYUV422PackedPlanar:
    		case OMX_COLOR_FormatYUV422SemiPlanar:
		case OMX_COLOR_FormatYUV411Planar:	
		case  OMX_COLOR_FormatYUV420PackedPlanar:
   		case OMX_COLOR_FormatYUV420SemiPlanar:
		case OMX_COLOR_FormatYUV411PackedPlanar:
		default:
			break;	
	}
	return ret_format;
}

