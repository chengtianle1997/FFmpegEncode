

#include <stdio.h>



#define __STDC_CONSTANT_MACROS


extern "C"

{

#include "libavcodec/avcodec.h"

#include "libavformat/avformat.h"

#include "libavformat/avio.h"

};




bool compressImageX264(Frame *f, int size, char* image) {
	codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec) {
		std::cout << "Codec not found" << std::endl;
		return false;
	}
	c = avcodec_alloc_context3(codec);
	if (!c) {
		std::cout << "Could not allocate video codec context" << std::endl;
		return false;
	}
	av_opt_set(c->priv_data, "preset", "veryfast", 0);
	av_opt_set(c->priv_data, "tune", "zerolatency", 0);
	c->width = std::stoi(f->width);
	c->height = std::stoi(f->height);
	c->gop_size = 10;
	c->max_b_frames = 1;
EDIT: c->pix_fmt = AV_PIX_FMT_YUV420P;
	/* open it */
	if (avcodec_open2(c, codec, NULL) < 0) {
		std::cout << "Could not open codec" << std::endl;
		return false;
	}

	AVFrame *avFrameRGB = av_frame_alloc();
	if (!avFrameRGB) {
		std::cout << "Could not allocate video frame" << std::endl;
		return false;
	}
	avFrameRGB->format = std::stoi(f->channels) > 1 ? AV_PIX_FMT_RGB24 : AV_PIX_FMT_GRAY8;
	avFrameRGB->width = c->width;
	avFrameRGB->height = c->height;

	int ret = av_image_alloc(
		avFrameRGB->data, avFrameRGB->linesize,
		avFrameRGB->width, avFrameRGB->height, AVPixelFormat(avFrameRGB->format),
		32);
	if (ret < 0) {
		std::cout << "Could not allocate raw picture buffer" << std::endl;
		return false;
	}

	uint8_t *p = avFrameRGB->data[0];
	for (int i = 0; i < size; i++) {
		*p++ = image[i];
	}
	AVFrame* avFrameYUV = av_frame_alloc();
	avFrameYUV->format = AV_PIX_FMT_YUV420P;
	avFrameYUV->width = c->width;
	avFrameYUV->height = c->height;
	ret = av_image_alloc(
		avFrameYUV->data, avFrameYUV->linesize,
		avFrameYUV->width, avFrameYUV->height, AVPixelFormat(avFrameYUV->format),
		32);

	SwsContext *img_convert_ctx = sws_getContext(c->width, c->height, AVPixelFormat(avFrameRGB->format),
		c->width, c->height, AV_PIX_FMT_YUV420P,
		SWS_FAST_BILINEAR, NULL, NULL, NULL);
	ret = sws_scale(img_convert_ctx,
		avFrameRGB->data, avFrameRGB->linesize,
		0, c->height,
		avFrameYUV->data, avFrameYUV->linesize);
	sws_freeContext(img_convert_ctx);

	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = NULL; // packet data will be allocated by the encoder
	pkt.size = 0;

	avFrameYUV->pts = frameCount; frameCount++; //GLOBAL VARIABLE
	int got_output;
	ret = avcodec_encode_video2(c, &pkt, avFrameYUV, &got_output);
	if (ret < 0) { //<-- Where the code broke
		std::cout << "Error encoding frame" << std::endl;
		return false;
	}
	if (got_output) {
		std::cout << "Write frame " << frameCount - 1 << "(size = " << pkt.size << ")" << std::endl;
		char* buffer = new char[pkt.size];
		for (int i = 0; i < pkt.size; i++) {
			buffer[i] = pkt.data[i];
		}
		f->buffer = buffer;
		f->size = std::to_string(pkt.size);
		f->compression = std::string("x264");
		av_free_packet(&pkt);
	}
	return true;
}
