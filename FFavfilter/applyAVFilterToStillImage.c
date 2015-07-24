
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG
#include "debug.h"

//#define ___DECODE_TJLIBRARY___
#define ___DECODE_FFMPEG___
#define ___DEBUG_MEMORY_CHECK___
#define ___PATCH_MEMORY_LEAK___
//#define ___DEBUG_SAVE_RAW___


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void *dfb_system_video_memory_virtual( unsigned int offset );

#ifdef __cplusplus
}
#endif /* __cplusplus */

int getFileSize (const char *path)
{
	struct stat st;

	if (stat(path, &st) < 0) {
		perror (path);
		return -1;
	}
	return (int)st.st_size;
}

#ifdef ___DECODE_TJLIBRARY___
#include <turbojpeg.h>
static void *readWholeFile (const char *path)
{
	off_t fsize;
	void *content = (void *) NULL;
	int fd;

	fsize = getFileSize (path);
	if (fsize > 0)
	{
		content = (void *) malloc (fsize);

		fd = open (path, O_RDONLY);
		if (fd > 0)
		{
			int rdSize = read (fd, content, fsize);
			int fgErr = 0;

			if (rdSize < 0) {
				fgErr = 1;
			} else if ( (int)fsize != rdSize ) {
				fgErr = 1;
			}
			if (fgErr) {
				DBG ("Err : Read %s, but file size error. Wanted size=%jd, Readed size=%d\n", path, fsize, rdSize);
				free (content);
				content = (void *) NULL;
			}
			close(fd);
		}
	}

	return content;
}


