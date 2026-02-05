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

#include "decoder.hpp"

namespace potamos {

class Demux : public PacketSource {
 public:
  Demux(std::istream& stream) : stream_(stream) {
    fmt_ctx = avformat_alloc_context();
    // if (!()) {
    //     ret = AVERROR(ENOMEM);
    //     goto end;
    // }

    avio_ctx_buffer = (uint8_t*)av_malloc(avio_ctx_buffer_size);
    // if (!avio_ctx_buffer) {
    //     ret = AVERROR(ENOMEM);
    //     goto end;
    // }
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0,
                                  this, &Demux::Read, nullptr, &Demux::Seek);
    if (!avio_ctx) {
      std::clog << " ?? " << std::endl;
    }

    fmt_ctx->pb = avio_ctx;

    int ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
    if (ret < 0) {
      std::string error(av_make_error_string(
          (char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE),
          AV_ERROR_MAX_STRING_SIZE, ret));
      std::clog << "Could not open input: " << error << std::endl;
      return;
    }

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
      std::clog << "Could not find stream information" << std::endl;
      return;
    }

    // av_dump_format(fmt_ctx, 0, "std::ifstream", 0);

    packets_queue_.resize(StreamsCount());
    decoders_.resize(StreamsCount());
  }

  ~Demux() {
    avformat_close_input(&fmt_ctx);
    if (avio_ctx) av_freep(&avio_ctx->buffer);
    avio_context_free(&avio_ctx);
  }

  int StreamsCount() const { return fmt_ctx->nb_streams; }

  std::string CodecType(AVMediaType type) const {
    switch (type) {
      case AVMEDIA_TYPE_UNKNOWN:
        return "Unknown";
      case AVMEDIA_TYPE_VIDEO:
        return "Video";
      case AVMEDIA_TYPE_AUDIO:
        return "Audio";
      case AVMEDIA_TYPE_DATA:
        return "Data";
      case AVMEDIA_TYPE_SUBTITLE:
        return "Subtitle";
      case AVMEDIA_TYPE_ATTACHMENT:
        return "Attachment";
      case AVMEDIA_TYPE_NB:
        return "NB";
    }
    return "?";
  }

  std::string Type(int n) const {
    return CodecType(fmt_ctx->streams[n]->codecpar->codec_type);
  }

  std::optional<Packet> read() {
    Packet packet;
    int ret = av_read_frame(fmt_ctx, packet.data());
    if (ret == AVERROR_EOF)
      return std::nullopt;
    else if (ret < 0) {
      std::cerr << "av_read_frame = " << ret << std::endl;
      return std::nullopt;
    } else
      return packet;
  }

  std::optional<Packet> ReadNextPacket(const int stream_index) override {
    if (!packets_queue_[stream_index].empty()) {
      auto packet = packets_queue_[stream_index].front();
      packets_queue_[stream_index].pop();
      return packet;
    }
    while (true) {
      auto packet = read();
      if (!packet) return std::nullopt;
      if (packet->StreamIndex() == stream_index) return packet;
      if (decoders_[packet->StreamIndex()])
        packets_queue_[packet->StreamIndex()].push(std::move(*packet));
    }
  }

  Decoder GetDecoder(int index) {
    decoders_[index] = true;
    return Decoder(fmt_ctx->streams[index], fmt_ctx, this, index);
  }

 private:
  static int Read(void* opaque, uint8_t* buf, int buf_size) {
    Demux* stream = static_cast<Demux*>(opaque);
    return stream->Read(buf, buf_size);
  }
  int Read(uint8_t* buf, int buf_size) {
    int count = stream_.readsome((char*)buf, buf_size);
    if (count == 0) {
      // stream_.peek();
      if (stream_.eof()) {
        return AVERROR_EOF;
      } else {
        return AVERROR_EOF;
        // return AVERROR_EXTERNAL;
      }
    } else {
      return count;
    }
  }
  static int64_t Seek(void* opaque, int64_t offset, int whence) {
    Demux* stream = static_cast<Demux*>(opaque);
    return stream->Seek(offset, whence);
  }
  int64_t Seek(int64_t offset, int whence) {
    switch (whence) {
      case AVSEEK_SIZE: {
        return -1;
      }
      case 0: {
        stream_.seekg(offset, std::ios_base::beg);
        return stream_.tellg();
      }
      case 1: {
        std::clog << "SEEK whence = 1: " << offset << " " << whence << " ("
                  << AVSEEK_SIZE << " / " << AVSEEK_FORCE << " ) " << std::endl;
        stream_.seekg(offset, std::ios_base::cur);
        return stream_.tellg();
      }
      case 2: {
        stream_.seekg(offset, std::ios_base::end);
        return stream_.tellg();
      }
      default: {
        std::clog << "SEEK whence = " << whence << ": " << offset << " "
                  << whence << " (" << AVSEEK_SIZE << " / " << AVSEEK_FORCE
                  << " ) " << std::endl;
      }
      case AVSEEK_FORCE: {
      }
    }

    stream_.seekg(offset);
    return stream_.tellg();
  }

  size_t avio_ctx_buffer_size = 4096;
  AVFormatContext* fmt_ctx = NULL;
  AVIOContext* avio_ctx = NULL;
  uint8_t* avio_ctx_buffer = NULL;
  std::vector<std::queue<Packet>> packets_queue_;
  std::vector<bool> decoders_;

  std::istream& stream_;
};

}  // namespace potamos