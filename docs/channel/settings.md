# Settings

> This command is for broadcaster only.


The `!set` command gives broadcasters ability to customize the bot as they need it to be more fitted for chat.


## Available features

+ `notify_7tv_updates` - Enable notifications for changes to the channel's 7TV emote set.

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

+ `Successfully enabled the "notify_7tv_updates" feature for this chat!`
+ `Successfully disabled the "notify_7tv_updates" feature for this chat!`

## Error handling

+ [Not enough arguments](/wiki/errors#0)
+ [Incorrect argument](/wiki/errors#2)
+ [Not found](/wiki/errors#12)
+ [Something went wrong](/wiki/errors#127)
