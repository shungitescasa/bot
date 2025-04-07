return {
    name = "massping",
    delay_sec = 5,
    options = {},
    subcommands = {},
    minimal_rights = "moderator",
    handle = function(request)
        chatters = twitch_get_chatters()

        m = ""

        if request.message ~= nil then
            m = request.message .. " ·"
        end

        base = "📣 " .. m .. " "
        userlines = { "" }
        index = 1

        max_line_length = 500

        for i = 1, #chatters, 1 do
            chatter = chatters[i]
            curmsg = userlines[index]
            x = "@" .. chatter.login

            if #base + #curmsg + 1 + #x >= max_line_length then
                index = index + 1
            end

            if index > #userlines then
                table.insert(userlines, x)
            else
                userlines[index] = curmsg .. " " .. x
            end
        end

        msgs = {}

        for i = 1, #userlines, 1 do
            table.insert(msgs, base .. userlines[i])
        end

        return msgs
    end
}
