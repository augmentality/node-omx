//
// node-omx - A media player for node.js on the Raspberry Pi
// Copyright (C) 2018 Augmentality Ltd <info@augmentality.uk>
//
// This file is part of node-omx.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; version 2.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
// Street, Fifth Floor, Boston, MA 02110-1301, USA.


#pragma once

#include "FFException.h"
#include "FFFrame.h"
#include <string>
#include "../ilclient/ilc.h"
#include <mutex>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
    #include <libavutil/opt.h>
}

class FFSource
{
    public:
        bool hasVideo = false;
        bool hasAudio = false;
        float fpsscale = 0.0f;
        float fpsrate = 0.0f;
        bool isMatroska = false;
        bool isAVI = false;
        bool isFlipped = false;
        int width = 1920;
        int height = 1080;
        int sampleRate = 0;
        int channels = 0;
        double lastVideoPTS = 0.0;
        double lastAudioPTS = 0.0;
        double lastVideoDTS = 0.0;
        double lastAudioDTS = 0.0;
        double baseVideoPTS = 0.0;
        double baseVideoDTS = 0.0;
        double baseAudioPTS = 0.0;
        double baseAudioDTS = 0.0;

        AVBitStreamFilterContext *annexb = nullptr;
        SwrContext *swr = nullptr;

        AVCodecContext *audioCodec = NULL;

        OMX_VIDEO_CODINGTYPE codingType = OMX_VIDEO_CodingMPEG4;
        std::string codecName = "";

