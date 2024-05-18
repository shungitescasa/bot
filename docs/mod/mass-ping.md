# Mass ping


> To use the `!massping` command, you must assign moderator to the bot.
> Following the Twitch API docs, only moderators have access to full chatter list.


The `!massping` command gives the ability to mass mentioning (mass ping) chatters.
This feature allows you to quickly and efficiently draw the attention of a large number of users to a certain message.

## Syntax
`!massping [message...]`

+ `[message...]` (optional) - A text message that will be sent along with a mention of all chatters.

## Usage

+ `!massping forsen`
+ `!massping good morning everyone!! have a nice day :3`
+ `!massping`

## Responses

+ `@chatter1, @chatter2, @chatter3, ..., forsen`
+ `@chatter1, @chatter2, @chatter3, ..., good morning everyone!! have a nice day :3`
+ `@chatter1, @chatter2, @chatter3, ...,`

## Error handling

+ [Insufficient rights](/wiki/errors#3)
