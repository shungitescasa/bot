local lines = {
    english = {
        ["external_api_error"] = "{sender.alias_name}: Failed to get followlist for %s. Try again later. (%s)",
        ["success"] = "{sender.alias_name}: %s",
    },
    russian = {
        ["external_api_error"] = "{sender.alias_name}: Не удалось получить фолловлист %s. Попробуйте позже. (%s)",
        ["success"] = "{sender.alias_name}: %s",
    },
}

return {
    name = "followlist",
    summary = "Get user's followlist.",
    description = [[
Read the user's follows as plain text.
After collecting the list of chatters, the bot returns a link to the paste from
the Pastebin-like service.

# Syntax

`!followlist <username>`

+ `<username>` - Twitch username *(optional)*.
]],
    delay_sec = 5,
    options = {},
    subcommands = {},
    aliases = { "flist", "follows" },
    minimal_rights = "user",
    handle = function(request)
        local username = request.sender.name

        if request.message ~= nil then
            username = request.message
        end

        local response =
            net_get_with_headers("https://tools.alright.party/" .. username .. "/follows",
                { Accept = "application/json" })

        if response.code ~= 200 then
            return l10n_custom_formatted_line_request(request, lines, "external_api_error", { username, response.code })
        end

        local followingList = json_parse(response.text)

        local body = followingList.totalCount .. " channels\r\n---------------------\r\n\r\n"

        for i = 1, #followingList.follows, 1 do
            local follow = followingList.follows[i]
            body = body .. follow.login .. "\r\n"
        end

        if #followingList.follows == 0 then
            body = body ..
                "It appears that Twitch restricted the request and did not send a list of channels the user follows."
        end

        local time = time_format(time_current(), "%d.%m.%Y %H:%M:%S %z")

        local link = paste_upload(body, username .. "'s followlist on " .. time)

        return l10n_custom_formatted_line_request(request, lines, "success", { link })
    end,
}
