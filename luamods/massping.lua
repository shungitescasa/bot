return {
    name = "massping",
    description = "Ping em! :MegaLUL:",
    delay_sec = 5,
    options = {},
    subcommands = {},
    aliases = {},
    minimal_rights = "moderator",
    handle = function(request)
        local chatters = twitch_get_chatters()

        local m = ""

        if request.message ~= nil then
            m = request.message .. " ·"
        end

        local base = "📣 " .. m .. " "

        local names = {}
        for i = 1, #chatters, 1 do
            table.insert(names, chatters[i].login)
        end

        return str_make_parts(base, names, "@", " ", 500)
    end
}
