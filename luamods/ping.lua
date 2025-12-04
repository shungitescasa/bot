return {
    name = "ping",
    description =
    "The `!ping` command checks to see if it's alive and gives a bunch of other data like memory usage, compiler version, etc.",
    delay_sec = 5,
    options = {},
    aliases = {},
    subcommands = {},
    minimal_rights = "user",
    handle = function(request)
        return request.sender.alias_name .. ": PotFriend Pong! " ..
            "Uptime: " .. time_humanize(bot_get_uptime()) ..
            " · Used memory: " .. math.ceil(bot_get_memory_usage() / 1024) ..
            "MB · Temperature: " .. tostring(math.ceil(bot_get_temperature())) .. "°C · Bot running on " .. bot_get_version() ..
            " (Last updated " .. time_humanize(time_current() - bot_get_compile_time()) .. " ago)"
    end
}
