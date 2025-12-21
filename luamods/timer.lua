local lines = {
	english = {
		["no_subcommand"] =
		"{sender.alias_name}: No subcommand provided. Use '{channel.prefix}help timer' for more info.",
		["no_message"] = "{sender.alias_name}: No message provided.",
		["list"] = "{sender.alias_name}: %s",
		["no_list"] = "{sender.alias_name}: There are no timers in this channel.",
		["namesake"] = "{sender.alias_name}: A timer with the same name already exists.",
		["new"] = "{sender.alias_name}: Created a new timer %s! First execution will be in %s.",
		["not_found"] = "{sender.alias_name}: Timer %s does not exist.",
		["delete"] = "{sender.alias_name}: Successfully deleted timer %s",
		["edit"] = "{sender.alias_name}: Edited the message for timer %s",
		["no_new_name"] = "{sender.alias_name}: No new name provided.",
		["rename"] = "{sender.alias_name}: Renamed timer from %s to %s",
		["no_interval"] = "{sender.alias_name}: No interval (in seconds) provided.",
		["bad_interval"] = "{sender.alias_name}: Interval (in seconds) must be a number. (%s)",
		["interval"] = "{sender.alias_name}: Changed an interval for timer %s. Next execution will be in %s.",
		["view"] = "{sender.alias_name}: ID %s | %s | Runs every %s | %s",
	},
	russian = {
		["no_subcommand"] =
		"{sender.alias_name}: Нет подкоманды. Используйте '{channel.prefix}help cmd', чтобы получить больше информации.",
		["no_message"] = "{sender.alias_name}: Сообщение не предоставлено.",
		["list"] = "{sender.alias_name}: %s",
		["no_list"] = "{sender.alias_name}: На этом канале нет таймеров.",
		["namesake"] = "{sender.alias_name}: Таймер с таким же именем уже существует.",
		["new"] = "{sender.alias_name}: Успешно создан новый таймер %s! Первый запуск будет через %s.",
		["not_found"] = "{sender.alias_name}: Таймер %s не существует.",
		["delete"] = "{sender.alias_name}: Успешно удален таймер %s",
		["edit"] = "{sender.alias_name}: Сообщение для таймера %s было успешно отредактировано.",
		["no_new_name"] = "{sender.alias_name}: Новое имя не было предоставлено.",
		["rename"] = "{sender.alias_name}: Таймер %s был переименован в %s",
		["no_interval"] = "{sender.alias_name}: Интервал (в секундах) не предоставлен.",
		["bad_interval"] = "{sender.alias_name}: Интервал (в секундах) должен быть числом. (%s)",
		["interval"] = "{sender.alias_name}: Изменил интервал для таймера %s. Следующий запуск будет через %s.",
		["view"] = "{sender.alias_name}: ID %s | %s | Каждые %s | %s",
	},
}

