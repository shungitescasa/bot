-- Your SQL goes here
CREATE TABLE IF NOT EXISTS lua_channel_storage(
    id BIGSERIAL PRIMARY KEY,
    room_id BIGINT NOT NULL REFERENCES rooms(id) ON DELETE CASCADE,
    lua_id TEXT NOT NULL,
    value TEXT NOT NULL DEFAULT '',

    UNIQUE (room_id, lua_id)
);

CREATE TABLE IF NOT EXISTS lua_user_storage(
    id BIGSERIAL PRIMARY KEY,
    sender_id BIGINT NOT NULL REFERENCES senders(id) ON DELETE CASCADE,
    lua_id TEXT NOT NULL,
    value TEXT NOT NULL DEFAULT '',

    UNIQUE (sender_id, lua_id)
);