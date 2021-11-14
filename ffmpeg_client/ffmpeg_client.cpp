// ffmpeg_client.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

//#include <iostream>
//#include <string>
//#include <stdio.h>
//#include <fstream>
#include "screen.h"
#include "audio.h"
//
//extern "C"
//{
//#include <libavformat\avformat.h>
//#include <libavdevice\avdevice.h>
//#include <libavcodec\avcodec.h>
//#include <libswscale\swscale.h>
//#include <libavutil\imgutils.h>
//}


int main(int argc, char* argv[])
{
	
	char c;
	if ((c = cin.get()) == '1') {
		screen s;

		s.initFormatContext("gdigrab", "desktop");
		s.initOutFormatContext();

		s.buildCodecContext();

		s.buildEncodecContext();

		s.buildSwsContext();

		s.record();

		s.write();
	}
	else
	{
		audio a;
		a.initFormatContext("audio=立体声混音(Realtek(R) Audio)");
		a.initOutFormatContext();
		a.buildCodecContext();
		a.buildEncodecContext();
		a.initAudioBuffer();
		a.buildSwrContext();
		a.record();
	}
	system("pause");
	return 0;
}

//void init_outFile() {
//	int ret = 0;
//	// 编码器
//	const AVCodec *outCode = avcodec_find_encoder(AV_CODEC_ID_H264);
//	AVCodecContext *outCodecCtx = avcodec_alloc_context3(outCode);
//
//	// 码率
//	outCodecCtx->bit_rate = 400000;
//	// 每25帧产生一个关键帧 I帧
//	outCodecCtx->gop_size = 10;
//	outCodecCtx->width = 1920;
//	outCodecCtx->height = 1080;
//	// 先转成 AV_PIX_FMT_YUV420P
//	outCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
//	outCodecCtx->codec_id = AV_CODEC_ID_H264;
//	outCodecCtx->time_base.num = 1;
//	outCodecCtx->time_base.den = 25;
//
//	outCodecCtx->framerate.num = 25;
//	// 如果编码器id是h264，
//	if (outCodecCtx->codec_id == AV_CODEC_ID_H264) {
//		// preset表示采用一个预先设定好的参数集，级别是slow，slow表示压缩速度是慢的，慢的可以保证视频质量，用快的会降低视频质量
//		av_opt_set(outCodecCtx->priv_data, "preset", "slow", 0);
//	}
//
//
//	// 打开编解码器
//	if ((ret = avcodec_open2(outCodecCtx, outCode, NULL)) < 0)
//	{
//		av_log(NULL, AV_LOG_ERROR, "Could not open codec.\n");
//		exit(1);
//	}
//
//	AVFrame *outFrame = av_frame_alloc();
//
//	outFrame->format = outCodecCtx->pix_fmt;
//	outFrame->width = outCodecCtx->width;
//	outFrame->height = outCodecCtx->width;
//	return ret;
//}
