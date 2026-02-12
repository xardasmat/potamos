#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>
#include <functional>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
}

#include "demux.hpp"
#include "subtitle.hpp"

namespace potamos {

using testing::_;
using testing::ElementsAre;
using testing::FieldsAre;

TEST(DecodeSubtitle, BasicTest) {
  const std::string input_file_name = "test_data/orders.srt";
  std::ifstream input_file(input_file_name);
  Demux demux(input_file);

  ASSERT_EQ(demux.StreamsCount(), 1);
  auto subtitle_decoder = demux.GetDecoder(0);
  SubtitleDecoder subtitle_codec(subtitle_decoder);

  EXPECT_THAT(subtitle_decoder.data()->codec_id, AV_CODEC_ID_SUBRIP);
  EXPECT_THAT(subtitle_decoder.data()->codec_descriptor->name,
              testing::StrEq("subrip"));

  std::vector<std::string> sub_pattern = {"- Awaiting orders!",
                                          "[Sound of silence]"};
  std::vector<double> time_start_pattern = {0.05, 0.7};
  std::vector<double> time_end_pattern = {0.7, 0.9};

  int index = 0;
  while (auto packet = demux.read()) {
    ASSERT_FALSE(subtitle_decoder.Write(std::move(*packet)));
    while (auto subtitle = subtitle_codec.Read()) {
      ASSERT_THAT(index, testing::Le(sub_pattern.size()));
      EXPECT_EQ(subtitle->text, sub_pattern[index])
          << "failed at index =" << index;
      EXPECT_THAT(double(subtitle->begin_timestamp),
                  testing::DoubleNear(time_start_pattern[index], 1e-6));
      EXPECT_THAT(double(subtitle->end_timestamp),
                  testing::DoubleNear(time_end_pattern[index], 1e-6));
      ++index;
    }
  }
  EXPECT_EQ(index, sub_pattern.size());
}

}  // namespace potamos