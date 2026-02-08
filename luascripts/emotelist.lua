local lines = {
    english = {
        ["no_provider"] = "{sender.alias_name}: Emote provider must be specified!",
        ["user_not_found"] = "{sender.alias_name}: Channel %s does not exist!",
        ["emotes_not_found"] = "{sender.alias_name}: Channel %s does not have any %s emotes!"
    },
    russian = {
        ["no_provider"] = "{sender.alias_name}: Провайдер эмоутов должен быть указан!",
        ["user_not_found"] = "{sender.alias_name}: Канал %s не существует!",
        ["emotes_not_found"] = "{sender.alias_name}: Канал %s нет каких-либо %s эмоутов!"
    },
}

return {
    name = "emotelist",
    summary = "Get channel emote list.",
    description = [[
Get channel emote list.

# Supported providers

+ Twitch (`twitch`)
+ FrankerFaceZ (`ffz`)
+ BetterTTV (`bttv`)
+ 7TV (`7tv`)
+ TinyEmotes (`tinyemotes`)

# Syntax

`!emotelist [provider] [channel_name]`

+ `[provider]` - Emote provider.
+ `[channel_name]` - Twitch channel name from which emotes will be fetched. The default value is the current channel name.

# Usage

+ `!emotelist twitch`
+ `!emotelist bttv forsen`

# Responses

+ `[a lot of emotes...]`
]],
    delay_sec = 5,
    options = {},
    subcommands = { "7tv", "bttv", "ffz", "twitch", "tinyemotes" },
    aliases = { "elist" },
    minimal_rights = "user",
    handle = function(request)
        if request.subcommand_id == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_provider", {})
        end

        local cfg = bot_config()

        local providers = { "7tv", "bttv", "ffz", "twitch" }
        if cfg.url.tinyemotes ~= nil then
            table.insert(providers, "tinyemotes")
        end

        local provider = nil
        for i = 1, #providers, 1 do
            if providers[i] == request.subcommand_id then
                provider = request.subcommand_id
            end
        end

        if provider == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_provider", {})
        end

        -- searching for user id
        local id = request.channel.alias_id
        local name = request.channel.alias_name
        if request.message ~= nil and #request.message > 0 then
            local users = twitch_get_users({ logins = { request.message } })
            if #users == 0 then
                return l10n_custom_formatted_line_request(request, lines, "user_not_found", { request.message })
            end

            local user = users[1]
            name = user.login
            id = tonumber(user.id)
        end

        local emotes = {}

        if provider == "7tv" then
            local response = net_get("https://7tv.io/v3/users/twitch/" .. id)
            if response.code == 200 then
                local json = json_parse(response.text)
                if json.emote_set ~= nil and json.emote_set.emotes ~= nil and #json.emote_set.emotes > 0 then
                    for i = 1, #json.emote_set.emotes, 1 do
                        table.insert(emotes, json.emote_set.emotes[i].name)
                    end
                end
            end
        elseif provider == "bttv" then
            local response = net_get("https://api.betterttv.net/3/cached/users/twitch/" .. id)
            if response.code == 200 then
                local json = json_parse(response.text)

                if json.channelEmotes ~= nil and #json.channelEmotes > 0 then
                    for i = 1, #json.channelEmotes, 1 do
                        table.insert(emotes, json.channelEmotes[i].code)
                    end
                end

                if json.sharedEmotes ~= nil and #json.sharedEmotes > 0 then
                    for i = 1, #json.sharedEmotes, 1 do
                        table.insert(emotes, json.sharedEmotes[i].code)
                    end
                end
            end
        elseif provider == "ffz" then
            local response = net_get("https://api.frankerfacez.com/v1/room/id/" .. id)
            if response.code == 200 then
                local json = json_parse(response.text)
                if json.sets ~= nil and next(json.sets) ~= nil then
                    for _, set in pairs(json.sets) do
                        for i = 1, #set.emoticons, 1 do
                            table.insert(emotes, set.emoticons[i].name)
                        end
                    end
                end
            end
        elseif provider == "twitch" then
            local emotes_temp = twitch_get_channel_emotes(id)
            for i = 1, #emotes_temp, 1 do
                table.insert(emotes, emotes_temp[i].name)
            end
        elseif provider == "tinyemotes" then
            local response = net_get_with_headers(
                cfg.url.tinyemotes .. "/users.php?alias_id=" .. id,
                { Accept = "application/json" })

            if response.code == 200 then
                local json = json_parse(response.text)

                if json.data ~= nil and json.data.active_emote_set_id ~= nil and json.data.emote_sets ~= nil then
                    for i = 1, #json.data.emote_sets, 1 do
                        local emote_set = json.data.emote_sets[i]
                        if emote_set.id == json.data.active_emote_set_id then
                            for j = 1, #emote_set.emotes, 1 do
                                table.insert(emotes, emote_set.emotes[j].code)
                            end
                        end
                    end
                end
            end
        end

        if #emotes == 0 then
            return l10n_custom_formatted_line_request(request, lines, "emotes_not_found", { name, provider })
        end

        return str_make_parts("", emotes, "", " ", 420)
    end
}
