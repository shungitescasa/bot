# Stream events

> This command is for broadcaster only.


The `!event` command gives broadcasters the ability to manage events for streams.


## Event types

+ live
+ offline
+ title
+ category
+ github (Placeholders for messages: `%0` - Commit hash, `%1` - Committer name, `%2` - Commit message)
+ custom

## Event flags

+ `massping` - Massping everyone in chat regardless of their subscription to the event.

## Syntax

### Create a new event

`!event on [name]:[type] [message...]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](#event-types).
+ `[message]` - The message that will be sent with the event.


> Events with types *category* and *title* use *{0}* and *{1}* placeholders
> for old and new values respectively.
> This means that the bot can show changes if you set them
> (e.g. *forsen changed the title from **{0}** to **{1}*** will replace
> with *forsen changed the title from **Just Chatting** to **PUBG***).


### Delete the event

`!event off [name]:[type]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](#event-types).

### Flag/unflag the event

`!event flag [name]:[type] [flag]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](#event-types).
+ `[flag]` - [Event flag](#event-flags).

### Call the event


> The bot requires moderator privileges on events with the **"massping"** flag.


`!event call [name]:[type]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](#event-types).

## Usage

### Creating a new event
+ `!event on forsen:live forsen live!`
+ `!event on ilotterytea/bot:github %1 just made a commit (%0): %2`

### Deleting the event
+ `!event off forsen:live`

### Flag/unflag the event
+ `!event flag forsen:live massping`

### Calling the event
+ `!event call forsen:live`

## Responses

### Creating a new event
+ `A new "forsen:live" event has been successfully created! It will send a message when the event occurs.`

### Deleting the event
+ `The "forsen:live" (ID ...) event has been successfully deleted!`

### Adding the flag to the event
+ `Flag "massping" is set for the "forsen:live" event.`

### Removing the flag from the event
+ `Flag "massping" has been removed from the "forsen:live" event.`

### Calling the event
+ `⚡ forsen live!` 

## Important notes

+ If the specified event name does not belong to a Twitch user,
the event type will automatically be considered ***custom***.

## Error handling

+ [Not enough arguments](/wiki/errors#0)
+ [Incorrect argument](/wiki/errors#2)
+ [Insufficient rights](/wiki/errors#3)
+ [Namesake creation](/wiki/errors#11)
+ [Not found](/wiki/errors#12)
+ [Something went wrong](/wiki/errors#127)
