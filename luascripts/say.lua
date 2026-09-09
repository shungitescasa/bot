local lines = {
    english = {
        ["sent"] = "{sender.alias_name}: Sent to %s (%s)!",
        ["no_message"] = "{sender.alias_name}: Message must be provided.",
        ["user_not_found"] = "{sender.alias_name}: User %s not found"
    },
    russian = {
        ["sent"] = "{sender.alias_name}: Отправлено в %s (%s)!",
        ["no_message"] = "{sender.alias_name}: Сообщение должно быть указано.",
        ["user_not_found"] = "{sender.alias_name}: Пользователь %s не найден"
    },
}

return {
    name = "say",
    summary = "Make the bot say something.",
    description = "Make the bot say something. Available only to superusers.",
    delay_sec = 5,
    options = {},
    subcommands = {},
    aliases = {},
    minimal_rights = "superuser",
    handle = function(request)
        if request.message == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_message", {})
        end

        local msg = request.message

        local parts = str_split(msg, " ")
        if #parts == 0 then
            return l10n_custom_formatted_line_request(request, lines, "no_message", {})
        end

        local room_name = request.room.name
        local room_id = request.room.alias_id
        if string.sub(parts[1], 1, 1) == "#" then
            room_name = string.sub(parts[1], 2, #parts[1])
            table.remove(parts, 1)
            msg = table.concat(parts, " ")
            local users = twitch_get_users({ logins = { room_name } })

            if #users == 0 then
                return l10n_custom_formatted_line_request(request, lines, "user_not_found", { room_name })
            end

            local user = users[1]

            room_name = user.login
            room_id = tonumber(user.id)
        end

        if room_name ~= request.room.name then
            irc_send_message({ login = room_name, id = room_id }, msg)
            return l10n_custom_formatted_line_request(request, lines, "sent", { room_name, tostring(room_id) })
        else
            return msg
        end
    end,
}
