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

  AVPacket* data() { return packet_; }
  const AVPacket* data() const { return packet_; }

  int StreamIndex() const { return packet_->stream_index; }

 private:
  AVPacket* packet_;
};

class Frame {
 public:
  Frame() { frame_ = av_frame_alloc(); }
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