        FFSource(std::string uri)
        {
            this->annexb = av_bitstream_filter_init("h264_mp4toannexb");
            this->swr = swr_alloc();

            // Open the file
            if (avformat_open_input(&this->fmt_ctx, uri.c_str(), nullptr, nullptr) < 0)
            {
                av_bitstream_filter_close(this->annexb);
                this->annexb = nullptr;
                throw FFException(std::string("Could not open the source"));
            }

            // Retrieve stream info
            if (avformat_find_stream_info(this->fmt_ctx, nullptr) < 0)
            {
                av_bitstream_filter_close(this->annexb);
                avformat_close_input(&this->fmt_ctx);
                this->annexb = nullptr;

                throw FFException(std::string("Could not get stream info"));
            }

            this->isMatroska = strncmp(this->fmt_ctx->iformat->name, "matroska", 8) == 0;
            this->isAVI = strcmp(this->fmt_ctx->iformat->name, "avi") == 0;

            for (unsigned int i = 0; i < this->fmt_ctx->nb_streams; i++)
            {
                AVStream * pStream = this->fmt_ctx->streams[i];
                switch (pStream->codec->codec_type)
                {
                    case AVMEDIA_TYPE_AUDIO:

                        if (this->audioStreamIdx == -1)
                        {
                            this->audioCodec = pStream->codec;
                            AVCodec *dec = avcodec_find_decoder(this->audioCodec->codec_id);
                            if (avcodec_open2(this->audioCodec, dec, nullptr) >= 0)
                            {
                                this->audioStreamIdx = i;
                                hasAudio = true;

                                this->channels = this->audioCodec->channels;
                                this->sampleRate = this->audioCodec->sample_rate;

                                av_opt_set_int(this->swr, "in_channel_layout",  this->audioCodec->channel_layout, 0);
                                av_opt_set_int(this->swr, "out_channel_layout", this->audioCodec->channel_layout,  0);
                                av_opt_set_int(this->swr, "in_sample_rate",     this->audioCodec->sample_rate, 0);
                                av_opt_set_int(this->swr, "out_sample_rate",    this->audioCodec->sample_rate, 0);
                                av_opt_set_sample_fmt(this->swr, "in_sample_fmt",  AV_SAMPLE_FMT_FLTP, 0);
                                av_opt_set_sample_fmt(this->swr, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);
                                swr_init(this->swr);
                            }
                        }
                        break;
                    case AVMEDIA_TYPE_VIDEO:
                        hasVideo = true;
                        this->width = pStream->codec->width;
                        this->height = pStream->codec->height;
                        this->videoStreamIdx = i;

                        this->fpsrate       = pStream->r_frame_rate.num;
                        this->fpsscale      = pStream->r_frame_rate.den;

                        if(this->isMatroska && pStream->avg_frame_rate.den && pStream->avg_frame_rate.num)
                        {
                            this->fpsrate      = pStream->avg_frame_rate.num;
                            this->fpsscale     = pStream->avg_frame_rate.den;
                        }
                        else if(pStream->r_frame_rate.num && pStream->r_frame_rate.den)
                        {
                            this->fpsrate      = pStream->r_frame_rate.num;
                            this->fpsscale     = pStream->r_frame_rate.den;
                        }
                        else
                        {
                            this->fpsscale     = 0;
                            this->fpsrate      = 0;
                        }

                        //(m_config.hints.codec
                        int codec = pStream->codec->codec_id;
                        int profile = pStream->codec->profile;
                        switch (codec)
                        {
                            case AV_CODEC_ID_H264:
                            {

                                switch (profile)
                                {
                                    case FF_PROFILE_H264_BASELINE:
                                        // (role name) video_decoder.avc
                                        // H.264 Baseline profile
                                        this->codingType = OMX_VIDEO_CodingAVC;
                                        this->codecName = "omx-h264";
                                        break;
                                    case FF_PROFILE_H264_MAIN:
                                        // (role name) video_decoder.avc
                                        // H.264 Main profile
                                        this->codingType = OMX_VIDEO_CodingAVC;
                                        this->codecName = "omx-h264";
                                        break;
                                    case FF_PROFILE_H264_HIGH:
                                        // (role name) video_decoder.avc
                                        // H.264 Main profile
                                        this->codingType = OMX_VIDEO_CodingAVC;
                                        this->codecName = "omx-h264";
                                        break;
                                    case FF_PROFILE_UNKNOWN:
                                        this->codingType = OMX_VIDEO_CodingAVC;
                                        this->codecName = "omx-h264";
                                        break;
                                    default:
                                        this->codingType = OMX_VIDEO_CodingAVC;
                                        this->codecName = "omx-h264";
                                        break;
                                }
                                break;
                            }
                            case AV_CODEC_ID_MPEG4:
                                this->codingType = OMX_VIDEO_CodingMPEG4;
                                this->codecName = "omx-mpeg4";
                                break;
                            case AV_CODEC_ID_MPEG1VIDEO:
                            case AV_CODEC_ID_MPEG2VIDEO:
                                this->codingType = OMX_VIDEO_CodingMPEG2;
                                this->codecName = "omx-mpeg2";
                                break;
                            case AV_CODEC_ID_H263:
                                this->codingType = OMX_VIDEO_CodingMPEG4;
                                this->codecName = "omx-h263";
                                break;
                            case AV_CODEC_ID_VP6:
                                // this form is encoded upside down
                                this->isFlipped = true;
                                // fall through
                            case AV_CODEC_ID_VP6F:
                            case AV_CODEC_ID_VP6A:
                                // (role name) video_decoder.vp6
                                // VP6
                                this->codingType = OMX_VIDEO_CodingVP6;
                                this->codecName = "omx-vp6";
                                break;
                            case AV_CODEC_ID_VP8:
                                // (role name) video_decoder.vp8
                                // VP8
                                this->codingType = OMX_VIDEO_CodingVP8;
                                this->codecName = "omx-vp8";
                                break;
                            case AV_CODEC_ID_THEORA:
                                // (role name) video_decoder.theora
                                // theora
                                this->codingType = OMX_VIDEO_CodingTheora;
                                this->codecName = "omx-theora";
                                break;
                            case AV_CODEC_ID_MJPEG:
                            case AV_CODEC_ID_MJPEGB:
                                this->codingType = OMX_VIDEO_CodingMJPEG;
                                this->codecName = "omx-mjpeg";
                                break;
                            case AV_CODEC_ID_VC1:
                            case AV_CODEC_ID_WMV3:
                                // (role name) video_decoder.vc1
                                // VC-1, WMV9
                                this->codingType = OMX_VIDEO_CodingWMV;
                                this->codecName = "omx-vc1";
                                break;
                            default:
                                break;
                        }
                        break;
                }
            }

            av_init_packet(&pkt);
            pkt.data = NULL;
            pkt.size = 0;

            av_init_packet(&pkt2);
            pkt2.data = NULL;
            pkt2.size = 0;

            freepkt1 = true;
            freepkt2 = true;
        }

