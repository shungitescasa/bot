local lines = {
    english = {
        ["no_message"] = "{sender.alias_name}: No username provided.",
        ["not_found"] = "{sender.alias_name}: %s not found.",
        ["external_api_error"] = "{sender.alias_name}: External API error. Try again later. (%s)",
        ["success"] = "{sender.alias_name}: %s %s (%s)%s"
    },
    russian = {
        ["no_message"] = "{sender.alias_name}: Имя пользователя должен быть предоставлен.",
        ["not_found"] = "{sender.alias_name}: %s не найден.",
        ["external_api_error"] = "{sender.alias_name}: Ошибка стороннего API. Попробуйте позже. (%s)",
        ["success"] = "{sender.alias_name}: %s %s (%s)%s"
    },
}

return {
    name = "userid",
    description = [[
The `!userid` command allows you to check if the specified users
exist, or if they are banned, or if they are OK.

## Syntax

`!userid [users...]`

+ `[users...]` - User ID or user names. Separated by **,** *(colon)*.

## Usage

+ `!userid drdisrespect`
+ `!userid 22484632`
+ `!userid drdisrespect,22484632,okayeg`

## Responses

+ `⛔ drdisrespect (17337557): TOS_INDEFINITE`
+ `✅ forsen (22484632)`
+ `✅ okayeg (489147225)`

## Important notes

+ User information is taken from the third-party API service ["ivr.fi"](https://api.ivr.fi/v2/docs)
]],
    delay_sec = 2,
    options = {},
    subcommands = {},
    aliases = { "uid", "banned", "isbanned", "isban", "bancheck" },
    minimal_rights = "user",
    handle = function(request)
        if request.message == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_message", {})
        end

        local parts = str_split(request.message, ' ')

        local ids = {}
        local logins = {}

        for i = 1, #parts, 1 do
            if #ids + #logins >= 3 then
                break
            end

            local part = parts[i]
            local id = tonumber(part)

            if id == nil then
                table.insert(logins, part)
            else
                table.insert(ids, id)
            end
        end

        local query = ""

        if #ids > 0 then
            query = "id=" .. table.concat(ids, ',')
        end

        if #logins > 0 then
            if #ids > 0 then
                query = query .. "&"
            end
            query = query .. "login=" .. table.concat(logins, ',')
        end

        if #query == 0 then
            return l10n_custom_formatted_line_request(request, lines, "no_message", {})
        end

        local response = net_get("https://api.ivr.fi/v2/twitch/user?" .. query)

        if response.code == 404 then
            return l10n_custom_formatted_line_request(request, lines, "not_found", { request.message })
        end

        if response.code ~= 200 then
            return l10n_custom_formatted_line_request(request, lines, "external_api_error", { response.code })
        end

        local j = json_parse(response.text)

        local msgs = {}

        for i = 1, #j, 1 do
            local u = j[i]

            local name = u.login
            local id = u.id

            local is_banned = "✅"
            if u.banned then
                is_banned = "⛔"
            end

            local ban_reason = ""
            if u.banReason ~= nil then
                ban_reason = ": " .. u.banReason
            end

            table.insert(msgs, l10n_custom_formatted_line_request(request, lines, "success", {
                is_banned, name, id, ban_reason
            }))
        end

        return msgs
    end
}
