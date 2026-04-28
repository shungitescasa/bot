local lines = {
    english = {
        ["command_unavailable"] = "{sender.alias_name}: This command is not available.",
        ["external_api_error"] = "{sender.alias_name}: Failed to get rankings. Try again later. (%s)",
        ["success"] = "{sender.alias_name}: %s",
    },
    russian = {
        ["command_unavailable"] = "{sender.alias_name}: Эта команда недоступна.",
        ["external_api_error"] = "{sender.alias_name}: Не удалось получить список. Попробуйте позже. (%s)",
        ["success"] = "{sender.alias_name}: %s",
    },
}

return {
    name = "mogranks",
    summary = "Get the rankings of the most influential chads in the Looksmaxxing verse.",
    description = [[
Get the Looksmaxxing ranking of the most influential chads in the world.

# Syntax

`!mogranks`

# Important notes

+ The information is stored and retrieved from the [Official Chad Rankings](https://officialchadrankings.com/).
]],
    delay_sec = 5,
    options = {},
    subcommands = {},
    aliases = { "mogchart", "chadranks", "moggers", "chads", "looksmaxxers" },
    minimal_rights = "user",
    handle = function(request)
        local cfg = bot_config()
        if cfg == nil then
            return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
        end

        if cfg.url.mogchart == nil then
            return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
        end

        local response = net_get_with_headers(cfg.url.mogchart, { Accept = "application/json" })

        if response.code ~= 200 then
            return l10n_custom_formatted_line_request(request, lines, "external_api_error", { response.code })
        end

        local list = json_parse(response.text)
        local rankings = {}

        for i = 1, #list, 1 do
            local mogger = list[i]
            if mogger.name ~= nil then
                local place = ""

                if i == 1 then
                    place = "🥇"
                elseif i == 2 then
                    place = "🥈"
                elseif i == 3 then
                    place = "🥉"
                else
                    place = tostring(i) .. "."
                end

                local str = place .. " " .. mogger.name

                if mogger.title ~= nil then
                    str = str .. " (" .. mogger.title .. ")"
                end

                if mogger.current_rank ~= nil and mogger.previous_rank ~= nil then
                    local rank = mogger.previous_rank - mogger.current_rank
                    if rank > 0 then
                        rank = "+" .. rank
                    end

                    if rank ~= 0 then
                        str = str .. " [" .. rank .. "]"
                    end
                end

                table.insert(rankings, str)
            end
        end

        return l10n_custom_formatted_line_request(request, lines, "success", { table.concat(rankings, ", ") })
    end,
}