int tjdecode_fhd (uint32_t yOffset, uint32_t yPitch, const char *fname)
{
//	const char *fname = "/mnt/ata/stream/play_stream/fhd.jpg";
	uint8_t *yBase = (uint8_t *)dfb_system_video_memory_virtual (yOffset);
	uint8_t *uBase = (uint8_t *)((uint32_t)yBase + yPitch * 2160);
	uint8_t *vBase = (uint8_t *)((uint32_t)uBase + yPitch * 2160 / 4);
	uint8_t *jbuf = NULL;
	uint8_t *yuvBuf = NULL;
	uint32_t jbufSize;
	int width, height, subSamp;
	int i;
	int j;
	tjhandle jpegHnd = tjInitDecompress();

	DBG("@@@@@ %s\n", fname);
	do   
	{    
		jbufSize = getFileSize (fname);
		if (jbufSize <= 0) { 
			DBG ("Err : file size = %d\n", jbufSize);
			break;
		}    
		jbuf = (uint8_t *) readWholeFile (fname);
		if (jbuf == NULL) {
			DBG ("Err : read file, %s\n", fname);
			break;
		}    

		if (tjDecompressHeader2 (jpegHnd, jbuf, jbufSize, &width, &height, &subSamp) != 0) { 
			DBG ("Err : tjDecompressHeader2 - %s\n", tjGetErrorStr());
			break;
		} else {
			DBG ("Success tjDecompressHeader2() : %d x %d, subSamp = %d\n", width, height, subSamp);
		}    

		yuvBuf = (uint8_t *) malloc (TJBUFSIZEYUV(width, height, TJSAMP_444));
		if (yuvBuf == NULL) {
			DBG ("Err : malloc - %s\n", strerror(errno));
			break;
		}    
		if ( tjDecompressToYUV (jpegHnd, jbuf, jbufSize, yuvBuf, 0) != 0 ) {
			DBG ("Err : tjDecompressToYUV - %s\n", tjGetErrorStr());
			break;
		}
#if 0
		if (subSamp == TJSAMP_420) {
			for (i=0; i<1080; i++) {
				memcpy (&yBase[i * 3840], &yuvBuf[i * 1920], 1920);
			}
			for (i=0; i<1080*2; i++) {
				memcpy (&uBase[i * 3840/4], &yuvBuf[1920*1080 + i * 1920/4], 1920/4);
			}
			DBG ("Success tjDecompressToYUV(420)\n");
		} else if ( subSamp == TJSAMP_422 ) {
			DBG("SKIP tjdecode_uhd cause of YUV422\n");
			for (i=0; i<1080; i++) {
				memcpy (yBase + i * 3840, yuvBuf + i * 1920, 1920);
			}
#if 0
			uint8_t *uBuf = yuvBuf + (1920 * 1080);	/* Data is 422 */
			for (i=0; i<1080; i+=1) {
				for (j=0; j<1920; j+=2) {
					uint32_t tmp;
					tmp = uBuf[    i*1920+j] + uBuf[    i*1920+(j+1)];
					uBase[(i*3840)>>1 + j>>1] = (tmp >> 1) & 0xFF;
				}
			}
			uBuf = yuvBuf + (1920 * 1080) + (1920 * 1080)/2;	/* Data is 422 */
			for (i=0; i<1080; i+=1) {
				for (j=0; j<1920; j+=2) {
					uint32_t tmp;
					tmp = uBuf[    i*1920+j] + uBuf[    i*1920+(j+1)];
					vBase[(i*3840)>>1 + j>>1] = (tmp >> 1) & 0xFF;
				}
			}
#endif
			DBG ("Success tjDecompressToYUV(422)\n");
		} else if ( subSamp == TJSAMP_444 ) {
			for (i=0; i<1080; i++) {
				memcpy (yBase + i * 3840, yuvBuf + i * 1920, 1920);
			}
#if 0
			uint8_t *uBuf = yuvBuf + (1920 * 1080);	/* Data is 444 */
			for (i=0; i<1080*1; i+=2) {
				for (j=0; j<1920; j+=2) {
					uint32_t tmp;
					tmp = uBuf[    i*1920+j] + uBuf[    i*1920+(j+1)]
						+ uBuf[(i+1)*1920+j] + uBuf[(i+1)*1920+(j+1)];
					vBase[(i*3840)>>2 + j>>1] = (tmp >> 2) & 0xFF;
				}
			}
			uBuf = yuvBuf + (1920 * 1080) * 2;
			for (i=0; i<1080*1; i+=2) {
				for (j=0; j<1920; j+=2) {
					uint32_t tmp;
					tmp = uBuf[    i*1920+j] + uBuf[    i*1920+(j+1)]
						+ uBuf[(i+1)*1920+j] + uBuf[(i+1)*1920+(j+1)];
					uBase[(i*3840)>>2 + j>>1] = (tmp >> 2) & 0xFF;
				}
			}
#endif
			DBG ("Success tjDecompressToYUV(444)\n");
		} else {
			DBG("tjdecoder_fhd decoded but unknow output format(%d)\n", subSamp);
		}
#else
		DBG("tjdecoder_fhd decoded and output format is %d.\n", subSamp);
		for (i=0; i<1080; i++) {
			memcpy (&yBase[i * 3840], &yuvBuf[i * 1920], 1920);
		}
#endif
	} while(0);

	tjDestroy (jpegHnd);

	if (yuvBuf) {
		free (yuvBuf);
	}    
	if (jbuf) {
		free (jbuf);
	}    

	return 0;
}

