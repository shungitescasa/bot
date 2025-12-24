local lines = {
    english = {
        ["no_value"] = "{sender.alias_name}: Value must be provided.",
        ["no_subcommand"] =
        "{sender.alias_name}: Subcommand must be provided. Use {channel.prefix}help to get more information.",
        ["locale_not_exists"] = "{sender.alias_name}: Language %s not found",
        ["set_locale"] = "{sender.alias_name}: This bot will speak English in this chat!",
        ["set_prefix"] = "{sender.alias_name}: Prefix \"%s\" has been set for this chat!",
    },
    russian = {
        ["no_value"] = "{sender.alias_name}: Значение требуется для этой команды.",
        ["no_subcommand"] =
        "{sender.alias_name}: Подкоманда требуется для этой команды. Используйте {channel.prefix}help для большей информации.",
        ["locale_not_exists"] = "{sender.alias_name}: Язык %s не найден",
        ["set_locale"] = "{sender.alias_name}: Этот бот будет говорить по-русски!",
        ["set_prefix"] = "{sender.alias_name}: Префикс \"%s\" установлен для этого чата!"
    },
}

return {
    name = "set",
    summary = "Bot settings in your chat.",
    description = [[
> This command is for broadcaster and moderators only.


The `!set` command gives broadcasters ability to customize the bot as they need it to be more fitted for chat.


# Syntax

## Set the bot localization for the chat
`!set locale [lang]`

+ `[lang]` - Language name in English and lowercase.
Available languages at the moment: **english**, **russian**.

## Set the bot prefix
`!set prefix [characters]`

+ `[characters]` - Characters to be set as a prefix.

# Usage

## Setting the bot localization

+ `!set locale russian`
+ `!set locale english`

## Setting the bot prefix

+ `!set prefix ~`
+ `!set prefix ?!`

# Responses

## Setting the bot localization

+ `Успешно установил язык чата на русский!`
+ `Successfully set the chat language to English!`

## Setting the bot prefix

+ `Successfully set the chat prefix to "~"`
+ `Successfully set the chat prefix to "?!"`
]],
    delay_sec = 1,
    options = {},
    subcommands = { "locale", "prefix" },
    aliases = {},
    minimal_rights = "moderator",
    handle = function(request)
        if request.subcommand_id == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_subcommand", {})
        end

        if request.message == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_value", {})
        end

        local parts = str_split(request.message, ' ')

        local value = parts[1]

        if request.subcommand_id == "locale" then
            local locals = l10n_get_localization_names()
            if not array_contains(locals, value) then
                return l10n_custom_formatted_line_request(request, lines, "locale_not_exists", { value })
            end

            db_execute('UPDATE channel_preferences SET locale = $1 WHERE id = $2', { value, request.channel.id })
            request['channel_preference']['language'] = value

            return l10n_custom_formatted_line_request(request, lines, "set_locale", {})
        elseif request.subcommand_id == "prefix" then
            value = value:gsub("&nbsp;", " ")
            db_execute('UPDATE channel_preferences SET prefix = $1 WHERE id = $2', { value, request.channel.id })
            return l10n_custom_formatted_line_request(request, lines, "set_prefix", { value })
        end
    end,
}
