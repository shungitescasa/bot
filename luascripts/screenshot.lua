local lines = {
    english = {
        ["external_api_error"] = "{sender.alias_name}: Failed to screenshot the message. Try again later. (%s)",
        ["success"] = "{sender.alias_name}: %s",
    },
    russian = {
        ["external_api_error"] = "{sender.alias_name}: Не удалось заскриншотить сообщение. Попробуйте позже. (%s)",
        ["success"] = "{sender.alias_name}: %s",
    },
}

return {
    name = "screenshot",
    summary = "Take a screenshot of the message.",
    description = [[
This command takes a screenshot of the message you've replied to.
Click the "Reply" button and type `!screenshot` to execute the command.
The bot then returns a link to the image.
]],
    delay_sec = 2,
    options = {},
    subcommands = {},
    aliases = { "scrn", "ttours", "prntsc", "shot", "caught" },
    minimal_rights = "user",
    handle = function(request)
        if request.reply == nil then
            return nil
        end

        local url = "https://ttours.alright.party/generate?output=base64&message_id=" ..
            request.reply.id .. "&channel_login=" .. request.channel.alias_name

        local response = net_get(url)

        if response.code ~= 200 then
            return l10n_custom_formatted_line_request(request, lines, "external_api_error", { response.code })
        end

        local shot = response.text

        local link = image_upload_base64(shot)

        return l10n_custom_formatted_line_request(request, lines, "success", { link })
    end,
}
