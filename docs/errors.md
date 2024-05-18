# Errors

## Not enough arguments

This error occurs when a command is called with insufficient parameters or arguments.


For example, if a command requires a username and a message, but only one of them is provided.


> You can always get help on the required command arguments via `!help <command_name>`


## Incorrect argument

This error means that an argument passed to a command is invalid.


For example, if a command expects an argument in a certain format *(e.g., 25.02, forsen:live)* but does not receive it in the required format.

## Insufficient rights

This error typically occurs when a user attempts to perform an action for which they do not have the necessary permissions or privileges. 
For instance, if a chatter tries to execute a moderator-only command.


Also, if the bot doesn't have any permissions, such as not being granted moderator rights, this error will also happen! 
For example, a bot without moderator rights will send this error when calling the `!massping` command.

## Incompatible name

This error indicates that the name being used is incompatible with the command. **Not used at the moment.**


For example, attempt to create a custom command with the name of a built-in command (e.g. `!ping`).

## Namesake creation

This error suggests that the name being attempted to be used is already in use.

## Not found

This error occurs when the requested resource *(e.g. timer, command, user)* cannot be found.

## External API error

This error indicates a problem with an external API that the bot relies on.


For example, if the bot is trying to fetch data from Stats API and encounters an issue with the connection or the API itself.

## Something went wrong

This is a generic error code used to indicate that an unexpected error occurred for which there isn't a specific error code defined. It serves as a catch-all for various unexpected errors.
