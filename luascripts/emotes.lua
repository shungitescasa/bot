local lines = {
    english = {
        ["no_subcommand"] = "{sender.alias_name}: No subcommand",
        ["no_message"] = "{sender.alias_name}: Emote URL or name must be provided.",
        ["emote_not_found"] = "{sender.alias_name}: %s Emote %s not found!",
        ["user_not_found"] = "{sender.alias_name}: %s User %s not found!",
        ["no_new_name"] = "{sender.alias_name}: New emote name must be provided!",
        ["unsupported_provider"] = "{sender.alias_name}: Unsupported emote provider (%s)",
    },
    russian = {
        ["no_subcommand"] = "{sender.alias_name}: Нет подкоманды.",
        ["no_message"] = "{sender.alias_name}: Ссылка или название эмоута должно быть указано.",
        ["emote_not_found"] = "{sender.alias_name}: %s эмоут %s не найден!",
        ["user_not_found"] = "{sender.alias_name}: %s пользователь %s не найден!",
        ["no_new_name"] = "{sender.alias_name}: Новое имя для эмоута должно быть указано!",
        ["unsupported_provider"] = "{sender.alias_name}: Неподдерживаемый провайдер эмоутов (%s)",
    }
}

return {
    name = "emotes",
    summary = "Edit your 7TV emoteset",
    description = [[
> To make any changes, the bot must be an editor in 7TV.

The `!emotes` command gives the ability to manage 7TV emoteset.

# Syntax

## 7TV

### Add emote

`!7tv add [name] [alias]`

+ `[name]` - Valid 7TV emote name.
+ `[alias]` *(optional)* - 7TV alias name.

### Remove emote

`!7tv remove [name]`

+ `[name]` - Valid 7TV emote name.

### Rename emote

`!7tv rename [name] [new name]`

+ `[name]` - Valid 7TV emote name.
+ `[new name]` - New 7TV emote name.

## Universal

### Add emote

`!emotes add [url] [alias]`

+ `[url]` - Valid emote URL.
+ `[alias]` *(optional)* - alias name.

### Remove emote

`!emotes remove [url]`

+ `[url]` - Valid emote URL.

### Rename emote

`!emotes rename [url] [new name]`

+ `[name]` - Valid emote URL.
+ `[new name]` - New emote name.

# Usage

## Adding an emote

+ `!emotes add https://7tv.app/emotes/01GQFT1WF80002Q9KS8SKQMHHY`
+ `!emotes add https://7tv.app/emotes/01FEEEBABG000FR0PC38FM26HW MUGA`
+ `!7tv add buh`
+ `!7tv add Weather $weather`

## Removing the emote

+ `!emotes remove https://7tv.app/emotes/01GQFT1WF80002Q9KS8SKQMHHY`
+ `!7tv remove buh`

## Renaming the emote

+ `!emotes rename https://7tv.app/emotes/01GQFT1WF80002Q9KS8SKQMHHY buhOriginal`
+ `!7tv rename buh buhOriginal`
]],
    delay_sec = 2,
    options = {},
    subcommands = { "add", "delete", "remove", "rename", "edit" },
    aliases = { "emote", "7tv" },
    minimal_rights = "moderator",
    handle = function(request)
        local scmd = request.subcommand_id
        if scmd == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_subcommand", {})
        end

        local text = request.message
        if text == nil and request.reply ~= nil then
            text = request.reply.message
        end

        if text == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_message", {})
        end

        local parts = str_split(text, " ")

        local emote_id = nil
        local user_id = nil
        local provider = nil

        if request.command_id == "7tv" then
            provider = "7tv"
        end

        local link = parts[1]
        local matches = { ["https://7tv.app/emotes/"] = "7tv", ["7tv.app/emotes/"] = "7tv" }

        for url, p in pairs(matches) do
            if str_startswith(link, url) then
                if provider == nil then
                    provider = p
                end
                emote_id = string.sub(link, string.len(url) + 1, string.len(link))
                break
            end
        end

        if provider == "7tv" then
            local user = stv_get_user(request.channel.alias_id)
            if user == nil then
                return l10n_custom_formatted_line_request(request, lines, "user_not_found",
                    { "7TV", request.channel.alias_name })
            end
            user_id = user.emote_set_id

            if scmd == "add" then
                if emote_id == nil then
                    local emotes = stv_search_emotes(parts[1])
                    if #emotes == 0 then
                        return l10n_custom_formatted_line_request(request, lines, "emote_not_found", { "7TV", parts[1] })
                    end

                    local emote = emotes[1]
                    emote_id = emote.id
                end

                if #parts == 1 then
                    stv_add_emote(user_id, emote_id)
                else
                    stv_add_named_emote(user_id, emote_id, parts[2])
                end

                return nil
            end

            if emote_id == nil then
                local emoteset = stv_get_emoteset(user_id)
                if emoteset == nil then
                    return l10n_custom_formatted_line_request(request, lines, "user_not_found", { "7TV", user_id })
                end

                for i = 1, #emoteset.emotes, 1 do
                    if emoteset.emotes[i].code == parts[1] then
                        emote_id = emoteset.emotes[i].id
                        break
                    end
                end

                if emote_id == nil then
                    return l10n_custom_formatted_line_request(request, lines, "emote_not_found", { "7TV", parts[1] })
                end
            end

            if scmd == "delete" or scmd == "remove" then
                stv_remove_emote(user_id, emote_id)
            elseif scmd == "edit" or scmd == "rename" then
                if #parts == 1 then
                    return l10n_custom_formatted_line_request(request, lines, "no_new_name", {})
                end

                stv_rename_emote(user_id, emote_id, parts[2])
            end
        elseif provider == nil then
            return l10n_custom_formatted_line_request(request, lines, "unsupported_provider", { text })
        end

        return nil
    end
}
