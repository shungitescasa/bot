local lines = {
    english = {
        ["no_value"] = "{sender.alias_name}: Value must be provided.",
        ["no_subcommand"] =
        "{sender.alias_name}: Subcommand must be provided. Use {channel.prefix}help to get more information.",
        ["locale_not_exists"] = "{sender.alias_name}: Language %s not found",
        ["set_locale"] = "{sender.alias_name}: This bot will speak English in this chat!",
        ["set_prefix"] = "{sender.alias_name}: Prefix \"%s\" has been set for this chat!",
        ["no_feature"] = "{sender.alias_name}: Feature %s not found",
        ["feature_disabled"] = "{sender.alias_name}: Feature %s has been disabled",
        ["feature_enabled"] = "{sender.alias_name}: Feature %s has been enabled"
    },
    russian = {
        ["no_value"] = "{sender.alias_name}: Значение требуется для этой команды.",
        ["no_subcommand"] =
        "{sender.alias_name}: Подкоманда требуется для этой команды. Используйте {channel.prefix}help для большей информации.",
        ["locale_not_exists"] = "{sender.alias_name}: Язык %s не найден",
        ["set_locale"] = "{sender.alias_name}: Этот бот будет говорить по-русски!",
        ["set_prefix"] = "{sender.alias_name}: Префикс \"%s\" установлен для этого чата!",
        ["no_feature"] = "{sender.alias_name}: Функция %s не найдена",
        ["feature_disabled"] = "{sender.alias_name}: Функция %s теперь выключена",
        ["feature_enabled"] = "{sender.alias_name}: Функция %s теперь включена"
    },
}

return {
    name = "set",
    description = [[
> This command is for broadcaster and moderators only.


The `!set` command gives broadcasters ability to customize the bot as they need it to be more fitted for chat.


## Available features

+ `markov_responses` - Enable Markov-generated responses *(triggered by "@teabot, " prefix)*
+ `random_markov_responses` - Enable Markov-generated responses on random messages. It is required that the feature `markov_responses` is enabled.

## Syntax

### Set the bot localization for the chat
`!set locale [lang]`

+ `[lang]` - Language name in English and lowercase.
Available languages at the moment: **english**, **russian**.

### Set the bot prefix
`!set prefix [characters]`

+ `[characters]` - Characters to be set as a prefix.

### Enable/disable the bot feature for the chat
`!set feature [feature]`

+ `[feature]` - [Available features](#available-features)

## Usage

### Setting the bot localization

+ `!set locale russian`
+ `!set locale english`

### Setting the bot prefix

+ `!set prefix ~`
+ `!set prefix ?!`

### Enabling/disabling the bot feature

+ `!set feature notify_7tv_updates`

## Responses

### Setting the bot localization

+ `Успешно установил язык чата на русский!`
+ `Successfully set the chat language to English!`

### Setting the bot prefix

+ `Successfully set the chat prefix to "~"`
+ `Successfully set the chat prefix to "?!"`

### Enabling/disabling the bot feature

+ `Successfully enabled the "markov_responses" feature for this chat!`
+ `Successfully disabled the "random_markov_responses" feature for this chat!`
]],
    delay_sec = 1,
    options = {},
    subcommands = { "locale", "prefix", "feature" },
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
        elseif request.subcommand_id == "feature" then
            local feature = str_to_feature(value)
            if feature == nil then
                return l10n_custom_formatted_line_request(request, lines, "no_feature", { value })
            end

            local channel_features = request.channel_preference.features

            local line_id = ""
            local query = ""

            if array_contains(channel_features, value) then
                line_id = "feature_disabled"
                query = 'UPDATE channel_preferences SET ' .. feature_to_str(feature) .. ' = 0 WHERE id = $1'
            else
                line_id = "feature_enabled"
                query = 'UPDATE channel_preferences SET ' .. feature_to_str(feature) .. ' = 1 WHERE id = $1'
            end

            db_execute(query, { request.channel.id })

            return l10n_custom_formatted_line_request(request, lines, line_id, { value })
        end
    end,
}
