-- Your SQL goes here
CREATE TABLE IF NOT EXISTS rooms (
    id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    alias_id TEXT UNIQUE,
    joined_at TIMESTAMP NOT NULL DEFAULT timezone('utc', now()),
    parted_at TIMESTAMP
);

CREATE TABLE IF NOT EXISTS room_preferences (
    id BIGINT PRIMARY KEY REFERENCES rooms(id) ON DELETE CASCADE,
    prefix TEXT,
    locale TEXT,
    silent_mode BOOLEAN NOT NULL DEFAULT FALSE
);

CREATE TABLE IF NOT EXISTS senders (
    id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    alias_id TEXT UNIQUE,
    joined_at TIMESTAMP NOT NULL DEFAULT timezone('utc', now()),
    parted_at TIMESTAMP
);

CREATE TABLE IF NOT EXISTS sender_rights (
    id BIGSERIAL PRIMARY KEY,
    sender_id BIGINT NOT NULL REFERENCES senders(id) ON DELETE CASCADE,
    room_id BIGINT NOT NULL REFERENCES rooms(id) ON DELETE CASCADE,
    level SMALLINT NOT NULL DEFAULT 1,

    UNIQUE (sender_id, room_id)
);

CREATE TABLE IF NOT EXISTS events (
  id BIGSERIAL PRIMARY KEY,

  room_id BIGINT NOT NULL REFERENCES rooms(id) ON DELETE CASCADE,
  name TEXT NOT NULL,

  event_type VARCHAR(32) NOT NULL,
  is_massping BOOLEAN NOT NULL DEFAULT FALSE,

  message TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS event_subscriptions (
  id BIGSERIAL PRIMARY KEY,
  event_id BIGINT NOT NULL REFERENCES events(id) ON DELETE CASCADE,
  sender_id BIGINT NOT NULL REFERENCES senders(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS custom_commands (
  id BIGSERIAL UNSIGNED PRIMARY KEY,
  room_id BIGINT UNSIGNED NOT NULL REFERENCES rooms(id) ON DELETE CASCADE,

  name TEXT NOT NULL,
  message TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS custom_command_aliases (
  id BIGSERIAL PRIMARY KEY,
  command_id BIGINT NOT NULL REFERENCES custom_commands(id) ON DELETE CASCADE,
  name TEXT NOT NULL,

  CONSTRAINT unique_command_alias UNIQUE (command_id, name)
);

CREATE TABLE IF NOT EXISTS timers (
  id BIGSERIAL PRIMARY KEY,
  room_id BIGINT NOT NULL REFERENCES rooms(id) ON DELETE CASCADE,

  name TEXT NOT NULL,
  message TEXT NOT NULL,

  interval BIGINT NOT NULL,

  last_executed_at TIMESTAMP NOT NULL DEFAULT timezone('utc', now())
);
