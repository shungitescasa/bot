local lines = {
    english = {
        ["no_subcommand"] =
        "{sender.alias_name}: No subcommand provided. Use {channel.prefix}help notify for more information.",
        ["no_message"] = "{sender.alias_name}: No message provided.",
        ["not_parseable"] = "{sender.alias_name}: This value cannot be parsed. (%s)",
        ["invalid_event"] = "{sender.alias_name}: Unknown event type. (%s)",
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
        ["invalid_event"] = "{sender.alias_name}: Неизвестный тип события. (%s)",
        ["not_found"] = "{sender.alias_name}: Событие %s не найдено.",
        ["namesake"] = "{sender.alias_name}: Вы уже подписаны.",
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

# Syntax

## Subscribe to the event
`!notify sub [name]:[type]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](/!event#event-types).

## Unsubscribe from the event
`!notify unsub [name]:[type]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](/!event#event-types).

## Get your event subscriptions
`!notify subs`

## Get available events to subscribe
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
WHERE e.room_id = $1 AND es.sender_id = $2
]],
                { request.room.id, request.sender.id })
            local n = {}
            for i = 1, #events, 1 do
                local e = events[i]
                table.insert(n, e.name .. ":" .. e.event_type)
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

        -- parsing target name & type
        local data = parts[1]
        local target = events_parse_target(data)
        if target == nil then
            return l10n_custom_formatted_line_request(request, lines, "not_parseable", { data })
        elseif not is_valid_event(target.type, target.name) then
            return l10n_custom_formatted_line_request(request, lines, "invalid_event", { data })
        end
        table.remove(parts, 1)

        local events = db_query([[
SELECT e.id, es.id AS sub_id
FROM events e
LEFT JOIN event_subscriptions es ON es.event_id = e.id AND es.sender_id = $1
WHERE e.name = $2 AND e.event_type = $3 AND e.room_id = $4
]],
            { request.sender.id, target.name, target.type, request.room.id })

        if #events == 0 then
            return l10n_custom_formatted_line_request(request, lines, "not_found", { data })
        end

        local event = events[1]

        if scid == "sub" then
            if event.sub_id ~= nil then
                return l10n_custom_formatted_line_request(request, lines, "namesake", { data })
            end

            db_execute('INSERT INTO event_subscriptions(event_id, sender_id) VALUES ($1, $2)',
                { event.id, request.sender.id })

            return l10n_custom_formatted_line_request(request, lines, "sub", { data })
        elseif scid == "unsub" then
            if event.sub_id == nil then
                return l10n_custom_formatted_line_request(request, lines, "not_subbed", { data })
            end

            db_execute('DELETE FROM event_subscriptions WHERE id = $1', { event.sub_id })
            return l10n_custom_formatted_line_request(request, lines, "unsub", { data })
        end
    end
}
