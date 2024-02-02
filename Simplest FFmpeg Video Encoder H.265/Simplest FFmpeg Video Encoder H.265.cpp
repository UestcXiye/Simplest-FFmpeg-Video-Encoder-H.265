// Simplest FFmpeg Video Encoder H.265.cpp : 定义控制台应用程序的入口点。
//

/**
* 最简单的基于 FFmpeg 的视频编码器
* Simplest FFmpeg Video Encoder
*
* 源程序：
* 雷霄骅 Lei Xiaohua
* leixiaohua1020@126.com
* 中国传媒大学/数字电视技术
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* 修改：
* 刘文晨 Liu Wenchen
* 812288728@qq.com
* 电子科技大学/电子信息
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* 本程序实现了 YUV 像素数据编码为视频码流（HEVC(H.265)，H264，MPEG2，VP8 等等）。
* 是最简单的 FFmpeg 视频编码方面的教程。
* 通过学习本例子可以了解 FFmpeg 的编码流程。
*
* This software encode YUV420P data to HEVC(H.265) bitstream (or
* H.264, MPEG2, VP8 etc.).
* It's the simplest video encoding software based on FFmpeg.
* Suitable for beginner of FFmpeg
*
*/

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>

// 解决报错：fopen() 函数不安全
#pragma warning(disable:4996)

// 解决报错：无法解析的外部符号 __imp__fprintf，该符号在函数 _ShowError 中被引用
#pragma comment(lib, "legacy_stdio_definitions.lib")
extern "C"
{
	// 解决报错：无法解析的外部符号 __imp____iob_func，该符号在函数 _ShowError 中被引用
	FILE __iob_func[3] = { *stdin, *stdout, *stderr };
}

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
// Windows
extern "C"
{
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
};
#else
// Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
};
#endif
#endif

int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index)
{
	int ret;
	int got_frame;
	AVPacket enc_pkt;

	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities & CODEC_CAP_DELAY))
	{
		return 0;
	}

	while (1)
	{
		printf("Flushing stream #%u encoder.\n", stream_index);
		enc_pkt.data = NULL;
		enc_pkt.size = 0;
		av_init_packet(&enc_pkt);
		ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
			NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_frame)
		{
			ret = 0;
			break;
		}
		printf("Flush Encoder: Succeed to encode 1 frame! size:%5d.\n", enc_pkt.size);
		// mux encoded frame
		ret = av_write_frame(fmt_ctx, &enc_pkt);
		if (ret < 0)
		{
			break;
		}
	}
	return ret;
}

int main(int argc, char* argv[])
{
	AVFormatContext* pFormatCtx;
	AVOutputFormat* fmt;
	AVStream* video_stream;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;

	AVPacket pkt;
	uint8_t* picture_buf;
	AVFrame* pFrame;

	int ret;
	int size = 0;
	int y_size = 0;
	int framecnt = 0;

	// Input raw YUV data 
	FILE *fp_in = fopen("ds_480x272.yuv", "rb");
	// FILE *in_file = fopen("src01_480x272.yuv", "rb");
	const int in_width = 480, in_height = 272; // Input data's width and height
	int framenum = 100; // Frames to encode 

	// Output Filepath 
	// const char* out_file = "ds.h264";
	const char* out_file = "ds.hevc";
	// const char* out_file = "src01.h264";              
	// const char* out_file = "src01.ts";
	
	av_register_all();

	// Method 1
	// pFormatCtx = avformat_alloc_context();
	// fmt = av_guess_format(NULL, out_file, NULL); // Guess Format
	// pFormatCtx->oformat = fmt;

	// Method 2
	avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);
	fmt = pFormatCtx->oformat;

	// Open output URL
	ret = avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE);
	if (ret < 0)
	{
		// 输出文件打开失败
		printf("Can't open output file.\n");
		return -1;
	}

	video_stream = avformat_new_stream(pFormatCtx, 0);
	video_stream->time_base.num = 1;
	video_stream->time_base.den = 25;

	if (video_stream == NULL)
	{
		printf("Can't create video stream.\n");
		return -1;
	}

	// Param that must set
	pCodecCtx = video_stream->codec;
	// pCodecCtx->codec_id = AV_CODEC_ID_HEVC;
	pCodecCtx->codec_id = fmt->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->width = in_width;
	pCodecCtx->height = in_height;
	pCodecCtx->bit_rate = 400000;
	pCodecCtx->gop_size = 250;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;
	// H.264
	// pCodecCtx->me_range = 16;
	// pCodecCtx->max_qdiff = 4;
	// pCodecCtx->qcompress = 0.6;
	pCodecCtx->qmin = 10;
	pCodecCtx->qmax = 51;

	// Optional Param
	pCodecCtx->max_b_frames = 3;

	// Set Option
	AVDictionary *param = 0;
	// H.264
	if (pCodecCtx->codec_id == AV_CODEC_ID_H264)
	{
		av_dict_set(&param, "preset", "slow", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);
		// av_dict_set(&param, "profile", "main", 0);
	}
	// H.265
	if (pCodecCtx->codec_id == AV_CODEC_ID_H265)
	{
		av_dict_set(&param, "x265-params", "qp=20", 0);
		av_dict_set(&param, "preset", "ultrafast", 0);
		av_dict_set(&param, "tune", "zero-latency", 0);
	}

	// Show some Information
	av_dump_format(pFormatCtx, 0, out_file, 1);

	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec)
	{
		// 没有找到合适的编码器
		printf("Can't find encoder.\n");
		return -1;
	}

	ret = avcodec_open2(pCodecCtx, pCodec, &param);
	if (ret < 0)
	{
		// 编码器打开失败
		printf("Failed to open encoder.\n");
		return -1;
	}

	pFrame = avcodec_alloc_frame();
	size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	picture_buf = (uint8_t *)av_malloc(size);
	avpicture_fill((AVPicture *)pFrame, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

	// Write File Header
	avformat_write_header(pFormatCtx, NULL);

	y_size = pCodecCtx->width * pCodecCtx->height;
	av_new_packet(&pkt, 3 * y_size);

	for (int i = 0; i < framenum; i++)
	{
		// Read raw YUV data
		if (fread(picture_buf, 1, y_size * 3 / 2, fp_in) <= 0)
		{
			// 文件读取错误
			printf("Failed to read raw YUV data.\n");
			return -1;
		}
		else if (feof(fp_in))
		{
			break;
		}

		pFrame->data[0] = picture_buf; // Y
		pFrame->data[1] = picture_buf + y_size; // U 
		pFrame->data[2] = picture_buf + y_size * 5 / 4; // V

		// PTS
		pFrame->pts = i;
		// pFrame->pts = i*(video_stream->time_base.den) / ((video_stream->time_base.num) * 25);

		int got_picture = 0;
		// Encode
		ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
		if (ret < 0)
		{
			printf("Failed to encode! \n");
			return -1;
		}
		if (got_picture == 1)
		{
			printf("Succeed to encode frame: %5d\tsize:%5d.\n", framecnt, pkt.size);
			framecnt++;
			pkt.stream_index = video_stream->index;
			ret = av_write_frame(pFormatCtx, &pkt);
			av_free_packet(&pkt);
		}
	}

	// Flush Encoder
	ret = flush_encoder(pFormatCtx, 0);
	if (ret < 0)
	{
		printf("Flushing encoder failed.\n");
		return -1;
	}

	// Write file trailer
	av_write_trailer(pFormatCtx);

	// Clean
	if (video_stream)
	{
		avcodec_close(video_stream->codec);
		av_free(pFrame);
		av_free(picture_buf);
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);

	fclose(fp_in);

	system("pause");
	return 0;
}