int tjdecode_uhd (uint32_t yOffset, uint32_t yPitch, const char *fname)
{
//	const char *fname = "/mnt/ata/stream/play_stream/UHD.jpg";
	uint8_t *yBase = (uint8_t *) dfb_system_video_memory_virtual (yOffset);
	uint8_t *uBase = (uint8_t *)((uint32_t)yBase + yPitch * 2160);
	uint8_t *vBase = (uint8_t *)((uint32_t)uBase + yPitch * 2160 / 4);
	uint8_t *jbuf = NULL;
	uint8_t *yuvBuf = NULL;
	uint32_t jbufSize;
	int width, height, subSamp;
	int i;
	int j;
	tjhandle jpegHnd = tjInitDecompress();

	do   
	{    
		jbufSize = getFileSize (fname);
		if (jbufSize <= 0) { 
			DBG ("Err : file size = %d\n", jbufSize);
			break;
		}    
		jbuf = (uint8_t *) readWholeFile (fname);
		if (jbuf == NULL) {
			DBG ("Err : read file, %s\n", fname);
			break;
		}    

		if (tjDecompressHeader2 (jpegHnd, jbuf, jbufSize, &width, &height, &subSamp) != 0) { 
			DBG ("Err : tjDecompressHeader2 - %s\n", tjGetErrorStr());
			break;
		} else {
			DBG ("Success tjDecompressHeader2() : %d x %d, subSamp = %d\n", width, height, subSamp);
		}    
		yuvBuf = (uint8_t *) malloc (TJBUFSIZEYUV(width, height, TJSAMP_444));
		if (yuvBuf == NULL) {
			DBG ("Err : malloc - %s\n", strerror(errno));
			break;
		}    
		if ( tjDecompressToYUV (jpegHnd, jbuf, jbufSize, yuvBuf, 0) != 0 ) {
			DBG ("Err : tjDecompressToYUV - %s\n", tjGetErrorStr());
			break;
		}

#if 0
		if (subSamp == TJSAMP_420) {
				for (i=0; i<2160; i++) {
					memcpy (&yBase[i * 3840], &yuvBuf[i * 3840], 3840);
				}
#if 0
				for (i=0; i<2160*2; i++) {
					memcpy (&uBase[i * 3840/4], &yuvBuf[3840*2160 + i * 3840/4], 3840/4);
				}
#endif
				DBG ("Success tjDecompressToYUV (420)\n");
		} else if (subSamp == TJSAMP_422) {
				for (i=0; i<2160; i++) {
					memcpy (&yBase[i * 3840], &yuvBuf[i * 3840], 3840);
				}
				DBG ("Success tjDecompressToYUV (422)\n");
		} else if (subSamp == TJSAMP_444) {
#if 0
			for (i=0; i<2160; i++) {
				memcpy (yBase + i * 3840, yuvBuf + i * 3840, 3840);
			}
			for (i=0; i<2160*2; i+=2) {
				for (j=0; j<3840; j+=2) {
					uBase[i/2+j/2] = yuvBuf[i*2160+j];
				}
			}
#else
			for (i=0; i<2160; i++) {
				memcpy (yBase + i * 3840, yuvBuf + i * 3840, 3840);
			}
# if 0
			uint8_t *uBuf = yuvBuf + (3840 * 2160);	/* Data is 444 */
			for (i=0; i<2160*2; i+=2) {
				for (j=0; j<3840; j+=2) {
					uint32_t tmp;
					tmp = uBuf[    i*3840+j] + uBuf[    i*3840+(j+1)]
						+ uBuf[(i+1)*3840+j] + uBuf[(i+1)*3840+(j+1)];
					uBase[(i*3840)>>2 + j>>1] = (tmp >> 2) & 0xFF;
				}
			}
# endif
#endif
			DBG ("Success tjDecompressToYUV(444)\n");
		} else {
			DBG("tjdecoder_uhd decoded but unknow output format(%d)\n", subSamp);
		}
#else
		DBG("tjdecoder_uhd decoded and output format is %d.\n", subSamp);
		for (i=0; i<2160; i++) {
			memcpy (&yBase[i * 3840], &yuvBuf[i * 3840], 3840);
		}
#endif
	} while(0);

	tjDestroy (jpegHnd);

	if (yuvBuf) {
		free (yuvBuf);
	}    
	if (jbuf) {
		free (jbuf);
	}    

	return 0;
}
#endif /* ___DECODE_TJLIBRARY___ */


#ifdef ___DECODE_FFMPEG___
#ifdef __cplusplus
extern "C" 
{
#endif
	    #include <libavutil/opt.h>
	    #include <libavcodec/avcodec.h>
	    #include <libavformat/avformat.h>
	    #include <libavutil/imgutils.h>
	    #include <libswscale/swscale.h>
	    #include <libavfilter/avfiltergraph.h>
	    #include <libavfilter/buffersink.h>
	    #include <libavfilter/buffersrc.h>
#ifdef __cplusplus
}
#endif

