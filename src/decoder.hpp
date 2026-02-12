#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <queue>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
}

#include "rational.hpp"
#include "stream_data.hpp"

namespace potamos {

class PacketSource {
 public:
  virtual std::optional<Packet> ReadNextPacket(const int stream_index) = 0;
};

class Decoder {
 public:
  Decoder(const AVStream* stream, PacketSource* packet_source)
      : stream_(stream),
        codec_param_(stream->codecpar),
        codec_(avcodec_find_decoder(codec_param_->codec_id)),
        context_(avcodec_alloc_context3(codec_)),
        packet_source_(packet_source) {
    int ret0 = avcodec_parameters_to_context(context_, codec_param_);
    context_->flags2 |= AV_CODEC_FLAG2_SKIP_MANUAL;
    if (ret0 < 0)
      std::cerr << "avcodec_parameters_to_context =" << ret0 << std::endl;
    int ret = avcodec_open2(context_, codec_, nullptr);
    if (ret < 0) std::cerr << "avcodec_open2 =" << ret << std::endl;
  }

  Decoder(const Decoder& d) = delete;
  Decoder(Decoder&& d)
      : stream_(d.stream_),
        codec_param_(d.codec_param_),
        codec_(d.codec_),
        context_(d.context_),
        packet_source_(d.packet_source_) {
    d.context_ = nullptr;
  }

  ~Decoder() {
    // cleanup
    if (context_ != nullptr) {
      avcodec_free_context(&context_);
    }
  }

  bool Write(const Packet& pkt) {
    if (Type() == AVMediaType::AVMEDIA_TYPE_SUBTITLE) {
      AVSubtitle frame;
      int got_sub;
      int ret =
          avcodec_decode_subtitle2(context_, &frame, &got_sub, pkt.data());
      if (got_sub) {
        if (frame.pts == AV_NOPTS_VALUE) {
          // Apparenlty avcodec_decode_subtitle2 fails to do it's job...
          frame.pts =
              av_rescale_q(pkt.data()->pts, stream_->time_base, AV_TIME_BASE_Q);
        }
        if (frame.end_display_time == 0) {
          // Apparenlty avcodec_decode_subtitle2 fails to do it's job...
          frame.end_display_time = av_rescale_q(
              pkt.data()->duration, stream_->time_base, av_make_q(1, 1000));
        }
        sub_buffer_.push(frame);
      }
      return ret < 0 && got_sub;
    }
    int ret = avcodec_send_packet(context_, pkt.data());
    return ret < 0;
  }

  std::optional<Frame> Read() {
    Frame frame;
    int ret = AVERROR(EAGAIN);
    while (ret == AVERROR(EAGAIN)) {
      ret = avcodec_receive_frame(context_, frame.data());
      if (ret == AVERROR(EAGAIN)) {
        auto packet = packet_source_->ReadNextPacket(stream_->index);
        if (!packet) return std::nullopt;
        Write(*packet);  // TODO handle error
      } else if (ret == AVERROR_EOF)
        return std::nullopt;
      else if (ret < 0)
        return std::nullopt;  // TODO better error handling
    }
    return frame;
  }

  std::optional<AVSubtitle> ReadSub() {
    if (sub_buffer_.empty()) return std::nullopt;
    AVSubtitle sub = sub_buffer_.front();
    sub_buffer_.pop();
    return sub;
  }

  AVCodecContext* data() { return context_; }
  const AVCodecContext* data() const { return context_; }

  AVMediaType Type() const { return stream_->codecpar->codec_type; }

  Rational<int64_t> TimeBase() const { return stream_->time_base; }
  int64_t StartTime() const { return stream_->start_time; }

 protected:
  const AVCodecParameters* codec_param_;
  const AVCodec* codec_;
  const AVStream* stream_;
  AVCodecContext* context_;
  std::queue<AVSubtitle> sub_buffer_;
  PacketSource* packet_source_;
};

}  // namespace potamos