local lines = {
	english = {
		["command_unavailable"] = "{sender.alias_name}: This command is not available.",
		["external_api_error"] = "{sender.alias_name}: Failed to screenshot the message. Try again later. (%s)",
		["success"] = "{sender.alias_name}: %s",
	},
	russian = {
		["command_unavailable"] = "{sender.alias_name}: Эта команда недоступна.",
		["external_api_error"] = "{sender.alias_name}: Не удалось заскриншотить сообщение. Попробуйте позже. (%s)",
		["success"] = "{sender.alias_name}: %s",
	},
}

return {
	name = "screenshot",
	summary = "Take a screenshot of the message.",
	description = [[
This command takes a screenshot of the message you've replied to.
Click the "Reply" button and type `!screenshot` to execute the command.
The bot then returns a link to the image.
]],
	delay_sec = 2,
	options = {},
	subcommands = {},
	aliases = { "scrn", "ttours", "prntsc", "shot", "caught" },
	minimal_rights = "user",
	handle = function(request)
		if request.reply == nil then
			return nil
		end

		local cfg = bot_config()
		if cfg == nil then
			return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
		end

		if
			cfg.url.paste_service == nil
			or cfg.commands.paste_body_name == nil
		then
			return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
		end

		local url = "https://ilt.su/ttours/generate.php?base64=true&message_id=" .. request.reply.id .. "&channel_login=" .. request.channel.alias_name

		local response = net_get(url)

		if response.code ~= 200 then
			return l10n_custom_formatted_line_request(request, lines, "external_api_error", { response.code })
		end

		local shot = response.text

		response = net_post_multipart_with_headers(cfg.url.paste_service, {
			["base64"] = shot,
		}, {
			Accept = "application/json",
		})

		if response.code ~= 201 and response.code ~= 200 then
			return l10n_custom_formatted_line_request(request, lines, "external_api_error", { response.code })
		end

		local body = json_parse(response.text)

		local link = json_get_value(body, cfg.commands.paste_path)
		if link == nil then
			return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
		end

		return l10n_custom_formatted_line_request(request, lines, "success", { link })
	end,
}
