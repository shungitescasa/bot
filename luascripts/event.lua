local lines = {
    english = {
        ["no_subcommand"] =
        "{sender.alias_name}: No subcommand provided. Use {channel.prefix}help event for more information.",
        ["no_message"] = "{sender.alias_name}: No message provided.",
        ["not_parseable"] = "{sender.alias_name}: This value cannot be parsed. (%s)",
        ["invalid_event"] = "{sender.alias_name}: Unknown event type. (%s)",
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
        ["invalid_event"] = "{sender.alias_name}: Неизвестный тип события. (%s)",
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
        ["massping_disabled"] = "{sender.alias_name}: Масспинг был выключен для события %s",
        ["massping_enabled"] = "{sender.alias_name}: Масспинг был включен для события %s",
        ["view"] = "{sender.alias_name}: ID %s | %s | %s подписчиков | Масспинг: %s | %s"
    },
}

return {
    name = "event",
    summary = "Manage events.",
    description = [[
> This command is for broadcaster and moderators only.


The `!event` command gives the ability to manage events.


<h1 id="event-types">Event types</h1>

## Twitch

+ twitch.live
+ twitch.offline
+ twitch.title
+ + Placeholders: `{new}` - new title
+ + Message example: This streamer has changed the title - {new}
+ twitch.game
+ + Placeholders: `{new}` - new game
+ + Message example: This streamer is now playing {new}
+ twitch.first-message
+ + Placeholders: `{message}` - user's first message, `{author}` - username, `{origin}` - room name
+ + Message example: {author} has just said their first words in #{origin}: {message}
+ + Target example: `forsen:twitch.first-message`
+ + **Note: To receive the first messages, the bot will join the target channel.**
+ twitch.message
+ + Placeholders: `{message}` - target's message, `{author}` - username, `{origin}` - room name
+ + Message example: {author} has just said in #{origin}: {message}
+ + Target example: `forsen:twitch.message`
+ + **Note: To receive the messages, the bot will join the target channel.**

## Kick
+ kick.live
+ kick.offline
+ kick.title
+ + Placeholders: `{new}` - new title
+ + Message example: This streamer has changed the title - {new}
+ kick.game
+ + Placeholders: `{new}` - new game
+ + Message example: This streamer is now playing {new}

## 7TV

+ 7tv.added-emote
+ + Placeholders: `{new}` - emote name, `{old}` - original emote name
+ + Message example: added a new 7TV emote: {new} (originally {old})
+ + Target example: `xqc:7tv.added-emote`

+ 7tv.deleted-emote
+ + Placeholders: `{new}` - emote name, `{old}` - original emote name
+ + Message example: added a new 7TV emote: {new} (originally {old})
+ + Target example: `xqc:7tv.deleted-emote`

+ 7tv.renamed-emote
+ + Placeholders: `{new}` - new emote name, `{old}` - previous emote name
+ + Message example: renamed a 7TV emote from {old} to {new}
+ + Target example: `xqc:7tv.renamed-emote`

## BetterTTV

+ bttv.added-emote
+ + Placeholders: `{new}` - emote name, `{old}` - original emote name
+ + Message example: New BTTV emote: {new} (originally {old})
+ + Target example: `forsen:bttv.added-emote`

+ bttv.deleted-emote
+ + Placeholders: `{new}` - emote name, `{old}` - original emote name
+ + Message example: BTTV emote {new} has been removed
+ + Target example: `forsen:bttv.deleted-emote`

+ bttv.renamed-emote
+ + Placeholders: `{new}` - new emote name, `{old}` - previous emote name
+ + Message example: BTTV emote {old} has been renamed to {new}
+ + Target example: `forsen:bttv.renamed-emote`

## Social media

+ telegram.post
+ + Placeholders: `{message}` - message content, `{link}` - message link
+ + Message example: Durov posted a new Telegram post: {message} ({link})
+ + Target example: `durov:telegram.post`

+ twitter.post
+ + Placeholders: `{message}` - message content, `{link}` - message link
+ + Message example: Notch posted a new tweet: {message} ({link})
+ + Target example: `notch:twitter.post`

## Miscellaneous

+ rss
+ + Placeholders: `{message}` - message content, `{link}` - message link
+ + Message example: new post on shungites.casa: {message} ({link})
+ + Target example: `https://shungites.casa/index.xml:rss`

# Syntax

> `[target]` is short for `[name]:[type]`. For `[name]`, look for *Target example* in your [event type](#event-types). If there is no such field, use Twitch username.

## Create a new event

`!event on [target] [message...]`

+ `[target]` - Event target.
+ `[message]` - The message that will be sent with the event.

## Delete the event

`!event off [target]`

+ `[target]` - Event target.

## Make the event massping everytime

`!event massping [target]`

+ `[target]` - Event target.

## Edit the event message

`!event edit [target] [message...]`

+ `[target]` - Event target.
+ `[message]` - New message.

## Set a new target for the event

`!event target [target] [new_target]`

+ `[target]` - Current event target.
+ `[new_target]` - New event target.


## Call the event


> The bot requires moderator privileges on events with the **"massping"** flag.


`!event call [target]`

+ `[target]` - Event target.

## View the event

`!event view [target]`

+ `[target]` - Event target.
]],
    delay_sec = 1,
    options = {},
    aliases = {},
    subcommands = { "on", "off", "list", "edit", "target", "massping", "call", "view" },
    minimal_rights = "moderator",
    handle = function(request)
        if request.subcommand_id == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_subcommand", {})
        end

        local scid = request.subcommand_id

        if scid == "list" then
            local names = {}

            local events = db_query('SELECT name, event_type FROM events WHERE room_id = $1', { request.room.id })
            local n = {}
            for i = 1, #events, 1 do
                local e = events[i]
                table.insert(n, e.name .. ":" .. e.event_type)
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

        -- parsing target name & type
        local data = parts[1]
        local target = events_parse_target(data)
        if target == nil then
            return l10n_custom_formatted_line_request(request, lines, "not_parseable", { data })
        elseif not is_valid_event(target.type, target.name) then
            return l10n_custom_formatted_line_request(request, lines, "invalid_event", { data })
        end
        table.remove(parts, 1)

        local events = db_query(
            'SELECT id, message, is_massping FROM events WHERE name = $1 AND event_type = $2 AND room_id = $3',
            { target.name, target.type, request.room.id })

        if scid == "on" then
            if #events > 0 then
                return l10n_custom_formatted_line_request(request, lines, "namesake", { data })
            end

            local message = table.concat(parts, ' ')
            if #message == 0 then
                return l10n_custom_formatted_line_request(request, lines, "no_message", {})
            end

            db_execute('INSERT INTO events(room_id, name, event_type, message) VALUES ($1, $2, $3, $4)',
                { request.room.id, target.name, target.type, message })

            return l10n_custom_formatted_line_request(request, lines, "on", { data, data })
        end

        if #events == 0 then
            return l10n_custom_formatted_line_request(request, lines, "not_found", { data })
        end

        local event = events[1]

        if scid == "off" then
            db_execute('DELETE FROM events WHERE id = $1', { event.id })
            return l10n_custom_formatted_line_request(request, lines, "off", { data })
        elseif scid == "edit" then
            local message = table.concat(parts, ' ')
            if #message == 0 then
                return l10n_custom_formatted_line_request(request, lines, "no_message", {})
            end

            db_execute('UPDATE events SET message = $1 WHERE id = $2', { message, event.id })

            return l10n_custom_formatted_line_request(request, lines, "edit", { data })
        elseif scid == "target" then
            if #parts == 0 then
                return l10n_custom_formatted_line_request(request, lines, "no_target", {})
            end

            local new_data = parts[1]
            local new_target = events_parse_target(new_data)
            if new_target == nil then
                return l10n_custom_formatted_line_request(request, lines, "not_parseable", { new_data })
            elseif not is_valid_event(new_target.type, new_target.name) then
                return l10n_custom_formatted_line_request(request, lines, "invalid_event", { new_data })
            end

            local existing_events = db_query(
                'SELECT id FROM events WHERE room_id = $1 AND name = $2 AND event_type = $3',
                { request.room.id, new_target.name, new_target.type })

            if #existing_events > 0 then
                return l10n_custom_formatted_line_request(request, lines, "namesake", { new_data })
            end

            db_execute('UPDATE events SET name = $1, event_type = $2 WHERE id = $3',
                { new_target.name, new_target.type, event.id })

            return l10n_custom_formatted_line_request(request, lines, "settarget", { data, new_data })
        elseif scid == "massping" then
            local line_id = ""
            local query = ""
            if event.is_massping == "1" then
                line_id = "massping_disabled"
                query = "UPDATE events SET is_massping = FALSE WHERE id = $1"
            else
                line_id = "massping_enabled"
                query = "UPDATE events SET is_massping = TRUE WHERE id = $1"
            end

            db_execute(query, { event.id })

            return l10n_custom_formatted_line_request(request, lines, line_id, { data })
        elseif scid == "call" then
            local names = {}

            if event.is_massping == "1" then
                local chatters = twitch_get_chatters(request.room.alias_id)
                for i = 1, #chatters, 1 do
                    table.insert(names, chatters[i].login)
                end
            else
                local subscriptions = db_query([[
SELECT s.name FROM senders s
INNER JOIN event_subscriptions es ON es.sender_id = s.id
INNER JOIN events e ON e.id = es.event_id
WHERE e.id = $1
]], { event.id })

                for i = 1, #subscriptions, 1 do
                    table.insert(names, subscriptions[i].name)
                end
            end

            local base = '⚡️ ' .. event.message
            if #names > 0 then
                base = base .. ' · '
            end

            return str_make_parts(base, names, "", " ", 500)
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
                { event.id, data, subscription_count[1].count, massping_flag, event.message })
        end
    end
}
