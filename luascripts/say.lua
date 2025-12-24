local lines = {
	english = {
		["no_message"] = "{sender.alias_name}: Message must be provided.",
		["user_not_found"] = "{sender.alias_name}: User %s not found"
	},
	russian = {
		["no_message"] = "{sender.alias_name}: Сообщение должно быть указано.",
		["user_not_found"] = "{sender.alias_name}: Пользователь %s не найден"
	},
}

return {
	name = "say",
	summary = "Make the bot say something.",
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
		local channel_id = request.channel.alias_id
		if string.sub(parts[1], 1, 1) == "#" then
			channel_name = string.sub(parts[1], 2, #parts[1])
			table.remove(parts, 1)
			msg = table.concat(parts, " ")
			local users = twitch_get_users({ logins = { channel_name } })

            if #users == 0 then
                return l10n_custom_formatted_line_request(request, lines, "user_not_found", { channel_name })
            end

			local user = users[1]

            channel_name = user.login
            channel_id = tonumber(user.id)
		end

		irc_send_message({login = channel_name, id = channel_id}, msg)

		return nil
	end,
}
