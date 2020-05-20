#include "gtest/gtest.h"

#include "Models/Glm/Encoders.hpp"
#include "LinAlg/Selector.hpp"
#include "distributions.hpp"

#include "test_utils/test_utils.hpp"
#include <fstream>

namespace {
  using namespace BOOM;
  using std::endl;
  using std::cout;

  class EncoderTest : public ::testing::Test {
   protected:
    EncoderTest() {
      GlobalRng::rng.seed(8675309);
      colors_.reset(new CatKey({"red", "blue", "green"}));
      sizes_.reset(new CatKey({"xs", "small", "med", "large"}));
    }
    Ptr<CatKey> colors_;
    Ptr<CatKey> sizes_;
  };

  TEST_F(EncoderTest, EffectsEncoderTest) {
    EffectsEncoder encoder(0, colors_);
    CategoricalData red("red", colors_);
    Vector enc = encoder.encode(red);
    EXPECT_TRUE(VectorEquals(enc, Vector{1, 0}));

    CategoricalData blue("blue", colors_);
    enc = encoder.encode(blue);
    EXPECT_TRUE(VectorEquals(enc, Vector{0, 1}));

    CategoricalData green("green", colors_);
    enc = encoder.encode(green);
    EXPECT_TRUE(VectorEquals(enc, Vector{-1, -1}));
  }

}  // namespace
