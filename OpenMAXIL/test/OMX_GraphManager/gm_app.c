/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 *  History :
 *  Date             Author       Version    Description
 *
 * 10/06/2008    b02736        0.1     Created
 *
 * OMX_GraphManager.c
 */
#include <sys/time.h>
#include "string.h"
#include "stdio.h"
#include "dirent.h"
#include "ctype.h"
#include "fsl_osal.h"
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Implement.h"
#include <sys/ioctl.h>
#include <fcntl.h>

#include "DrmApi.h"

#include "OMX_GraphManager.h"
#include "OMX_ImageConvert.h"




#if 0
/*data structure definition*/

#define GM_MAX_FILENAME_LEN 1024
#define GM_MAX_PORT_COUNT 8
#define GM_MAX_PORT_BUFFER_COUNT 32

/*components names definition*/
#define AUDIO_RENDER_NAME "alsa.alsasink"
#define VIDEO_SCH_NAME "video_scheduler.binary"
#define CLOCK_NAME  "clocksrc"
//audio volume has some problems, so use a invalid name to avoid load audio volume
#define AUDIO_VOLUME_ROLE "audio_volume.pcm"
#ifdef USE_BELLAGIO
#define AUDIO_FILE_READER_NAME "OMX.st.audio_filereader"
#define VIDEO_FILE_READER_NAME "OMX.st.video_filereader"
#define VIDEO_RENDER_NAME "fbdev.fbdev_sink"
#else
#define AUDIO_FILE_READER_NAME "OMX.Freescale.std.audio.file_read.sw-based"
#define VIDEO_FILE_READER_NAME "OMX.Freescale.std.video.file_read.sw-based"
#define VIDEO_RENDER_NAME "video.v4lsink"
#endif


#define FOR_SAMSUNG_BOARD_ONLY
#ifdef FOR_SAMSUNG_BOARD_ONLY
#define DEFAULT_OUTPUT_HEIGHT 240
#define DEFAULT_OUTPUT_WIDTH 400
#else
#define DEFAULT_OUTPUT_HEIGHT 360
#define DEFAULT_OUTPUT_WIDTH 640
#endif

#endif

#if 0
#define DEFAULT_TVOUT_NTSC_HEIGHT 480
#define DEFAULT_TVOUT_NTSC_WIDTH  720
#define DEFAULT_TVOUT_PAL_HEIGHT 576
#define DEFAULT_TVOUT_PAL_WIDTH 720
#endif

typedef enum{
    PLAYLIST,
    PLAYFILE
}PlayItemType;

typedef enum{
    DIRECT_CURRENT,
    DIRECT_PREVIOUS,
    DIRECT_NEXT    
}PlayItemDirection;

typedef struct _PlayItem{
    char * name;
    PlayItemType type;
    OMX_S32 curIndex;
    void * buffer;
    struct _PlayItem * prev;
    struct _PlayItem * next;
}PlayItem;


typedef struct {
    PlayItem * playItemsHead;
    PlayItem * playItemsTail;
    PlayItem * curPlayItem;
    OMX_BOOL repeat;
}GmPlayItems;


typedef struct {
    char * debugLevel;
    char * debugFile;
    OMX_BOOL aging;
    OMX_BOOL startPlay;
    OMX_BOOL repeat;
    OMX_BOOL quiet;
    GmPlayItems gpi;
}GM_Options;
#define START_PLAYTIME_INFO_THREAD(thread, gm)\
    do{\
        if (options.quiet == OMX_FALSE && thread == NULL){\
            bexituserinput = OMX_FALSE;\
            fsl_osal_thread_create(&(thread), NULL, userinput, (gm));\
        }\
    }while(0)

#define STOP_PLAYTIME_INFO_THREAD(thread)\
    do{\
        if((thread && bexituserinput == OMX_FALSE)) {\
            bexituserinput = OMX_TRUE;\
            fsl_osal_thread_destroy((thread));\
            (thread)=NULL;\
        }\
    }while(0)


#define START_SHOW_PLAYTIME_INFO \
            do{\
                bstartuserinput = OMX_TRUE;\
            }while(0) 

#define STOP_SHOW_PLAYTIME_INFO \
    do{\
        bstartuserinput = OMX_FALSE;\
    }while(0) 

#define SCENESNUM		22
#define BANDSINGRP      10

typedef struct {
	OMX_U32 Fc;
	OMX_S32 Gain;
	OMX_S32 Q_value;
	OMX_S32 FilterType;
} FILTER_PARA;

typedef struct {
	OMX_U32 bandsnumber;
	FILTER_PARA filterPara[BANDSINGRP];
} PREDETEMIED_SCENES;

/*	Constant data (may be stored in ROM) */
const PREDETEMIED_SCENES  scene[SCENESNUM] = {
	/* Load predetermined scenes */
#include "peq_tables/acoustic.h" 
#include "peq_tables/bass_booster.h" 
#include "peq_tables/bass_reducer.h" 
#include "peq_tables/classical.h"
#include "peq_tables/dance.h" 
#include "peq_tables/deep.h" 
#include "peq_tables/electronic.h" 
#include "peq_tables/hip_hop.h" 
#include "peq_tables/jazz.h" 
#include "peq_tables/latin.h" 
#include "peq_tables/loudness.h" 
#include "peq_tables/lounge.h" 
#include "peq_tables/piano.h" 
#include "peq_tables/pop.h" 
#include "peq_tables/r_b.h" 
#include "peq_tables/rock.h" 
#include "peq_tables/small_speakers.h" 
#include "peq_tables/spoken_word.h" 
#include "peq_tables/treble_booster.h" 
#include "peq_tables/treble_reducer.h" 
#include "peq_tables/vocal_booster.h" 
#include "peq_tables/flat.h"
};

/*test code here*/
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <signal.h>

#define FILE_NAME_LEN 1024

fsl_osal_ptr thread=NULL;
OMX_BOOL bexituserinput = OMX_FALSE;
OMX_BOOL bstartuserinput = OMX_FALSE;

OMX_BOOL bAgingtest = OMX_FALSE;
fsl_osal_sem gplaytestsem;
OMX_S64 gtestcnt=0, gtimoutcnt=0;


volatile sig_atomic_t quitflag = 0;
void my_int_signal_handler(int signum)
{
	quitflag = 1;
}

int eventhandler(void* context, GM_EVENT eventID, void* Eventpayload)
{
    OMX_GraphManager* gm = (OMX_GraphManager*) context;
    switch(eventID) {
        case GM_EVENT_EOS:
			{
                STOP_SHOW_PLAYTIME_INFO;
				gm->stop(gm);
                if(bAgingtest)
                    fsl_osal_sem_post(gplaytestsem);
                else {
                    printf("\nPlayback done, choose next command\n");
                }
                break;
            }
        case GM_EVENT_FF_EOS:
            printf("%s, receive FF EOS event\n",__FUNCTION__);
            break;
        case GM_EVENT_BW_BOS:
            printf("%s, receive BW BOS event\n",__FUNCTION__);
            break;
		case GM_EVENT_CORRUPTED_STREM:
			{
				printf("%s,%d,corrupted stream encountered during playing.\n",__FUNCTION__,__LINE__);
					STOP_SHOW_PLAYTIME_INFO;
				gm->stop(gm);
				if(bAgingtest)
					fsl_osal_sem_post(gplaytestsem);
				else {
					printf("\nPlayback done, choose next command\n");
				}
			}
			break;
        default:
            break;
    }

    return 1;
}
#ifdef MFW_AVI_SUPPORT_DIVX_DRM

OMX_BOOL bDrmRentalNeedSecondLoad = OMX_FALSE;
int rental_count_remained = 0;
/*GUI player that use graph manage should redefine this function.*/
OMX_S32 cb_drm_app(void* context, sFslDrmInfo* pFslDrmInfo)
{
	/*
	the context should be assigned to the GUI player's handle.
	and the pFslDrmInfo should carry the user input value to the caller.
	 */
	if (pFslDrmInfo->bDivxDrmPresent)
	{
		printf("%s,%d, divx drm present!\n",__FUNCTION__,__LINE__);
	}
	else
	{
		printf("%s,%d, divx drm not present!\n",__FUNCTION__,__LINE__);
	}
	
	
	/*when UI call gm_load the second time, it must be user comfirmed*/
    if (bDrmRentalNeedSecondLoad && rental_count_remained)
    {
        pFslDrmInfo->userInput = E_FSL_INPUT_CONFIRM;
        rental_count_remained = 0;
        bDrmRentalNeedSecondLoad = OMX_FALSE;
    }
    else
    {
        pFslDrmInfo->userInput = E_FSL_INPUT_CANCEL;
        rental_count_remained = pFslDrmInfo->use_limit - pFslDrmInfo->use_count;
        if (rental_count_remained > 0)
            bDrmRentalNeedSecondLoad = OMX_TRUE;
    }
    return 0;
}
static int s_readFragmentLocal( unsigned int fragmentNum,
                                unsigned char* data,
                                unsigned int dataLength )
{
    unsigned int returnCode = DRM_LOCAL_SUCCESS;
	FILE *input;
    unsigned char i;

    printf("%s,%d.\n",__FUNCTION__,__LINE__);
   
    input = fopen( "./playervis.db",
                   "rb" );
    if ( input == NULL )
    {
        returnCode = DRM_ERROR_READING_MEMORY;
        return returnCode;
    }


    if ( fread( data, 1, dataLength, input ) != dataLength )
    {
        returnCode = DRM_ERROR_READING_MEMORY;
        return returnCode;
    }
    
    fclose(input);
    printf("%s,%d.\n",__FUNCTION__,__LINE__);
    return returnCode;
}

