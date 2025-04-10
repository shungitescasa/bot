local lines = {
	english = {
		["command_unavailable"] = "{sender.alias_name}: This command is not available.",
		["external_api_error"] = "{sender.alias_name}: Failed to get a random post. Try again later. (%s)",
		["success"] = "{sender.alias_name}: %s",
	},
	russian = {
		["command_unavailable"] = "{sender.alias_name}: Эта команда недоступна.",
		["external_api_error"] = "{sender.alias_name}: Не удалось получить случайный пост. Попробуйте позже. (%s)",
		["success"] = "{sender.alias_name}: %s",
	},
}

return {
	name = "randompost",
	delay_sec = 5,
	options = {},
	subcommands = {},
	aliases = { "rpost", "rps", "rtnd" },
	minimal_rights = "user",
	handle = function(request)
		cfg = bot_config()
		if cfg == nil then
			return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
		end

		if cfg.commands.rpost_path == nil or cfg.url.randompost == nil then
			return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
		end

		response = net_get_with_headers(cfg.url.randompost, { Accept = "application/json" })

		if response.code ~= 200 then
			return l10n_custom_formatted_line_request(request, lines, "external_api_error", { response.code })
		end

		body = json_parse(response.text)

		link = json_get_value(body, cfg.commands.rpost_path)
		if link == nil then
			return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
		end

		return l10n_custom_formatted_line_request(request, lines, "success", { link })
	end,
}
