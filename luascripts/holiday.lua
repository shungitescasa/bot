local lines = {
    english = {
        ["no_holidays"] = "{sender.alias_name}: No holidays found on %s.%s.",
        ["external_api_error"] = "{sender.alias_name}: External API error. Try again later. (%s)",
        ["holiday"] = "{sender.alias_name}: HolidayPresent Holiday on %s.%s (%s/%s): %s",
        ["all_holidays"] = "{sender.alias_name}: HolidayPresent %s holidays on %s.%s: %s"
    },
    russian = {
        ["no_holidays"] = "{sender.alias_name}: Никаких праздников на %s.%s.",
        ["external_api_error"] = "{sender.alias_name}: Ошибка API. Попробуйте позже. (%s)",
        ["holiday"] = "{sender.alias_name}: HolidayPresent Праздник на %s.%s (%s/%s): %s",
        ["all_holidays"] = "{sender.alias_name}: HolidayPresent %s праздников на %s.%s: %s"
    },
}

return {
    name = "holiday",
    summary = "Get today's holidays",
    description = [[
The `!holiday` command allows you to get information about today's and upcoming holidays.


> **So far, only Russian holidays and in Russian.**


## Syntax

`!holiday [date]`

+ `[date]` (Optional) - The date on which you want to get a random holiday.
If it's not specified, today's date will be used.

## Usage

+ `!holiday` - Get today's random holiday.
+ `!holiday 25.02` - Get a random holiday on February 25th.
+ `!holiday 08/16` - Get a random holiday on August 16th.
+ `!holiday all` - Get all today's holidays.
+ `!holiday all 02.03` - Get all holidays on March 2nd.

## Important notes

+ The information is obtained from the third-party service ["ilotterytea/holidays"](https://ilt.su/holidays/)
]],
    delay_sec = 1,
    options = {},
    subcommands = { "all" },
    aliases = { "holidays", "праздник", "праздники", "hol" },
    minimal_rights = "user",
    handle = function(request)
        local current_time = time_current()
        local month = time_format(current_time, "%m")
        local day = time_format(current_time, "%d")

        local parts = {}

        if request.message ~= nil then
            parts = str_split(request.message, " ")
        end

        if #parts > 0 then
            local normaltime = false
            local dayparts = str_split(parts[1], "/")
            if #dayparts <= 1 then
                dayparts = str_split(parts[1], ".")
                normaltime = true
            end

            if #dayparts > 1 then
                if normaltime then
                    day = dayparts[1]
                    month = dayparts[2]
                else
                    month = dayparts[1]
                    day = dayparts[2]
                end
                day = tonumber(day)
                month = tonumber(month)
            end
        end

        local response = net_get_with_headers("https://ilt.su/holidays/?month=" .. month .. "&day=" .. day,
            { Accept = "application/json" })

        if response.code ~= 200 then
            return l10n_custom_formatted_line_request(request, lines, "external_api_error", { response.code })
        end

        local holidays = json_parse(response.text)
        local holiday_names = {}

        for i = 1, #holidays, 1 do
            table.insert(holiday_names, holidays[i].name)
        end

        if #holiday_names == 0 then
            return l10n_custom_formatted_line_request(request, lines, "no_holidays",
                { day, month })
        end

        if request.subcommand_id ~= nil and request.subcommand_id == "all" then
            return l10n_custom_formatted_line_request(request, lines, "all_holidays",
                { #holiday_names, day, month, table.concat(holiday_names, ", ") })
        end

        local index = math.random(1, #holiday_names)

        return l10n_custom_formatted_line_request(request, lines, "holiday",
            { day, month, index, #holiday_names, holiday_names[index] })
    end
}