static int readWholeFileToBuffer (const char *path, uint8_t **buf)
{
	int fsize;
	void *content = (void *) NULL;
	int fd;

	fsize = getFileSize (path);
	if (fsize > 0)
	{
		content = (void *) malloc (fsize + FF_INPUT_BUFFER_PADDING_SIZE);

		fd = open (path, O_RDONLY);
		if (fd > 0)
		{
			int rdSize = read (fd, content, fsize);
			int fgErr = 0;

			if (rdSize < 0) {
				fgErr = 1;
			} else if ( (int)fsize != rdSize ) {
				fgErr = 1;
			}
			if (fgErr) {
				DBG ("Err : Read %s, but file size error. Wanted size=%d, Readed size=%d\n", path, fsize, rdSize);
				free (content);
				content = (void *) NULL;
			}
			close(fd);
		}
	}
	*buf = (uint8_t *)content ;

	return fsize ;
}

static enum AVCodecID ff_getCodecId (const char *path)
{
	enum AVCodecID codecId = AV_CODEC_ID_NONE ;
	AVFormatContext *pCTX = NULL ;

	do
	{
		if (avformat_open_input (&pCTX, path, NULL, NULL) < 0) {
			ERR("Error in avformat_open_input().\n") ;
			break;
		}

		AVStream *stream = pCTX->streams[0] ;
		AVCodecContext *pCodecCtX = stream->codec ;
		codecId = pCodecCtX->codec_id ;

	} while (0) ;

	if (pCTX) {
		avformat_close_input (&pCTX) ;
	}

	return codecId ;
}

static int ff_convertPixelFormatWithScale 
		(struct AVCodecContext *pCCtx, struct AVFrame *iframe, int owidth, int oheight, enum AVPixelFormat opixfmt, struct AVFrame *oframe)
{
	struct SwsContext *pSCtx = NULL ;
	int rv = (-1) ;

	do
	{
		pSCtx = sws_getContext(pCCtx->width, pCCtx->height, pCCtx->pix_fmt, owidth, oheight, opixfmt, SWS_BILINEAR, NULL, NULL, NULL) ;
		if (pSCtx == NULL) {
			ERR("@@ Failed sws_getContext()\n") ;
			break ;
		}

		rv = sws_scale (pSCtx, (const uint8_t * const*)iframe->data, iframe->linesize, 0, pCCtx->height, oframe->data, oframe->linesize) ;
		if (rv < 0) {
			ERR ("@@ Failed sws_scale()\n") ;
		}

		if (oframe->width <= 0) {
			oframe->width = owidth ;
		}
		if (oframe->height <= 0) {
			oframe->height = oheight ;
		}
		if ((enum AVPixelFormat)oframe->format != opixfmt) {
			oframe->format = opixfmt ;
		}
	}
	while (0) ;

	if (pSCtx) {
		sws_freeContext (pSCtx) ;
		pSCtx = NULL ;
	}

	return rv ;
}

