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

int main1()
{
	int ret = 0;
	avdevice_register_all();
	const AVInputFormat *pAVInputFormat = av_find_input_format("gdigrab");

	AVFormatContext	*formatContext = NULL;

	if ((ret = avformat_open_input(&formatContext, "desktop", pAVInputFormat, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open input stream.\n");
		return ret;
	}
	if ((ret = avformat_find_stream_info(formatContext, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Couldn't find stream information.\n");
		return ret;
	}
	int videoIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	AVStream *stream = formatContext->streams[videoIndex];

	const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);

	AVCodecParameters *avCodeParams = stream->codecpar;

	AVCodecContext *codecCtx = avcodec_alloc_context3(codec);

	avcodec_parameters_to_context(codecCtx, avCodeParams);

	if ((ret = avcodec_open2(codecCtx, codec, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not open codec.\n");
		return ret;
	}

	AVPacket *packet = av_packet_alloc();
	AVFrame *frame = av_frame_alloc();


	//½âÂëÆ÷
	AVFormatContext *outFormatContext = NULL;
	avformat_alloc_output_context2(&outFormatContext, NULL, NULL, "test.h264");
	const AVOutputFormat *outformat = outFormatContext->oformat;
	AVStream *outStream = avformat_new_stream(outFormatContext, NULL);
	const AVCodec *outCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	AVCodecContext *outCodecCtx = avcodec_alloc_context3(outCodec);
	outCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	outCodecCtx->bit_rate = 2000000;
	outCodecCtx->gop_size = 250;
	outCodecCtx->width = 1920;
	outCodecCtx->height = 1080;
	outCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	outCodecCtx->codec_id = AV_CODEC_ID_H264;
	outCodecCtx->time_base.num = 1;
	outCodecCtx->time_base.den = 25;
	outCodecCtx->framerate.num = 25;
	if ((ret = avcodec_open2(outCodecCtx, outCodec, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not open codec.\n");
		exit(1);
	}
	AVFrame *outFrame = av_frame_alloc();
	outFrame->format = outCodecCtx->pix_fmt;
	outFrame->width = outCodecCtx->width;
	outFrame->height = outCodecCtx->width;
	outFrame->pts = 0;

	SwsContext *swsCtx = sws_getContext(codecCtx->width,
		codecCtx->height,
		codecCtx->pix_fmt,
		outCodecCtx->width,
		outCodecCtx->height,
		outCodecCtx->pix_fmt,
		SWS_BICUBIC, NULL, NULL, NULL);

	ret = av_frame_get_buffer(outFrame, 32);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "nerror in filling image array");
		return -1;
	}
	//outPacket = av_packet_alloc();
	if (avio_open(&outFormatContext->pb, "test.h264", AVIO_FLAG_READ_WRITE) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to open output file! \n");
		return -1;
	}
	outFormatContext->streams[0]->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	outFormatContext->streams[0]->codecpar->width = 1920;
	outFormatContext->streams[0]->codecpar->height = 1080;
	ret = avformat_write_header(outFormatContext, NULL);
	if (ret < 0) {
		std::cout << "\nerror in writing the header context";
		return ret;
	}

	AVPacket *outPacket = av_packet_alloc();



	int count = 0;
	int j = 0;
	int pts = 0;
	//AVRational time_base = outCodecCtx->time_base;
	while (av_read_frame(formatContext, packet) >= 0) {
		//AVPacket orig_pkt = packet;
		if (count++ > 498) {
			break;
		}
		if (packet->stream_index != videoIndex) {
			av_log(NULL, AV_LOG_ERROR, "not videoIndex\n");
			continue;
		}
		ret = avcodec_send_packet(codecCtx, packet);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Error sending a packet for decoding\n");
			exit(1);
		}
		while (ret >= 0) {
			ret = avcodec_receive_frame(codecCtx, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				break;
			}
			else if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "Error during decoding\n");
				exit(1);
			}

			sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height, outFrame->data,outFrame->linesize);
			//pts = j * ((1 / 25) / 60);
			//pkt->pts = av_rescale_q_rnd(pkt->pts, *time_base, st->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
			//pkt->dts = av_rescale_q_rnd(pkt->dts, *time_base, st->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
			//pkt->duration = av_rescale_q(pkt->duration, *time_base, st->time_base);
			outFrame->pts = pts ++;
	
			
			ret = avcodec_send_frame(outCodecCtx, outFrame);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "Error sending a frame for encoding\n");
				exit(1);
			}
			while (ret >= 0) {
				ret = avcodec_receive_packet(outCodecCtx, outPacket);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
				}
				else if (ret < 0) {
					fprintf(stderr, "Error during encoding\n");
					exit(1);
				}
				/*if (outPacket.pts != AV_NOPTS_VALUE)
					outPacket.pts = av_rescale_q(outPacket.pts, outStream->codec->time_base, outStream->time_base);
				if (outPacket.dts != AV_NOPTS_VALUE)
					outPacket.dts = av_rescale_q(outPacket.dts, outStream->codec->time_base, outStream->time_base);*/

					/* rescale output packet timestamp values from codec to stream timebase */
				

				/*AVRational *time_base = &outFormatContext->streams[outPacket->stream_index]->time_base;
				outPacket->pts = av_rescale_q_rnd(outPacket->pts, *time_base, outStream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
				outPacket->dts = av_rescale_q_rnd(outPacket->dts, *time_base, outStream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
				outPacket->duration = av_rescale_q(outPacket->duration, *time_base, outStream->time_base);
				outPacket->stream_index = outStream->index;*/

				av_packet_rescale_ts(outPacket, outCodecCtx->time_base, outStream->time_base);
				outPacket->stream_index = outStream->index;
				printf("Write frame %3d (size= %2d)\n", j++, outPacket->size / 1000);
				if (av_interleaved_write_frame(outFormatContext, outPacket) != 0) {
					std::cout << "\nerror in writing video frame";
				}
				av_packet_unref(outPacket);
			}
			
		}
		av_packet_unref(packet);
	}
	
	ret = av_write_trailer(outFormatContext);
	if (ret < 0) {
		std::cout << "\nerror in writing av trailer";
		exit(1);
	}


	av_packet_free(&packet);

	av_frame_free(&frame);
	av_frame_free(&outFrame);
	
	avcodec_close(codecCtx);
	avcodec_close(outCodecCtx);

	avformat_close_input(&formatContext);
	avformat_close_input(&outFormatContext);
}