-- Your SQL goes here
CREATE TABLE IF NOT EXISTS lua_channel_storage(
    id INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
    channel_id INTEGER NOT NULL REFERENCES channels(id) ON DELETE CASCADE,
    lua_id TEXT NOT NULL,
    value TEXT NOT NULL DEFAULT '',

    CONSTRAINT unique_lua_script_per_channel UNIQUE (channel_id, lua_id)
);

CREATE TABLE IF NOT EXISTS lua_user_storage(
    id INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    lua_id TEXT NOT NULL,
    value TEXT NOT NULL DEFAULT '',

    CONSTRAINT unique_lua_script_per_channel UNIQUE (user_id, lua_id)
);