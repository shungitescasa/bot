local function parse_target(value)
    local parts = str_split(value, ':')
    if #parts ~= 2 then
        return nil
    end

    local data = {
        target = parts[1],
        type = str_to_event_type(parts[2])
    }

    if event_type_to_str(data.type) ~= parts[2] then
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
        "{sender.alias_name}: No subcommand provided. Use {channel.prefix}help event for more information.",
        ["no_message"] = "{sender.alias_name}: No message provided.",
        ["not_parseable"] = "{sender.alias_name}: This value cannot be parsed. (%s)",
        ["unknown_type"] = "{sender.alias_name}: Unknown event type. (%s)",
        ["user_not_found"] = "{sender.alias_name}: User not found. (%s)",
        ["not_found"] = "{sender.alias_name}: Event %s not found.",
        ["no_target"] = "{sender.alias_name}: Next event target must be provided.",
        ["namesake"] = "{sender.alias_name}: Same event already exists.",
        ["list"] = "{sender.alias_name}: %s",
        ["empty_list"] = "{sender.alias_name}: There are no events in this channel.",
        ["on"] =
        "{sender.alias_name}: Successfully created a new event %s. Use '{channel.prefix}notify sub %s' to subscribe.",
        ["off"] = "{sender.alias_name}: Successfully deleted event %s",
        ["edit"] = "{sender.alias_name}: Edited a message for event %s",
        ["settarget"] = "{sender.alias_name}: Changed event target from %s to %s",
        ["massping_disabled"] = "{sender.alias_name}: Massping has been disabled for event %s",
        ["massping_enabled"] = "{sender.alias_name}: Massping has been enabled for event %s",
        ["view"] = "{sender.alias_name}: ID %s | %s | %s subs | Massping: %s | %s"
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
        ["list"] = "{sender.alias_name}: %s",
        ["empty_list"] = "{sender.alias_name}: На этом канале нету событий.",
        ["on"] =
        "{sender.alias_name}: Успешно создано событие %s. Используйте '{channel.prefix}notify sub %s' для подписки.",
        ["off"] = "{sender.alias_name}: Успешно удалено событие %s",
        ["edit"] = "{sender.alias_name}: Сообщение для события %s было отредактировано.",
        ["settarget"] = "{sender.alias_name}: Цель события было изменено с %s на %s",
        ["flag_disabled"] = "{sender.alias_name}: Флаг %s был отключен для события %s",
        ["flag_enabled"] = "{sender.alias_name}: Флаг %s был включен для события %s",
        ["view"] = "{sender.alias_name}: ID %s | %s | %s подписчиков | Флаги: %s | %s"
    },
}

