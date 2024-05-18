# User ban check

The `!userid` command allows you to check if the specified users
exist, or if they are banned, or if they are OK.

## Syntax

`!userid [users...]`

+ `[users...]` - User ID or user names. Separated by **,** *(colon)*.

## Usage

+ `!userid drdisrespect`
+ `!userid 22484632`
+ `!userid drdisrespect,22484632,okayeg`

## Responses

+ `⛔ drdisrespect (17337557): TOS_INDEFINITE`
+ `✅ forsen (22484632)`
+ `✅ okayeg (489147225)`

## Important notes

+ User information is taken from the third-party API service ["ivr.fi"](https://api.ivr.fi/v2/docs)

## Error handling

+ [Not enough arguments](/wiki/errors#0)
+ [Not found](/wiki/errors#12)
+ [Something went wrong](/wiki/errors#127)
