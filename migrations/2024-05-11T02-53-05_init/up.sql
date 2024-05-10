-- Your SQL goes here
CREATE TABLE IF NOT EXISTS "channels"  (
  "id" SERIAL NOT NULL UNIQUE PRIMARY KEY,
  "alias_id" INTEGER NOT NULL UNIQUE,
  "alias_name" VARCHAR NOT NULL UNIQUE,
  "joined_at" TIMESTAMP NOT NULL DEFAULT timezone('utc', now()),
  "opted_out_at" TIMESTAMP
);

CREATE TABLE IF NOT EXISTS "users"  (
  "id" SERIAL NOT NULL UNIQUE PRIMARY KEY,
  "alias_id" INTEGER NOT NULL UNIQUE,
  "alias_name" VARCHAR NOT NULL UNIQUE,
  "joined_at" TIMESTAMP NOT NULL DEFAULT timezone('utc', now()),
  "opted_out_at" TIMESTAMP
);

CREATE TABLE IF NOT EXISTS "channel_preferences" (
  "channel_id" INTEGER NOT NULL UNIQUE PRIMARY KEY REFERENCES "channels"("id"),
  "prefix" VARCHAR,
  "locale" VARCHAR
);

CREATE TABLE IF NOT EXISTS "user_rights" (
  "id" SERIAL NOT NULL UNIQUE PRIMARY KEY,
  "user_id" INTEGER NOT NULL REFERENCES "users"("id"),
  "channel_id" INTEGER NOT NULL REFERENCES "channels"("id"),
  "level" INTEGER NOT NULL DEFAULT 1,

  CONSTRAINT "unique_user_channel" UNIQUE ("user_id", "channel_id")
);

CREATE TABLE IF NOT EXISTS "actions" (
  "id" SERIAL NOT NULL UNIQUE PRIMARY KEY,
  "user_id" INTEGER NOT NULL REFERENCES "users"("id"),
  "channel_id" INTEGER NOT NULL REFERENCES "channels"("id"),
  "command" VARCHAR NOT NULL,
  "arguments" VARCHAR,
  "full_message" VARCHAR NOT NULL,
  "sent_at" TIMESTAMP NOT NULL DEFAULT timezone('utc', now())
);

CREATE TABLE IF NOT EXISTS "events" (
  "id" SERIAL NOT NULL UNIQUE PRIMARY KEY,
  "channel_id" INTEGER NOT NULL REFERENCES "channels"("id"),
  "target_alias_id" INTEGER,
  "custom_alias_id" VARCHAR,
  "event_type" INTEGER NOT NULL,
  "flags" INTEGER[] NOT NULL DEFAULT ARRAY[]::INTEGER[],
  "message" VARCHAR NOT NULL,

  CONSTRAINT check_target_alias_id CHECK (("target_alias_id" IS NULL AND "custom_alias_id" IS NOT NULL) OR ("target_alias_id" IS NOT NULL AND "custom_alias_id" IS NULL)),
  CONSTRAINT check_event_type CHECK (("target_alias_id" IS NOT NULL AND "event_type" != 99) OR ("custom_alias_id" IS NOT NULL AND "event_type" = 99))
);

CREATE TABLE IF NOT EXISTS "event_subscriptions" (
  "id" SERIAL NOT NULL UNIQUE PRIMARY KEY,
  "event_id" INTEGER NOT NULL REFERENCES "events"("id"),
  "user_id" INTEGER NOT NULL REFERENCES "users"("id")
);

CREATE TABLE IF NOT EXISTS "custom_commands" (
  "id" SERIAL NOT NULL UNIQUE PRIMARY KEY,
  "channel_id" INTEGER NOT NULL REFERENCES "channels"("id"),
  "name" VARCHAR NOT NULL,
  "message" VARCHAR NOT NULL
);

CREATE TABLE IF NOT EXISTS "timers" (
  "id" SERIAL NOT NULL UNIQUE PRIMARY KEY,
  "channel_id" INTEGER NOT NULL REFERENCES "channels"("id"),
  "name" VARCHAR NOT NULL,
  "message" VARCHAR NOT NULL,
  "interval_sec" INTEGER NOT NULL,
  "last_executed_at" TIMESTAMP NOT NULL DEFAULT timezone('utc', now())
);
