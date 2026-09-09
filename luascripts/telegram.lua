local lines = {
    english = {
        ["not_configured"] = "{sender.alias_name}: This command is not set up properly. Try again later.",
        ["not_found"] = "{sender.alias_name}: Channel %s not found.",
        ["no_value"] = "{sender.alias_name}: Public Telegram channel must be specified.",
        ["message"] = "{sender.alias_name}: %s's last post: %s (posted %s ago) (%s)",
        ["no_messages"] = "{sender.alias_name}: This Telegram channel does not have any posts."
    },
    russian = {
        ["not_configured"] = "{sender.alias_name}: Команда не настроена. Попробуйте позже.",
        ["not_found"] = "{sender.alias_name}: Канал %s не найден.",
        ["no_value"] = "{sender.alias_name}: Нужно указать публичный Telegram канал.",
        ["message"] = "{sender.alias_name}: Последний пост %s: %s (опубликовано %s назад) (%s)",
        ["no_messages"] = "{sender.alias_name}: Этот Telegram канал не содержит каких-либо постов."
    },
}

return {
    name = "telegram",
    summary = "Get the latest post from Telegram channel.",
    description = [[
Get the latest post from the specified public Telegram channel.

# Syntax

`!telegram [username]`

+ `[username]` - Valid public Telegram channel.

# Usage

+ `!telegram durov`
]],
    delay_sec = 5,
    options = {},
    subcommands = {},
    aliases = { "tg", "tgc", "тгк", "тг" },
    minimal_rights = "user",
    handle = function(request)
        local cfg = bot_config()
        if cfg.rss.url == nil then
            return l10n_custom_formatted_line_request(request, lines, "not_configured", {})
        end

        if request.message == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_value", {})
        end

        local events = events_get("telegram.post", request.message)
        if #events == 0 then
            return l10n_custom_formatted_line_request(request, lines, "no_messages", {})
        end

        local event = events[1]
        local post_time = "N/A"
        if event.timestamp ~= 0 then
            post_time = time_humanize(time_current() - event.timestamp)
        end

        return l10n_custom_formatted_line_request(request, lines, "message",
            { request.message, event.title, post_time, event.link })
    end,
}
