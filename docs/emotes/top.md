# Emote usage leaderboard

> The only supported emote provider is **7TV**


The `!etop` command provides the ability to find out the top emotes by usage in a specified chat room.
This is a useful tool for those who want to keep track of the popularity and frequency of use of specific emotes in the community.


## Syntax

`!etop [desc/asc] [channel name] [amount]`

+ `[desc/asc]` (optional) - Parameter to control sorting of emotes. 
The default setting is **desc** *(descending)*.
+ `[channel name]` (optional) - Parameter to specify a specific channel. 
By default, the name of the channel from which the command was sent is used.
+ `[amount]` (optional) - Parameter to set the number of emotes in the result list. 
The default setting is 10, the maximum value is 50.

## Usage

+ `!etop` - Shows the top 10 emotes in descending order in the current channel.
+ `!etop asc` - Shows the top 10 emotes in ascending order in the current channel.
+ `!etop forsen` - Shows the top 10 emotes in descending order in forsen's channel.
+ `!etop asc forsen 10` - Shows the top 20 emotes in ascending order in forsen's channel.
+ `!etop asc 20` - Shows the top 20 emotes in ascending order in the current channel.

## Responses

+ `forsen's top 10 emotes (descending): forsenHoppedIn (750), FailFors (600), forsenGravity (599), ...`
+ `forsen's top 10 emotes (ascending): forsenMushroom (1), forsenDank (5), forsenPirate (60), ...`

## Important notes

+ Emote information is stored and retrieved from an external 
["ilotterytea/stats"](https://stats.ilotterytea.kz) service.
+ Emotes data may be temporarily unavailable if a bot has just joined a chat.

## Error handling

+ [Not found](/wiki/errors#12)
+ [External API error](/wiki/errors#20)
+ [Something went wrong](/wiki/errors#127)
