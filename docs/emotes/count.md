# Check the usage of emote

> The only supported emote provider is **7TV**


The `!ecount` command is designed to track the number of times an emote has been used in a chat.
This feature allows users to find out how many times a certain emote has been used in messages.
To use the command, you must specify the name of the emote after the command.

## Syntax

`!ecount [name]`

+ `[name]` - The name of the emote.

## Usage

+ `!ecount forsenHoppedIn`

## Responses

+ `forsenHoppedIn has been used 1337 times`

## Important notes

+ Emote information is stored and retrieved from an external 
["ilotterytea/stats"](https://stats.ilotterytea.kz) service.
+ Emotes data may be temporarily unavailable if a bot has just joined a chat.

## Error handling

+ [Not enough arguments](/wiki/errors#0)
+ [External API error](/wiki/errors#20)
