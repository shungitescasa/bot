# Spam


> It is recommended to give the bot moderator rights in the chat room. This will speed up the sending of messages.


The `!spam` command gives users the ability to repeat a given message a certain number of times in a chat room.
This feature can be useful for highlighting important information.

## Syntax
`!spam [amount] [message...]`

+ `[amount]` (optional) - A number that specified how many times the message should be repeated.\
If not specified, the default value is 5. The maximum value is 30.
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
```

## Error handling
+ [Not enough arguments](/wiki/errors#0)
