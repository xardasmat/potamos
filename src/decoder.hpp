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

#include "stream_data.hpp"

namespace potamos {

class Decoder {
 public:
  Decoder(const AVCodecParameters* codec_param, const AVFormatContext* fmt_ctx)
      : codec_param_(codec_param),
        fmt_ctx_(fmt_ctx),
        codec_(avcodec_find_decoder(codec_param->codec_id)),
        context_(avcodec_alloc_context3(codec_)) {
    int ret0 = avcodec_parameters_to_context(context_, codec_param_);
    if (ret0 < 0)
      std::cerr << "avcodec_parameters_to_context =" << ret0 << std::endl;
    int ret = avcodec_open2(context_, codec_, nullptr);
    if (ret < 0) std::cerr << "avcodec_open2 =" << ret << std::endl;
  }

  Decoder(const Decoder& d) = delete;
  Decoder(Decoder&& d) : codec_(d.codec_), context_(d.context_) {
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
    int ret = avcodec_receive_frame(context_, frame.data());
    if (ret < 0) return std::nullopt;
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

 protected:
  const AVCodecParameters* codec_param_;
  const AVFormatContext* fmt_ctx_;
  const AVCodec* codec_;
  AVCodecContext* context_;
  std::queue<AVSubtitle> sub_buffer_;
};

}  // namespace potamos