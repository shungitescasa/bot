local lines = {
	english = {
		["no_message"] = "{sender.alias_name}: Message must be provided.",
	},
	russian = {
		["no_message"] = "{sender.alias_name}: Сообщение должно быть указано.",
	},
}

return {
	name = "say",
	description = "Make the bot say something. Available only to superusers.",
	delay_sec = 5,
	options = {},
	subcommands = {},
	aliases = {},
	minimal_rights = "superuser",
	handle = function(request)
		if request.message == nil then
			return l10n_custom_formatted_line_request(request, lines, "no_message", {})
		end

		local msg = request.message

		local parts = str_split(msg, " ")
		if #parts == 0 then
			return l10n_custom_formatted_line_request(request, lines, "no_message", {})
		end

		local channel_name = request.channel.alias_name
		if string.sub(parts[1], 1, 1) == "#" then
			channel_name = string.sub(parts[1], 2, #parts[1])
			table.remove(parts, 1)
			msg = table.concat(parts, " ")
		end

		irc_send_message(channel_name, msg)

		return nil
	end,
}
