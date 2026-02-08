local lines = {
    english = {
        ["emote_not_found"] = "{sender.alias_name}: Failed to find any emotes. Try again later!"
    },
    russian = {
        ["emote_not_found"] = "{sender.alias_name}: Не получилось найти какой-либо эмоут. Попробуйте позже!"
    },
}

return {
    name = "randomemote",
    summary = "Get a random emote.",
    description = [[
Get a random emote.

# Supported providers

+ Twitch
+ FrankerFaceZ
+ BetterTTV
+ 7TV
+ TinyEmotes
]],
    delay_sec = 1,
    options = {},
    subcommands = {},
    aliases = { "erand" },
    minimal_rights = "user",
    handle = function(request)
        local cfg = bot_config()

        local providers = { "7tv", "bttv", "ffz", "twitch" }
        if cfg.url.tinyemotes ~= nil then
            table.insert(providers, "tinyemotes")
        end

        local emote = nil

        while emote == nil do
            if #providers == 0 then
                break
            end

            local provider_i = math.random(1, #providers)
            local provider = providers[provider_i]
            local emotes = {}

            if provider == "7tv" then
                -- local emotes
                local response = net_get("https://7tv.io/v3/users/twitch/" .. tostring(request.channel.alias_id))
                if response.code == 200 then
                    local json = json_parse(response.text)
                    if json.emote_set ~= nil and json.emote_set.emotes ~= nil and #json.emote_set.emotes > 0 then
                        for i = 1, #json.emote_set.emotes, 1 do
                            table.insert(emotes, json.emote_set.emotes[i].name)
                        end
                    end
                end

                -- global emotes
                response = net_get("https://7tv.io/v3/emote-sets/global")
                if response.code == 200 then
                    local json = json_parse(response.text)
                    if json.emotes ~= nil and #json.emotes > 0 then
                        for i = 1, #json.emotes, 1 do
                            table.insert(emotes, json.emotes[i].name)
                        end
                    end
                end
            elseif provider == "bttv" then
                -- local emotes
                local response = net_get("https://api.betterttv.net/3/cached/users/twitch/" ..
                    tostring(request.channel.alias_id))
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

                -- global emotes
                response = net_get("https://api.betterttv.net/3/cached/emotes/global")
                if response.code == 200 then
                    local json = json_parse(response.text)

                    if #json > 0 then
                        for i = 1, #json, 1 do
                            table.insert(emotes, json[i].code)
                        end
                    end
                end
            elseif provider == "ffz" then
                -- local emotes
                local response = net_get("https://api.frankerfacez.com/v1/room/id/" ..
                    tostring(request.channel.alias_id))
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

                -- global emotes
                response = net_get("https://api.frankerfacez.com/v1/set/global")
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
                -- local emotes
                local emotes_temp = twitch_get_channel_emotes(request.channel.alias_id)
                for i = 1, #emotes_temp, 1 do
                    table.insert(emotes, emotes_temp[i].name)
                end

                -- global emotes
                emotes_temp = twitch_get_global_emotes()
                for i = 1, #emotes_temp, 1 do
                    table.insert(emotes, emotes_temp[i].name)
                end
            elseif provider == "tinyemotes" then
                -- local emotes
                local response = net_get_with_headers(
                    cfg.url.tinyemotes .. "/users.php?alias_id=" .. request.channel.alias_id,
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

                -- global emotes
                response = net_get_with_headers(
                    cfg.url.tinyemotes .. "/emotesets.php?id=global",
                    { Accept = "application/json" })

                if response.code == 200 then
                    local json = json_parse(response.text)

                    if json.data ~= nil and json.data.emotes ~= nil then
                        for i = 1, #json.data.emotes, 1 do
                            table.insert(emotes, json.data.emotes[i].code)
                        end
                    end
                end
            end

            ::skip::
            if #emotes > 0 then
                emote = emotes[math.random(1, #emotes)]
            end

            if emote == nil then
                table.remove(providers, provider_i)
            end
        end

        if emote == nil then
            return l10n_custom_formatted_line_request(request, lines, "emote_not_found", {})
        end

        return emote
    end
}
