#include "screen.h"

using namespace std;

screen::screen() {

	// 注册设备
	avdevice_register_all();
	avformat_network_init();
}


screen::~screen() {


	av_packet_free(&packet);

	av_packet_free(&outPacket);

	av_frame_free(&frame);

	av_frame_free(&outFrame);

	sws_freeContext(swsCtx);
	// 关闭转码器
	avcodec_close(codecCtx);
	// 关闭转码器
	avcodec_close(outCodecCtx);


	avformat_close_input(&outFormatContext);
	// 关闭流
	avformat_close_input(&formatContext);
		
}


int screen::initFormatContext(string input, string url) {
	int ret = 0;
	// 结构体描述了一个媒体文件或媒体流的构成和基本信息 avformat_open_input自动设置 AVFormatContext
	// avformat_alloc_context();

	// 输入设备 为 gdigrab  mac:avfoundation
	const AVInputFormat *pAVInputFormat = av_find_input_format(input.c_str());
	
	// AVDictionary是一个健值对存储工具，类似于c++中的map，ffmpeg中有很多 API 通过它来传递参数。 录制参数设置  转码参数设置
	AVDictionary* options = NULL;

	// 打开输入流 desktop title={窗口名称}
	if ((ret = avformat_open_input(&formatContext, url.c_str(), pAVInputFormat, &options)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open input stream.\n");
		return ret;
	}
	// 获取流信息 更新AVStream结构体
	if ((ret = avformat_find_stream_info(formatContext, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Couldn't find stream information.\n");
		return ret;
	}

	return ret;
}

int screen::initOutFormatContext() {
	int ret = 0;
	
	avformat_alloc_output_context2(&outFormatContext, NULL, NULL, "test.h264");
	const AVOutputFormat *outformat = outFormatContext->oformat;

	
	outStream = avformat_new_stream(outFormatContext, NULL);

	//av_dump_format(outFormatContext, 0, "test.h264", 1);

	return ret;
}

int screen::buildCodecContext() {
	int ret = 0;
	// 
	videoIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	//cout << videoIndex << endl;
	// 获取流信息 AVStream是存储每一个视频/音频流信息的结构体
	AVStream *stream = formatContext->streams[videoIndex];
	// 查找解码器 
	codec = avcodec_find_decoder(stream->codecpar->codec_id);
	if (codec == NULL)
	{
		av_log(NULL, AV_LOG_ERROR, "Codec not found.\n");
		return -1;
	}
	// 从流通道中获取解码器编解码器参数 例如我们将 H264 和 AAC 码流存储为MP4文件的时候，就需要在 MP4文件中增加两个流通道，一个存储Video：H264，一个存储Audio：AAC。（假设H264和AAC只包含单个流通道）。
	AVCodecParameters *avCodeParams = stream->codecpar;
	// 配置解码器
	codecCtx = avcodec_alloc_context3(codec);
	// 视频流中获取编解码器参数复制到 AVCodecContext
	avcodec_parameters_to_context(codecCtx, avCodeParams);
	// bmp
	cout << avcodec_get_name(codecCtx->codec_id) << endl;
	// AV_PIX_FMT_ABGR
	cout << codecCtx->pix_fmt << endl;
	// 打开解码器
	if ((ret = avcodec_open2(codecCtx, codec, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not open codec.\n");
		return -1;
	}

	// 从输入设备读取的是原生的数据流，也就是经过设备编码之后的数据。 需要先将原生数据进行解码，变成程序可读的数据，在编码成输出设备可识别的数据
    // AVPacket 解码前 AVFrame 表示解码后的视频帧
    // 该结构存储压缩数据 对于视频，它通常应包含一个压缩帧 对于音频，它可能包含几个压缩帧
	packet = av_packet_alloc();
	frame = av_frame_alloc();

	return ret;
}

int screen::buildEncodecContext() {
	int ret = 0;
	// 编码器
	outCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	// 配置解码器
	outCodecCtx = avcodec_alloc_context3(outCodec);
	outCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	// 码率
	outCodecCtx->bit_rate = 2000000;
	// 每25帧产生一个关键帧 I帧
	outCodecCtx->gop_size = 250;
	outCodecCtx->width = 1920;
	outCodecCtx->height = 1080;
	// 先转成 AV_PIX_FMT_YUV420P
	outCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	outCodecCtx->codec_id = AV_CODEC_ID_H264;
	outCodecCtx->time_base.num = 1;
	outCodecCtx->time_base.den = 25;

	// fps
	outCodecCtx->framerate.num = 25;
	// 如果编码器id是h264
	if (outCodecCtx->codec_id == AV_CODEC_ID_H264) {
		// preset表示采用一个预先设定好的参数集，级别是slow，slow表示压缩速度是慢的，慢的可以保证视频质量，用快的会降低视频质量
		av_opt_set(outCodecCtx->priv_data, "preset", "slow", 0);
	}

	// 打开编解码器
	if ((ret = avcodec_open2(outCodecCtx, outCodec, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not open codec.\n");
		exit(1);
	}

	outFrame = av_frame_alloc();
	outFrame->format = outCodecCtx->pix_fmt;
	outFrame->width = outCodecCtx->width;
	outFrame->height = outCodecCtx->width;
	//outFrame->f
	outFrame->pts = 0;
	//  av_frame_get_buffer可以理解为自动为AVFrame分配空间，而av_image_fill_arrays可以理解为手动填充。
	//  如果用一个AVFrame初始化是为了继承sws转换的数据，可以选择av_frame_get_buffer进行初始化，这样在释放时直接调用av_frame_free即可，不用担心内存泄漏问题。
	// 为什么是32? rgb 8:8:8:8 32bp
	ret = av_frame_get_buffer(outFrame, 32);

	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "nerror in filling image array");
		return -1;
	}

	// 将编码好的数据保存在AVPacket
	outPacket = av_packet_alloc();

	if (avio_open(&outFormatContext->pb, "test.h264", AVIO_FLAG_READ_WRITE) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to open output file! \n");
		return -1;
	}
	outFormatContext->streams[0]->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	outFormatContext->streams[0]->codecpar->width = 1920;
	outFormatContext->streams[0]->codecpar->height = 1080;

	ret = avformat_write_header(outFormatContext, NULL);
	if (ret < 0) {
		cout << "\nerror in writing the header context";
		return ret;
	}


	return ret;
}

int screen::buildSwsContext() {
	int ret = 0;
	// 创建视频重采样上下文
	// 参数1：被转换源的宽 参数2：被转换源的高 
	// 参数3：被转换源的格式，eg：YUV、RGB……（枚举格式，也可以直接用枚举的代号表示eg：AV_PIX_FMT_YUV420P这些枚举的格式在libavutil / pixfmt.h中列出）
	// 参数4：转换后指定的宽 参数5：转换后指定的高 
	// 参数6：转换后指定的格式同参数3的格式 参数7：转换所使用的算法，
	swsCtx = sws_getContext(codecCtx->width,
		codecCtx->height,
		codecCtx->pix_fmt,
		outCodecCtx->width,
		outCodecCtx->height,
		outCodecCtx->pix_fmt,
		SWS_BICUBIC, NULL, NULL, NULL);
	return ret;
}


int screen::record() {
	
	//ofstream outfile("test.h264", ios::binary);

	int ret = 0;
	int count = 0;
	int j = 0;
	int pts = 0;
	while (av_read_frame(formatContext, packet) >= 0) {
		//AVPacket orig_pkt = packet;
		if (count++ > 498) {
			break;
		}
		if (packet->stream_index != videoIndex) {
			av_log(NULL, AV_LOG_ERROR, "not videoIndex\n");
			continue;
		}

		// 使用avcodec_send_packet将输入设备的数据发往解码器进行解码
		ret = avcodec_send_packet(codecCtx, packet);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Error sending a packet for decoding\n");
			return -1;
		}
		
		while (ret >= 0) {

			//读取解码数据
			ret = avcodec_receive_frame(codecCtx, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				break;
			}
			else if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "Error during decoding\n");
				exit(1);
			}
			// pCodecCtx->pix_fmt AV_PIX_FMT_ABGR
			// 转码
			int ret = sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height, outFrame->data, outFrame->linesize);
			// 调用 send_frame 之前设置 outFrame 的 pts。
			outFrame->pts = pts ++;
			//outFrame->pts = count++ * (outCodecCtx->time_base.num * 1000 / outCodecCtx->time_base.den);
			// 转码完成后的数据 再次编码
			//cout << outFrame->pts << endl;
			ret = avcodec_send_frame(outCodecCtx, outFrame);
			
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "Error sending frame to encoding\n");
				return -1;
			}

			ret = av_frame_make_writable(outFrame);
			if (ret < 0) {
				return -1;
			}
			
			
			while (ret >= 0) {
				ret = avcodec_receive_packet(outCodecCtx, outPacket);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
				}
				else if (ret < 0) {
					av_log(NULL, AV_LOG_ERROR, "Error during encoding\n");
					exit(1);
				}

				//写入H264文件
				// outfile.write((char*)outPacket->data, outPacket->size);
				printf("Write frame %3d (size= %2d)\n", j++, outPacket->size / 1000);
				ret = av_interleaved_write_frame(outFormatContext, outPacket);

				// 释放outPacket
				av_packet_unref(outPacket);

			}

		}
		// 释放Packet
		av_packet_unref(packet);

	}
	//outfile.close();
	//Write file trailer
	av_write_trailer(outFormatContext);
	return ret;
}