local lines = {
    english = {
        ["success"] = "{sender.alias_name}: %s",
    },
    russian = {
        ["success"] = "{sender.alias_name}: %s",
    },
}

return {
    name = "chatters",
    summary = "Get a list of chatters.",
    description = [[
> To use the `!chatters` command, you must assign moderator to the bot.
> Following the [Twitch API docs](https://dev.twitch.tv/docs/api/reference/#get-chatters), only moderators have access to full chatter list.

The `!chatters` command allows you to get a list of chatters as plain text.
After collecting the list of chatters, the bot returns a link to the paste from
[the Pastebin-like service](https://tnd.quest).
]],
    delay_sec = 30,
    options = {},
    subcommands = {},
    aliases = { "chatterlist", "clist", "ulist", "userlist" },
    minimal_rights = "user",
    handle = function(request)
        local chatters = twitch_get_chatters(request.channel.alias_id)
        local body = #chatters .. " chatters\r\n---------------------\r\n\r\n"

        for i = 1, #chatters, 1 do
            local chatter = chatters[i]
            body = body .. chatter.login .. "\r\n"
        end

        local time = time_format(time_current(), "%d.%m.%Y %H:%M:%S %z")
        local link = paste_upload(body, request.channel.alias_name .. "'s chatter list on " .. time)

        return l10n_custom_formatted_line_request(request, lines, "success", { link })
    end,
}
