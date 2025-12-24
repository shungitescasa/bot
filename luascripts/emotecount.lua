local lines = {
    english = {
        ["not_configured"] = "{sender.alias_name}: This command is not ready!",
        ["not_found"] = "{sender.alias_name}: Emote %s not found or has not been used yet.",
        ["no_message"] = "{sender.alias_name}: Emote must be provided.",
        ["external_api_error"] = "{sender.alias_name}: External API error. Try again later. (%s)",
        ["channel_not_found"] = "{sender.alias_name}: Channel %s does not exist for API. Try again later.",
        ["success"] = "{sender.alias_name}: (%s) %s has been used %s times."
    },
    russian = {
        ["not_configured"] = "{sender.alias_name}: Эта команда не настроена!",
        ["not_found"] = "{sender.alias_name}: Эмоут %s не найден или ещё не был использован.",
        ["no_message"] = "{sender.alias_name}: Эмоут должен быть предоставлен.",
        ["external_api_error"] = "{sender.alias_name}: Ошибка API. Попробуйте позже. (%s)",
        ["channel_not_found"] = "{sender.alias_name}: Канал %s не существует для API. Попробуйте позже.",
        ["success"] = "{sender.alias_name}: (%s) %s был использован %s раз."
    },
}

return {
    name = "emotecount",
    summary = "Track the usage count of emote.",
    description = [[
The `!emotecount` command is designed to track the number of times an emote has been used in a chat.

# Syntax

`!emotecount [name]`

+ `[name]` - The name of the emote.

# Usage

+ `!emotecount forsenHoppedIn`

# Important notes

+ Emotes data may be temporarily unavailable if the bot has just joined a chat.
+ Emote information is stored and retrieved from [the external API](https://stats.ilt.su).
]],
    delay_sec = 1,
    options = {},
    subcommands = {},
    aliases = { "ecount" },
    minimal_rights = "user",
    handle = function(request)
        local cfg = bot_config()

        if cfg.url.stats == nil then
            return l10n_custom_formatted_line_request(request, lines, "not_configured", {})
        end

        if request.message == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_message", {})
        end

        local response = net_get_with_headers(cfg.url.stats .. "/channels/?alias_id=" .. request.channel.alias_id,
            { Accept = "application/json" })

        if response.code == 404 then
            return l10n_custom_formatted_line_request(request, lines, "channel_not_found", { request.channel.alias_name })
        elseif response.code ~= 200 then
            return l10n_custom_formatted_line_request(request, lines, "external_api_error", { response.code })
        end

        local json = json_parse(response.text)

        local emote_name = request.message:gsub("%s+", "")
        local emote_providers = json.data.stats.emotes
        local emote_provider = nil
        local emote_count = -1

        for provider_name, emotes in pairs(emote_providers) do
            for i = 1, #emotes, 1 do
                local e = emotes[i]
                if e.code == emote_name and e.usage_count > emote_count then
                    emote_count = e.usage_count
                    emote_provider = e.provider_name_short
                end
            end
        end

        if emote_provider == nil then
            return l10n_custom_formatted_line_request(request, lines, "not_found", { request.message })
        end

        return l10n_custom_formatted_line_request(request, lines, "success",
            { emote_provider, request.message, emote_count })
    end
}
