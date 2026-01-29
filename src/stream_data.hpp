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

class Packet {
 public:
  Packet() { packet_ = av_packet_alloc(); }
  Packet(const Packet& p) { packet_ = av_packet_clone(p.packet_); }
  Packet(Packet&& p) {
    packet_ = p.packet_;
    p.packet_ = nullptr;
  }
  ~Packet() {
    if (packet_ != nullptr) {
      av_packet_unref(packet_);
    }
  }

  Packet& operator=(Packet&& p) {
    packet_ = p.packet_;
    p.packet_ = nullptr;
    return *this;
  }
  Packet& operator=(const Packet& p) {
    packet_ = av_packet_clone(p.packet_);
    return *this;
  }

  AVPacket* data() { return packet_; }
  const AVPacket* data() const { return packet_; }

  int StreamIndex() const { return packet_->stream_index; }

 private:
  AVPacket* packet_;
};

class Frame {
 public:
  Frame() { frame_ = av_frame_alloc(); }
  Frame(int format, const AVChannelLayout* ch_layout, int nb_samples) {
    frame_ = av_frame_alloc();
    frame_->format = format;
    frame_->nb_samples = nb_samples;
    av_channel_layout_copy(&frame_->ch_layout, ch_layout);
    int ret = av_frame_get_buffer(frame_, 0);
    if (ret < 0) std::cerr << "av_frame_get_buffer = " << ret << std::endl;
  }
  Frame(const Frame& f) { frame_ = av_frame_clone(f.frame_); }
  Frame(Frame&& f) {
    frame_ = f.frame_;
    f.frame_ = nullptr;
  }
  ~Frame() {
    if (frame_ != nullptr) {
      av_frame_unref(frame_);
    }
  }

  Frame& operator=(Frame&& f) {
    frame_ = f.frame_;
    f.frame_ = nullptr;
    return *this;
  }
  Frame& operator=(const Frame& f) {
    frame_ = av_frame_clone(f.frame_);
    return *this;
  }

  AVFrame* data() { return frame_; }
  const AVFrame* data() const { return frame_; }

 private:
  AVFrame* frame_;
};
