#pragma once

#include <fstream>
#include <functional>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
}

#include "encoder.hpp"
#include "stream_data.hpp"

namespace potamos {

class Mux : public PacketDestination {
 public:
  Mux(std::ostream& stream, const std::string& format,
      const std::vector<const AVCodecParameters*>& streams)
      : stream_(stream) {
    // Create the muxer context
    fmt_ctx = avformat_alloc_context();

    // Choose the muxer type
    fmt_ctx->oformat = av_guess_format(format.c_str(), NULL, NULL);

    // Create streams
    for (auto codec : streams) {
      streams_.push_back(avformat_new_stream(fmt_ctx, nullptr));
      // avcodec_parameters_from_context(streams_.back()->codecpar, ctx);
      avcodec_parameters_copy(streams_.back()->codecpar, codec);
      streams_.back()->time_base =
          (AVRational){1, streams_.back()->codecpar->sample_rate};
    }

    avio_ctx_buffer = (uint8_t*)av_malloc(avio_ctx_buffer_size);
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 1,
                                  this, nullptr, &Mux::Write, &Mux::Seek);
    if (!avio_ctx) {
      std::cerr << "avio_alloc_context failed" << std::endl;
    }

    fmt_ctx->pb = avio_ctx;
    fmt_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;
  }

  ~Mux() {
    EnsureHeader();
    EnsureTrailer();

    if (avio_ctx) av_freep(&avio_ctx->buffer);
    // av_freep(&avio_ctx_buffer);
    avio_context_free(&avio_ctx);
    avformat_free_context(fmt_ctx);
  }

  void EnsureHeader() {
    if (header_) return;
    // If this is not called the next function do it anyway
    int ret1 = avformat_init_output(fmt_ctx, nullptr);
    if (ret1 < 0) std::cerr << "avformat_init_output = " << ret1 << std::endl;
    //
    int ret2 = avformat_write_header(fmt_ctx, nullptr);
    if (ret2 < 0) std::cerr << "avformat_write_header = " << ret2 << std::endl;

    header_ = true;
  }

  void EnsureTrailer() {
    if (trailer_) return;
    av_write_trailer(fmt_ctx);
    trailer_ = true;
  }

  bool Write(Packet&& packet) {
    EnsureHeader();

    int ret = av_write_frame(fmt_ctx, packet.data());
    return ret < 0;
  }

  Encoder GetEncoder(int index) {
    return Encoder(fmt_ctx->streams[index], this);
  }

  bool WriteNextPacket(Packet packet, const int stream_index) override {
    return Write(std::move(packet));
  }

 private:
  static int Write(void* opaque, const uint8_t* buf, int buf_size) {
    Mux* stream = static_cast<Mux*>(opaque);
    return stream->Write(buf, buf_size);
  }
  int Write(const uint8_t* buf, int buf_size) {
    int64_t before = stream_.tellp();
    if (stream_.write((char*)buf, buf_size)) {
      return stream_.tellp() - before;
    } else if (stream_.eof()) {
      return AVERROR_EOF;
    } else {
      return AVERROR_EOF;
    }
  }
  static int64_t Seek(void* opaque, int64_t offset, int whence) {
    Mux* stream = static_cast<Mux*>(opaque);
    return stream->Seek(offset, whence);
  }
  int64_t Seek(int64_t offset, int whence) {
    switch (whence) {
      case AVSEEK_SIZE: {
        return -1;
      }
      case 0: {
        stream_.seekp(offset, std::ios_base::beg);
        return stream_.tellp();
      }
      case 1: {
        std::clog << "SEEK whence = 1: " << offset << " " << whence << " ("
                  << AVSEEK_SIZE << " / " << AVSEEK_FORCE << " ) " << std::endl;
        stream_.seekp(offset, std::ios_base::cur);
        return stream_.tellp();
      }
      case 2: {
        stream_.seekp(offset, std::ios_base::end);
        return stream_.tellp();
      }
      default: {
        std::clog << "SEEK whence = " << whence << ": " << offset << " "
                  << whence << " (" << AVSEEK_SIZE << " / " << AVSEEK_FORCE
                  << " ) " << std::endl;
      }
      case AVSEEK_FORCE: {
      }
    }

    stream_.seekp(offset);
    return stream_.tellp();
  }

  size_t avio_ctx_buffer_size = 4096;
  AVFormatContext* fmt_ctx = NULL;
  AVIOContext* avio_ctx = NULL;
  uint8_t* avio_ctx_buffer = NULL;

  bool header_ = false, trailer_ = false;

  std::vector<AVStream*> streams_;

  std::ostream& stream_;
};

}  // namespace potamos