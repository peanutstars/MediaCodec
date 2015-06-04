/*
 * Copyright (c) 2010 Nicolas George
 * Copyright (c) 2011 Stefano Sabatini
 * Copyright (c) 2014 Andrey Utkin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * API example for demuxing, decoding, filtering, encoding and muxing
 * @example transcoding.c
 */

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

static AVFormatContext *ifmt_ctx;
static unsigned int sg_codec_tag = 0;

static int open_input_file(const char *filename)
{
    int ret;
    unsigned int i;

    ifmt_ctx = NULL;
    if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *stream;
        AVCodecContext *codec_ctx;
        stream = ifmt_ctx->streams[i];
        codec_ctx = stream->codec;
        /* Reencode video & audio and remux subtitles etc. */
        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
                || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            /* Open decoder */
            ret = avcodec_open2(codec_ctx, avcodec_find_decoder(codec_ctx->codec_id), NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                return ret;
            }
        }

		if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
			if (codec_ctx->codec_id == AV_CODEC_ID_HEVC) {
				sg_codec_tag = codec_ctx->codec_tag;
			}
//			printf ("CODEC ID : %d, %X\n", codec_ctx->codec_id, codec_ctx->codec_tag);
		}
    }

    av_dump_format(ifmt_ctx, 0, filename, 0);
    return 0;
}

static void hevc_hexdump (const char *msg, const uint8_t *data, int dsize, int pretty)
{
	int i;
	int fgNL = 0;

	if (msg) {
		printf ("@@@ %s : %s", msg, (pretty?"\n":""));
	}

	for (i=0; i<dsize; i++) {
		fgNL = 0;
		if (pretty && (i%16) == 0) {
			printf ("%4X : ", i);
		}
		printf ("%02hhX ", data[i]);
		if (pretty && (i%16) == 7) {
			printf (" ");
		}
		if (pretty && (i%16) == 15) {
			printf ("\n");
			fgNL = 1;
		}
	}
	
	if ( ! fgNL) {
		printf ("\n");
	}
}


static void hevc_sei_dump (int type, const uint8_t *data, int size)
{
	char msg[64];
	snprintf (msg, sizeof(msg), "%3d:%02hhX(%d)", type, type, size);
	hevc_hexdump (msg, data, size, 0);
}

static void hevc_sei_parse (int nalType, const uint8_t *data, int dsize)
{
#define GetData()			data[idx++]
#define GetDataPosition()	&data[idx]
#define GetRemainCount()	(dsize-idx-1)
	int type = 0;
	int size = 0;
	int idx = 0;
	int byte = 0xFF;
	
//	printf ("### %02hhX %02hhX S(%d)\n", data[idx], data[idx+1], dsize);
	while (byte == 0xFF) {
		byte = GetData();
		type += byte;
	}
	byte = 0xFF;
	while (byte == 0xFF) {
		byte = GetData();
		size += byte;
	}
	if (nalType == 39) {			// Prefix
		if (type == 137) {
			hevc_sei_dump (type, GetDataPosition(), size);
		}
	} else if (nalType == 40) {		// Subfix
	} else {
		hevc_sei_dump (type, GetDataPosition(), size);
	}

	idx += size;
//	printf ("## Remain Size : %d(%d-%d-1-%d)  %02hhX %02hhX %02hhX\n", 
//			GetRemainCount(), dsize, idx, size,
//			data[idx], data[idx+1], data[idx+2]);
	if (GetRemainCount() > 2) {
		hevc_sei_parse (nalType, GetDataPosition(), GetRemainCount());
	}
#undef GetRemainCount
#undef GetDataPosition	
#undef GetData
}

