return {
    name = "hello",
    delay_sec = 5,
    options = {},
    aliases = {},
    subcommands = {},
    minimal_rights = "user",
    handle = function(request)
        irc_send_message({ login = "#ilotterytea" }, "test")
        return "hello, " .. request.sender.alias_name .. "!"
    end
}
