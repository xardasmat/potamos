#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <optional>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
}

#include "stream_data.hpp"

class Encoder {
 public:
  Encoder(const AVCodecParameters* codec_param, const AVFormatContext* fmt_ctx)
      : codec_param_(codec_param),
        fmt_ctx_(fmt_ctx),
        codec_(avcodec_find_encoder(codec_param->codec_id)),
        context_(avcodec_alloc_context3(codec_)) {
    int ret0 = avcodec_parameters_to_context(context_, codec_param_);
    if (ret0 < 0)
      std::cerr << "avcodec_parameters_to_context =" << ret0 << std::endl;
    int ret = avcodec_open2(context_, codec_, nullptr);
    if (ret < 0) std::cerr << "avcodec_open2 =" << ret << std::endl;
  }

  Encoder(const Encoder& e) = delete;
  Encoder(Encoder&& e) : codec_(e.codec_), context_(e.context_) {
    e.context_ = nullptr;
  }

  ~Encoder() {
    // cleanup
    if (context_ != nullptr) {
      avcodec_free_context(&context_);
    }
  }

  bool Write(const Frame& frame) {
    int ret = avcodec_send_frame(context_, frame.data());
    return ret < 0;
  }

  bool Flush() {
    int ret = avcodec_send_frame(context_, nullptr);
    return ret < 0;
  }

  std::optional<Packet> Read() {
    Packet packet;
    int ret = avcodec_receive_packet(context_, packet.data());
    if (ret < 0) return std::nullopt;
    packet.data()->pts = samples_written;
    packet.data()->dts = samples_written;
    samples_written += packet.data()->duration;
    av_packet_rescale_ts(packet.data(),
                         (AVRational){1, codec_param_->sample_rate},
                         context_->time_base);
    return packet;
  }

  Frame MakeFrame() const {
    return Frame(codec_param_->format, &codec_param_->ch_layout, 1152);
  }

  AVCodecContext* data() { return context_; }
  const AVCodecContext* data() const { return context_; }

 protected:
  const AVCodecParameters* codec_param_;
  const AVFormatContext* fmt_ctx_;
  const AVCodec* codec_;
  AVCodecContext* context_;
  int64_t samples_written = 0;
};