static void hevc_nal_parse_other (const uint8_t *data, int dsize)
{
#define GetData()			data[idx++]
#define GetDataPosition()	&data[idx]
	int idx=0;

	/* XX XX XX XX : First 4 bytes is meaned to NAL length.
	   NN NN       : Next 2 Bytes is meaned to NAL Header		*/

	while (idx < dsize)
	{
		uint32_t pktSize = 0;
		uint32_t nalHeader = 0;
		int nalType;
		int i;
#if 0
		{
			int x;
			printf ("@@ ");
			for (x=0; x<8; x++) {
				printf ("%02hhX ", data[idx+x]);
			}
			printf ("\n");
		}
#endif 
		for (i=0; i<4; i++) {
			pktSize <<= 8;
			pktSize |= (GetData() & 0xFF);
		}
		for (i=0; i<2; i++) {
			nalHeader <<= 8;
			nalHeader |= (GetData() & 0xFF);
		}
		pktSize -= 2;	// decrease NAL Header's size

//		printf ("Size : %X, NAL : %X\n", pktSize, nalHeader);

		if (nalHeader & 0xFFFF81F8) {
			printf ("It is wrong format about NAL Header, %04X and skip this NAL.\n", nalHeader);
			idx += pktSize;
			continue;
		}
		nalType = nalHeader >> 9;
		if (nalType == 39 || nalType == 40) {
			hevc_sei_parse (nalType, GetDataPosition(), pktSize);
		}
		idx += pktSize;
	}
#undef GetDataPosition	
#undef GetData
}
static void hevc_nal_parse (const uint8_t *data, int dsize)
{
	int idx;
	uint32_t *sync;
	uint32_t syncValue = 0;
	int fgDump = 0;

	for (idx=0; idx<(dsize-4); idx++) {
		/* 00 00 00 01 : 0x0100_0000
		   00 00 01 HH : 0xHH01_0000	*/

		syncValue = 0;
		sync = (uint32_t *) &data[idx];
		if ( *sync == 0x01000000) {
			syncValue = 4;
		} else {
			if ( (*sync & 0x00FFFFFF) == 0x010000) {
				syncValue = 3;
			}
		}

		if (syncValue > 0) {
			int x;
			int nalType = data[idx+syncValue] >> 1;
			int startIdx;

			if (fgDump) {
				printf (" %d %02hhX %02hhX (NT:%2d) : ", syncValue, data[idx+syncValue], data[idx+syncValue+1], nalType);
			}
			idx += syncValue;

			startIdx = idx;

			for (x=0; (idx+x)<dsize; x++) {
				syncValue = 0;
				sync = (uint32_t *) &data[idx+x];
				if ( *sync == 0x01000000) {
					syncValue = 4;
				} else {
					if ( (*sync & 0x00FFFFFF) == 0x010000) {
						syncValue = 3;
					}
				}

				if (syncValue != 0 || x >= 1024) {
					idx += x;
					if (fgDump) {
						printf ("\n");
					}
					break;
				}
				if (fgDump) {
					printf ("%02hhX ", data[idx+x]);
				}
			}

			if (nalType == 39 || nalType == 40) {
				hevc_sei_parse (nalType, &data[startIdx+2], x-2);
			}
		}
	}
}

int main(int argc, char **argv)
{
    int ret;
    AVPacket packet = { .data = NULL, .size = 0 };
    AVFrame *frame = NULL;
    unsigned int i;

    if (argc != 2) {
        av_log(NULL, AV_LOG_ERROR, "Usage: %s <input file> <output file>\n", argv[0]);
        return 1;
    }

    av_register_all();
    avfilter_register_all();

    if ((ret = open_input_file(argv[1])) < 0)
        goto end;

    /* read all packets */
    while (1) {
        if ((ret = av_read_frame(ifmt_ctx, &packet)) < 0)
            break;
		if ( ifmt_ctx->streams[packet.stream_index]->codec->codec_type == AVMEDIA_TYPE_VIDEO ) {
			uint8_t *data = packet.data;

			if (sg_codec_tag == MKTAG('H','E','V','C')) {
				hevc_nal_parse (data, packet.size);
			} else {
				hevc_nal_parse_other (data, packet.size);
			}
		}
        av_free_packet(&packet);

    }
end:
    av_free_packet(&packet);
    av_frame_free(&frame);
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        avcodec_close(ifmt_ctx->streams[i]->codec);
    }
    avformat_close_input(&ifmt_ctx);

    if (ret < 0)
        av_log(NULL, AV_LOG_ERROR, "Error occurred: %s\n", av_err2str(ret));

    return ret ? 1 : 0;
}
