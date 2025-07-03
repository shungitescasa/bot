local lines = {
	english = {
		["no_subcommand"] = "{sender.alias_name}: No subcommand provided. Use '{channel.prefix}help cmd' for more info.",
		["no_message"] = "{sender.alias_name}: No message provided.",
		["not_enough_rights"] = "{sender.alias_name}: You do not have enough rights to perform this command.",
		["command_list"] = "{sender.alias_name}: %s",
		["namesake"] = "{sender.alias_name}: A command with the same name already exists.",
		["new_cmd"] = "{sender.alias_name}: Created a new command! Use %s to run it.",
		["not_found"] = "{sender.alias_name}: Command %s does not exist.",
		["delete_command"] = "{sender.alias_name}: Successfully deleted %s command.",
		["edit_command"] = "{sender.alias_name}: Edited the message for %s command.",
		["no_new_name"] = "{sender.alias_name}: No new name provided.",
		["rename_command"] = "{sender.alias_name}: Renamed command from %s to %s",
		["no_alias"] = "{sender.alias_name}: No alias provided.",
		["namesake_alias"] = "{sender.alias_name}: Same alias already exists for %s command.",
		["alias_command"] = "{sender.alias_name}: Successfully created alias %s for %s command.",
		["no_cmd_alias"] = "{sender.alias_name}: There is no command with alias %s",
		["delalias_command"] = "{sender.alias_name}: Successfully removed alias %s from %s command.",
		["setglobal_command"] = "{sender.alias_name}: Command %s is now available in all chats.",
		["delglobal_command"] = "{sender.alias_name}: Command %s is now available only in this chat.",
		["view_command"] = "{sender.alias_name}: ID %s | %s | Aliases: %s | %s",
	},
	russian = {
		["no_subcommand"] =
		"{sender.alias_name}: Нет подкоманды. Используйте '{channel.prefix}help cmd', чтобы получить больше информации.",
		["no_message"] = "{sender.alias_name}: Сообщение не предоставлено.",
		["not_enough_rights"] = "{sender.alias_name}: У вас недостаточно прав для выполнения этой команды.",
		["command_list"] = "{sender.alias_name}: %s",
		["namesake"] = "{sender.alias_name}: Команда с таким же именем уже существует.",
		["new_cmd"] = "{sender.alias_name}: Создана новая команда! Используйте %s для её запуска.",
		["not_found"] = "{sender.alias_name}: Команда %s не существует.",
		["delete_command"] = "{sender.alias_name}: Команда %s успешно удалена.",
		["edit_command"] = "{sender.alias_name}: Изменил сообщение для команды %s",
		["no_new_name"] = "{sender.alias_name}: Новое имя для команды не предоставлено.",
		["rename_command"] = "{sender.alias_name}: Переименовал команду %s в %s",
		["no_alias"] = "{sender.alias_name}: Алиас не предоставлен.",
		["namesake_alias"] = "{sender.alias_name}: Такой же алиас уже существует для команды %s",
		["alias_command"] = "{sender.alias_name}: Успешно привязал алиас %s к команде %s",
		["no_cmd_alias"] = "{sender.alias_name}: Нет команд с алиасом %s",
		["delalias_command"] = "{sender.alias_name}: Успешно удалил алиас %s от команды %s",
		["setglobal_command"] = "{sender.alias_name}: Команда %s доступна во всех чатах.",
		["delglobal_command"] = "{sender.alias_name}: Команда %s доступна только в этом чате.",
		["view_command"] = "{sender.alias_name}: ID %s | %s | Алиасы: %s | %s",
	},
}

