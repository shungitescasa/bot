# Timers

> This command is for broadcaster only


The `!timer` command gives broadcasters the ability to create timers that sends messages to the chat room every specified interval.


## Syntax

### Create a new timer
`!timer new [name] [interval] [message...]`

+ `[name]` - The name for new timer. It should be unique for your chat.
+ `[interval]` - Message sending interval *(in seconds)*.
+ `[message]` - Text that will be sent after the interval has passed.

### Delete the timer
`!timer delete [name]`

+ `[name]` - The name of the timer.

### Edit the message for the timer
`!timer message [name] [message...]`

+ `[name]` - The name of the timer.
+ `[message]` - Text with which to replace.

### Edit the interval for the timer
`!timer interval [name] [interval]`

+ `[name]` - The name of the timer.
+ `[interval]` - An interval *(in seconds)* with which to replace.

### Toggle (enable/disable) the timer
`!timer toggle [name]`

+ `[name]` - The name of the timer.

### Check the information about the timer
`!timer info [name]`

+ `[name]` - The name of the timer.

### Call the timer
`!timer call [name]`

+ `[name]` - The name of the timer.

### Get the list of created timers
`!timer list`

## Usage

### Creating a new timer
+ `!timer new sub_ads 120 Buy a Twitch sub and be like all of us`

### Deleting the timer
+ `!timer delete sub_ads`

### Editing the message for the timer
+ `!timer message sub_ads Buy a Prime sub and be like all of us `

### Editing the interval for the timer
+ `!timer interval sub_ads 180`

### Toggling the timer
+ `!timer toggle sub_ads`

### Checking the information about the timer
+ `!timer info sub_ads`

### Calling the timer
+ `!timer call sub_ads`


## Responses

### Creating a new timer
+ `A new timer with "sub_ads" name has been successfully created!`

### Deleting the timer
+ `The "sub_ads" (ID ...) timer has been deleted!`

### Editing the message for the timer
+ `The message for "sub_ads" (ID ...) timer has been changed!`

### Editing the interval for the timer
+ `The interval for "sub_ads" (ID ...) timer has been changed!`

### Toggling the timer
+ If the timer was enabled

`The "sub_ads" (ID ...) timer has been disabled!` 

+ If the timer was disabled

`The "sub_ads" (ID ...) timer has been enabled!`

### Checking the information about the timer
+ `✅ sub_ads (ID ...) | Interval: 120s | Message: Buy a Twitch sub and be like all of us`

### Calling the timer
+ `Buy a Twitch sub and be like all of us` 

## Error handling

+ [Not enough arguments](/wiki/errors#0)
+ [Incorrect argument](/wiki/errors#2)
+ [Namesake creation](/wiki/errors#11)
+ [Not found](/wiki/errors#12)
+ [Something went wrong](/wiki/errors#127)
