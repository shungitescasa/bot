-- Your SQL goes here
ALTER TABLE custom_commands ADD COLUMN is_global BOOLEAN NOT NULL DEFAULT FALSE;