static void ff_renderFrame420 (uint32_t screenOffset, uint32_t screenPitch, struct AVFrame *pFrame)
{
#if 0
	uint8_t *pDstY = (uint8_t *) dfb_system_video_memory_virtual (screenOffset) ;
	uint8_t *pDstU = (uint8_t *)((uint32_t)pDstY + screenPitch * 2160) ;
	uint8_t *pDstV = (uint8_t *)((uint32_t)pDstU + screenPitch * 2160 / 4) ;
	uint8_t *pSrcY = pFrame->data[0] ;
	uint8_t *pSrcU = pFrame->data[1] ;
	uint8_t *pSrcV = pFrame->data[2] ;
	int size ;
	int i ;

	DBG("@@ pix_fmt    : %d\n", pFrame->format) ;
	DBG("@@ resolution : %d x %d\n", pFrame->width, pFrame->height) ;
	for (i = 0; i<AV_NUM_DATA_POINTERS; i++) {
		if (pFrame->data[i] == NULL) {
			continue ;
		}
		DBG("@@ AVFrame : %d - %d - %p\n", i, pFrame->linesize[i], pFrame->data[i]) ;
	}

	size = pFrame->height ;
	for (i=0; i<size i++) {
		memcpy (&pDstY[i*screenPitch],      &pSrcY[i*pFrame->linesize[0]], pFrame->linesize[0]) ;
	}
	size = pFrame->height >> 1 ;
	for (i=0; i<size; i++) {
		memcpy (&pDstU[i*(screenPitch>>1)], &pSrcU[i*pFrame->linesize[1]], pFrame->linesize[1]) ;
		memcpy (&pDstV[i*(screenPitch>>1)], &pSrcV[i*pFrame->linesize[2]], pFrame->linesize[2]) ;
	}
#else
	int i ;
	DBG("@@ pix_fmt    : %d\n", pFrame->format) ;
	DBG("@@ resolution : %d x %d\n", pFrame->width, pFrame->height) ;
	for (i = 0; i<AV_NUM_DATA_POINTERS; i++) {
		if (pFrame->data[i] == NULL) {
			continue ;
		}
		DBG("@@ AVFrame : %d - %d - %p\n", i, pFrame->linesize[i], pFrame->data[i]) ;
	}
#endif
}

static int ff_convertFilter (AVCodecContext *pCCtx, const char *strFilterCmd, struct AVFrame *iframe, enum AVPixelFormat opixfmt, struct AVFrame *oframe)
{
	AVFilterGraph		*filterGraph = NULL ;
	AVFilterContext		*bufferSrcCTX = NULL ;
	AVFilterContext		*bufferSinkCTX = NULL ;
	AVFilterInOut		*outputs = NULL ;
	AVFilterInOut		*inputs = NULL ;
	enum AVPixelFormat	pixFmts[] = { opixfmt, AV_PIX_FMT_NONE } ;
	AVFilter			*bufferSrc = avfilter_get_by_name("buffer") ;
	AVFilter			*bufferSink = avfilter_get_by_name("buffersink") ;
	char filterArgs[512] ;
	int rv = (-1) ;

	outputs = avfilter_inout_alloc() ;
	inputs = avfilter_inout_alloc() ;
	filterGraph = avfilter_graph_alloc() ;

	do
	{
		/* init */
		if ( ! outputs || ! inputs || ! filterGraph ) {
			ERR("Failed to alloc avfilter variables.\n") ;
			break;
		}
		snprintf(filterArgs, sizeof(filterArgs), "video_size=%dx%d:pix_fmt=%d:time_base=1/1:pixel_aspect=%d/%d",
				iframe->width, iframe->height, opixfmt, pCCtx->sample_aspect_ratio.num, pCCtx->sample_aspect_ratio.den) ;
		DBG("@@ Filter Options : %s\n", filterArgs);

		rv = avfilter_graph_create_filter(&bufferSrcCTX, bufferSrc, "in", filterArgs, NULL, filterGraph) ;
		if (rv < 0) {
			ERR("@@ Failed avfilter_graph_create_filter(in) - %d\n", rv) ;
			break;
		}
		rv = avfilter_graph_create_filter(&bufferSinkCTX, bufferSink, "out", NULL, NULL, filterGraph) ;
		if (rv < 0) {
			ERR("@@ Failed avfilter_graph_create_filter(out)\n") ;
			break;
		}
		rv = av_opt_set_int_list(bufferSinkCTX, "pix_fmts", pixFmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) ;
		if (rv < 0) {
			ERR("@@ Failed av_opt_set_int_list()\n") ;
			break;
		}

		outputs->name		= av_strdup("in") ;
		outputs->filter_ctx	= bufferSrcCTX ;
		outputs->pad_idx	= 0 ;
		outputs->next		= NULL ;

		inputs->name		= av_strdup("out") ;
		inputs->filter_ctx	= bufferSinkCTX ;
		inputs->pad_idx		= 0 ;
		inputs->next		= NULL ;

		rv = avfilter_graph_parse_ptr(filterGraph, strFilterCmd, &inputs, &outputs, NULL) ;
		if (rv < 0) {
			ERR("@@ Failed avfilter_graph_parse_ptr()\n") ;
			break;
		}
		rv = avfilter_graph_config(filterGraph, NULL) ;
		if (rv < 0) {
			ERR("@@ Failed avfilter_graph_config()\n") ;
			break;
		}

		/* filtering */
		iframe->pts = av_frame_get_best_effort_timestamp(iframe) ;

		DBG("@@ filter : %d x %d, format : %d\n", iframe->width, iframe->height, iframe->format) ;

		rv = av_buffersrc_add_frame_flags(bufferSrcCTX, iframe, AV_BUFFERSRC_FLAG_KEEP_REF) ;
		if (rv < 0) {
			ERR("@@ Failed av_buffersrc_add_frame_flags()\n") ;
			break ;
		}

		while (1) {
			rv = av_buffersink_get_frame(bufferSinkCTX, oframe) ;
			if (rv == AVERROR(EAGAIN)) {
				rv = 0 ;
				break;
			}
			else if (rv == AVERROR_EOF) {
				DBG("@@ Failed av_buffersink_get_frame(EOF)\n") ;
				rv = 0 ;
			}
			else if (rv < 0) {
				DBG("@@ Failed av_buffersink_get_frame(%d)\n", rv) ;
				break;
			}
		}

		if (rv == 0) {
			if (oframe->width <= 0) {
				oframe->width = iframe->width ;
			}
			if (oframe->height <= 0) {
				oframe->height = iframe->height ;
			}
			if (oframe->format == -1) {
				oframe->format = opixfmt ;
			}
		}
	}
	while (0) ;

	/* de-init */
	if (bufferSinkCTX) {
		avfilter_free (bufferSinkCTX) ;
	}
	if (bufferSrcCTX) {
		avfilter_free (bufferSrcCTX) ;
	}
	if (outputs) {
		avfilter_inout_free(&outputs) ;
	}
	if (inputs) {
		avfilter_inout_free(&inputs) ;
	}
	if (filterGraph) {
		avfilter_graph_free(&filterGraph) ;
	}

	return rv ;
}

