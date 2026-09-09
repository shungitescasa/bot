local lines = {
    english = {
        ["not_configured"] = "{sender.alias_name}: This command is not set up properly. Try again later.",
        ["not_found"] = "{sender.alias_name}: Account %s not found.",
        ["no_value"] = "{sender.alias_name}: Valid Twitter account must be specified.",
        ["message"] = "{sender.alias_name}: %s's last post: %s (posted %s ago) (%s)",
        ["no_messages"] = "{sender.alias_name}: This Twitter account does not have any posts."
    },
    russian = {
        ["not_configured"] = "{sender.alias_name}: Команда не настроена. Попробуйте позже.",
        ["not_found"] = "{sender.alias_name}: Аккаунт %s не найден.",
        ["no_value"] = "{sender.alias_name}: Нужно указать Twitter аккаунт.",
        ["message"] = "{sender.alias_name}: Последний пост %s: %s (опубликовано %s назад) (%s)",
        ["no_messages"] = "{sender.alias_name}: Этот Twitter аккаунт не содержит каких-либо постов."
    },
}

return {
    name = "twitter",
    summary = "Get the latest post from Twitter account.",
    description = [[
Get the latest post from the specified Twitter account.

# Syntax

`!twitter [username]`

+ `[username]` - Valid Twitter username.

# Usage

+ `!twitter forsen`
+ `!x twitch`
]],
    delay_sec = 5,
    options = {},
    subcommands = {},
    aliases = { "xitter", "x", "lt" },
    minimal_rights = "user",
    handle = function(request)
        local cfg = bot_config()
        if cfg.rss.url == nil then
            return l10n_custom_formatted_line_request(request, lines, "not_configured", {})
        end

        if request.message == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_value", {})
        end

        local events = events_get("twitter.post", request.message)
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
