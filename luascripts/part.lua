local lines = {
    english = {
        ["command_unavailable"] = "{sender.alias_name}: This command is not available.",
        ["user_not_found"] = "{sender.alias_name}: User %s not found",
        ["already_out"] = "{sender.alias_name}: I've already left this chat room!",
        ["success"] = "{sender.alias_name}: Bye!",
    },
    russian = {
        ["command_unavailable"] = "{sender.alias_name}: Эта команда недоступна.",
        ["user_not_found"] = "{sender.alias_name}: Пользователь %s не найден",
        ["already_out"] = "{sender.alias_name}: Я уже ушёл из этого чата!",
        ["success"] = "{sender.alias_name}: Пока!",
    },
}

return {
    name = "part",
    summary = "Remove the bot from your channel.",
    description = "Remove the bot from your channel.",
    delay_sec = 1,
    options = {},
    aliases = {},
    subcommands = {},
    minimal_rights = "moderator",
    handle = function(request)
        local cfg = bot_config()
        if cfg == nil then
            return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
        end

        local room_name = request.room.name
        local room_id = request.room.alias_id

        if request.message ~= nil and array_contains(cfg.instance.supernicks, request.sender.name) then
            local users = twitch_get_users({ logins = { request.message } })

            if #users == 0 then
                return l10n_custom_formatted_line_request(request, lines, "user_not_found", { request.message })
            end

            local user = users[1]

            room_name = user.login
            room_id = tonumber(user.id)
        end

        local db_channels = db_query('SELECT id FROM rooms WHERE name = $1 AND parted_at IS NULL',
            { room_name })

        if #db_channels == 0 then
            return l10n_custom_formatted_line_request(request, lines, "already_out", {})
        end

        irc_send_message(
            { login = room_name, id = room_id },
            l10n_custom_formatted_line_request(request, lines, "success", {})
        )

        irc_part_channel({ login = room_name, id = room_id })

        db_execute('UPDATE rooms SET parted_at = UTC_TIMESTAMP() WHERE name = $1', { room_name })

        return nil
    end
}
