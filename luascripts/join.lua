local lines = {
    english = {
        ["command_unavailable"] = "{sender.alias_name}: This command is not available.",
        ["user_not_found"] = "{sender.alias_name}: User %s not found",
        ["join_not_allowed"] = "{sender.alias_name}: The bot cannot join chat rooms!",
        ["join_from_bot_channel"] =
        "{sender.alias_name}: In order for the bot to join your chat, you need to send {default.prefix}join directly in %s chat.",
        ["rejoined"] = "{sender.alias_name}: I have rejoined your chat room!",
        ["already_in"] = "{sender.alias_name}: I'm already in your chat room!",
        ["chat_response"] =
        "Hi, I'm %s and I'll be serving this chat. Send {default.prefix}help to learn about the commands!",
        ["join"] = "{sender.alias_name}: Successfully joined your chat room!",
    },
    russian = {
        ["command_unavailable"] = "{sender.alias_name}: Эта команда недоступна.",
        ["user_not_found"] = "{sender.alias_name}: Пользователь %s не найден",
        ["join_not_allowed"] = "{sender.alias_name}: Этот бот не может заходить в чаты!",
        ["join_from_bot_channel"] =
        "{sender.alias_name}: Вам нужно отправить {default.prefix}join прямо в чат %s, чтобы бот мог зайти к Вам в чат",
        ["rejoined"] = "{sender.alias_name}: Я перезашёл в этот чат!",
        ["already_in"] = "{sender.alias_name}: Я уже в этом чате!",
        ["chat_response"] =
        "Привет, я %s и я буду обслуживать этот чат. Отправьте {default.prefix}help, чтобы узнать больше о командах!",
        ["join"] = "{sender.alias_name}: Успешно зашёл в чат!",
    },
}

return {
    name = "join",
    summary = "Add the bot to your channel.",
    description = "Add the bot to your channel.",
    delay_sec = 1,
    options = {},
    aliases = {},
    subcommands = { "silent" },
    minimal_rights = "user",
    handle = function(request)
        local cfg = bot_config()
        if cfg == nil then
            return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
        end

        if not cfg.commands.join_allowed then
            return l10n_custom_formatted_line_request(request, lines, "join_not_allowed", {})
        end

        local channel_name = request.sender.alias_name
        local channel_id = request.sender.alias_id
        local silent_mode = false

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

            silent_mode = request.subcommand_id ~= nil and request.subcommand_id == "silent"
        end

        if not cfg.commands.join_allow_from_other_chats and request.channel.alias_name ~= bot_username() then
            return l10n_custom_formatted_line_request(request, lines, "join_from_bot_channel", { bot_username() })
        end

        local db_channels = db_query('SELECT id, alias_id, alias_name, opted_out_at FROM channels WHERE alias_id = $1',
            { channel_id })

        if #db_channels > 0 then
            local db_channel = db_channels[1]

            if db_channel.opted_out_at ~= nil then
                db_execute('UPDATE channels SET opted_out_at = NULL WHERE id = $1', { db_channel.id })

                irc_join_channel({login = channel_name, id = channel_id})
                irc_send_message(
                    {login = channel_name, id = channel_id},
                    l10n_custom_formatted_line_request(request, lines, "chat_response", { bot_username() })
                )
                return l10n_custom_formatted_line_request(request, lines, "rejoined", {})
            end

            return l10n_custom_formatted_line_request(request, lines, "already_in", {})
        end

        db_execute('INSERT INTO channels(alias_id, alias_name) VALUES ($1, $2)',
            { channel_id, channel_name })

        irc_join_channel({login = channel_name, id = channel_id})

        if not silent_mode then
            irc_send_message(
                {login = channel_name, id = channel_id},
                l10n_custom_formatted_line_request(request, lines, "chat_response", { bot_username() })
            )
        else
            local db_channel = db_query('SELECT id FROM channels WHERE alias_id = $1', { channel_id })[1]
            db_execute('INSERT IGNORE INTO channel_preferences(id, silent_mode) VALUES ($1, $2)',
                { db_channel.id, silent_mode })
        end

        return l10n_custom_formatted_line_request(request, lines, "join", {})
    end
}