static int s_writeFragmentLocal( unsigned int fragmentNum,
                                unsigned char* data,
                                unsigned int dataLength )
{
    unsigned int returnCode = DRM_LOCAL_SUCCESS;

    FILE *output;
    unsigned char i;

    printf("%s,%d.\n",__FUNCTION__,__LINE__);


    output = fopen( "./playervis.db",
                    "wb");
    if ( output == NULL )
    {
        returnCode = DRM_ERROR_WRITING_MEMORY;
        return returnCode;
    }
    

    if ( fwrite( data, 1, dataLength, output ) != dataLength )
    {
        returnCode = DRM_ERROR_WRITING_MEMORY;
        return returnCode;
    }

    
    fclose( output );
    printf("%s,%d.\n",__FUNCTION__,__LINE__);
    return returnCode;
}

DivxDrmMemAccessFunc divx_drm_func = 
{
	s_writeFragmentLocal,
	s_readFragmentLocal,
};
#endif

void userinput(void* param)
{
    OMX_GraphManager* gm = 	(OMX_GraphManager*)param;

    int h,m,s;
    OMX_TICKS sCur,sDur;
	OMX_U32 Hours, Hours1;
    OMX_U32 Minutes, Minutes1;
    OMX_U32 Seconds, Seconds1;

    while(bexituserinput == OMX_FALSE) 
    {
        if (bstartuserinput)
		{
			sDur = 0;
			gm->getDuration(gm, &sDur);
			sDur += 1000000;
			Hours1 = ((OMX_U64)sDur/1000000) / 3600;
			Minutes1 = ((OMX_U64)sDur/ (60*1000000)) % 60;
			Seconds1 = (((OMX_U64)sDur %(3600*1000000)) % (60*1000000))/1000000;

			sCur = 0;
            if(gm->getPosition(gm, &sCur))
            {
				Hours = ((OMX_U64)sCur/1000000) / 3600;
				Minutes = ((OMX_U64)sCur/ (60*1000000)) % 60;
				Seconds = (((OMX_U64)sCur %(3600*1000000)) % (60*1000000))/1000000;

#if 1
                printf("\r[Current Media Time] %03d:%02d:%02d/%03d:%02d:%02d", 
                        Hours, Minutes, Seconds,
                        Hours1, Minutes1, Seconds1);
                fflush(stdout);
#endif
            }

			fsl_osal_sleep(500000);
        }
        else
            fsl_osal_sleep(50000);
    }

    return;
}

void OMX_CONF_ListComponents() 
{ 
#if 0
    OMX_ERRORTYPE  eError = OMX_ErrorNone; 
    OMX_U32 i = 0; 
    OMX_S8 cCompEnumName[OMX_MAX_STRINGNAME_SIZE]; 
    OMX_U8 cRoles[128];
    OMX_HANDLETYPE comp;
    void* privData;
    OMX_COMPONENTTYPE *hCompTypeHandle;
    OMX_U32 nIndex = 0;
  
    printf("\nAvailable Components:\n\n"); 

    /* Initialize OpenMax */ 
    if (OMX_ErrorNone == (eError = OMX_Init())) 
    { 
        while (OMX_ErrorNone == eError) 
        { 
            /* loop through all enumerated components to determine if the component name 
            specificed for the test is enumerated by the OMX core */ 
            eError = OMX_ComponentNameEnum((OMX_STRING) cCompEnumName, OMX_MAX_STRINGNAME_SIZE, i); 
            if (OMX_ErrorNone != eError)
                break;
            
            if (OMX_ErrorNone == OMX_GetHandle(&comp, (OMX_STRING)(cCompEnumName), 0x0, &gCallBacks))
            {
                printf("\tComponent name: %s\n", cCompEnumName); 
                fsl_osal_memset(cRoles,'0',128);
                
                hCompTypeHandle = (OMX_COMPONENTTYPE*)comp;
                if (NULL != hCompTypeHandle->ComponentRoleEnum)
                    if (OMX_ErrorNone == hCompTypeHandle->ComponentRoleEnum(comp,cRoles,nIndex))
                        printf("\tComponent role: %s\n", cRoles); 
                OMX_FreeHandle(comp);
                comp = NULL;
            }
            i++; 
        } 
    } 

    OMX_Deinit(); 
	#endif
} 

void gm_usage()
{
        printf("\n");
        printf("Usage\n");
        printf("Playback : omxgm_arm11_elinux input_file [--log_level 4] \n");
        printf("Thumbnail: omxgm_arm11_elinux input_file -thumb \n");
        printf("  [--log_file log_file] \n");
        printf("\n");
        printf("  omxgm_arm11_elinux -h         display the usage \n");
        printf("  omxgm_arm11_elinux -ls        list all components and it's role \n");
        printf("\n");
        printf("input_file:\n");
        printf("  The file to be playbacked. \n");
        printf("\n");
        printf("  use -ls to display all debug module name. For every component, the module name is \n");
        printf("  component role \n");
        printf("log_level:\n");
        printf("  The level of the debug (1 ~ 6). \n");
        printf("  1: The error messages will be printed. \n");
        printf("  2: The warning messages will be printed. \n");
        printf("  3: The informational messages will be printed. \n");
        printf("  4: The debugging messages will be printed. \n");
        printf("  5: The debugging buffer messages will be printed. \n");
        printf("  6: The log messages will be printed. \n");
        printf("  The log level can be set by envirenment varable LOG_LEVEL. \n");
        printf("  export LOG_LEVEL=4 \n");
        printf("log_file:\n");
        printf("  The debug information log file. The debug log file can be set by \n");
        printf("  envirenment varable LOG_FILE. export LOG_FILE=./debug_log.log \n");
        printf("Example: \n");
        printf("  omxgm_arm11_elinux input_file --log_level 6 \n");
        printf("  omxgm_arm11_elinux input_file --log_level 6 \n");
        printf("  --log_file debug_log.log \n");
        printf("playlist:\n");
        printf("Accept the input file as playlist format, each line for one clip path name\n")  ;
        printf("Example:\n");
        printf("omxgm_arm11_elinux input_file --playlist time:60 \n");
        printf("\t\t run the playlist for 60 minutes.\n");
        printf("omxgm_arm11_elinux input_file --playlist loop:10 \n");
        printf("\t\t run the playlist for 10 times.\n");
}
void save_to_jpeg(OMX_IMAGE_PORTDEFINITIONTYPE *pIn_fmt, OMX_CONFIG_RECTTYPE *pCropRect, OMX_U8 *in, OMX_IMAGE_PORTDEFINITIONTYPE *pOut_fmt, char *file_name);
void save_to_rgba(OMX_IMAGE_PORTDEFINITIONTYPE *pIn_fmt, OMX_CONFIG_RECTTYPE *pCropRect, OMX_U8 *in, OMX_IMAGE_PORTDEFINITIONTYPE *pOut_fmt);
OMX_BOOL seek_forward_test(OMX_GraphManager* gm, const char* filename);
OMX_BOOL seek_backward_test(OMX_GraphManager* gm, const char* filename);
OMX_BOOL audio_sanity_test(OMX_GraphManager* gm, const char* filename);
OMX_BOOL seek_snapshot_test(OMX_GraphManager* gm, const char* filename);
OMX_BOOL audio_skip_test(OMX_GraphManager* gm, const char* filename);
OMX_BOOL ff_test(OMX_GraphManager* gm, const char* filename);
OMX_BOOL rwd_test(OMX_GraphManager* gm, const char* filename);

