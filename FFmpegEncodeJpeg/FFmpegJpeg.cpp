

#include <stdio.h>



#define MAX_PATH 100
#define __STDC_CONSTANT_MACROS



#ifdef _WIN32

//Windows

extern "C"

{

#include "libavcodec/avcodec.h"

#include "libavformat/avformat.h"

#include "libavformat/avio.h"

};

#else

//Linux...

#ifdef __cplusplus

extern "C"

{

#endif

#include <libavcodec/avcodec.h>

#include <libavformat/avformat.h>

#ifdef __cplusplus

};

#endif

#endif





int main(int argc, char* argv[])

{

	AVFormatContext* pFormatCtx;

	AVOutputFormat* fmt;

	AVStream* video_st;

	AVCodecContext* pCodecCtx;

	AVCodec* pCodec;

	uint8_t* picture_buf;

	AVFrame* picture;

	AVPacket pkt;

	int CameraNum = 0;

	int y_size;

	int got_picture = 0;

	int size;

	int framecount = 100;

	int ret = 0;

	int i;

	FILE *in_file = NULL;                            //YUV source

	int in_w = 480, in_h = 272;                           //YUV's width and height

	//const char* out_file = "encode.peg";    //Output file

	AVCodecID codec_id = AV_CODEC_ID_MJPEG;

	in_file = fopen("ds_480x272.yuv", "rb");

	av_register_all();

	// 分配AVFormatContext对象
	pFormatCtx = avformat_alloc_context();

	//Guess format
	fmt = av_guess_format("mjpeg", NULL, NULL);

	pFormatCtx->oformat = fmt;

	//Method 2. More simple

	//avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);

	//fmt = pFormatCtx->oformat;
	video_st = avformat_new_stream(pFormatCtx, 0);

	if (video_st == NULL) {

		return -1;

	}


	pCodecCtx = video_st->codec;

	pCodecCtx->codec_id = fmt->video_codec;

	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;

	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;

	pCodecCtx->bit_rate = 400000;

	pCodecCtx->width = in_w;

	pCodecCtx->height = in_h;

	pCodecCtx->time_base.num = 1;

	pCodecCtx->time_base.den = 25;

	pCodecCtx->framerate.num = 25;

	pCodecCtx->framerate.den = 1;

	AVDictionary *param = 0;

	av_dict_set(&param, "preset", "slow", 0);

	av_dict_set(&param, "tune", "zerolatecy", 0);

	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);

	if (!pCodec) {

		printf("Codec not found.");

		return -1;

	}

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {

		printf("Could not open codec.");

		return -1;

	}

	avcodec_parameters_from_context(video_st->codecpar, pCodecCtx);

	picture = av_frame_alloc();//为每帧图片分配内存

	size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

	picture->width = pCodecCtx->width;

	picture->height = pCodecCtx->height;

	picture->format = pCodecCtx->pix_fmt;

	picture_buf = (uint8_t *)av_malloc(size);

	if (!picture_buf)

	{

		return -1;

	}

	avpicture_fill((AVPicture *)picture, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);






	y_size = pCodecCtx->width * pCodecCtx->height;

	for (i = 0; i < framecount; i++) {

		char* out_file = 0;

		out_file = (char*)malloc(25);
		
		//输出文件
		sprintf(out_file, "Camera%d_%d.jpeg", CameraNum,i);
	
		//Output some information

		av_dump_format(pFormatCtx, 0, out_file, 1);
		
		//Output URL
		if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {

			printf("Couldn't open output file.");

			return -1;
		}

		avformat_init_output(pFormatCtx, &param);
		//Write Header

		avformat_write_header(pFormatCtx, &param);



		av_new_packet(&pkt, y_size * 3);

		//Read YUV

		if (fread(picture_buf, 1, y_size * 3 / 2, in_file) <= 0)

		{

			printf("Could not read input file.");

			return -1;

		}

		picture->data[0] = picture_buf;              // Y

		picture->data[1] = picture_buf + y_size;      // U 

		picture->data[2] = picture_buf + y_size * 5 / 4;  // V

		picture->pts = i;

		//Encode

		ret = avcodec_encode_video2(pCodecCtx, &pkt, picture, &got_picture);

		if (ret < 0) {

			printf("Encode Error.\n");

			return -1;

		}

		if (got_picture == 1) {

			pkt.stream_index = video_st->index;

			ret = av_write_frame(pFormatCtx, &pkt);

		}

		av_free_packet(&pkt);
		
		//Write Trailer
		av_write_trailer(pFormatCtx);

		free(out_file);

	}
	
	printf("Encode Successful.\n");



	if (video_st) {

		avcodec_close(video_st->codec);

		av_free(picture);

		av_free(picture_buf);

	}

	avio_close(pFormatCtx->pb);

	avformat_free_context(pFormatCtx);



	fclose(in_file);



	return 0;

}
