/**
 *  Copyright (c) 2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <sys/time.h>
#include "string.h"
#include "stdio.h"
#include "dirent.h"
#include "ctype.h"
#include "fsl_osal.h"
#include "Log.h"
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Implement.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include "OMX_Recorder.h"
#include "stdlib.h"
#include "string.h"
#include <signal.h>


typedef struct {
	char * debugLevel;
	char * debugFile;
	char *pFilename;
	audio_encoder_gm audioFormat;
	video_encoder_gm videoFormat;
	OMX_S32 width;
	OMX_S32 height;
	OMX_S32 framerate;
	OMX_TICKS timeUs;
	OMX_S64 bytes;
	OMX_S32 durationUs;
	OMX_S32 sampleRate;
	OMX_S32 channels;
	OMX_S32 audiobitRate;
	OMX_S32 videobitRate;
	OMX_S32 degrees;
	OMX_S32 seconds;
	OMX_S32 profile;
	OMX_S32 level;
	OMX_BOOL quiet;
}RECORDER_Options;

#define START_PLAYTIME_INFO_THREAD(thread, mRecorder)\
	do{\
		if (options.quiet == OMX_FALSE && thread == NULL){\
			bexituserinput = OMX_FALSE;\
			fsl_osal_thread_create(&(thread), NULL, userinput, (mRecorder));\
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

#define FILE_NAME_LEN 1024

fsl_osal_ptr thread=NULL;
OMX_BOOL bexituserinput = OMX_FALSE;
OMX_BOOL bstartuserinput = OMX_FALSE;

volatile sig_atomic_t quitflag = 0;
void my_int_signal_handler(int signum)
{
	quitflag = 1;
}

int eventhandler(void* context, RECORDER_EVENT eventID, void* Eventpayload)
{
	OMX_Recorder* mRecorder = (OMX_Recorder*) context;
	switch(eventID) {
		case RECORDER_EVENT_MAX_DURATION_REACHED:
			{
				STOP_SHOW_PLAYTIME_INFO;
				mRecorder->stop(mRecorder);
				mRecorder->close(mRecorder);
				printf("\nRecorder done, choose next command\n");
				break;
			}
		case RECORDER_EVENT_MAX_FILESIZE_REACHED:
			{
				STOP_SHOW_PLAYTIME_INFO;
				mRecorder->stop(mRecorder);
				mRecorder->close(mRecorder);
				printf("\nRecorder done, choose next command\n");
				break;
			}
		default:
			break;
	}

	return 1;
}

void* userinput(void* param)
{
	OMX_Recorder* mRecorder = 	(OMX_Recorder*)param;
	OMX_TICKS sCur;
	OMX_U32 Hours;
	OMX_U32 Minutes;
	OMX_U32 Seconds;
	OMX_S32 max;

	while(bexituserinput == OMX_FALSE) 
	{
		if (bstartuserinput)
		{
			sCur = 0;
			if(mRecorder->getMediaTime(mRecorder, &sCur))
			{
				Hours = ((OMX_U64)sCur/1000000) / 3600;
				Minutes = ((OMX_U64)sCur/ (60*1000000)) % 60;
				Seconds = (((OMX_U64)sCur %(3600*1000000)) % (60*1000000))/1000000;

				mRecorder->getMaxAmplitude(mRecorder, &max);
#if 1
				printf("\r[Current Media Time] %03d:%02d:%02d    [Max Audio Amplitude] %d", 
						Hours, Minutes, Seconds, max);
				fflush(stdout);
#endif
			}

			fsl_osal_sleep(500000);
		}
		else
			fsl_osal_sleep(50000);
	}

}

void recorder_usage()
{
	printf("\n");
	printf("recorder_test_arm11_elinux output_file <recorder options> \n");
	printf(" -h/--help     display this usage.\n");
	printf(" --log_level   define log level.\n");
	printf(" --log_file    output log to file.\n");
	printf(" --audio_format      1 AMR NB; 6 MP3.\n");
	printf(" --video_format      1 H263; 2 H264.\n");
	printf(" --width       video width.\n");
	printf(" --height      video height.\n");
	printf(" --framerate   video framerate.\n");
	printf(" --timeUs      recorder file max duration.\n");
	printf(" --bytes       recorder file max size.\n");
	printf(" --durationUs  recorder file interleave duration.\n");
	printf(" --sampleRate  audio sample rate.\n");
	printf(" --channels    audio channels.\n");
	printf(" --audiobitRate     audio bit rate.\n");
	printf(" --videobitRate     video bit rate.\n");
	printf(" --degrees     video rotation.\n");
	printf(" --seconds     video I frame interval.\n");
	printf(" --profile     video encoder profile.\n");
	printf(" --level       video encoder level.\n");

	printf("\n");
}

void recorder_main_menu()
{
	printf("\nSelect command:\n");
	printf("\t[r]Recorde\n");
	printf("\t[a]Pause/Resume\n");
	printf("\t[s]Stop\n");
	printf("\t[x]Exit\n\n");

}

int recorder_parse_options(int argc, char* argv[], RECORDER_Options * pOpt)
{
	int ret = 0;
	int i;
	int parseindex = 0;

	memset(pOpt, 0, sizeof(RECORDER_Options));
	for (i=1;i<argc;i++){
		if ((strlen(argv[i])) && (argv[i][0]=='-')){
			if ((strcmp(argv[i], "-h")==0)||(strcmp(argv[i], "--help")==0)){
				ret = 1;
				goto parseOptionErr;
			}

			if ((strcmp(argv[i], "--log_level")==0)){
				parseindex=1;
				continue;
			}

			if ((strcmp(argv[i], "--log_file")==0)){
				parseindex=2;
				continue;
			}

			if ((strcmp(argv[i], "--width")==0)){
				parseindex=3;
				continue;
			}

			if ((strcmp(argv[i], "--height")==0)){
				parseindex=4;
				continue;
			}

			if ((strcmp(argv[i], "--framerate")==0)){
				parseindex=5;
				continue;
			}

			if ((strcmp(argv[i], "--timeUs")==0)){
				parseindex=6;
				continue;
			}

			if ((strcmp(argv[i], "--bytes")==0)){
				parseindex=7;
				continue;
			}

			if ((strcmp(argv[i], "--durationUs")==0)){
				parseindex=8;
				continue;
			}

			if ((strcmp(argv[i], "--sampleRate")==0)){
				parseindex=9;
				continue;
			}

			if ((strcmp(argv[i], "--channels")==0)){
				parseindex=10;
				continue;
			}

			if ((strcmp(argv[i], "--audiobitRate")==0)){
				parseindex=11;
				continue;
			}

			if ((strcmp(argv[i], "--videobitRate")==0)){
				parseindex=12;
				continue;
			}

			if ((strcmp(argv[i], "--degrees")==0)){
				parseindex=13;
				continue;
			}

			if ((strcmp(argv[i], "--seconds")==0)){
				parseindex=14;
				continue;
			}

			if ((strcmp(argv[i], "--profile")==0)){
				parseindex=15;
				continue;
			}

			if ((strcmp(argv[i], "--level")==0)){
				parseindex=16;
				continue;
			}

			if ((strcmp(argv[i], "--audio_format")==0)){
				parseindex=17;
				continue;
			}

			if ((strcmp(argv[i], "--video_format")==0)){
				parseindex=18;
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
				pOpt->width=atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 4:
				pOpt->height=atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 5:
				pOpt->framerate=atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 6:
				pOpt->timeUs=atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 7:
				pOpt->bytes=atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 8:
				pOpt->durationUs=atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 9:
				pOpt->sampleRate=atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 10:
				pOpt->channels=atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 11:
				pOpt->audiobitRate=atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 12:
				pOpt->videobitRate=atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 13:
				pOpt->degrees=atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 14:
				pOpt->seconds=atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 15:
				pOpt->profile=atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 16:
				pOpt->level=atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 17:
				pOpt->audioFormat=(audio_encoder_gm)atoi(argv[i]);
				parseindex=0;
				continue;
				break;
			case 18:
				pOpt->videoFormat=(video_encoder_gm)atoi(argv[i]);
				parseindex=0;
				continue;
				break;

			default:
				break;
		}

        pOpt->pFilename = argv[i];
	}
	
	if (pOpt->pFilename == NULL) {
		ret = 1;
	}

	return ret;

parseOptionErr:
	return ret;
}

int main(int argc, char* argv[])
{
	OMX_Recorder* mRecorder = NULL;
	OMX_BOOL bexit = OMX_FALSE;
	char *pFilename, pFileNameBuf[FILE_NAME_LEN];
	pFilename=pFileNameBuf;
	char rep[128];
	OMX_BOOL readinput = OMX_TRUE;
	OMX_BOOL bPause = OMX_FALSE;
	RECORDER_Options options;

	int ret;
	struct sigaction act;
	act.sa_handler = my_int_signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);

#ifdef FSL_MEM_CHECK
	init_mm_mutex();
#endif
	ret = recorder_parse_options(argc,argv,&options);

	if (ret<0){
		return ret;
	}else if (ret==1){
		recorder_usage();
		return ret;
	}
	if (options.debugLevel){
		LogInit(fsl_osal_atoi(options.debugLevel), options.debugFile);
	}else{
		LogInit(-1, options.debugFile);
	}

	mRecorder = OMX_RecorderCreate();
	if (mRecorder == NULL)
		goto exitMain;

	printf("create mRecorder successfully\n");

	mRecorder->registerEventHandler(mRecorder, mRecorder, eventhandler);

	mRecorder->init(mRecorder);

	START_PLAYTIME_INFO_THREAD(thread, mRecorder);

	while(bexit == OMX_FALSE) {
		{
			if (readinput){
				recorder_main_menu();
				scanf("%s", rep);
			}
			readinput=OMX_TRUE;
			if (quitflag)
			{
				printf("receive Ctrl+c user input.\n");
				STOP_SHOW_PLAYTIME_INFO;
				mRecorder->stop(mRecorder);
				mRecorder->close(mRecorder);
				mRecorder->deleteIt(mRecorder);        
				goto exitMain;
			}
			if(rep[0] == 'r') {
				mRecorder->setOutputFile(mRecorder, options.pFilename);
				int camera = 0;
				mRecorder->setCamera(mRecorder, &camera, NULL);

				if (options.audioFormat)
					mRecorder->setAudioEncoder(mRecorder, options.audioFormat);
				if (options.videoFormat)
					mRecorder->setVideoEncoder(mRecorder, options.videoFormat);
				if (options.width && options.height)
					mRecorder->setVideoSize(mRecorder, options.width, options.height);
				if (options.framerate)
					mRecorder->setVideoFrameRate(mRecorder, options.framerate);
				if (options.timeUs)
					mRecorder->setParamMaxFileDurationUs(mRecorder, options.timeUs);
				if (options.bytes)
					mRecorder->setParamMaxFileSizeBytes(mRecorder, options.bytes);
				if (options.durationUs)
					mRecorder->setParamInterleaveDuration(mRecorder, options.durationUs);
				if (options.sampleRate)
					mRecorder->setParamAudioSamplingRate(mRecorder, options.sampleRate);
				if (options.channels)
					mRecorder->setParamAudioNumberOfChannels(mRecorder, options.channels);
				if (options.audiobitRate)
					mRecorder->setParamAudioEncodingBitRate(mRecorder, options.audiobitRate);
				if (options.videobitRate)
					mRecorder->setParamVideoEncodingBitRate(mRecorder, options.videobitRate);
				if (options.degrees)
					mRecorder->setParamVideoRotation(mRecorder, options.degrees);
				if (options.seconds)
					mRecorder->setParamVideoIFramesInterval(mRecorder, options.seconds);
				if (options.profile)
					mRecorder->setParamVideoEncoderProfile(mRecorder, options.profile);
				if (options.level)
					mRecorder->setParamVideoEncoderLevel(mRecorder, options.level);
	
				ret = mRecorder->prepare(mRecorder);
				if (ret == OMX_FALSE)
				{
					printf("Recorder prepare %s failed.\n", pFilename);
					mRecorder->stop(mRecorder);
					mRecorder->close(mRecorder);
					mRecorder->deleteIt(mRecorder);        
					goto exitMain;
				}

				bPause = OMX_FALSE;
				mRecorder->start(mRecorder);
				START_SHOW_PLAYTIME_INFO;
				printf("start recorde %s\n", pFilename);
			}
			else if(rep[0] == 'a') {
				if(bPause != OMX_TRUE) {
					mRecorder->pause(mRecorder);
					bPause = OMX_TRUE;
				}
				else {
					mRecorder->start(mRecorder);
					bPause = OMX_FALSE;
				}
			}
			else if(rep[0] == 's') {
				STOP_SHOW_PLAYTIME_INFO;
				mRecorder->stop(mRecorder);
				mRecorder->close(mRecorder);
			}
			else if(rep[0] == 'x')
			{
				STOP_SHOW_PLAYTIME_INFO;
				mRecorder->stop(mRecorder);
				mRecorder->close(mRecorder);
				mRecorder->deleteIt(mRecorder);        
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
#ifdef FSL_MEM_CHECK
	print_non_free_resource();
#endif

	LogDeInit();

	return 0;
}

/* File EOF */
