#include <iostream>
#include <fstream>
#include <functional>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mux.hpp"

TEST(MuxTest, BasicTest)
{
    const std::string output_file_name = "test_data/test.mp3";
    std::ofstream output_file(output_file_name);
    Mux mux(output_file);

    // EXPECT_THAT(mux ??? );
}