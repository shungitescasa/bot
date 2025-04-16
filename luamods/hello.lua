return {
    name = "hello",
    delay_sec = 5,
    options = {},
    aliases = {},
    subcommands = {},
    minimal_rights = "user",
    handle = function(request)
        return "hello, " .. request.sender.alias_name .. "!"
    end
}
