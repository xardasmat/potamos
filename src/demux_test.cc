#include <iostream>
#include <fstream>
#include <functional>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "demux.hpp"

TEST(DemuxTest, BasicTest) {
  const std::string input_file_name = "test_data/orders.mp3";
  std::ifstream input_file(input_file_name);
  Demux demux(input_file);

  EXPECT_THAT(demux.StreamsCount(), 1);
}