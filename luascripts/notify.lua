local function parse_target(value)
    local parts = str_split(value, ':')
    if #parts < 2 then
        return nil
    end

    local type = parts[#parts]
    local target = ""
    for i = 1, #parts - 1, 1 do
        target = target .. parts[i]
        if i + 1 < #parts then
            target = target .. ":"
        end
    end

    local data = {
        target = target,
        type = str_to_event_type(type)
    }

    if event_type_to_str(data.type) ~= type then
        data.type = nil
    end

    if data.type == nil then
        return data
    end

    if data.type < 40 then
        local users = {}

        -- kick
        if data.type >= 4 and data.type <= 7 then
            users = kick_get_channels(data.target)
        else
            users = twitch_get_users({ logins = { data.target } })
        end

        if #users == 0 then
            data.target = nil
            return data
        end

        data.target = users[1]
    end

    return data
end

local lines = {
    english = {
        ["no_subcommand"] =
        "{sender.alias_name}: No subcommand provided. Use {channel.prefix}help notify for more information.",
        ["no_message"] = "{sender.alias_name}: No message provided.",
        ["not_parseable"] = "{sender.alias_name}: This value cannot be parsed. (%s)",
        ["unknown_type"] = "{sender.alias_name}: Unknown event type. (%s)",
        ["user_not_found"] = "{sender.alias_name}: User not found. (%s)",
        ["not_found"] = "{sender.alias_name}: Event %s not found.",
        ["namesake"] = "{sender.alias_name}: You have already subscribed to this event.",
        ["list"] =
        "{sender.alias_name}: You can use '{channel.prefix}event list' to find out which events you can subscribe to.",
        ["subs"] = "{sender.alias_name}: Your subscriptions: %s",
        ["empty_subs"] = "{sender.alias_name}: You do not have any event subscriptions in this channel.",
        ["sub"] =
        "{sender.alias_name}: You have successfully subscribed to event %s",
        ["unsub"] = "{sender.alias_name}: You have successfully unsubscribed from event %s",
        ["not_subbed"] = "{sender.alias_name}: You were not subscribed to event %s"
    },
    russian = {
        ["no_subcommand"] =
        "{sender.alias_name}: Подкоманда не предоставлена. Используйте {channel.prefix}help event для большей информации.",
        ["no_message"] = "{sender.alias_name}: Сообщение не предоставлено.",
        ["not_parseable"] = "{sender.alias_name}: Это значение не может быть использовано. (%s)",
        ["unknown_type"] = "{sender.alias_name}: Неизвестный тип события. (%s)",
        ["user_not_found"] = "{sender.alias_name}: Пользователь не найден. (%s)",
        ["not_found"] = "{sender.alias_name}: Событие %s не найдено.",
        ["no_target"] = "{sender.alias_name}: Следующие значение события должно быть предоставлено.",
        ["namesake"] = "{sender.alias_name}: Такое же событие уже существует.",
        ["list"] =
        "{sender.alias_name}: Вы можете использовать '{channel.prefix}event list', чтобы узнать на какие события Вы можете подписаться.",
        ["subs"] = "{sender.alias_name}: Ваши подписки: %s",
        ["empty_subs"] = "{sender.alias_name}: Вы не подписаны на какие-либо события в этом канале.",
        ["sub"] =
        "{sender.alias_name}: Вы успешно подписались на событие %s",
        ["unsub"] = "{sender.alias_name}: Вы отписались от события %s",
        ["not_subbed"] = "{sender.alias_name}: Вы не были подписаны на событие %s"
    },
}

return {
    name = "notify",
    summary = "Manage event subscriptions.",
    description = [[
The `!notify` command gives users the ability to manage event subscriptions.

> Event must be created before using `!notify` command. See about `!event` command [here](/!event).

## Syntax

### Subscribe to the event
`!notify sub [name]:[type]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](/scripts.php?id=event#event-types).

### Unsubscribe from the event
`!notify unsub [name]:[type]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](/scripts.php?id=event#event-types).

### Get your event subscriptions
`!notify subs`

### Get available events to subscribe
`!notify list`
]],
    delay_sec = 1,
    options = {},
    aliases = {},
    subcommands = { "sub", "unsub", "subs", "list" },
    minimal_rights = "user",
    handle = function(request)
        if request.subcommand_id == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_subcommand", {})
        end

        local scid = request.subcommand_id

        if scid == "list" then
            return l10n_custom_formatted_line_request(request, lines, "list", {})
        elseif scid == "subs" then
            local names = {}

            local events = db_query([[
SELECT e.name, e.event_type FROM events e
INNER JOIN event_subscriptions es ON es.event_id = e.id
WHERE e.channel_id = $1 AND es.user_id = $2
]],
                { request.channel.id, request.sender.id })
            local user_ids = {}
            for i = 1, #events, 1 do
                local e = events[i]
                local t = tonumber(e.event_type)
                if t < 10 then
                    local id = tonumber(e.name)
                    table.insert(names, { name = id, type = t })
                    table.insert(user_ids, id)
                    print(id)
                else
                    table.insert(names, { name = e.name, type = event_type_to_str(t) })
                end
            end
            if #user_ids > 0 then
                local users = twitch_get_users({ ids = user_ids })
                for i = 1, #users, 1 do
                    local user = users[i]
                    for j = 1, #names, 1 do
                        if type(names[j].name) == "number" and
                            type(names[j].type) == "number" and
                            names[j].type < 10 and names[j].name == user.id
                        then
                            names[j].name = user.login
                            names[j].type = event_type_to_str(names[j].type)
                        end
                    end
                end
            end

            -- finalizing
            local n = {}
            for i = 1, #names, 1 do
                table.insert(n, names[i].name .. ':' .. names[i].type)
            end

            local line_id = "subs"
            if #n == 0 then
                line_id = "empty_subs"
            end

            return l10n_custom_formatted_line_request(request, lines, line_id, { table.concat(n, ', ') })
        end

        if request.message == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_message", {})
        end

        local parts = str_split(request.message, ' ')

        local data_original = parts[1]
        local data = parse_target(data_original)
        table.remove(parts, 1)
        local data_name = nil

        if data == nil then
            return l10n_custom_formatted_line_request(request, lines, "not_parseable", { data_original })
        elseif data.type == nil then
            return l10n_custom_formatted_line_request(request, lines, "unknown_type", { data_original })
        elseif type(data.target) == "nil" then
            return l10n_custom_formatted_line_request(request, lines, "user_not_found", { data_original })
        elseif type(data.target) == "string" then
            data_name = data.target
        elseif type(data.target) == "table" then
            data_name = data.target.id
        end

        local events = db_query([[
SELECT e.id, es.id AS sub_id
FROM events e
LEFT JOIN event_subscriptions es ON es.event_id = e.id AND es.user_id = $1
WHERE e.name = $2 AND e.event_type = $3
]],
            { request.sender.id, data_name, data.type })

        if #events == 0 then
            return l10n_custom_formatted_line_request(request, lines, "not_found", { data_original })
        end

        local event = events[1]

        if scid == "sub" then
            if event.sub_id ~= nil then
                return l10n_custom_formatted_line_request(request, lines, "namesake", { data_original })
            end

            db_execute('INSERT INTO event_subscriptions(event_id, user_id) VALUES ($1, $2)',
                { event.id, request.sender.id })

            return l10n_custom_formatted_line_request(request, lines, "sub", { data_original })
        elseif scid == "unsub" then
            if event.sub_id == nil then
                return l10n_custom_formatted_line_request(request, lines, "not_subbed", { data_original })
            end

            db_execute('DELETE FROM event_subscriptions WHERE id = $1', { event.sub_id })
            return l10n_custom_formatted_line_request(request, lines, "unsub", { data_original })
        end
    end
}