OMX_U32 pxlfmt2bpp_gmapp(OMX_COLOR_FORMATTYPE omx_pxlfmt)
{
	OMX_U32 bpp; // bit per pixel

	switch(omx_pxlfmt) {
	case OMX_COLOR_FormatMonochrome:
	  bpp = 1;
	  break;
	case OMX_COLOR_FormatL2:
	  bpp = 2;
      break;
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

char *
GetNameByIndex(PlayItem * item, char * buffer)
{
    int idx;
    char * retbuf=NULL;
    FILE * fp=fopen(item->name, "r");
    
    if (fp==NULL)
        return NULL;
    
    idx=item->curIndex;
    //printf("get %s idx %d\n", item->name, idx);
    if (idx<0){
        idx = 0;
        while(1){
            buffer[0] = '\0';
            if (fscanf(fp, "%s\n", buffer)==EOF){
                fseek(fp, 0, SEEK_SET);
                break;
            }
            if ((buffer[0]=='#')||(buffer[0]==' ')){
                continue;
            }
            idx++;
        }
        item->curIndex=idx;
    }

    while(idx>0){
        buffer[0] = '\0';
        if (fscanf(fp, "%s\n", buffer)==EOF){
            //item->curIndex--;
            fclose(fp);
            return NULL;
        }
        if ((buffer[0]=='#')||(buffer[0]==' ')){
            continue;
        }
        retbuf=buffer;
        idx--;
    }
    fclose(fp);
    return retbuf;

    
}



char * 
GetPlayItemName(GmPlayItems * gmPIs, PlayItemDirection direction, char * buffer)
{
    PlayItem * item;
    char * retbuf = NULL;
    item=gmPIs->curPlayItem;
    
    if (direction==DIRECT_CURRENT){
        if (item->type==PLAYLIST){
            return GetNameByIndex(item, buffer);
        }else{
            strcpy(buffer, item->name);
            return buffer;
        }
    }else{
        while(item){
            if (item->type==PLAYLIST){
                if (direction==DIRECT_PREVIOUS){
                    item->curIndex--;
                }else{
                    item->curIndex++;
                }
                retbuf=GetNameByIndex(item, buffer);
                if (retbuf)
                    return retbuf;
                if (direction==DIRECT_PREVIOUS){
                    item=item->prev;
                }else{
                    item=item->next;
                }
                if (item){
                    item->curIndex=0;
                    gmPIs->curPlayItem=item;
                    if (item->type==PLAYFILE){
                        strcpy(buffer, item->name);
                        return buffer;
                    }
                }
            }else{
                if (direction==DIRECT_PREVIOUS){
                    item=item->prev;
                }else{
                    item=item->next;
                }
                if (item){
                    item->curIndex=0;
                    gmPIs->curPlayItem=item;
                    if (item->type==PLAYFILE){
                        strcpy(buffer, item->name);
                        return buffer;
                    }
                }
            }
        };
        //eos or bos
        if (direction==DIRECT_PREVIOUS){
            gmPIs->curPlayItem->curIndex++;
        }else{
            gmPIs->curPlayItem->curIndex--;
        }

        if (gmPIs->repeat){
            if (direction==DIRECT_PREVIOUS){
                SetPlayItem(gmPIs, OMX_FALSE);
            }else{
                SetPlayItem(gmPIs, OMX_TRUE);
            }
            retbuf = GetPlayItemName(gmPIs,DIRECT_CURRENT , buffer);
        }
        return retbuf;
    }
}

void SetPlayItem(GmPlayItems * gmPIs, OMX_BOOL setBegin)
{
    if (setBegin){
        gmPIs->curPlayItem=gmPIs->playItemsHead;
        gmPIs->curPlayItem->curIndex=1;
    }else{
        gmPIs->curPlayItem=gmPIs->playItemsTail;
        gmPIs->curPlayItem->curIndex=-1;
    }
}

void InitPlayItems(GmPlayItems * gmPIs)
{
    gmPIs->curPlayItem=gmPIs->playItemsHead=gmPIs->playItemsTail=NULL;
    gmPIs->repeat=OMX_FALSE;
}

void DeinitPlayItems(GmPlayItems * gmPIs)
{
    PlayItem * item=gmPIs->playItemsHead, * itemnext;
    while(item){
        if (item->buffer){
            fsl_osal_dealloc(item->buffer);
        }
        itemnext=item->next;
        fsl_osal_dealloc(item);
        item=itemnext;
    }
    gmPIs->playItemsHead=gmPIs->playItemsTail=gmPIs->curPlayItem = NULL;
}


PlayItem *
AddPlayItem(GmPlayItems * gmPIs, char * name, OMX_BOOL forcelist)
{
    PlayItem * tmp= fsl_osal_malloc_new(sizeof(PlayItem));
    int slen;
    if (tmp){
        tmp->buffer = NULL;
        if ((forcelist)||(((slen=strlen(name))>5)&&(strcmp(name+slen-5, ".list")==0))){
            char  buffer[FILE_NAME_LEN];
            tmp->curIndex=1;
            tmp->name=name;
            if (GetNameByIndex(tmp, buffer)==NULL){
                fsl_osal_dealloc(tmp);
                tmp=NULL;
                return tmp;
            }
            tmp->type=PLAYLIST;
        }else{
            tmp->type=PLAYFILE;
        }
        tmp->prev = gmPIs->playItemsTail;
        tmp->next = NULL;
        tmp->curIndex=1;
        tmp->name=name;
        if (gmPIs->playItemsHead==NULL){
            gmPIs->playItemsHead = gmPIs->playItemsTail = gmPIs->curPlayItem = tmp;
        }else{
            gmPIs->playItemsTail->next = tmp;
            gmPIs->playItemsTail = tmp;
        }
    }
    return tmp;
}

void
AddPlayItemDir(GmPlayItems * gmPIs, char * name)
{
    int n, i;
    struct dirent **namelist = NULL;
    PlayItem * tmp;
    char * buffer;
        
    n = scandir(name, &namelist, 0, alphasort);  

    if (namelist==NULL)
        return;

    for (i=0;i<n;i++){
        if (namelist[i]){
            if ((namelist[i]->d_type!=0x8)
                ||(namelist[i]->d_name[0]=='.')){
                fsl_osal_dealloc(namelist[i]);
                continue;
            }

            buffer = fsl_osal_malloc_new(strlen(name)+strlen(namelist[i]->d_name)+2);
            if (buffer==NULL){
                fsl_osal_dealloc(namelist[i]);
                continue;
            }
            sprintf(buffer, "%s/%s", name, namelist[i]->d_name);
            tmp = AddPlayItem(gmPIs, buffer, OMX_FALSE);
            if (tmp){
                tmp->buffer=buffer;
            }else
                fsl_osal_dealloc(buffer);
            fsl_osal_dealloc(namelist[i]);
        }
        
    }
    fsl_osal_dealloc(namelist);
    

}
void
SetPlayItemRepeatMode(GmPlayItems  * gmPIs,OMX_BOOL repeat){
   gmPIs->repeat = repeat;
}

void gm_main_menu()
{
        printf("\nSelect command:\n");
        printf("\t[p]Play\n");
        printf("\t[i]Info\n");
        printf("\t[a]Pause/Resume\n");
        printf("\t[e]Seek\n");
        printf("\t[c]Speed Control\n");
        printf("\t[s]Stop\n");
        printf("\t[d]Audio Track Selection\n");
        printf("\t[f]Audio Effect\n");
        printf("\t[v]Audio PostProcess Component Add/Remove\n");
        printf("\t[+]Volume Up\n");
        printf("\t[-]Volume Down\n");
        //printf("\t[z]Display Region\n");
        //printf("\t[w]Video Crop Retangle\n");
        printf("\t[t]Rotate\n");
        printf("\t[y]Screen Mode\n");
        //printf("\t[o]Video Output\n");
        printf("\t[n]Snapshot\n");
        printf("\t[u]Thumbnail\n");
        printf("\t[x]Exit\n\n");
            
}

int
gm_parse_options(int argc, char* argv[], GM_Options * pOpt)
{
    int ret = 0;
    int i;
    int parseindex = 0;

    memset(pOpt, 0, sizeof(GM_Options));
    for (i=1;i<argc;i++){
        if ((strlen(argv[i])) && (argv[i][0]=='-')){
            if ((strcmp(argv[i], "-h")==0)||(strcmp(argv[i], "--help")==0)){
                ret = 1;
                goto parseOptionErr;
            }
            if ((strcmp(argv[i], "-l")==0)||(strcmp(argv[i], "--list-modules")==0)){
                ret = 2;
                goto parseOptionErr;
            }
            if ((strcmp(argv[i], "-r")==0)||(strcmp(argv[i], "--repeat")==0)){
                SetPlayItemRepeatMode(&pOpt->gpi, OMX_TRUE);
                parseindex=0;
                continue;
            }
            if ((strcmp(argv[i], "-p")==0)||(strcmp(argv[i], "--playback")==0)){
                pOpt->startPlay=OMX_TRUE;
                parseindex=0;
                continue;
            }

            if ((strcmp(argv[i], "-d")==0)||(strcmp(argv[i], "--dir")==0)){
                parseindex=4;
                continue;
            }

            if ((strcmp(argv[i], "--play-list")==0)){
                parseindex=3;
                continue;
            }

            if ((strcmp(argv[i], "--aging")==0)){
                pOpt->aging=OMX_TRUE;
                continue;
            }

            if ((strcmp(argv[i], "--log_level")==0)){
                parseindex=1;
                continue;
            }

            if ((strcmp(argv[i], "--log_file")==0)){
                parseindex=2;
                continue;
            }

            if ((strcmp(argv[i], "--quiet")==0)){
                pOpt->quiet = OMX_TRUE;
                continue;
            }
        
            parseindex=0;
            continue;
        }

        switch (parseindex){
            case 1:
                pOpt->debugLevel=argv[i];
                parseindex=0;
                continue;
                break;
            case 2:
                pOpt->debugFile=argv[i];
                parseindex=0;
                continue;
                break;
            case 3:
                AddPlayItem(&pOpt->gpi, argv[i],OMX_TRUE);
                parseindex=0;
                continue;
                break;
            case 4:
                AddPlayItemDir(&pOpt->gpi, argv[i]);
                parseindex=0;
                continue;
                break;
            default:
                break;
        }

        AddPlayItem(&pOpt->gpi, argv[i],OMX_FALSE);


    }
    if (pOpt->gpi.curPlayItem==NULL)
        ret = -1;

    return ret;
    
parseOptionErr:
    DeinitPlayItems(&pOpt->gpi);
    return ret;
}

void ListAudioEffect()
{
	printf("1 acoustic\n"); 
	printf("2 bass_booster\n");
	printf("3 bass_reducer\n");
	printf("4 classical\n");
	printf("5 dance\n");
	printf("6 deep\n");
	printf("7 electronic\n");
	printf("8 gipop\n");
	printf("9 jazz\n");
	printf("10 latin\n");
	printf("11 loudness\n");
	printf("12 lounge\n");
	printf("13 piano\n");
	printf("14 pop\n");
	printf("15 r_b\n");
	printf("16 rock\n");
	printf("17 small_speakers\n");
	printf("18 spoken_word\n");
	printf("19 treble_booster\n");
	printf("20 treble_reducer\n");
	printf("21 vocal_booster\n");
	printf("22 flat\n");
}	


            
int main(int argc, char* argv[])
{
    OMX_GraphManager* gm = NULL;
    OMX_BOOL bexit = OMX_FALSE;
    OMX_BOOL bPlaylist = OMX_FALSE;
    OMX_S32 file_idx = 0;

    
    char *pFilename, pFileNameBuf[FILE_NAME_LEN];
    pFilename=pFileNameBuf;
    char rep[128];
    OMX_BOOL readinput = OMX_TRUE;
    OMX_BOOL bPause = OMX_FALSE;

    GM_Options options;
    
    int ret;
    struct sigaction act;
    act.sa_handler = my_int_signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

#ifdef FSL_MEM_CHECK
    init_mm_mutex();
#endif
    InitPlayItems(&options.gpi);
    ret = gm_parse_options(argc,argv,&options);

    if (ret<0){
        return ret;
    }else if (ret==1){
        gm_usage();
        return ret;
    }else if (ret==2)    {
        OMX_CONF_ListComponents();
        return ret;
    }
	if (options.debugLevel){
		LogInit(fsl_osal_atoi(options.debugLevel), options.debugFile);
    }else{
		LogInit(-1, options.debugFile);
    }

    gm = OMX_GraphManagerCreate();
    if (gm == NULL)
        goto exitMain;

    printf("create gm successfully\n");

    gm->registerEventHandler(gm, gm, eventhandler);
#ifdef MFW_AVI_SUPPORT_DIVX_DRM
	/*GUI player that use graph manage should redefine the function cb_drm_app(),
	  and give the handle of the GUI player as the second param.*/
	gm->setdivxdrmcallback(gm, NULL,cb_drm_app,(void*)&divx_drm_func);
#endif

    gm->setVideoRenderType(gm, OMX_VIDEO_RENDER_IPULIB);

    if (options.aging){
        rep[0] = 'a';
        readinput = OMX_FALSE;
    }else if (options.startPlay){
        rep[0] = 'p';
        readinput = OMX_FALSE;
    }

    START_PLAYTIME_INFO_THREAD(thread, gm);

    while(bexit == OMX_FALSE) {
		

		{
		if (readinput){
        gm_main_menu();
        scanf("%s", rep);
        }
        readinput=OMX_TRUE;
        if  (quitflag)
        {
            printf("receive Ctrl+c user input.\n");
			STOP_SHOW_PLAYTIME_INFO;
            gm->stop(gm);
			gm->deleteIt(gm);        
			goto exitMain;
		}
        if(rep[0] == 'p') {

#if 0
            if (((OMX_GraphManagerPrivateData*)gm->pData)->video_FF_EOS == OMX_TRUE ||
                ((OMX_GraphManagerPrivateData*)gm->pData)->video_REW_EOS == OMX_TRUE)
            {
                gm_seek(gm, OMX_TIME_SeekModeFast, 0);
                gm->resume(gm);
                ((OMX_GraphManagerPrivateData*)gm->pData)->video_FF_EOS = OMX_FALSE;
                ((OMX_GraphManagerPrivateData*)gm->pData)->video_REW_EOS = OMX_FALSE;
                continue;
            }
#endif
            pFilename = GetPlayItemName(&options.gpi, DIRECT_CURRENT, pFileNameBuf);
#ifdef TEST_LOADING_TIME
            struct timeval tv, tv1;
            gettimeofday (&tv, NULL);
#endif
TRY_AGAIN:            
            gm->AddRemoveAudioPP(gm, OMX_TRUE);
            ret = gm->load(gm, pFilename, strlen(pFilename));
            if (ret == OMX_FALSE)
            {
                printf("load file %s failed.\n", pFilename);
                gm->stop(gm);
                if (bDrmRentalNeedSecondLoad)
                {
                    printf("Rental File, remained %d, Preceed to play?[0/1]\n", rental_count_remained);
                    int user_input;
                    char rep[128];
                    scanf("%s",rep);
                    user_input = atoi(rep);
                    if (user_input == 1)
                    {
                        printf("%s, user confirmed.\n",__FUNCTION__);
                        goto TRY_AGAIN;
                    }
                    else
                    {
                        printf("%s, user canceled.\n",__FUNCTION__);
                        continue;
                        gm->deleteIt(gm);        
                        goto exitMain;
                    }   
                }
                if(bPlaylist) {
                    continue;
                }
                else {
                    //continue;
                    gm->deleteIt(gm);        
                    goto exitMain;
                }
            }

#if 0
            printf("input play satrt position(seconds):");
            scanf("%d", &start_pos);
            if(start_pos > 0)
                gm->seek(gm, OMX_TIME_SeekModeAccurate, start_pos*OMX_TICKS_PER_SECOND);
#endif
//#define SET_TV_OUT_ON_LAYER2_FIRST
#if 0
            /*if you want test set tv out before start, use these function call*/
#ifdef SET_TV_OUT_ON_LAYER2_FIRST
				gm->settvout(gm,OMX_TRUE, NTSC, OMX_TRUE);
	#else
                gm->settvout(gm,OMX_FALSE, NV_MODE, OMX_TRUE);
#endif
#endif
            bPause = OMX_FALSE;
            gm->start(gm);
#ifdef TEST_LOADING_TIME
            gettimeofday (&tv1, NULL);
            printf("loading time: %d\n", (tv1.tv_sec-tv.tv_sec)*1000+(tv1.tv_usec-tv.tv_usec)/1000) ;
#endif
			START_SHOW_PLAYTIME_INFO;
			printf("start play %s\n", pFilename);
        }
        else if(rep[0] == '>') {
                
            STOP_SHOW_PLAYTIME_INFO;
            gm->stop(gm);


            pFilename = GetPlayItemName(&options.gpi, DIRECT_NEXT, pFileNameBuf);
            if(pFilename == NULL) {
                printf("End of the playlist.\n");
                continue;
            }
            ret = gm->load(gm, pFilename, strlen(pFilename));
            if (ret == OMX_FALSE)
            {
                printf("load file %s failed.\n", pFilename);
                gm->stop(gm);
                continue;
            }
            gm->start(gm);
            START_SHOW_PLAYTIME_INFO;
            
            printf("start play %s\n", pFilename);
        }
        else if(rep[0] == '<') {
                
            STOP_SHOW_PLAYTIME_INFO;
            gm->stop(gm);


            pFilename = GetPlayItemName(&options.gpi, DIRECT_PREVIOUS, pFileNameBuf);
            if(pFilename== NULL) {
                    printf("End of the playlist.\n");
                    continue;
            }
            ret = gm->load(gm, pFilename, strlen(pFilename));
            if (ret == OMX_FALSE)
            {
                printf("load file %s failed.\n", pFilename);
                gm->stop(gm);
                continue;
            }
            gm->start(gm);
            START_SHOW_PLAYTIME_INFO;
            printf("start play %s\n", pFilename);
        }
#if 0
        else if(rep[0] == 'g') {
            bAgingtest = OMX_TRUE;
            int round = 0, nn = 0;
            fsl_osal_sem_init(&gplaytestsem, 0, 0);
            if(1){//(bPlaylist) {
                pFilename = GetPlayItemName(&options.gpi, DIRECT_CURRENT, pFileNameBuf);
                while(pFilename) {
                    
                    ret = gm->load(gm, pFilename, strlen(pFilename));
                    if (ret == OMX_FALSE)
                    {
                        printf("load file %s failed.\n", pFilename);
                        gm->stop(gm);
                        pFilename = GetPlayItemName(&options.gpi, DIRECT_NEXT, pFileNameBuf);
                        continue;
                    }

                    if ((options.gpi.curPlayItem==options.gpi.playItemsHead) && 
                        (options.gpi.curPlayItem->curIndex<2)){
                        nn++;
                        round=0;
                    }
                    printf("aging file [%d:%d]. %s\n", nn, ++round, pFilename);
                    gm->start(gm);
                    fsl_osal_sem_wait(gplaytestsem);
                    if(quitflag == 1)
                        break;
                    pFilename = GetPlayItemName(&options.gpi, DIRECT_NEXT, pFileNameBuf);
                }
            }
            #if 0
            else {
                for(;;) {
                    ret = gm->load(gm, filename, strlen(filename));
                    if (ret == OMX_FALSE)
                    {
                        FILE *fp_timeout = fopen("timeout.log", "at");
                        gm->stop(gm);
                        if(fp_timeout != NULL) {
                            fprintf(fp_timeout, "The %lld time load file %s failed.\n", ++gtimoutcnt, filename);
                            fclose(fp_timeout);
                        }
                        continue;
                    }
                    printf("%lld, load successful!\n", ++gtestcnt);
                    gm->start(gm);
                    printf("%lld, start successful!\n", gtestcnt);
                    fsl_osal_sem_wait(gplaytestsem);
                    printf("%lld, receive EOS command!\n", gtestcnt);

                    if(quitflag == 1)
                        break;
                }
            }
            #endif
            fsl_osal_sem_destroy(gplaytestsem);
        }
#endif
        else if(rep[0] == 'i') {
            GM_STREAMINFO sStreamInfo;
            printf("\n");
            if(gm->getStreamInfo(gm, GM_AUDIO_STREAM_INDEX, &sStreamInfo))
            {
                char *format = NULL;
                if(sStreamInfo.streamFormat.audio_info.eEncoding == OMX_AUDIO_CodingMP3)
                    format = "MP3";
                else if(sStreamInfo.streamFormat.audio_info.eEncoding == OMX_AUDIO_CodingAAC)
                    format = "AAC";
                //else if(sStreamInfo.streamFormat.audio_info.eEncoding == OMX_AUDIO_CodingBSAC)
                //    format = "BSAC";
                else if(sStreamInfo.streamFormat.audio_info.eEncoding == OMX_AUDIO_CodingAC3)
                    format = "AC3";
                else if(sStreamInfo.streamFormat.audio_info.eEncoding == OMX_AUDIO_CodingWMA)
                    format = "WMA";
                else if(sStreamInfo.streamFormat.audio_info.eEncoding == OMX_AUDIO_CodingAMR)
                    format = "AMR";
                else if(sStreamInfo.streamFormat.audio_info.eEncoding == OMX_AUDIO_CodingFLAC)
                    format = "FLAC";
                else if(sStreamInfo.streamFormat.audio_info.eEncoding == OMX_AUDIO_CodingVORBIS)
                    format = "VORBIS";		   
                else if(sStreamInfo.streamFormat.audio_info.eEncoding == OMX_AUDIO_CodingPCM)
                    format = "PCM";
                else if(sStreamInfo.streamFormat.audio_info.eEncoding == OMX_AUDIO_CodingRA)
                    format = "REAL";
                printf("[Audio] format: %s, channels: %d, sample rate: %d, bit rate: %d\n",
                        format, 
                        sStreamInfo.streamFormat.audio_info.nChannels,
                        sStreamInfo.streamFormat.audio_info.nSampleRate,
                        sStreamInfo.streamFormat.audio_info.nBitRate);
            }

            if(gm->getStreamInfo(gm, GM_VIDEO_STREAM_INDEX, &sStreamInfo))
            {
                char *format = NULL;
                if(sStreamInfo.streamFormat.video_info.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
                    format = "MPEG4";
                else if(sStreamInfo.streamFormat.video_info.eCompressionFormat == OMX_VIDEO_CodingAVC)
                    format = "AVC";
                else if(sStreamInfo.streamFormat.video_info.eCompressionFormat == OMX_VIDEO_CodingWMV)
                    format = "WMV";
                else if(sStreamInfo.streamFormat.video_info.eCompressionFormat == OMX_VIDEO_CodingDIVX)
                    format = "DIVX";
                else if(sStreamInfo.streamFormat.video_info.eCompressionFormat == OMX_VIDEO_CodingDIV3)
                    format = "DIV3";    
                else if(sStreamInfo.streamFormat.video_info.eCompressionFormat == OMX_VIDEO_CodingH263)
                    format = "H263";    
                else if(sStreamInfo.streamFormat.video_info.eCompressionFormat == OMX_VIDEO_CodingDIV4)
                    format = "DIV4";    
                else if(sStreamInfo.streamFormat.video_info.eCompressionFormat == OMX_VIDEO_CodingXVID)
                    format = "XVID";
                else if(sStreamInfo.streamFormat.video_info.eCompressionFormat == OMX_VIDEO_CodingRV)
                    format = "REAL";
                else if(sStreamInfo.streamFormat.video_info.eCompressionFormat == OMX_VIDEO_CodingWMV)
                    format = "WMV78";
                else if(sStreamInfo.streamFormat.video_info.eCompressionFormat == OMX_VIDEO_CodingWMV9)
                    format = "WMV9";
               else if(sStreamInfo.streamFormat.video_info.eCompressionFormat == OMX_VIDEO_CodingMJPEG)
		      format = "MJPEG";				

                printf("[Video] format: %s, width: %d, height: %d, frame rate: %3.1f\n",
                        format, 
                        sStreamInfo.streamFormat.video_info.nFrameWidth,
                        sStreamInfo.streamFormat.video_info.nFrameHeight,
                        sStreamInfo.streamFormat.video_info.xFramerate);
            }
        }
        else if(rep[0] == 'a') {
            if(bPause != OMX_TRUE) {
                gm->pause(gm);
                bPause = OMX_TRUE;
            }
            else {
                gm->resume(gm);
                bPause = OMX_FALSE;
            }
        }
        else if(rep[0] == 'e') {
            OMX_U32 mode, pos;
            OMX_TIME_SEEKMODETYPE seek_mode;
            OMX_TICKS sDur, time;

            printf("\nSeek mode [0 - Fast, 1 - Accurate]:\n");
            scanf("%s",rep);
            mode = atoi(rep);
            seek_mode = mode>0 ? OMX_TIME_SeekModeAccurate : OMX_TIME_SeekModeFast;

            if(seek_mode == OMX_TIME_SeekModeFast) {
                printf("\nSeek to percentage [0:100]: \n");
                scanf("%s",rep);
                pos = atoi(rep);
                if(pos > 100)
                    pos = 100;
                if(pos < 0)
                    pos = 0;

                printf("Want seek to percentage: %d\n", pos);
                gm->getDuration(gm, &sDur);
                time = sDur*pos/100;
            }

            if(seek_mode == OMX_TIME_SeekModeAccurate) {
                printf("\nSeek to second: \n");
                scanf("%s",rep);
                pos = atoi(rep);

                OMX_S32 Hours = pos / 3600;
                OMX_S32 Minutes = (pos/ 60) % 60;
                OMX_S32 Seconds = ((pos % 3600) % 60);
                printf("Want seek to %d:%d:%d\n", Hours, Minutes, Seconds);

                time = pos * OMX_TICKS_PER_SECOND;
            }

            gm->seek(gm, seek_mode, time);
        }
            else if(rep[0] == 'c') {
                OMX_S32 speed;
                float inputspeed;
                printf("\nSet speed to [-16, 16]: \n");
                scanf("%s",rep);	
                inputspeed = atof(rep);
                speed = (int)(inputspeed * Q16_SHIFT); // Q16
                gm->setPlaySpeed(gm, speed);

            }
        else if(rep[0] == 's') {
            STOP_SHOW_PLAYTIME_INFO;
            gm->stop(gm);
        }
        else if(rep[0] == 'd') {
			OMX_U32 nAudioTrackNum, nCurrentTrack, nSelectedTrack;
            nAudioTrackNum = gm->GetAudioTrackNum(gm);
            nCurrentTrack = gm->GetCurAudioTrack(gm);
			printf("\nAudio Track Number: %d \nCurrent Audio Track: %d \nPlease select the track [%d, %d]: \n", \
					nAudioTrackNum, nCurrentTrack, nAudioTrackNum?1:0, nAudioTrackNum);
            scanf("%s",rep);
			nSelectedTrack = atoi(rep);
			if (nSelectedTrack >= 1 && nSelectedTrack <= nAudioTrackNum)
			{
				gm->SelectAudioTrack(gm, nSelectedTrack);
			}
			else
				printf("\nInput audio track is out of range.\n");
        }
        else if(rep[0] == 'f') {
			OMX_BOOL bAudioEffect;

			printf("\nEnable or disable audio PEQ [Enable:1/Disable:0]: \n");
            scanf("%s",rep);
			bAudioEffect = atoi(rep);

			if (bAudioEffect)
			{
				OMX_U32 nAudioEffect;

				gm->EnableDisablePEQ(gm, bAudioEffect);
				ListAudioEffect();
				printf("\nPlease select the audio effect: \n");
				scanf("%s",rep);
				nAudioEffect = atoi(rep);

				if (nAudioEffect >= 1 && nAudioEffect <= SCENESNUM)
				{
					OMX_U32 i;
					nAudioEffect -= 1;
					for (i = 0; i < gm->GetBandNumPEQ(gm); i ++)
					{
						gm->SetAudioEffectPEQ(gm, i, scene[nAudioEffect].filterPara[i].Fc/10, \
								scene[nAudioEffect].filterPara[i].Gain);
					}
				}
				else
					printf("\nAudio effect number is out of range.\n");
			}
			else
				gm->EnableDisablePEQ(gm, bAudioEffect);
		}
        else if(rep[0] == 'v') {
			OMX_BOOL bAddComponent;
			printf("\nAudio PostProcess Component Add/Remove [Add:1/Remove:0]: \n");
            scanf("%s",rep);
			bAddComponent = atoi(rep);
            gm->AddRemoveAudioPP(gm, bAddComponent);
        }
		else if(rep[0] == '+') {
            ret = gm->AdjustAudioVolume(gm, OMX_TRUE);
        }
		else if(rep[0] == '-') {
            ret = gm->AdjustAudioVolume(gm, OMX_FALSE);
        }
            else if(rep[0] == 'z') {
                OMX_CONFIG_RECTTYPE sRect;
                printf("\nSet display region: x,y,width,height \n");
                scanf("%d,%d,%d,%d", &sRect.nLeft, &sRect.nTop, &sRect.nWidth, &sRect.nHeight);
                gm->setDisplayRect(gm, &sRect);
            }
            else if(rep[0] == 'w') {
                OMX_CONFIG_RECTTYPE sRect;
                printf("\nSet video crop rectangle: x,y,width,height \n");
                scanf("%d,%d,%d,%d", &sRect.nLeft, &sRect.nTop, &sRect.nWidth, &sRect.nHeight);
                gm->setVideoRect(gm, &sRect);
            }
 	    else if(rep[0] == 't') {
			OMX_S32 rotate;
			printf("\nSet Rotate to [0(None), 4(clockwise 90), 3(Clockwize 180) ,7(clockwise 270)]:\n");
			scanf("%s",rep);
			rotate = atoi(rep);
            gm->rotate(gm, rotate);
        }
		else if(rep[0] == 'y') {
            GM_VIDEO_MODE vmode;
            printf("\nselect video display mode? [0(Normal), 1(Fullscreen), 2(Zoom)] :\n");
            scanf("%s",rep);
			vmode = atoi(rep);
            gm->setvideomode(gm, vmode);
        }
        else if(rep[0] == 'o') {
            GM_VIDEO_DEVICE vdevice;
            printf("\nselect device type? [0(LCD), 1(NTSC), 2(PAL)] :\n");
            scanf("%s",rep);
			vdevice = atoi(rep);
            gm->setvideodevice(gm, vdevice, OMX_TRUE);
        }
        else if(rep[0] == 'n') {
            //get the video format
            OMX_IMAGE_PORTDEFINITIONTYPE in_format;
            OMX_CONFIG_RECTTYPE sCropRect;
            gm->getScreenShotInfo(gm, &in_format, &sCropRect);

            //allocate buffer for save snapshot picture
            OMX_U8 *snapshot_buf = NULL;
#ifdef USE_OLD_FSL_OSAL
            fsl_osal_mem_req mem;
            mem.align = 4;
            mem.mem_type_speed = E_FSL_FAST_MEMORY;
            mem.size = in_format.nFrameWidth * in_format.nFrameHeight
                       * pxlfmt2bpp_gmapp(in_format.eColorFormat)/8;
            snapshot_buf = FSL_Alloc(&mem);
            fsl_osal_memset(snapshot_buf, 0, mem.size);
#else 
            int size = in_format.nFrameWidth * in_format.nFrameHeight
                       * pxlfmt2bpp_gmapp(in_format.eColorFormat)/8;

            snapshot_buf = fsl_osal_malloc_new(size);
            fsl_osal_memset(snapshot_buf, 0, size);
#endif

            //get snapshot
            gm->getSnapshot(gm, snapshot_buf);

            //convert thumbnail picture to required format and resolution
            OMX_IMAGE_PORTDEFINITIONTYPE out_format;
            fsl_osal_memset(&out_format, 0, sizeof(OMX_IMAGE_PORTDEFINITIONTYPE));

            out_format.nFrameWidth = in_format.nFrameWidth;
            out_format.nFrameHeight = in_format.nFrameHeight;

            //save_to_rgba(&in_format, &sCropRect, snapshot_buf, &out_format);
            save_to_jpeg(&in_format, &sCropRect, snapshot_buf, &out_format, pFilename);

            fsl_osal_dealloc(snapshot_buf);
        }
        else if(rep[0] == 'u') {
            gm->disableStream(gm, GM_AUDIO_STREAM_INDEX);
            pFilename = GetPlayItemName(&options.gpi, DIRECT_CURRENT, pFileNameBuf);
            ret = gm->load(gm, pFilename, strlen(pFilename));
            if (ret == OMX_FALSE)
            {
                printf("load file %s failed.\n", pFilename);
                gm->stop(gm);
                gm->deleteIt(gm);        
                goto exitMain;
            }

            //get the thumbnail from position
            printf("Postion(in seconds) of the thumbnail:");
            OMX_U32 pos;
            scanf("%d", &pos);

            //get the video format
            OMX_IMAGE_PORTDEFINITIONTYPE in_format;
            OMX_CONFIG_RECTTYPE sCropRect;
            gm->getScreenShotInfo(gm, &in_format, &sCropRect);

            //allocate buffer for save thumbnail picture
            OMX_U8 *thumb_buf = NULL;
#ifdef USE_OLD_FSL_OSAL
            fsl_osal_mem_req mem;
            mem.align = 4;
            mem.mem_type_speed = E_FSL_FAST_MEMORY;
            mem.size = in_format.nFrameWidth * in_format.nFrameHeight
                       * pxlfmt2bpp_gmapp(in_format.eColorFormat)/8;
            thumb_buf = FSL_Alloc(&mem);
            fsl_osal_memset(thumb_buf, 0, mem.size);
#else 
            int size = in_format.nFrameWidth * in_format.nFrameHeight
                       * pxlfmt2bpp_gmapp(in_format.eColorFormat)/8;

            thumb_buf = fsl_osal_malloc_new(size);
            fsl_osal_memset(thumb_buf, 0, size);
#endif 

            //get thumbnail
            gm->getThumbnail(gm, (OMX_TICKS)(pos*OMX_TICKS_PER_SECOND), thumb_buf);

            //convert thumbnail picture to required format and resolution
            OMX_IMAGE_PORTDEFINITIONTYPE out_format;
            fsl_osal_memset(&out_format, 0, sizeof(OMX_IMAGE_PORTDEFINITIONTYPE));

            printf("Fomat for thumbnai(width, height):");
            scanf("%d,%d", &(out_format.nFrameWidth), &(out_format.nFrameHeight));
            printf("pic_width: %d, pic_height: %d\n", out_format.nFrameWidth, out_format.nFrameHeight);

            save_to_jpeg(&in_format, &sCropRect, thumb_buf, &out_format, pFilename);

            fsl_osal_dealloc(thumb_buf);
            gm->stop(gm);
        }
        else if(rep[0] == 'x')
        {
			STOP_SHOW_PLAYTIME_INFO;
            gm->stop(gm);
            gm->deleteIt(gm);        
            bexit = OMX_TRUE;
        }
		else if(rep[0] == '*') {
            sleep(1);
        }
		else if(rep[0] == '#') {
            sleep(10);
        }
	}
	}

exitMain:

	STOP_PLAYTIME_INFO_THREAD(thread);
	DeinitPlayItems(&options.gpi);
#ifdef FSL_MEM_CHECK
	print_non_free_resource();
#endif

	LogDeInit();

	return 0;
}


OMX_ERRORTYPE MakeThumbnailFileName(char *media_file_name, char *thumbnail_file_name, OMX_S32 index)
{
    OMX_STRING thumb_path = NULL;
    char * file_name = thumbnail_file_name;

    thumb_path = fsl_osal_getenv_new("FSL_OMX_THUMB_PATH");
    if(thumb_path != NULL) {
        OMX_STRING ptr, ptr1;
        ptr = fsl_osal_strrchr(media_file_name, '/') + 1;
        if ((OMX_STRING)0x1 == ptr)
            ptr = media_file_name;
        strcpy(file_name, thumb_path);
        ptr1 = file_name + strlen(file_name);
        ptr1[0] = '/';
        fsl_osal_strcpy(ptr1+1, ptr);
        ptr1 = fsl_osal_strrchr(file_name, '.');
        sprintf(ptr1, "%d", index);
        fsl_osal_strcpy(ptr1+1, ".jpg");
    }
    else {
        /* get idx file from the same folder of media file */
        OMX_STRING ptr, ptr1;
        strcpy(file_name, media_file_name);
        ptr = fsl_osal_strrchr(media_file_name, '/') + 1;
        if ((OMX_STRING)0x1 == ptr)
            ptr = media_file_name;

        ptr1 = fsl_osal_strrchr(file_name, '/') + 1;
        if ((OMX_STRING)0x1 == ptr1)
            ptr1 = file_name;

        fsl_osal_strcpy(ptr1+1, ptr);
        ptr1 = fsl_osal_strrchr(file_name, '.');
        sprintf(ptr1, "%d", index);
        fsl_osal_strcpy(ptr1+1, ".jpg");
    }

    return OMX_ErrorNone;
}

void save_to_jpeg(OMX_IMAGE_PORTDEFINITIONTYPE *pIn_fmt, OMX_CONFIG_RECTTYPE *pCropRect, OMX_U8 *in, OMX_IMAGE_PORTDEFINITIONTYPE *pOut_fmt, char *file_name)
{
    OMX_ImageConvert* ic;
    ic = OMX_ImageConvertCreate();
    if(ic == NULL) {
        printf("failed to load image converter.\n");
        return;
    }

    pOut_fmt->eColorFormat = OMX_COLOR_FormatYUV420Planar;

    //alloc memory to save converted image
    OMX_U8 *pic_buf = NULL;
#ifdef USE_OLD_FSL_OSAL
    fsl_osal_mem_req mem;
    mem.align = 4;
    mem.mem_type_speed = E_FSL_FAST_MEMORY;
    mem.size = pOut_fmt->nFrameWidth * pOut_fmt->nFrameHeight
        * pxlfmt2bpp_gmapp(pOut_fmt->eColorFormat)/8;
    pic_buf = FSL_Alloc(&mem);
    fsl_osal_memset(pic_buf, 0, mem.size);
#else 
    int size = pOut_fmt->nFrameWidth * pOut_fmt->nFrameHeight
        * pxlfmt2bpp_gmapp(pOut_fmt->eColorFormat)/8;

    pic_buf = fsl_osal_malloc_new(size);
    fsl_osal_memset(pic_buf, 0, size);
#endif 

    //resize image
    Resize_mode resize_mode = KEEP_DEST_RESOLUTION;
    ic->set_property(ic, PROPERTY_RESIZE_MODE, &resize_mode);
    ic->resize(ic, pIn_fmt, pCropRect, in, pOut_fmt, pic_buf);

    /*encode resized image to jpeg format.
     *as jpeg encoder using glibc file operation function,
     * so we use fopen here */
    OMX_U8 cFileName[256];
    static OMX_U32 nThumbCnt;
    FILE *fp_thumb;
    if(OMX_ErrorNone != MakeThumbnailFileName(file_name, &cFileName, nThumbCnt++)) {
        fsl_osal_dealloc(pic_buf);
        return;
    }

    fp_thumb = fopen(cFileName,"wb");
    if(fp_thumb == NULL) {
        printf("Open File Error\n");
        fsl_osal_dealloc(pic_buf);
        return;
    }

    ic->set_property(ic, PROPERTY_OUT_FILE, fp_thumb);
    ic->jpeg_enc(ic, pOut_fmt, pic_buf);

    fclose(fp_thumb);
    fsl_osal_dealloc(pic_buf);
    ic->delete_it(ic);

    return;
}

void save_to_rgba(OMX_IMAGE_PORTDEFINITIONTYPE *pIn_fmt, OMX_CONFIG_RECTTYPE *pCropRect, OMX_U8 *in, OMX_IMAGE_PORTDEFINITIONTYPE *pOut_fmt)
{
    OMX_ImageConvert* ic;
    ic = OMX_ImageConvertCreate();
    if(ic == NULL) {
        printf("failed to load image converter.\n");
        return;
    }

    pOut_fmt->eColorFormat = OMX_COLOR_Format32bitARGB8888;

    //alloc memory to save converted image
    OMX_U8 *pic_buf = NULL;
#ifdef USE_OLD_FSL_OSAL 
    fsl_osal_mem_req mem;
    mem.align = 4;
    mem.mem_type_speed = E_FSL_FAST_MEMORY;
    mem.size = pOut_fmt->nFrameWidth * pOut_fmt->nFrameHeight
        * pxlfmt2bpp_gmapp(pOut_fmt->eColorFormat)/8;
    pic_buf = FSL_Alloc(&mem);
    fsl_osal_memset(pic_buf, 0, mem.size);
#else 
    int size1 = pOut_fmt->nFrameWidth * pOut_fmt->nFrameHeight
        * pxlfmt2bpp_gmapp(pOut_fmt->eColorFormat)/8;

    pic_buf = fsl_osal_malloc_new(size1);
    fsl_osal_memset(pic_buf, 0, size1);
#endif

    //resize image
    Resize_mode resize_mode = KEEP_DEST_RESOLUTION;
    ic->set_property(ic, PROPERTY_RESIZE_MODE, &resize_mode);
    ic->resize(ic, pIn_fmt, pCropRect, in, pOut_fmt, pic_buf);

    /*encode resized image to jpeg format.
     *as jpeg encoder using glibc file operation function,
     * so we use fopen here */
    efsl_osal_return_type_t osal_ret = E_FSL_OSAL_SUCCESS;
    fsl_osal_file fp_snapshot;
    static OMX_U32 nSnapshotCnt;
    OMX_U8 cFileName[256];
    OMX_U32 size;

    sprintf(cFileName,"Snapshot%d.rgba", nSnapshotCnt++);
    osal_ret = fsl_osal_fopen(cFileName,"wb", &fp_snapshot);
    if(osal_ret != E_FSL_OSAL_SUCCESS) {
        printf("Open File Error\n");
        fsl_osal_dealloc(pic_buf);
        return;
    }
#ifdef USE_OLD_FSL_OSAL
    fsl_osal_fwrite(pic_buf, mem.size, fp_snapshot, &size);
#else 
    fsl_osal_fwrite(pic_buf, size1, fp_snapshot, &size);
#endif
    fsl_osal_fclose(fp_snapshot);
    fsl_osal_dealloc(pic_buf);

    ic->delete_it(ic);

    return;
}

OMX_BOOL rwd_test(OMX_GraphManager* gm, const char* filename)
{
    OMX_BOOL ret = OMX_TRUE;
    OMX_TICKS sDur, sCur, div;
    OMX_U32 nCnt;

    ret = gm->load(gm, filename, strlen(filename));
    if (ret == OMX_FALSE)
    {
        printf("load file %s failed.\n", filename);
        gm->stop(gm);
        gm->deleteIt(gm);        
        return ret;
    }

    gm->start(gm);
    sleep(1);

    while(1) {
        if(quitflag)
            break;
        gm->setPlaySpeed(gm, (int)(-4*Q16_SHIFT));
        sleep(1);
    }

    gm->stop(gm);
    gm->deleteIt(gm);        

    return OMX_TRUE;
}

#if 0
OMX_BOOL seek_forward_test(OMX_GraphManager* gm, const char* filename)
{
    OMX_BOOL ret = OMX_TRUE;
    OMX_TICKS sDur, sPos;

    ret = gm->load(gm, filename, strlen(filename));
    if (ret == OMX_FALSE)
    {
        printf("load file %s failed.\n", filename);
        gm->stop(gm);
        gm->deleteIt(gm);        
        return ret;
    }

    sDur = 0;
    gm->getDuration(gm, &sDur);

    gm->settvout(gm,OMX_FALSE, NV_MODE, OMX_TRUE);
    gm->start(gm);
    gm->pause(gm);

    sDur -= OMX_TICKS_PER_SECOND;
    sPos = 0;
    while(sPos < sDur) {
        printf("seek to position: %lld\n", sPos);
        gm->seek(gm, OMX_TIME_SeekModeFast, sPos);
        sPos += 10*OMX_TICKS_PER_SECOND;
        usleep(50000);
    }

    gm->stop(gm);
    gm->deleteIt(gm);        

    return ret;
}

OMX_BOOL seek_backward_test(OMX_GraphManager* gm, const char* filename)
{
    OMX_BOOL ret = OMX_TRUE;
    OMX_TICKS sDur;

    ret = gm->load(gm, filename, strlen(filename));
    if (ret == OMX_FALSE)
    {
        printf("load file %s failed.\n", filename);
        gm->stop(gm);
        gm->deleteIt(gm);        
        return ret;
    }

    sDur = 0;
    gm->getDuration(gm, &sDur);

    gm->settvout(gm,OMX_FALSE, NV_MODE, OMX_TRUE);
    gm->start(gm);
    gm->pause(gm);

    sDur -= OMX_TICKS_PER_SECOND;
    while((OMX_S64)sDur >= 0) {
        printf("seek to position: %d\n");
        gm->seek(gm, OMX_TIME_SeekModeFast, sDur);
        sDur -= 10*OMX_TICKS_PER_SECOND;
    }

    gm->stop(gm);
    gm->deleteIt(gm);        

    return ret;
}

OMX_BOOL ff_test(OMX_GraphManager* gm, const char* filename)
{
    OMX_BOOL ret = OMX_TRUE;
    OMX_TICKS sDur, sCur, div;
    OMX_U32 nCnt;

    ret = gm->load(gm, filename, strlen(filename));
    if (ret == OMX_FALSE)
    {
        printf("load file %s failed.\n", filename);
        gm->stop(gm);
        gm->deleteIt(gm);        
        return ret;
    }

    sDur = 0;
    gm->getDuration(gm, &sDur);
    gm->start(gm);

    sleep(1);
    while(1) {
        if(quitflag)
            break;
        gm->setPlaySpeed(gm, 4*Q16_SHIFT);
        usleep(100000);
        gm->setPlaySpeed(gm, Q16_SHIFT);
        usleep(100000);
    }

    gm->stop(gm);
    gm->deleteIt(gm);        

}

OMX_BOOL rwd_test(OMX_GraphManager* gm, const char* filename)
{
    OMX_BOOL ret = OMX_TRUE;
    OMX_TICKS sDur, sCur, div;
    OMX_U32 nCnt;

    ret = gm->load(gm, filename, strlen(filename));
    if (ret == OMX_FALSE)
    {
        printf("load file %s failed.\n", filename);
        gm->stop(gm);
        gm->deleteIt(gm);        
        return ret;
    }

    sDur = 0;
    gm->getDuration(gm, &sDur);
    gm->start(gm);
    gm->seek(gm, OMX_TIME_SeekModeFast, (sDur-5*OMX_TICKS_PER_SECOND));
    sleep(1);

    while(1) {
        if(quitflag)
            break;
        gm->etPlaySpeed(gm, (int)(-4*Q16_SHIFT));
        usleep(500000);
        gm->setPlaySpeed(gm, Q16_SHIFT);
        usleep(500000);
    }

    gm->stop(gm);
    gm->deleteIt(gm);        

}

OMX_BOOL seek_snapshot_test(OMX_GraphManager* gm, const char* filename)
{
    OMX_BOOL ret = OMX_TRUE;
    OMX_TICKS sDur, sCur, div;
    OMX_U32 nCnt;

    ret = gm->load(gm, filename, strlen(filename));
    if (ret == OMX_FALSE)
    {
        printf("load file %s failed.\n", filename);
        gm->stop(gm);
        gm->deleteIt(gm);        
        return ret;
    }

    sDur = 0;
    gm->getDuration(gm, &sDur);

    gm->settvout(gm,OMX_FALSE, NV_MODE, OMX_TRUE);
    gm->start(gm);
    gm->pause(gm);

    div = sDur/17;
    div = div/OMX_TICKS_PER_SECOND*OMX_TICKS_PER_SECOND;
    sCur = div;
    nCnt = 0;
    while(nCnt<16) {
        printf("mosaic seek %d:%"OMX_TIME_FORMAT_SHORT"\n", nCnt++, OMX_TIME_ARGS_SHORT(sCur));
        //printf("seeking ...\n");
        ret = gm->seek(gm, OMX_TIME_SeekModeFast, sCur);
        if (OMX_FALSE == ret)
        {
            sCur += div;
            continue;
        }
        //printf("seek done.\n");
        sCur += div;

        //get the video format
        OMX_IMAGE_PORTDEFINITIONTYPE in_format;
        OMX_CONFIG_RECTTYPE sCropRect;
        gm->getScreenShotInfo(gm, &in_format, &sCropRect);

        //allocate buffer for save snapshot picture
        OMX_U8 *snapshot_buf = NULL;
#ifdef USE_OLD_FSL_OSAL 
        fsl_osal_mem_req mem;
        mem.align = 4;
        mem.mem_type_speed = E_FSL_FAST_MEMORY;
        mem.size = in_format.nFrameWidth * in_format.nFrameHeight 
            * pxlfmt2bpp_gmapp(in_format.eColorFormat)/8;
        snapshot_buf = FSL_Alloc(&mem);
        fsl_osal_memset(snapshot_buf, 0, mem.size);
#else 
        int size = in_format.nFrameWidth * in_format.nFrameHeight 
            * pxlfmt2bpp_gmapp(in_format.eColorFormat)/8;

        snapshot_buf = fsl_osal_malloc_new(size);
        fsl_osal_memset(snapshot_buf, 0, size);
#endif 

        //get snapshot
        gm->getSnapshot(gm, snapshot_buf);

        //convert thumbnail picture to required format and resolution
        OMX_IMAGE_PORTDEFINITIONTYPE out_format;
        gm_setheader(&out_format, sizeof(OMX_IMAGE_PORTDEFINITIONTYPE));

        out_format.nFrameWidth = 104;
        out_format.nFrameHeight = 64;

        save_to_jpeg(&in_format, &sCropRect, snapshot_buf, &out_format, filename);
        //save_to_rgba(&in_format, &sCropRect, snapshot_buf, &out_format);

        fsl_osal_dealloc(snapshot_buf);
    }

    gm->stop(gm);
    gm->deleteIt(gm);        

}

OMX_BOOL audio_sanity_test(OMX_GraphManager* gm, const char* filename)
{
	OMX_GraphManagerPrivateData* data=(OMX_GraphManagerPrivateData*)gm->pData;
    OMX_BOOL ret = OMX_TRUE;
    OMX_TICKS sDur, sCur, div;
    OMX_U32 nCnt;
    OMX_S32 speed;
    char rep[128];

    gm->AddRemoveAudioPP(gm, OMX_FALSE);
    ret = gm->load(gm, filename, strlen(filename));
    if (ret == OMX_FALSE)
    {
        printf("load file %s failed.\n", filename);
        gm->stop(gm);
        gm->deleteIt(gm);        
        return ret;
    }

    gm->start(gm);
	START_SHOW_PLAYTIME_INFO;

    printf("start play %s\n", filename);
    sleep(2);

    printf("test stop ...\n");
    gm->stop(gm);
    STOP_SHOW_PLAYTIME_INFO;
    sleep(1);
    printf("test start ...\n");
    ret = gm->load(gm, filename, strlen(filename));
    if (ret == OMX_FALSE)
    {
        printf("load file %s failed.\n", filename);
        gm->stop(gm);
        gm->deleteIt(gm);        
        return ret;
    }

    gm->start(gm);
	START_SHOW_PLAYTIME_INFO;
    sDur = 0;
    gm->getDuration(gm, &sDur);
    div = sDur;
    div = div/OMX_TICKS_PER_SECOND*OMX_TICKS_PER_SECOND;
    sCur = div;

    sleep(2);
    
    printf("test pause ...\n");
    gm->pause(gm);
    sleep(2);

    printf("test resume ...\n");
    gm->resume(gm);
    sleep(2);

    printf("test sound effect ...\n");
    gm->AddRemoveAudioPP(gm, OMX_TRUE);
    sleep(2);
    printf("test sound effect unload ...\n");
    gm->AddRemoveAudioPP(gm, OMX_FALSE);
    sleep(2);

    printf("test seek ...\n");
    gm->seek(gm, OMX_TIME_SeekModeFast, sCur/2);
    sleep(2);

    printf("test change speed to x1.4 ...\n");
    speed = (int)(1.4 * Q16_SHIFT); 
    gm->setPlaySpeed(gm, speed);
    sleep(2);
    printf("test change speed to x1 ...\n");
    speed = (int)(1 * Q16_SHIFT); 
    gm->setPlaySpeed(gm, speed);
    sleep(2);

    if (data->bVideo == OMX_TRUE)
    {
        printf("test change speed to x4 ...\n");
        speed = (int)(4 * Q16_SHIFT); 
        gm->setPlaySpeed(gm, speed);
        sleep(2);
        printf("test change speed to x1 ...\n");
        speed = (int)(1 * Q16_SHIFT); 
        gm->setPlaySpeed(gm, speed);
        sleep(2);

        printf("test change speed to x-2 ...\n");
        speed = (int)(-2 * Q16_SHIFT); 
        gm->setPlaySpeed(gm, speed);
        sleep(2);
        printf("test change speed to x1 ...\n");
        speed = (int)(1 * Q16_SHIFT); 
        gm->setPlaySpeed(gm, speed);
        sleep(2);
    }

    printf("test pause sound effect ...\n");
    gm->pause(gm);
    gm->AddRemoveAudioPP(gm, OMX_TRUE);
    gm->resume(gm);
    sleep(2);
    printf("test pause sound effect unload ...\n");
    gm->pause(gm);
    gm->AddRemoveAudioPP(gm, OMX_FALSE);
    gm->resume(gm);
    sleep(2);

    printf("test pause seek ...\n");
    gm->pause(gm);
    gm->seek(gm, OMX_TIME_SeekModeFast, sCur/2);
    gm->resume(gm);
    sleep(2);

    printf("test pause change speed to x1.4 ...\n");
    gm->pause(gm);
    speed = (int)(1.4 * Q16_SHIFT); 
    gm->setPlaySpeed(gm, speed);
    gm->resume(gm);
    sleep(2);
    printf("test pause change speed to x1 ...\n");
    gm->pause(gm);
    speed = (int)(1 * Q16_SHIFT); 
    gm->setPlaySpeed(gm, speed);
    gm->resume(gm);
    sleep(2);

    if (data->bVideo == OMX_TRUE)
    {
        printf("test pause change speed to x4 ...\n");
        gm->pause(gm);
        speed = (int)(4 * Q16_SHIFT); 
        gm->setPlaySpeed(gm, speed);
        sleep(2);
        printf("test pause change speed to x1 ...\n");
        speed = (int)(1 * Q16_SHIFT); 
        gm->setPlaySpeed(gm, speed);
        sleep(2);
        gm->resume(gm);

        printf("test pause change speed to x-2 ...\n");
        gm->pause(gm);
        speed = (int)(-2 * Q16_SHIFT); 
        gm->setPlaySpeed(gm, speed);
        sleep(2);
        printf("test pause change speed to x1 ...\n");
        speed = (int)(1 * Q16_SHIFT); 
        gm->setPlaySpeed(gm, speed);
        sleep(2);
        gm->resume(gm);
    }

    printf("test EOS ...\n");
    gm->seek(gm, OMX_TIME_SeekModeFast, sCur);

    // wait for stop
    while ((data->eGmState != GM_STATE_STOP))
        sleep(1);

    sleep(1);
    ret = gm->load(gm, filename, strlen(filename));
    if (ret == OMX_FALSE)
    {
        printf("load file %s failed.\n", filename);
        gm->stop(gm);
        gm->deleteIt(gm);        
        return ret;
    }
    
    gm->start(gm);
	START_SHOW_PLAYTIME_INFO;
    
    if (data->bVideo == OMX_TRUE)
    {
        printf("test change speed to x4 to end ...\n");
        speed = (int)(4 * Q16_SHIFT);
        gm->setPlaySpeed(gm, speed);
        // wait for stop
        while ((data->eGmState != GM_STATE_STOP))
            sleep(1);
        
        sleep(1);
        ret = gm->load(gm, filename, strlen(filename));
        if (ret == OMX_FALSE)
        {
            printf("load file %s failed.\n", filename);
            gm->stop(gm);
            gm->deleteIt(gm);        
            return ret;
        }
        
        gm->start(gm);
		START_SHOW_PLAYTIME_INFO;
        
        printf("test change speed to x-2 to end ...\n");
        gm->seek(gm, OMX_TIME_SeekModeFast, sCur);
        speed = (int)(-2 * Q16_SHIFT); 
        gm->setPlaySpeed(gm, speed);
        // wait for stop
        while ((data->eGmState != GM_STATE_PLAYING))
            sleep(1);
        sleep(2);
        
        gm->stop(gm);
        STOP_SHOW_PLAYTIME_INFO;

        sleep(1);
        ret = gm->load(gm, filename, strlen(filename));
        if (ret == OMX_FALSE)
        {
            printf("load file %s failed.\n", filename);
            gm->stop(gm);
            gm->deleteIt(gm);        
            return ret;
        }
        
        gm->start(gm);
		START_SHOW_PLAYTIME_INFO;
        
        printf("test pause change speed to x4 to end ...\n");
        speed = (int)(4 * Q16_SHIFT);
        gm->pause(gm);
        gm->setPlaySpeed(gm, speed);
        // wait for stop
        while ((data->eGmState != GM_STATE_STOP))
            sleep(1);
        
        sleep(1);
        ret = gm->load(gm, filename, strlen(filename));
        if (ret == OMX_FALSE)
        {
            printf("load file %s failed.\n", filename);
            gm->stop(gm);
            gm->deleteIt(gm);        
            return ret;
        }
        
        gm->start(gm);
		START_SHOW_PLAYTIME_INFO;
        
        printf("test pause change speed to x-2 to end ...\n");
        gm->seek(gm, OMX_TIME_SeekModeFast, sCur);
        speed = (int)(-2 * Q16_SHIFT);
        gm->pause(gm);
        gm->setPlaySpeed(gm, speed);
        // wait for stop
        while ((data->eGmState != GM_STATE_PAUSE))
            sleep(1);
        gm->resume(gm);
        sleep(2);
    }

    gm->stop(gm);
    STOP_SHOW_PLAYTIME_INFO;

}


OMX_BOOL audio_skip_test(OMX_GraphManager* gm, const char* filename)
{
	OMX_GraphManagerPrivateData* data=(OMX_GraphManagerPrivateData*)gm->pData;
    OMX_BOOL ret = OMX_TRUE;
    OMX_TICKS sDur, sCur, div;
    OMX_U32 nCnt;
    OMX_S32 speed;
    char rep[128];

    gm->AddRemoveAudioPP(gm, OMX_FALSE);
    ret = gm->load(gm, filename, strlen(filename));
    if (ret == OMX_FALSE)
    {
        printf("load file %s failed.\n", filename);
        gm->stop(gm);
        gm->deleteIt(gm);        
        return ret;
    }

    gm->start(gm);
    START_SHOW_PLAYTIME_INFO;

    printf("start play %s\n", filename);
    sleep(2);

    gm->stop(gm);
    STOP_SHOW_PLAYTIME_INFO;

}
#endif

/************************************ File EOF ********************************/