        void setLoop(bool pLoop)
        {
            std::unique_lock<std::mutex> lk(loopMutex);
            loop = pLoop;
        }

        ~FFSource()
        {
            if (this->swr != nullptr)
            {
                swr_free(&this->swr);
                this->swr = nullptr;
            }
            if (this->annexb != nullptr)
            {
                av_bitstream_filter_close(this->annexb);
                this->annexb = nullptr;
            }
            if (this->audioCodec != nullptr)
            {
                avcodec_close(this->audioCodec);
                this->audioCodec = nullptr;
            }
            if (this->fmt_ctx != nullptr)
            {
                avformat_close_input(&this->fmt_ctx);
                this->fmt_ctx = nullptr;
            }
            if (freepktdata2 && pkt2.data != NULL)
            {
                av_freep(&pkt2.data);
            }
            if (freepktdata1 && pkt.data != NULL)
            {
                av_freep(&pkt.data);
            }
            if (freepkt2)
            {
                av_free_packet(&pkt2);
            }
            if (freepkt1)
            {
                av_free_packet(&pkt);
            }
        }
        int getPacket(FFFrame * frame)
        {
            if (fmt_ctx == nullptr)
            {
                throw FFException(std::string("GetPacket called with no open stream"));
            }
            if (frame->frame != nullptr)
            {
                av_frame_free(&frame->frame);
                frame->frame = nullptr;
            }
            if (frame->convertedAudio != nullptr)
            {
                delete[] frame->convertedAudio;
                frame->convertedAudio = nullptr;
            }
            int result = 0;
            while (true)
            {
                if (this->pkt2.data != NULL)
                {
                    av_freep(&this->pkt2.data);
                    av_free_packet(&this->pkt2);
                    freepkt2 = false;
                    freepktdata2 = false;
                    pkt2.data = NULL;
                    pkt2.size = 0;
                }
                if (this->pkt.data != NULL)
                {
                    // DON'T zero data here or av_read_frame won't free it
                    // (and if we try to free it, we'll crash)
                    av_free_packet(&this->pkt);
                    freepkt1 = false;
                }
                frame->pkt = nullptr;

                result = av_read_frame(this->fmt_ctx, &this->pkt);
		        if (result < 0)
                {
                    std::unique_lock<std::mutex> lk(loopMutex);
		            if (!loop)
                    {
		                return -1;
                    }
                    frame->loopedVideo = true;
                    frame->loopedAudio = true;

                    baseAudioDTS += lastAudioDTS;
                    baseVideoDTS += lastVideoDTS;
                    baseAudioPTS += lastAudioPTS;
                    baseVideoPTS += lastVideoPTS;



                    result = av_seek_frame(this->fmt_ctx, -1, 0, AVSEEK_FLAG_BACKWARD);
                    if (result < 0)
                    {
                        return result;
                    }
                    else
                    {
                        result = av_read_frame(this->fmt_ctx, &this->pkt);

                        if (result < 0)
                        {
                            return result;
                        }
                    }
                }
                freepkt1 = true;
		        freepktdata1 = true;


                AVStream *pStream = this->fmt_ctx->streams[this->pkt.stream_index];
                if (isMatroska && pStream->codec && pStream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
                {
                    if (this->pkt.dts == 0)
                    {
                        this->pkt.dts = AV_NOPTS_VALUE;
                    }
                    if (this->pkt.pts == 0)
                    {
                        this->pkt.pts = AV_NOPTS_VALUE;
                    }
                }
                if (isAVI && pStream->codec && pStream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
                {
                    this->pkt.pts = AV_NOPTS_VALUE;
                }

                frame->dts = convertTimestamp(this->pkt.dts, pStream->time_base.den, pStream->time_base.num);
                frame->pts = convertTimestamp(this->pkt.pts, pStream->time_base.den, pStream->time_base.num);
                frame->duration = convertTimestamp(this->pkt.duration, pStream->time_base.den, pStream->time_base.num);

                if (pkt.stream_index == this->videoStreamIdx)
                {
                    lastVideoPTS = frame->pts + frame->duration;
                    lastVideoDTS = frame->dts + frame->duration;
                    frame->pts += baseVideoPTS;
                    frame->dts += baseVideoDTS;

                    int a = av_bitstream_filter_filter(annexb, this->fmt_ctx->streams[pkt.stream_index]->codec, NULL,
                                                       &pkt2.data, &pkt2.size, pkt.data, pkt.size,
                                                       pkt.flags & AV_PKT_FLAG_KEY);
                    freepkt1 = false;
                    freepkt2 = true;
                    freepktdata2 = true;
                    frame->video = true;
                    frame->pkt = &pkt2;
                    return result;
                }
                else if (pkt.stream_index == this->audioStreamIdx)
                {
                    bool skipAudio = false;
                    if (baseAudioPTS > baseVideoPTS)
                    {
                        double diff = baseAudioPTS - baseVideoPTS;
                        if (diff > frame->duration)
                        {
                            baseAudioPTS -= frame->duration;
                            baseAudioDTS -= frame->duration;
                            double newDiff = baseAudioPTS - baseVideoPTS;
                            skipAudio = true;
                            double diffTime = diff / DVD_TIME_BASE;
                            double newDiffTime = newDiff / DVD_TIME_BASE;
                        }
                    }
                    if (!skipAudio)
                    {
                        lastAudioPTS = frame->pts + frame->duration;
                        lastAudioDTS = frame->dts + frame->duration;
                        frame->pts += baseAudioPTS;
                        frame->dts += baseAudioDTS;

                        AVFrame * out = av_frame_alloc();
                        int gotFrame = 0;
                        freepkt1 = false;
                        avcodec_decode_audio4(this->audioCodec, out, &gotFrame, &pkt);
                        if (gotFrame)
                        {
                            int bufsize = av_samples_get_buffer_size(NULL, this->audioCodec->channels, out->nb_samples,
                                                                     this->audioCodec->sample_fmt, 1);
                            uint8_t * outputBuffer = new uint8_t[bufsize];
                            swr_convert(this->swr, &outputBuffer, out->nb_samples, (const uint8_t **)out->data,
                                        out->nb_samples);

                            frame->video = false;
                            frame->frame = out;
                            frame->convertedSize = bufsize;
                            frame->convertedAudio = outputBuffer;

                            return result;
                        }
                        else
                        {
                            delete out;
                        }
                    }
                }
            }
        }


    private:
        std::mutex loopMutex;
        bool loop = false;
        bool freepkt1 = false;
        bool freepkt2 = false;
        bool freepktdata1 = false;
        bool freepktdata2 = false;
        AVPacket pkt;
        AVPacket pkt2;
        AVPacket buffer_pkt;
        AVFormatContext *fmt_ctx = nullptr;
        AVStream *video_stream = nullptr;
        AVStream *audio_stream = nullptr;
        int audioStreamIdx = -1;
        int videoStreamIdx = -1;

        int openCodecContext(int *stream_idx, AVFormatContext *fmt_ctx, enum AVMediaType type)
        {
            int ret, stream_index;
            AVStream *st;
            AVCodecContext *dec_ctx = NULL;
            AVCodec *dec = NULL;
            AVDictionary *opts = NULL;

            ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
            return ret;
        }

        double convertTimestamp(int64_t pts, int den, int num)
        {
            if(this->fmt_ctx == NULL)
            {
                return DVD_NOPTS_VALUE;
            }

            if (pts == (int64_t)AV_NOPTS_VALUE)
            {
                return DVD_NOPTS_VALUE;
            }

            double timestamp = (double)pts * num  / den;
            double starttime = 0.0f;

            if (this->fmt_ctx->start_time != (int64_t)AV_NOPTS_VALUE)
                starttime = (double)this->fmt_ctx->start_time / AV_TIME_BASE;

            if(timestamp > starttime)
                timestamp -= starttime;
            else if( timestamp + 0.1f > starttime )
                timestamp = 0;

            return timestamp * DVD_TIME_BASE;
        }
};
