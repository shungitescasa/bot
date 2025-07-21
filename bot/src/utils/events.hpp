#pragma once

#include <string>
#include <vector>

#include "api/twitch/helix_client.hpp"
#include "database.hpp"
#include "schemas/event.hpp"

namespace bot::utils {
  std::vector<schemas::Event> get_events(std::unique_ptr<db::BaseDatabase> conn,
                                         api::twitch::HelixClient &api_client,
                                         int moderator_id, int type,
                                         const std::string &name);
};