return {
    name = "event",
    description = [[
> This command is for broadcaster and moderators only.


The `!event` command gives the ability to manage events.


## Event types (and their placeholders)

+ live
+ offline
+ title *(`{new}` - new title, `{old}` - old title)*
+ game *(`{new}` - new game, `{old}` - old game)*
+ kick_live
+ kick_offline
+ kick_title *(`{new}` - new title, `{old}` - old title)*
+ kick_game *(`{new}` - new game, `{old}` - old game)*
+ github *(`{sha}` - commit ID, `{author}` - committer, `{msg}` - message)*
+ 7tv_new_emote *(`{emote}` - emote name, `{old_emote}` - original emote name, `{author}` - name of the person who added it)*
+ 7tv_deleted_emote *(`{emote}` - emote name, `{old_emote}` - original emote name, `{author}` - name of the person who added it)*
+ 7tv_updated_emote *(`{emote}` - new emote name, `{old_emote}` - previous emote name, `{author}` - name of the person who added it)*
+ custom

## How to use placeholders?

Some event types have placeholders that can enrich your event message. You can easily use them by simply inserting them into your message.
Here are some basic examples to inspire you:

+ `!event on forsen:game Forsen is now playing {new} (previously, he played {old})`
+ `!event on torvalds/linux:github {author} made a new commit in linux kernel: {msg} (ID {sha})` - Please also note that the event name has the following format: **username/repository** *(https://github.com/ **username/repository**)*
+ `!event on forsen:7tv_new_emote {author} added a new 7TV emote: {emote}`
+ `!event on forsen:7tv_updated_emote {author} renamed a 7TV emote from {old_emote} to {emote}`

## Syntax

### Create a new event

`!event on [name]:[type] [message...]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](#event-types).
+ `[message]` - The message that will be sent with the event.

### Delete the event

`!event off [name]:[type]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](#event-types).

### Make the event massping everytime

`!event setmassping [name]:[type]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](#event-types).

### Edit the event message

`!event edit [name]:[type] [message...]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](#event-types).
+ `[message]` - New message.

### Set a new target for the event

`!event settarget [name]:[type] [new_name]:[new_type]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](#event-types).
+ `[new_name]` - New Twitch username or event name *(custom type only)*.
+ `[new_type]` - [New event type](#event-types).


### Call the event


> The bot requires moderator privileges on events with the **"massping"** flag.


`!event call [name]:[type]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](#event-types).

### View the event

`!event view [name]:[type]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](#event-types).
]],
    delay_sec = 1,
    options = {},
    aliases = {},
    subcommands = { "on", "off", "list", "edit", "settarget", "setmassping", "call", "view" },
    minimal_rights = "moderator",
    handle = function(request)
        if request.subcommand_id == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_subcommand", {})
        end

        local scid = request.subcommand_id

        if scid == "list" then
            local names = {}

            local events = db_query('SELECT name, event_type FROM events WHERE channel_id = $1',
                { request.channel.id })
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

            local line_id = "list"
            if #n == 0 then
                line_id = "empty_list"
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

        local events = db_query(
            'SELECT id, message, is_massping FROM events WHERE name = $1 AND event_type = $2',
            { data_name, data.type })

        if scid == "on" then
            if #events > 0 then
                return l10n_custom_formatted_line_request(request, lines, "namesake", { data_original })
            end

            local message = table.concat(parts, ' ')
            if #message == 0 then
                return l10n_custom_formatted_line_request(request, lines, "no_message", {})
            end

            db_execute('INSERT INTO events(channel_id, name, event_type, message) VALUES ($1, $2, $3, $4)',
                { request.channel.id, data_name, data.type, message })

            return l10n_custom_formatted_line_request(request, lines, "on", { data_original, data_original })
        end

        if #events == 0 then
            return l10n_custom_formatted_line_request(request, lines, "not_found", { data_original })
        end

        local event = events[1]

        if scid == "off" then
            db_execute('DELETE FROM events WHERE id = $1', { event.id })
            return l10n_custom_formatted_line_request(request, lines, "off", { data_original })
        elseif scid == "edit" then
            local message = table.concat(parts, ' ')
            if #message == 0 then
                return l10n_custom_formatted_line_request(request, lines, "no_message", {})
            end

            db_execute('UPDATE events SET message = $1 WHERE id = $2', { message, event.id })

            return l10n_custom_formatted_line_request(request, lines, "edit", { data_original })
        elseif scid == "settarget" then
            if #parts == 0 then
                return l10n_custom_formatted_line_request(request, lines, "no_target", {})
            end

            local new_data_original = parts[1]
            local data = parse_target(new_data_original)
            local data_name = nil
            if data == nil then
                return l10n_custom_formatted_line_request(request, lines, "not_parseable", { new_data_original })
            elseif data.type == nil then
                return l10n_custom_formatted_line_request(request, lines, "unknown_type", { new_data_original })
            elseif type(data.target) == "nil" then
                return l10n_custom_formatted_line_request(request, lines, "user_not_found", { new_data_original })
            elseif type(data.target) == "string" then
                data_name = data.target
            elseif type(data.target) == "table" then
                data_name = data.target.id
            end

            local existing_events = db_query(
                'SELECT id FROM events WHERE channel_id = $1 AND name = $2 AND event_type = $3',
                { request.channel.id, data_name, data.type })

            if #existing_events > 0 then
                return l10n_custom_formatted_line_request(request, lines, "namesake", { new_data_original })
            end

            db_execute('UPDATE events SET name = $1, event_type = $2 WHERE id = $3',
                { data_name, data.type, event.id })

            return l10n_custom_formatted_line_request(request, lines, "settarget", { data_original, new_data_original })
        elseif scid == "setmassping" then
            local line_id = ""
            local query = ""
            if event.is_massping == "1" then
                line_id = "massping_disabled"
                query = "UPDATE events SET is_massping = 0 WHERE id = $1"
            else
                line_id = "massping_enabled"
                query = "UPDATE events SET is_massping = 1 WHERE id = $1"
            end

            db_execute(query, { event.id })

            return l10n_custom_formatted_line_request(request, lines, line_id, { data_original })
        elseif scid == "call" then
            local names = {}

            if event.is_massping == "1" then
                local chatters = twitch_get_chatters()
                for i = 1, #chatters, 1 do
                    table.insert(names, chatters[i].login)
                end
            else
                local subscriptions = db_query([[
SELECT u.alias_name FROM users u
INNER JOIN event_subscriptions es ON es.user_id = u.id
INNER JOIN events e ON e.id = es.event_id
WHERE e.id = $1
]], { event.id })

                for i = 1, #subscriptions, 1 do
                    table.insert(names, subscriptions[i].alias_name)
                end
            end

            local base = '⚡️ ' .. event.message
            if #names > 0 then
                base = base .. ' · '
            end

            return str_make_parts(base, names, "@", " ", 500)
        elseif scid == "view" then
            local subscription_count = db_query([[
SELECT COUNT(es.id) AS count FROM event_subscriptions es
INNER JOIN events e ON e.id = es.event_id
WHERE e.id = $1
]], { event.id })

            local massping_flag = "OFF"
            if event.is_massping == "1" then
                massping_flag = "ON"
            end

            return l10n_custom_formatted_line_request(request, lines, "view",
                { event.id, data_original, subscription_count[1].count, massping_flag, event.message })
        end
    end
}
