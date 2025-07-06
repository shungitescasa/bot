local lines = {
    english = {
        ["no_message"] = "{sender.alias_name}: No username provided.",
        ["not_found"] = "{sender.alias_name}: %s not found.",
        ["external_api_error"] = "{sender.alias_name}: External API error. Try again later. (%s)",
        ["success"] = "{sender.alias_name}: %s's profile picture: %s"
    },
    russian = {
        ["no_message"] = "{sender.alias_name}: Имя пользователя должен быть предоставлен.",
        ["not_found"] = "{sender.alias_name}: %s не найден.",
        ["external_api_error"] = "{sender.alias_name}: Ошибка стороннего API. Попробуйте позже. (%s)",
        ["success"] = "{sender.alias_name}: Аватарка пользователя %s: %s"
    },
}

return {
    name = "profilepicture",
    description = [[
Get user profile pictures.

## Syntax

`!pfp [users...]`

+ `[users...]` - User ID or user names. Separated by space.

## Usage

+ `!pfp drdisrespect`
+ `!pfp 22484632`
+ `!pfp drdisrespect 22484632 okayeg`

## Responses

+ `drdisrespect's profile picture: https://static-cdn.jtvnw.net/jtv_user_pictures/72a69c72-14b9-4be8-b8cb-802bc3e5f8ed-profile_image-600x600.png`

## Important notes

+ User information is taken from the third-party API service ["ivr.fi"](https://api.ivr.fi/v2/docs)
]],
    delay_sec = 2,
    options = {},
    subcommands = {},
    aliases = { "pfp", "avatar" },
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

        if #j == 0 then
            return l10n_custom_formatted_line_request(request, lines, "not_found", { request.message })
        end

        local msgs = {}

        for i = 1, #j, 1 do
            local u = j[i]
            local url = u.logo
            local name = u.login
            table.insert(msgs, l10n_custom_formatted_line_request(request, lines, "success", { name, url }))
        end

        return msgs
    end
}