static int ff_compareAVFrame (AVFrame *frameA, AVFrame *frameB)
{
	int rv = 1 ;

	do
	{
		if ( ! frameA || frameA->width <= 0 || frameA->height <= 0 || (enum AVPixelFormat)frameA->format == AV_PIX_FMT_NONE) {
			ERR("Frame A is not enough to info about width, height or format.\n");
			break;
		}
		if ( ! frameB || frameB->width <= 0 || frameB->height <= 0 || (enum AVPixelFormat)frameB->format == AV_PIX_FMT_NONE) {
			ERR("Frame B is not enough to info about width, height or format.\n");
			break;
		}

		if ( frameA->width != frameB->width || frameA->height != frameB->height) {
			ERR("It it not match resolution each frames.\n") ;
			break;
		}

		if (frameA->format == AV_PIX_FMT_YUV420P && frameB->format == AV_PIX_FMT_YUV420P) {
			int sizeY = frameA->width * frameA->height ;
			int sizeU = sizeY / 4 ;
			int sizeV = sizeY / 4 ;

			if (memcmp(frameA->data[0], frameB->data[0], sizeY) != 0) {
				DBG("Not matched Y.\n");
				break;
			}
			if (memcmp(frameA->data[1], frameB->data[1], sizeU) != 0) {
				DBG("Not matched Y.\n");
				break;
			}
			if (memcmp(frameA->data[2], frameB->data[2], sizeV) != 0) {
				DBG("Not matched Y.\n");
				break;
			}

			rv = 0;
		} else {
			ERR("Frame A's format is %d and B's format is %d.\n", frameA->format, frameB->format) ;
			ERR("Not Support frame, just supporting format is AV_PIX_FMT_YUV420P.\n") ;
		}
	}
	while (0) ;

	return rv;
}

