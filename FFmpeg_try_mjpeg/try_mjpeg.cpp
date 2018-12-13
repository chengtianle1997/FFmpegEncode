#include <stdio.h>
#define __STDC_CONSTANT_MACROS
#define _CRT_SECURE_NO_WARNINGS

#include "iostream"




#ifdef _WIN64

//Windows

extern "C"

{

#include "libavutil/opt.h"

#include "libavcodec/avcodec.h"

#include "libavutil/imgutils.h"

#include "libavformat/avformat.h"

#include "libavformat/avio.h"

};

#else

//Linux...

#ifdef __cplusplus

extern "C"

{

#endif

#include <libavutil/opt.h>

#include <libavcodec/avcodec.h>

#include <libavutil/imgutils.h>

#ifdef __cplusplus

};

#endif

#endif

#pragma comment(lib, "avcodec.lib")   

#pragma comment(lib, "avutil.lib")  

//test different codec

#define TEST_H264  1

#define TEST_HEVC  0



int main(int argc, char* argv[])

{
	//InitCamera();

	//<span style = "white-space:pre">	< / span>
	AVCodec *pCodec;

	AVCodecContext *pCodecCtx = NULL;

	int i, ret, got_output(0);

	FILE *fp_in;

	FILE *fp_out;

	AVFrame *pFrame;

	AVPacket pkt;



	int y_size;

	int framecnt = 0;
	char filename_in[] = "ds_480x272.yuv";





#if TEST_HEVC

	AVCodecID codec_id = AV_CODEC_ID_HEVC;

	char filename_out[] = "ds.hevc";

#else
	//转码后的视频格式
	AVCodecID codec_id = AV_CODEC_ID_MJPEG;

	char filename_out[] = "ds.mjpeg";

#endif

	int in_w = 480, in_h = 272;

	int framenum = 100;
	//注册所有编译器
	avcodec_register_all();
	//查找编码器
	pCodec = avcodec_find_encoder(codec_id);

	if (!pCodec) {

		printf("Codec not found\n");

		return -1;

	}
	//申请AVcodecContext空间
	pCodecCtx = avcodec_alloc_context3(pCodec);

	if (!pCodecCtx) {

		printf("Could not allocate video codec context\n");

		return -1;

	}

	pCodecCtx->bit_rate = 400000;

	pCodecCtx->width = in_w;

	pCodecCtx->height = in_h;

	pCodecCtx->time_base.num = 1;

	pCodecCtx->time_base.den = 25;

	pCodecCtx->gop_size = 100;

	pCodecCtx->framerate.den = 1;

	pCodecCtx->framerate.num = 25;

	//pCodecCtx->max_b_frames = 1;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;

	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;

	if (codec_id == AV_CODEC_ID_MJPEG) {
		av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);
	}
		

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {

		printf("Could not open codec\n");

		return -1;

	}


	//frame申请空间
	pFrame = av_frame_alloc();

	if (!pFrame) {

		printf("Could not allocate video frame\n");

		return -1;

	}

	pFrame->format = pCodecCtx->pix_fmt;

	pFrame->width = pCodecCtx->width;

	pFrame->height = pCodecCtx->height;




	ret = av_image_alloc(pFrame->data, pFrame->linesize, pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 16);
	/*
	pointers[4]：保存图像通道的地址。如果是RGB，则前三个指针分别指向R, G, B的内存地址。第四个指针保留不用
		linesizes[4]：保存图像每个通道的内存对齐的步长，即一行的对齐内存的宽度，此值大小等于图像宽度。
		w : 要申请内存的图像宽度。
		h : 要申请内存的图像高度。
		pix_fmt : 要申请内存的图像的像素格式。
		align : 用于内存对齐的值。
		返回值：所申请的内存空间的总大小。如果是负值，表示申请失败。
		/**
		 * Allocate an image with size w and h and pixel format pix_fmt, and

		 * fill pointers and linesizes accordingly.

		 * The allocated image buffer has to be freed by using

		 * av_freep(&pointers[0]).

		 *

		 * @param align the value to use for buffer size alignment

		 * @return the size in bytes required for the image buffer, a negative

		 * error code in case of failure

		 */
	if (ret < 0) {

		printf("Could not allocate raw picture buffer\n");

		return -1;

	}

	//Input raw data
	fp_in = fopen(filename_in, "rb");

	if (!fp_in) {

		printf("Could not open %s\n", filename_in);

		return -1;

	}

	//Output bitstream

	fp_out = fopen(filename_out, "wb");

	if (!fp_out) {

		printf("Could not open %s\n", filename_out);

		return -1;

	}





	y_size = pCodecCtx->width * pCodecCtx->height;

	//Encode

	for (i = 0; i < framenum; i++) {

		//got_output = 0;

		av_init_packet(&pkt);

		pkt.data = NULL;    // packet data will be allocated by the encoder

		pkt.size = 0;

		//Read raw YUV data

		if (fread(pFrame->data[0], 1, y_size, fp_in) <= 0 ||		// Y

			fread(pFrame->data[1], 1, y_size / 4, fp_in) <= 0 ||	// U

			fread(pFrame->data[2], 1, y_size / 4, fp_in) <= 0) {	// V

			return -1;

		}
		else if (feof(fp_in)) {

			break;

		}

		pFrame->pts = i;

		/* encode the image */

		ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_output);

		if (ret < 0) {

			printf("Error encoding frame\n");

			return -1;

		}

		if (got_output) {

			printf("Succeed to encode frame: %5d\tsize:%5d\n", framecnt, pkt.size);

			framecnt++;

			fwrite(pkt.data, 1, pkt.size, fp_out);

			av_free_packet(&pkt);

		}

	}
	av_write_trailer();

	//Flush Encoder

	for (got_output = 1; got_output; i++) {

		ret = avcodec_encode_video2(pCodecCtx, &pkt, NULL, &got_output);

		if (ret < 0) {
			printf("Error encoding frame\n");
			return -1;

		}

		if (got_output) {

			printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", pkt.size);

			fwrite(pkt.data, 1, pkt.size, fp_out);

			av_free_packet(&pkt);

		}

	}

	fclose(fp_out);

	avcodec_close(pCodecCtx);

	av_free(pCodecCtx);

	av_freep(&pFrame->data[0]);

	av_frame_free(&pFrame);

	return 0;

}