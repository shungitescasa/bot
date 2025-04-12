local lines = {
    english = {
        ["no_message"] = "{sender.alias_name}: No IP or hostname provided.",
        ["not_found"] = "{sender.alias_name}: %s not found.",
        ["external_api_error"] = "{sender.alias_name}: External API error. Try again later. (%s)",
        ["success"] = "{sender.alias_name}: %s %s (%s) | %s | %s | %s"
    },
    russian = {
        ["no_message"] = "{sender.alias_name}: IP или хост должен быть предоставлен.",
        ["not_found"] = "{sender.alias_name}: %s не найден.",
        ["external_api_error"] = "{sender.alias_name}: Ошибка стороннего API. Попробуйте позже. (%s)",
        ["success"] = "{sender.alias_name}: %s %s (%s) | %s | %s | %s"
    },
}

return {
    name = "mcsrv",
    description = [[
The `!mcsrv` command allows you to quickly find out the status of Minecraft server.
This is a handy command that solves the problem of
logging into Minecraft and waiting for 20 seconds to load to check the server.

## Syntax
`!mcsrv [address]`

+ `[address]` - IP address or name address of the server.

## Usage

+ `!mcsrv mc.hypixel.net`
+ `!mcsrv 12.255.56.21`

## Responses

+ `✅ hypixel.net (209.222.114.115) | 36911/200000 | Hypixel Network [1.8-1.20]; HOLIDAYS EVENT | TRIPLE COINS AND EXP | 1.8.9`
+ `⛔ 12.255.56.21 (127.0.0.1)`

## The meanings of the parts of the message *(separated by |)*

+ Alphabetic and numeric IP addresses.
+ The number of people playing at the moment and the maximum number of players.
+ The MOTD of the server. Separated by **;** *(semicolon)*.
+ Server version.


## Important notes

+ The server status is taken from the third-party API ["mcsrvstat.us"](https://mcsrvstat.us).
]],
    delay_sec = 10,
    options = {},
    subcommands = {},
    aliases = {},
    minimal_rights = "user",
    handle = function(request)
        if request.message == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_message", {})
        end

        local response = net_get("https://api.mcsrvstat.us/3/" .. request.message)

        if response.code == 404 then
            return l10n_custom_formatted_line_request(request, lines, "not_found", { request.message })
        end

        if response.code ~= 200 then
            return l10n_custom_formatted_line_request(request, lines, "external_api_error", { response.code })
        end

        local j = json_parse(response.text)

        local online = "⛔"
        if j.online ~= nil and j.online then
            online = "✅"
        end

        local ip = "IP N/A"
        if j.ip ~= nil then
            ip = j.ip
        end

        local players = "PLAYERS N/A"
        if j.players ~= nil and j.players.online ~= nil and j.players.max ~= nil then
            players = j.players.online .. '/' .. j.players.max
        end

        local version = "VERSION N/A"
        if j.protocol ~= nil and j.protocol.name ~= nil then
            version = j.protocol.name
        end

        local motd = "MOTD N/A"
        if j.motd ~= nil and j.motd.clean ~= nil then
            motd = ""
            for i = 1, #j.motd.clean, 1 do
                motd = motd .. j.motd.clean[i]
                if i + 1 < #j.motd.clean then
                    motd = motd .. " / "
                end
            end
            motd = '"' .. motd .. '"'
        end

        return l10n_custom_formatted_line_request(request, lines, "success", {
            online, request.message, ip, players, version, motd
        })
    end
}
