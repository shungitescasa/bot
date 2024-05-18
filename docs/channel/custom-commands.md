# Custom commands

> This command is for broadcaster only


The `!cmd` command gives broadcasters the ability to create their own chat commands.


## Syntax


### Create a new custom command

`!cmd new [name] [message...]`

+ `[name]` - The name for new custom command. It should be unique for your chat. 
A prefix must be specified if you want a prefixed command, e.g. `!sub`, `!server`.
+ `[message]` - Text that will be sent when the custom command is invoked.

### Delete the custom command

`!cmd delete [name]`

+ `[name]` - Name of custom command.

### Edit the message for custom command
`!cmd message [name] [message...]`

+ `[name]` - Name of custom command.
+ `[message]` - Text with which to replace

### Toggle (enable/disable) the custom command
`!cmd toggle [name]`

+ `[name]` - Name of custom command.

### Check the information about custom command
`!cmd info [name]`

+ `[name]` - Name of custom command

### Get the list of created custom commands
`!cmd list`

## Usage

### Creating a new custom command
+ `!cmd new !sub Buy a Twitch sub at this link and become like the rest of us 😎`

### Deleting the custom command
+ `!cmd delete !sub`

### Editing the message for custom command
+ `!cmd message !sub Buy a Prime sub at this link and become like the rest of us 😎`

### Toggling the custom command
+ `!cmd toggle !sub`

### Checking the information about the custom command
+ `!cmd info !sub`

## Responses

### Creating a new custom command
+ `A new custom command with "!sub" name has been successfully created!`

### Deleting the custom command
+ `The "!sub" (ID ...) custom command has been deleted!`

### Editing the message for custom command
+ `The message for "!sub" (ID ...) custom command has been changed!`

### Toggling the custom command
+ If the command was enabled

`The "!sub" (ID ...) custom command has been disabled!`

+ If the command was disabled

`The "!sub" (ID ...) custom command has been enabled!`

### Checking the information about the custom command
+ `✅ !sub (ID ...) | Message: Buy a Prime sub at this link and become like the rest of us 😎`

## Error handling

+ [Not enough arguments](/wiki/errors#0)
+ [Namesake creation](/wiki/errors#11)
+ [Not found](/wiki/errors#12)
+ [Something went wrong](/wiki/errors#127)
