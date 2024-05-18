# Emote set similarity check


> The only supported emote provider is **7TV**


The `!esim` command provides the ability to compare two emote sets and determine the similarity between them.
This is a useful tool for analyzing the similarity of emotes in different chat rooms.

## Syntax

`!esim [origin channel] [target channel]`

+ `[target channel]` - parameter representing the target channel to be compared.
+ `[origin channel]` (optional) - parameter representing the origin channel to be compared. 
If not specified, the channel from which the command was sent is used.

## Usage

+ `!esim forsen`
+ `!esim forsen xqc`

## Responses
+ `...'s emote set is 95% similar to forsen's emote set (570 of 600).`
+ `forsen's emote set is not similar to xqc's emote set.`

## Error handling

+ [Not enough arguments](/wiki/errors#0)
+ [Not found](/wiki/errors#12)
