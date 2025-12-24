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

        local channel_name = request.channel.alias_name
        local channel_id = request.channel.alias_id

        if request.message ~= nil and
            array_contains_int(cfg.twitch.superuser_ids, request.sender.alias_id)
        then
            local users = twitch_get_users({ logins = { request.message } })

            if #users == 0 then
                return l10n_custom_formatted_line_request(request, lines, "user_not_found", { request.message })
            end

            local user = users[1]

            channel_name = user.login
            channel_id = tonumber(user.id)
        end

        local db_channels = db_query('SELECT id FROM channels WHERE alias_id = $1 AND opted_out_at IS NULL',
            { channel_id })

        if #db_channels == 0 then
            return l10n_custom_formatted_line_request(request, lines, "already_out", {})
        end

        irc_send_message(
            {login = channel_name, id = channel_id},
            l10n_custom_formatted_line_request(request, lines, "success", {})
        )

        irc_part_channel({login = channel_name, id = channel_id})

        db_execute('UPDATE channels SET opted_out_at = UTC_TIMESTAMP() WHERE alias_id = $1',
            { channel_id })

        return nil
    end
}
