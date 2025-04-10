local lines = {
    english = {
        ["no_message"] = "{sender.alias_name}: Message must be provided.",
    },
    russian = {
        ["no_message"] = "{sender.alias_name}: Сообщение должно быть предоставлено.",
    },
}

return {
    name = "spam",
    description = [[
> It is recommended to give the bot moderator rights in the chat room. This will speed up the sending of messages.

The `!spam` command gives users the ability to repeat a given message a certain number of times in a chat room.
This feature can be useful for highlighting important information.

## Syntax
`!spam [amount] [message...]`

+ `[amount]` (optional) - A number that specified how many times the message should be repeated.
+ `[message...]` - The text of the message to be repeated.

## Usage

+ `!spam forsen`
+ `!spam 100 forsen forsen forsen`

## Responses

```
forsen forsen forsen
forsen forsen forsen
forsen forsen forsen
forsen forsen forsen
forsen forsen forsen
forsen forsen forsen
forsen forsen forsen
forsen forsen forsen
forsen forsen forsen
forsen forsen forsen
...
]],
    delay_sec = 10,
    options = {},
    subcommands = {},
    aliases = {},
    minimal_rights = "moderator",
    handle = function(request)
        if request.message == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_message", {})
        end

        local parts = str_split(request.message, " ")

        local max_count = 20
        local count = tonumber(parts[1])

        if count == nil then
            count = 5
        elseif count > max_count then
            count = max_count
            table.remove(parts, 1)
        else
            table.remove(parts, 1)
        end

        local msg = table.concat(parts, ' ')
        local o = {}

        for i = 1, count, 1 do
            table.insert(o, msg)
        end

        return o
    end
}
