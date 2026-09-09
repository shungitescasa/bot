local lines = {
    english = {
        ["success"] = "{sender.alias_name}: %s"
    },
    russian = {
        ["success"] = "{sender.alias_name}: %s"
    },
}

return {
    name = "randompost",
    delay_sec = 5,
    options = {},
    subcommands = {},
    aliases = { "rpost", "rps", "rtnd" },
    minimal_rights = "user",
    handle = function(request)
        return l10n_custom_formatted_line_request(request, lines, "success", { image_random() })
    end,
}
