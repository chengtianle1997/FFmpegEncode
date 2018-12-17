

#include <stdio.h>



#define __STDC_CONSTANT_MACROS



#ifdef _WIN32

//Windows

extern "C"

{

#include "libavcodec/avcodec.h"

#include "libavformat/avformat.h"

#include "libavformat/avio.h"

#include  "libavutil/pixdesc.h"

#include  "mjpegenc.h"

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


int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index) {
	int ret;
	int got_frame;
	AVPacket enc_pkt;
	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities & 0x0020))
		return 0;
	while (1) {
		enc_pkt.data = NULL;
		enc_pkt.size = 0;
		av_init_packet(&enc_pkt);
		ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
			NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_frame) {
			ret = 0;
			break;
		}
		printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
		/* mux encoded frame */
		ret = av_write_frame(fmt_ctx, &enc_pkt);
		if (ret < 0)
			break;
	}
	return ret;
}


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

	int y_size;

	int got_picture = 0;

	int size;

	int framecount = 100;

	int ret = 0;

	int i;

	FILE *in_file = NULL;                            //YUV source

	int in_w = 480, in_h = 272;                           //YUV's width and height

	const char* out_file = "encode.mjpeg";    //Output file

	AVCodecID codec_id = AV_CODEC_ID_MJPEG;

	in_file = fopen("ds_480x272.yuv", "rb");

	av_register_all();




	//Method 1

	pFormatCtx = avformat_alloc_context();

	//Guess format

	fmt = av_guess_format("mjpeg", NULL, NULL);

	pFormatCtx->oformat = fmt;

	//Output URL

	if (avio_open(&pFormatCtx->pb, out_file, 1 | 2) < 0) {

		printf("Couldn't open output file.");

		return -1;

	}



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

	//Output some information

	av_dump_format(pFormatCtx, 0, out_file, 1);

	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);

	if (!pCodec) {

		printf("Codec not found.");

		return -1;

	}

	if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) {

		printf("Could not open codec.");

		return -1;

	}

	avcodec_parameters_from_context(video_st->codecpar, pCodecCtx);

	//alloc huffman
	MpegEncContext *mpegCtx;

	ret = ff_mjpeg_encode_init(mpegCtx);

	if (ret) {
		printf("ff_mjpeg_encode_init error!\n");
	}

	picture = av_frame_alloc();

	size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

	picture->width = pCodecCtx->width;

	picture->height = pCodecCtx->height;

	picture->format = AV_PIX_FMT_YUVJ420P;

	picture_buf = (uint8_t *)av_malloc(size);

	if (!picture_buf)

	{

		return -1;

	}

	avpicture_fill((AVPicture *)picture, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

	//Write Header

	avformat_write_header(pFormatCtx, NULL);



	y_size = pCodecCtx->width * pCodecCtx->height;

	for (i = 0; i < framecount; i++) {

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

			printf("Succeed to encode frame: %5d\tsize:%5d\n", i, pkt.size);

			pkt.stream_index = video_st->index;

			ret = av_write_frame(pFormatCtx, &pkt);

		}



		av_free_packet(&pkt);

	}

	//Flush Encoder
	ret = flush_encoder(pFormatCtx, 0);
	if (ret < 0) {
		printf("Flushing encoder failed\n");
		return -1;
	}

	//Write Trailer

	av_write_trailer(pFormatCtx);



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
