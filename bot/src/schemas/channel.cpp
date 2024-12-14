#include "channel.hpp"

namespace bot::schemas {
  std::optional<ChannelFeature> string_to_channel_feature(
      const std::string &value) {
    if (value == "markov_responses") {
      return MARKOV_RESPONSES;
    } else if (value == "random_markov_responses") {
      return RANDOM_MARKOV_RESPONSES;
    } else {
      return std::nullopt;
    }
  }
}