#ifdef ___DEBUG_SAVE_RAW___
static int ff_saveAVFrame (const char *path, AVFrame *frame)
{
	int rv = (-1) ;
	int fd ;

	do
	{
		if ((enum AVPixelFormat)frame->format != AV_PIX_FMT_YUV420P) {
			ERR("Not support a pixel format.\n") ;
			break;
		}
		if (frame->width <= 0 || frame->height <= 0) {
			ERR("Unknown resolution of frame.\n") ;
			break;
		}

		fd = open (path, O_CREAT|O_TRUNC|O_WRONLY, 0666) ;
		if (fd >= 0) {
			int lenY = frame->width * frame->height ;
			int lenUV = lenY / 4 ;

			write (fd, frame->data[0], lenY) ;
			write (fd, frame->data[1], lenUV) ;
			write (fd, frame->data[2], lenUV) ;

			rv = 0 ;
			close(fd) ;
		}
	}
	while (0) ;

	return rv ;
}
#endif /* ___DEBUG_SAVE_RAW___ */

void __attribute__((constructor)) ___init_ffmpeg (void)
{
	DBG("@@ INIT FFMPEG @@\n") ;
	av_register_all() ;
	avcodec_register_all() ;
	avfilter_register_all() ;
}
void __attribute__((destructor)) ___deinit_ffmpeg (void)
{
	DBG("@@ DE-INIT FFMPEG @@\n") ;
}

