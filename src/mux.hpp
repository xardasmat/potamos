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

class Mux {
 public:
  Mux(std::ostream& stream, const std::string& format,
      const std::vector<const AVCodecParameters*>& streams)
      : stream_(stream) {
    // Create the muxer context
    fmt_ctx = avformat_alloc_context();
    // if (!()) {
    //     ret = AVERROR(ENOMEM);
    //     goto end;
    // }

    // Choose the muxer type
    fmt_ctx->oformat = av_guess_format(format.c_str(), NULL, NULL);

    // Create streams
    for (auto codec : streams) {
      streams_.push_back(avformat_new_stream(fmt_ctx, nullptr));
      // avcodec_parameters_from_context(streams_.back()->codecpar, ctx);
      avcodec_parameters_copy(streams_.back()->codecpar, codec);
      streams_.back()->time_base = (AVRational){1, streams_.back()->codecpar->sample_rate};

      std::clog << " :: codec type = " << streams_.back()->codecpar->bits_per_coded_sample
                << std::endl;
      // avcodec_free_context(&ctx);
    }

    avio_ctx_buffer = (uint8_t*)av_malloc(avio_ctx_buffer_size);
    // if (!avio_ctx_buffer) {
    //     ret = AVERROR(ENOMEM);
    //     goto end;
    // }
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 1,
                                  this, nullptr, &Mux::Write, &Mux::Seek);
    if (!avio_ctx) {
      std::clog << " ?? " << std::endl;
    }

    fmt_ctx->pb = avio_ctx;
    fmt_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;

    // int ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
    // if (ret < 0) {
    //     std::string
    //     error(av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE),
    //     AV_ERROR_MAX_STRING_SIZE, ret)); std::clog << "Could not open input:
    //     " << error << std::endl; return;
    // }

    // ret = avformat_find_stream_info(fmt_ctx, NULL);
    // if (ret < 0) {
    //     std::clog << "Could not find stream information" << std::endl;
    //     return;
    // }

    // av_dump_format(fmt_ctx, 0, "std::ifstream", 0);
  }

  ~Mux() {
    std::clog << __FILE__ << " " << __LINE__ << " EnsureHeader" << std::endl;
    EnsureHeader();
    std::clog << __FILE__ << " " << __LINE__ << " EnsureTrailer" << std::endl;
    EnsureTrailer();

    std::clog << __FILE__ << " " << __LINE__ << " av_freep" << std::endl;
    if (avio_ctx) av_freep(&avio_ctx->buffer);
    // av_freep(&avio_ctx_buffer);
    std::clog << __FILE__ << " " << __LINE__ << " avio_context_free"
              << std::endl;
    avio_context_free(&avio_ctx);
    std::clog << __FILE__ << " " << __LINE__ << " avformat_free_context"
              << std::endl;
    avformat_free_context(fmt_ctx);
    std::clog << __FILE__ << " " << __LINE__ << " eof ~Mux" << std::endl;
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
    std::clog << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << " " << packet.data()->size << ", " << packet.data()->pts
              << std::endl;
    // Packet local_packet = std::move(packet);
    packet.data()->side_data = nullptr;
    packet.data()->side_data_elems = 0;
    packet.data()->stream_index = 0;

    int ret = av_write_frame(fmt_ctx, packet.data());
    std::clog << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__
              << std::endl;
    return ret < 0;
  }

  Encoder MakeEncoder(int index) const {
    return Encoder(fmt_ctx->streams[index]->codecpar, fmt_ctx);
  }

 private:
  static int Write(void* opaque, const uint8_t* buf, int buf_size) {
    std::clog << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__
              << std::endl;
    Mux* stream = static_cast<Mux*>(opaque);
    return stream->Write(buf, buf_size);
  }
  int Write(const uint8_t* buf, int buf_size) {
    std::clog << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__
              << std::endl;
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
    std::clog << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__
              << std::endl;
    Mux* stream = static_cast<Mux*>(opaque);
    return stream->Seek(offset, whence);
  }
  int64_t Seek(int64_t offset, int whence) {
    std::clog << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "("
              << offset << ", " << whence << ")" << std::endl;
    // std::clog << "SEEK: " << offset << " " << whence  << " (" << AVSEEK_SIZE
    // << " / " << AVSEEK_FORCE << " ) " << std::endl;
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