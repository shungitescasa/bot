local lines = {
    english = {
        ["no_provider"] = "{sender.alias_name}: Emote provider must be specified!",
        ["emotes_not_found"] = "{sender.alias_name}: This channel does not have any %s emotes!"
    },
    russian = {
        ["no_provider"] = "{sender.alias_name}: Провайдер эмоутов должен быть указан!",
        ["emotes_not_found"] = "{sender.alias_name}: На этом канале нет каких-либо %s эмоутов!"
    },
}

return {
    name = "globalemotelist",
    summary = "Get global emote list.",
    description = [[
Get global emote list.

# Supported providers

+ Twitch (`twitch`)
+ FrankerFaceZ (`ffz`)
+ BetterTTV (`bttv`)
+ 7TV (`7tv`)
+ TinyEmotes (`tinyemotes`)

# Syntax

`!globalemotelist [provider]`

+ `[provider]` - Emote provider.

# Usage

+ `!globalemotelist twitch`
+ `!globalemotelist ffz`
+ `!globalemotelist bttv`
+ `!globalemotelist 7tv`
+ `!globalemotelist tinyemotes`

# Responses

+ `[a lot of emotes...]`
]],
    delay_sec = 5,
    options = {},
    subcommands = { "7tv", "bttv", "ffz", "twitch", "tinyemotes" },
    aliases = { "gelist" },
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

        local emotes = {}

        if provider == "7tv" then
            local response = net_get("https://7tv.io/v3/emote-sets/global")
            if response.code == 200 then
                local json = json_parse(response.text)
                if json.emotes ~= nil and #json.emotes > 0 then
                    for i = 1, #json.emotes, 1 do
                        table.insert(emotes, json.emotes[i].name)
                    end
                end
            end
        elseif provider == "bttv" then
            local response = net_get("https://api.betterttv.net/3/cached/emotes/global")
            if response.code == 200 then
                local json = json_parse(response.text)

                if #json > 0 then
                    for i = 1, #json, 1 do
                        table.insert(emotes, json[i].code)
                    end
                end
            end
        elseif provider == "ffz" then
            local response = net_get("https://api.frankerfacez.com/v1/set/global")
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
            local emotes_temp = twitch_get_global_emotes()
            for i = 1, #emotes_temp, 1 do
                table.insert(emotes, emotes_temp[i].name)
            end
        elseif provider == "tinyemotes" then
            local response = net_get_with_headers(
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

        if #emotes == 0 then
            return l10n_custom_formatted_line_request(request, lines, "emotes_not_found", { provider })
        end

        return str_make_parts("", emotes, "", " ", 420)
    end
}
