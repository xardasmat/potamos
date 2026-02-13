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

namespace potamos {

class PacketDestination {
 public:
  virtual bool WriteNextPacket(Packet packet, const int stream_index) = 0;
};

class Encoder {
 public:
  Encoder(const AVStream* stream, PacketDestination* packet_dst)
      : stream_(stream), packet_dst_(packet_dst) {
    const AVCodec* codec = avcodec_find_encoder(stream->codecpar->codec_id);
    context_ = avcodec_alloc_context3(codec);
    if (context_ == nullptr) {
      std::cerr << "avcodec_alloc_context3 failed to allocate codec context"
                << std::endl;
    }
    int ret0 = avcodec_parameters_to_context(context_, stream->codecpar);
    if (ret0 < 0)
      std::cerr << "avcodec_parameters_to_context =" << ret0 << std::endl;
    int ret = avcodec_open2(context_, codec, nullptr);
    if (ret < 0) std::cerr << "avcodec_open2 =" << ret << std::endl;
  }

  Encoder(const Encoder& e) = delete;
  Encoder(Encoder&& e)
      : stream_(e.stream_), packet_dst_(e.packet_dst_), context_(e.context_) {
    e.stream_ = nullptr;
    e.packet_dst_ = nullptr;
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
    if (ret < 0) return false;
    while (auto packet = Read()) {
      packet_dst_->WriteNextPacket(std::move(*packet), stream_->index);
    }
    return true;
  }

  bool Flush() {
    int ret = avcodec_send_frame(context_, nullptr);
    if (ret < 0) return false;
    while (auto packet = Read()) {
      packet_dst_->WriteNextPacket(std::move(*packet), stream_->index);
    }
    return true;
  }

  std::optional<Packet> Read() {
    Packet packet;
    int ret = avcodec_receive_packet(context_, packet.data());
    if (ret < 0) return std::nullopt;
    return packet;
  }

  Frame MakeFrame() const {
    // TODO: move this logic to Audio encoder as it is audio specific code.
    return Frame(stream_->codecpar->format, &stream_->codecpar->ch_layout,
                 context_->frame_size > 0 ? context_->frame_size : 1024);
  }

  AVCodecContext* data() { return context_; }
  const AVCodecContext* data() const { return context_; }

 protected:
  const AVStream* stream_;
  PacketDestination* packet_dst_;
  AVCodecContext* context_;
};

}  // namespace potamos