return {
	name = "cmd",
	description = [[
> This command is for broadcaster and moderators only


The `!cmd` command gives the ability to create their own chat commands.


## Syntax


### Create a new custom command

`!cmd new [name] [message...]`

+ `[name]` - The name for new custom command. It should be unique for your chat.
**A prefix must be specified if you want a prefixed command, e.g. `!sub`, `!server`.**
+ `[message]` - Text that will be sent when the custom command is invoked.

### Delete the custom command

`!cmd delete [name]`

+ `[name]` - Name of custom command.

### Edit the message for custom command
`!cmd edit [name] [message...]`

+ `[name]` - Name of custom command.
+ `[message]` - Text with which to replace

### Edit the custom command name

`!cmd rename [name] [new_name]`

+ `[name]` - Name of custom command.
+ `[new_name]` - New name for custom command.

### Create a new alias for custom command

`!cmd alias [name] [alias]`

+ `[name]` - Name of custom command.
+ `[alias]` - New alias for custom command.

### Delete alias from custom command

`!cmd delalias [name] [alias]`

+ `[name]` - Name of custom command.
+ `[alias]` - Alias of custom command.

### Check the information about custom command
`!cmd view [name]`

+ `[name]` - Name of custom command

### Get the list of created custom commands
`!cmd list`

## Usage

### Creating a new custom command
+ `!cmd new !sub Buy a Twitch sub at this link and become like the rest of us 😎`

### Deleting the custom command
+ `!cmd delete !sub`

### Editing the message for custom command
+ `!cmd edit !sub Buy a Prime sub at this link and become like the rest of us 😎`

### Renaming the custom command
+ `!cmd rename !sub buysub`

### Creating a new alias for custom command
+ `!cmd alias !sub buy_subscription`

### Deleting the alias for custom command
+ `!cmd delalias !sub subscription`

### Checking the information about the custom command
+ `!cmd view !sub`
]],
	delay_sec = 1,
	options = {},
	subcommands = { "new", "delete", "edit", "rename", "alias", "delalias", "view", "list", "setglobal" },
	aliases = { "scmd" },
	minimal_rights = "moderator",
	handle = function(request)
		if request.subcommand_id == nil then
			return l10n_custom_formatted_line_request(request, lines, "no_subcommand", {})
		end

		local scid = request.subcommand_id

		if scid == "list" then
			local cmds = db_query('SELECT name FROM custom_commands WHERE channel_id = $1', { request.channel.id })
			local names = {}
			for i = 1, #cmds, 1 do
				table.insert(names, cmds[i].name)
			end

			return l10n_custom_formatted_line_request(request, lines, "command_list", { table.concat(names, ', ') })
		end

		if request.message == nil then
			return l10n_custom_formatted_line_request(request, lines, "no_message", {})
		end

		local parts = str_split(request.message, ' ')

		local name = parts[1]
		table.remove(parts, 1)

		local cmds = db_query(
			'SELECT id, name, message, is_global FROM custom_commands WHERE name = $1 AND channel_id = $2',
			{ name, request.channel.id })

		if scid == "new" then
			local internal_commands = bot_get_loaded_command_names()

			if #cmds > 0 or array_contains(internal_commands, name) then
				return l10n_custom_formatted_line_request(request, lines, "namesake", {})
			end

			if #parts == 0 then
				return l10n_custom_formatted_line_request(request, lines, "no_message", {})
			end

			local message = table.concat(parts, ' ')

			db_execute('INSERT INTO custom_commands(channel_id, name, message) VALUES ($1, $2, $3)',
				{ request.channel.id, name, message })

			return l10n_custom_formatted_line_request(request, lines, "new_cmd", { name })
		end

		if #cmds == 0 then
			return l10n_custom_formatted_line_request(request, lines, "not_found", { name })
		end

		local cmd = cmds[1]

		if scid == "delete" then
			db_execute('DELETE FROM custom_commands WHERE id = $1', { cmd.id })
			return l10n_custom_formatted_line_request(request, lines, "delete_command", { name })
		elseif scid == "edit" then
			if #parts == 0 then
				return l10n_custom_formatted_line_request(request, lines, "no_message", {})
			end

			local message = table.concat(parts, ' ')

			db_execute('UPDATE custom_commands SET message = $1 WHERE id = $2', { message, cmd.id })

			return l10n_custom_formatted_line_request(request, lines, "edit_command", { name })
		elseif scid == "rename" then
			if #parts == 0 then
				return l10n_custom_formatted_line_request(request, lines, "no_new_name", {})
			end

			local new_name = parts[1]

			local new_cmds = db_query('SELECT id FROM custom_commands WHERE name = $1', { new_name })
			if #new_cmds > 0 then
				return l10n_custom_formatted_line_request(request, lines, "namesake", {})
			end

			local internal_commands = bot_get_loaded_command_names()
			if array_contains(internal_commands, new_name) then
				return l10n_custom_formatted_line_request(request, lines, "namesake", {})
			end

			db_execute('UPDATE custom_commands SET name = $1 WHERE id = $2', { new_name, cmd.id })

			return l10n_custom_formatted_line_request(request, lines, "rename_command", { name, new_name })
		elseif scid == "alias" then
			if #parts == 0 then
				return l10n_custom_formatted_line_request(request, lines, "no_alias", {})
			end

			local new_alias = parts[1]

			local cmd_alias = db_query('SELECT name FROM custom_command_aliases WHERE name = $1 AND command_id = $2',
				{ new_alias, cmd.id })

			if #cmd_alias > 0 then
				return l10n_custom_formatted_line_request(request, lines, "namesake_alias", { new_alias })
			end

			local internal_commands = bot_get_loaded_command_names()
			if array_contains(internal_commands, new_alias) then
				return l10n_custom_formatted_line_request(request, lines, "namesake", {})
			end

			db_execute('INSERT INTO custom_command_aliases(name, command_id) VALUES ($1, $2)',
				{ new_alias, cmd.id })

			return l10n_custom_formatted_line_request(request, lines, "alias_command", { new_alias, name })
		elseif scid == "delalias" then
			if #parts == 0 then
				return l10n_custom_formatted_line_request(request, lines, "no_alias", {})
			end

			local old_alias = parts[1]

			local cmd_alias = db_query('SELECT id FROM custom_command_aliases WHERE name = $1 AND command_id = $2',
				{ old_alias, cmd.id })

			if #cmd_alias == 0 then
				return l10n_custom_formatted_line_request(request, lines, "no_cmd_alias", { old_alias })
			end

			db_execute('DELETE FROM custom_command_aliases WHERE id = $1',
				{ cmd_alias[1].id })

			return l10n_custom_formatted_line_request(request, lines, "delalias_command", { old_alias, name })
		elseif scid == "setglobal" then
			local cfg = bot_config()
			if cfg == nil or cfg.owner == nil or request.sender.alias_id ~= cfg.owner.id then
				return l10n_custom_formatted_line_request(request, lines, "not_enough_rights", {})
			end

			local line_id = ""
			local query = ""
			if cmd.is_global == "1" then
				line_id = "delglobal_command"
				query = "UPDATE custom_commands SET is_global = FALSE WHERE id = $1"
			else
				line_id = "setglobal_command"
				query = "UPDATE custom_commands SET is_global = TRUE WHERE id = $1"
			end

			db_execute(query, { cmd.id })

			return l10n_custom_formatted_line_request(request, lines, line_id, { name })
		elseif scid == "view" then
			local aliases_db = db_query('SELECT name FROM custom_command_aliases WHERE command_id = $1',
				{ cmd.id })

			local aliases = {}

			for i = 1, #aliases_db, 1 do
				table.insert(aliases, aliases_db[i].name)
			end

			local aliases_str = table.concat(aliases, ', ')

			if #aliases_str == 0 then
				aliases_str = "-"
			end

			return l10n_custom_formatted_line_request(request, lines, "view_command",
				{ cmd.id, cmd.name, aliases_str, cmd.message })
		end
	end,
}
