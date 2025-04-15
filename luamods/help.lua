local lines = {
	english = {
		["command_unavailable"] = "{sender.alias_name}: This command is not available.",
		["help_default"] = "{sender.alias_name}: More info can be found here: %s",
		["help_command"] = "{sender.alias_name}: More info about {channel.prefix}%s: %s",
	},
	russian = {
		["command_unavailable"] = "{sender.alias_name}: Эта команда недоступна.",
		["help_default"] = "{sender.alias_name}: Больше информации можно найти здесь: %s",
		["help_command"] = "{sender.alias_name}: Больше информации о {channel.prefix}%s: %s",
	},
}

return {
	name = "help",
	description = [[
Get an information about commands.
]],
	delay_sec = 1,
	options = {},
	subcommands = {},
	aliases = {},
	minimal_rights = "user",
	handle = function(request)
		local cfg = bot_config()
		if cfg == nil then
			return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
		end

		if cfg.url.help == nil then
			return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
		end

		local line_id = "help_default"
		local args = { cfg.url.help }
		if request.message ~= nil then
			local parts = str_split(request.message, ' ')
			local command = parts[1]

			if array_contains(bot_get_loaded_command_names(), command) then
				line_id = "help_command"
				args = {}
				table.insert(args, command)
				table.insert(args, cfg.url.help .. '/!' .. command)
			end
		end

		return l10n_custom_formatted_line_request(request, lines, line_id, args)
	end,
}