static int ff_decodeStillImage420 (uint32_t screenOffset, uint32_t screenPitch, const char *filePath, int width, int height)
{
	struct AVCodec *pCodec = NULL ;
	struct AVCodecContext *pCodecCtx = NULL ;
	enum AVCodecID codecId ;
	struct AVPacket packet ;
	struct AVFrame *pFrame = NULL ;
	struct AVFrame *pScaleFrame = NULL ;
	struct AVFrame *pFilterFrame = NULL ;
	int rv = (-1) ;


#ifdef  ___DEBUG_MEMORY_CHECK___
	system ("ps -e o pid,command,pmem,rsz,vsz k +rsz | grep testJpeg &") ;
#endif /* ___DEBUG_MEMORY_CHECK___ */

	do
	{
		/* init */
		av_init_packet (&packet) ;
		codecId = ff_getCodecId (filePath) ;
		if (codecId == AV_CODEC_ID_NONE) {
			ERR("@@ Do not find a CODEC ID.\n") ;
			break ;	
		}
		pFrame = av_frame_alloc() ;
		pCodec = avcodec_find_decoder (codecId) ;
		if (pCodec == NULL) {
			ERR("@@ Do not find a DECODER.\n") ;
			break ;
		}
		pCodecCtx = avcodec_alloc_context3 (pCodec) ;
		if (pCodecCtx == NULL) {
			ERR("@@ Failed avcodec_alloc_context3()\n");
			break ;
		}
		if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
			ERR("@@ Failed avcodec_open2()\n") ;
			break ;
		}
	
		/* decode */
		int gotFrame = 0 ;
		packet.size = readWholeFileToBuffer (filePath, &packet.data) ;
		if (packet.size <= 0) {
			ERR("Failed to read a image at buffer.\n") ;
			break ;
		}
		if (avcodec_decode_video2(pCodecCtx, pFrame, &gotFrame, &packet) < 0) {
			ERR("@@ Failed avcodec_decode_video2()\n") ;
			break ;
		}
		if (gotFrame) {
#ifdef ___PATCH_MEMORY_LEAK___
			pid_t cPid ;
#endif /* ___PATCH_MEMORY_LEAK___ */

			pScaleFrame = av_frame_alloc() ;
			if (av_image_alloc(pScaleFrame->data, pScaleFrame->linesize, width, height, AV_PIX_FMT_YUV420P, 1) < 0) {
				ERR("@@ Failed av_image_alloc() for scaling.\n") ;
				break ;
			}

			if (ff_convertPixelFormatWithScale(pCodecCtx, pFrame, width, height, AV_PIX_FMT_YUV420P, pScaleFrame) < 0) {
				ERR("@@\n") ;
				break ;
			}

			pFilterFrame = av_frame_alloc() ;
			if (av_image_alloc(pFilterFrame->data, pFilterFrame->linesize, width, height, AV_PIX_FMT_YUV420P, 1) < 0) {
				ERR("@@ Failed av_image_alloc() for filtering.\n") ;
				break ;
			}

#ifdef ___PATCH_MEMORY_LEAK___
			cPid = fork() ;
			if (cPid >= 0) {
				if (cPid == 0) {
					/* child */
					/* XXX - It is occurring memmory leak in the avfilter module, but do not find a cause.
							 Temporarily, change code with fork, but it is too expensively. */
					do
					{
#endif /* ___PATCH_MEMORY_LEAK___ */
						if (ff_convertFilter(pCodecCtx, "colormatrix=bt601:bt709", pScaleFrame, AV_PIX_FMT_YUV420P, pFilterFrame) < 0) {
							ERR("@@ Failed ff_convertFilter()\n") ;
							break ;
						}

						// ff_compareAVFrame (pScaleFrame, pFilterFrame) ;
						ff_renderFrame420 (screenOffset, screenPitch, pFilterFrame) ;

#ifdef ___DEBUG_SAVE_RAW___
						{
							static int fgFirst = 0 ;
							if (fgFirst++ == 0) {
								ff_saveAVFrame ("ff_scaled.image", pScaleFrame);
								ff_saveAVFrame ("ff_filterd.image", pFilterFrame);
								fgFirst = 1 ;
							}
						}
#endif /* ___DEBUG_SAVE_RAW___ */

#ifdef ___PATCH_MEMORY_LEAK___
						rv = 0 ;
					} 
					while (0) ;
					
					exit (rv) ;
				} else {
					int status ;
					int rv ;
					/* parent */
					rv = waitpid (cPid, &status, 0) ;
					rv = WEXITSTATUS(rv) ;
				}
			} else {
				ERR2("Fork Failed.\n") ;
				break;
			}
#endif /* ___PATCH_MEMORY_LEAK___ */
		}

		rv = 0 ;
	}
	while (0) ;
	
	/* de-init */
	if (pFilterFrame) {
#ifdef ___PATCH_MEMORY_LEAK___
		if (pFilterFrame->data[0]) {
			av_freep (&pFilterFrame->data[0]) ;
		}
#endif /* ___PATCH_MEMORY_LEAK___ */
		av_frame_free (&pFilterFrame) ;
	}
	if (pScaleFrame) {
		if (pScaleFrame->data[0]) {
			av_freep (&pScaleFrame->data[0]) ;
		}
		av_frame_free (&pScaleFrame) ;
	}
	if (pFrame) {
		av_frame_free (&pFrame) ;
	}
	if (packet.size > 0 && packet.data) {
		free (packet.data) ;
		packet.size = 0 ;
		packet.data = NULL ;
		av_free_packet (&packet) ;
	}
	DBG("\n");
	if (pCodecCtx) {
		avcodec_close (pCodecCtx) ;
		av_free (pCodecCtx) ;
	}

	return rv;
}

int tjdecode_fhd (uint32_t yOffset, uint32_t yPitch, const char *fname)
{
	return ff_decodeStillImage420 (yOffset, yPitch, fname, 1920, 1080) ;
}

int tjdecode_uhd (uint32_t yOffset, uint32_t yPitch, const char *fname)
{
	return ff_decodeStillImage420 (yOffset, yPitch, fname, 3840, 2160) ;
}

int main (int argc, char *argv[])
{
	int i;
	for (i=0; i<100; i++) {
		DBG("Count : %d\n", i) ;
		tjdecode_uhd (0, 0, "./3840x2160.jpg") ;
	}
	return 0;
}

#endif /* ___DECODE_FFMPEG___ */
