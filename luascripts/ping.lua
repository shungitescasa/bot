return {
    name = "ping",
    summary = "Check if the bot is alive.",
    description =
    "The `!ping` command checks to see if it's alive and gives a bunch of other data like memory usage, compiler version, etc.",
    delay_sec = 5,
    options = {},
    aliases = {},
    subcommands = {},
    minimal_rights = "user",
    handle = function(request)
        local temperature = math.ceil(bot_get_temperature())
        local temperature_str = ""
        if temperature > 0 then
            temperature_str = " · Temperature: " .. tostring(temperature) .. "°C"
        end
        return request.sender.alias_name .. ": PotFriend Pong! " ..
            "Uptime: " .. time_humanize(bot_get_uptime()) ..
            " · Used memory: " .. math.ceil(bot_get_memory_usage() / 1024) ..
            "MB" .. temperature_str .. " · Bot running on " .. bot_get_version() ..
            " (Last updated " .. time_humanize(time_current() - bot_get_compile_time()) .. " ago)"
    end
}
