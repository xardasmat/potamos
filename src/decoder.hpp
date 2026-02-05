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
  Decoder(const AVStream* stream, const AVFormatContext* fmt_ctx,
          PacketSource* packet_source, int stream_index)
      : stream_(stream),
        codec_param_(stream->codecpar),
        fmt_ctx_(fmt_ctx),
        codec_(avcodec_find_decoder(codec_param_->codec_id)),
        context_(avcodec_alloc_context3(codec_)),
        packet_source_(packet_source),
        stream_index_(stream_index) {
    int ret0 = avcodec_parameters_to_context(context_, codec_param_);
    if (ret0 < 0)
      std::cerr << "avcodec_parameters_to_context =" << ret0 << std::endl;
    int ret = avcodec_open2(context_, codec_, nullptr);
    if (ret < 0) std::cerr << "avcodec_open2 =" << ret << std::endl;
  }

  Decoder(const Decoder& d) = delete;
  Decoder(Decoder&& d)
      : codec_(d.codec_),
        context_(d.context_),
        packet_source_(d.packet_source_),
        stream_index_(d.stream_index_) {
    d.context_ = nullptr;
  }

  ~Decoder() {
    // cleanup
    if (context_ != nullptr) {
      avcodec_free_context(&context_);
    }
  }

  bool Write(const Packet& pkt) {
    if (IsSubCodec()) {
      AVSubtitle frame;
      int got_sub;
      int ret =
          avcodec_decode_subtitle2(context_, &frame, &got_sub, pkt.data());
      if (got_sub) sub_buffer_.push(frame);
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
        auto packet = packet_source_->ReadNextPacket(stream_index_);
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

  bool IsSubCodec() const {
    const AVCodecDescriptor* desc = avcodec_descriptor_get(context_->codec_id);
    return desc && (desc->props & AV_CODEC_PROP_TEXT_SUB);
  }

  Rational<int64_t> TimeBase() const { return stream_->time_base; }
  int64_t StartTime() const { return stream_->start_time; }

 protected:
  const AVCodecParameters* codec_param_;
  const AVFormatContext* fmt_ctx_;
  const AVStream* stream_;
  const AVCodec* codec_;
  AVCodecContext* context_;
  std::queue<AVSubtitle> sub_buffer_;
  PacketSource* packet_source_;
  const int stream_index_;
};

}  // namespace potamos