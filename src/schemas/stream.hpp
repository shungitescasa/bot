#pragma once

namespace bot::schemas {
  enum EventType { LIVE, OFFLINE, TITLE, GAME, CUSTOM = 99 };
  enum EventFlag { MASSPING };
}
