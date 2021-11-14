#pragma once

#include <iostream>
#include <string>
#include <stdio.h>
#include <fstream>


extern "C"
{
#include <libavformat\avformat.h>
#include <libavdevice\avdevice.h>
#include <libavcodec\avcodec.h>
#include <libswscale\swscale.h>
#include <libavutil\imgutils.h>
#include <libavutil\avutil.h>
}

using namespace std;

class screen
{
public:
	screen();
	~screen();

	// 初始化输入context;
	int initFormatContext(string input, string url);

	// 初始化输出context;
	int initOutFormatContext();

	// 构造解码器
	int buildCodecContext();

	// 构造编码器
	int buildEncodecContext();

	// 构造图像转换context
	int buildSwsContext();

	// 录制
	int record();

private:

	AVFormatContext	*formatContext = NULL;

	AVFormatContext *outFormatContext = NULL;

	AVCodecContext *codecCtx;

	AVCodecContext *outCodecCtx;

	const AVCodec *codec;

	const AVCodec *outCodec;

	AVPacket *packet;

	AVFrame *frame;

	AVPacket *outPacket;

	AVFrame *outFrame;

	AVDictionary *options;

	SwsContext *swsCtx;

	AVStream *outStream;

	int videoIndex;

};