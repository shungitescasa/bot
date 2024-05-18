# Stream notifications

The `!notify` command gives users the ability to manage event subscriptions.

## Syntax

### Subscribe to the event
`!notify sub [name]:[type]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](/wiki/stream/events#event-types).

### Unsubscribe from the event
`!notify unsub [name]:[type]`

+ `[name]` - Twitch username or event name *(custom type only)*.
+ `[type]` - [Event type](/wiki/stream/events#event-types).

### Get your event subscriptions
`!notify subs`

### Get available events to subscribe
`!notify list`

## Usage

### Subscribing to the event

+ `!notify sub forsen:live`

### Unsubscribing from the event

+ `!notify unsub forsen:live`

## Responses

### Subscribing to the event

+ If you're not a subscriber

`You have successfully subscribed to the "forsen:live" event!`

+ If you're already a subscriber

`You're already a subscriber to the "forsen:live" event.`

### Unsubscribing from the event

+ If you're not a subscriber

`You're not subscribed to the "forsen:live" event.`

+ If you're a subscriber

`You have successfully unsubscribed from the "forsen:live" event!`

### Getting event subscriptions

+ If you're subscribed to at least one event

`Your subscriptions: forsen:live, xqc:offline, nymn:title, ...`

+ Otherwise,

`You're not subscribed to any events.`

## Important notes

+ If the specified event name does not belong to a Twitch user, 
the event type will automatically be considered ***custom***.

## Error handling

+ [Not enough arguments](/wiki/errors#0)
+ [Incorrect argument](/wiki/errors#2)
+ [Not found](/wiki/errors#12)
+ [Something went wrong](/wiki/errors#127)