return {
	name = "timer",
	summary = "Send message every specified interval.",
	description = [[
> This command is for broadcaster and moderators only


The `!timer` command gives the ability to create timers that sends messages to the chat room every specified interval.


## Syntax

### Create a new timer
`!timer new [name] [interval] [message...]`

+ `[name]` - The name for new timer. It should be unique for your chat.
+ `[interval]` - Message sending interval *(in seconds)*.
+ `[message]` - Text that will be sent after the interval has passed.

### Delete the timer
`!timer delete [name]` OR `!timer remove [name]`

+ `[name]` - The name of the timer.

### Edit the message for the timer
`!timer edit [name] [message...]`

+ `[name]` - The name of the timer.
+ `[message]` - Text with which to replace.

### Edit the interval for the timer
`!timer interval [name] [interval]`

+ `[name]` - The name of the timer.
+ `[interval]` - An interval *(in seconds)* with which to replace.

### Check the information about the timer
`!timer view [name]`

+ `[name]` - The name of the timer.

### Call the timer
`!timer call [name]`

+ `[name]` - The name of the timer.

### Get the list of created timers
`!timer list`
]],
	delay_sec = 1,
	options = {},
	subcommands = { "new", "delete", "remove", "interval", "edit", "rename", "list", "view", "call" },
	aliases = {},
	minimal_rights = "moderator",
	handle = function(request)
		if request.subcommand_id == nil then
			return l10n_custom_formatted_line_request(request, lines, "no_subcommand", {})
		end

		local scid = request.subcommand_id

		if scid == "list" then
			local cmds = db_query('SELECT name FROM timers WHERE channel_id = $1', { request.channel.id })
			local names = {}
			for i = 1, #cmds, 1 do
				table.insert(names, cmds[i].name)
			end

			if #names == 0 then
				return l10n_custom_formatted_line_request(request, lines, "no_list", {})
			end

			return l10n_custom_formatted_line_request(request, lines, "list", { table.concat(names, ', ') })
		end

		if request.message == nil then
			return l10n_custom_formatted_line_request(request, lines, "no_message", {})
		end

		local parts = str_split(request.message, ' ')

		local name = parts[1]
		table.remove(parts, 1)

		local timers = db_query(
			'SELECT id, name, interval_sec, message FROM timers WHERE name = $1 AND channel_id = $2',
			{ name, request.channel.id })

		if scid == "new" then
			if #timers > 0 then
				return l10n_custom_formatted_line_request(request, lines, "namesake", {})
			end

			if #parts == 0 then
				return l10n_custom_formatted_line_request(request, lines, "no_interval", {})
			end

			local interval_sec = tonumber(parts[1])
			if interval_sec == nil then
				return l10n_custom_formatted_line_request(request, lines, "bad_interval", { parts[1] })
			end
			table.remove(parts, 1)

			if #parts == 0 then
				return l10n_custom_formatted_line_request(request, lines, "no_message", {})
			end

			local message = table.concat(parts, ' ')

			db_execute('INSERT INTO timers(channel_id, name, interval_sec, message) VALUES ($1, $2, $3, $4)',
				{ request.channel.id, name, interval_sec, message })

			return l10n_custom_formatted_line_request(request, lines, "new", { name, time_humanize(interval_sec) })
		end

		if #timers == 0 then
			return l10n_custom_formatted_line_request(request, lines, "not_found", { name })
		end

		local timer = timers[1]

		if scid == "delete" or scid == "remove" then
			db_execute('DELETE FROM timers WHERE id = $1', { timer.id })
			return l10n_custom_formatted_line_request(request, lines, "delete", { name })
		elseif scid == "edit" then
			if #parts == 0 then
				return l10n_custom_formatted_line_request(request, lines, "no_message", {})
			end

			local message = table.concat(parts, ' ')

			db_execute('UPDATE timers SET message = $1 WHERE id = $2', { message, timer.id })

			return l10n_custom_formatted_line_request(request, lines, "edit", { name })
		elseif scid == "rename" then
			if #parts == 0 then
				return l10n_custom_formatted_line_request(request, lines, "no_new_name", {})
			end

			local new_name = parts[1]

			local new_timers = db_query('SELECT id FROM timers WHERE name = $1', { new_name })
			if #new_timers > 0 then
				return l10n_custom_formatted_line_request(request, lines, "namesake", {})
			end

			db_execute('UPDATE timers SET name = $1 WHERE id = $2', { new_name, timer.id })

			return l10n_custom_formatted_line_request(request, lines, "rename", { name, new_name })
		elseif scid == "interval" then
			if #parts == 0 then
				return l10n_custom_formatted_line_request(request, lines, "no_interval", {})
			end

			local interval_sec = tonumber(parts[1])
			if interval_sec == nil then
				return l10n_custom_formatted_line_request(request, lines, "bad_interval", { parts[1] })
			end

			db_execute('UPDATE timers SET interval_sec = $1 WHERE id = $2',
				{ interval_sec, timer.id })

			return l10n_custom_formatted_line_request(request, lines, "interval",
				{ name, time_humanize(interval_sec) })
		elseif scid == "view" then
			return l10n_custom_formatted_line_request(request, lines, "view",
				{ timer.id, timer.name, time_humanize(tonumber(timer.interval_sec)), timer.message })
		elseif scid == "call" then
			return timer.message
		end
	end,
}
