local lines = {
    english = {
        ["not_configured"] = "{sender.alias_name}: This command is not set up properly. Try again later.",
        ["not_found"] = "{sender.alias_name}: Account %s not found.",
        ["no_value"] = "{sender.alias_name}: Valid Twitter account must be specified.",
        ["message"] = "{sender.alias_name}: %s's last post: %s (posted %s ago)",
        ["no_messages"] = "{sender.alias_name}: This Twitter account does not have any posts."
    },
    russian = {
        ["not_configured"] = "{sender.alias_name}: Команда не настроена. Попробуйте позже.",
        ["not_found"] = "{sender.alias_name}: Аккаунт %s не найден.",
        ["no_value"] = "{sender.alias_name}: Нужно указать Twitter аккаунт.",
        ["message"] = "{sender.alias_name}: Последний пост %s: %s (опубликовано %s назад)",
        ["no_messages"] = "{sender.alias_name}: Этот Twitter аккаунт не содержит каких-либо постов."
    },
}

return {
    name = "twitter",
    description = [[
Get the latest post from the specified Twitter account.

## Syntax

`!twitter [username]`

+ `[username]` - Valid Twitter username.

## Usage

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
        if cfg.url.rssbridge == nil then
            return l10n_custom_formatted_line_request(request, lines, "not_configured", {})
        end

        if request.message == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_value", {})
        end

        local url = str_format(cfg.url.rssbridge, { "FarsideNitterBridge", request.message })
        local channel = rss_get(url)
        if channel == nil then
            return l10n_custom_formatted_line_request(request, lines, "not_found", { request.message })
        end

        local posts = channel.messages
        if #posts == 0 then
            return l10n_custom_formatted_line_request(request, lines, "no_messages", {})
        end

        local latest_post = posts[1]
        local post_time = "N/A"
        if latest_post.timestamp ~= 0 then
            post_time = time_humanize(time_current() - latest_post.timestamp)
        end

        return l10n_custom_formatted_line_request(request, lines, "message",
            { request.message, latest_post.title, post_time })
    end,
}
