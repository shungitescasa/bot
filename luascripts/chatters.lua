local lines = {
	english = {
		["command_unavailable"] = "{sender.alias_name}: This command is not available.",
		["external_api_error"] = "{sender.alias_name}: Failed to post a chatter list. Try again later. (%s)",
		["channel_not_found"] = "{sender.alias_name}: Channel %s not found",
		["success"] = "{sender.alias_name}: %s",
	},
	russian = {
		["command_unavailable"] = "{sender.alias_name}: Эта команда недоступна.",
		["external_api_error"] = "{sender.alias_name}: Не удалось отправить список чаттеров. Попробуйте позже. (%s)",
		["channel_not_found"] = "{sender.alias_name}: Канал %s не найден",
		["success"] = "{sender.alias_name}: %s",
	},
}

return {
	name = "chatters",
	summary = "Get a list of chatters.",
	description = [[
> To use the `!chatters` command, you must assign moderator to the bot.
> Following the [Twitch API docs](https://dev.twitch.tv/docs/api/reference/#get-chatters), only moderators have access to full chatter list.

The `!chatters` command allows you to get a list of chatters as plain text.
After collecting the list of chatters, the bot returns a link to the paste from
[the Pastebin-like service](https://tnd.quest).
]],
	delay_sec = 30,
	options = {},
	subcommands = {},
	aliases = { "chatterlist", "clist", "ulist", "userlist" },
	minimal_rights = "user",
	handle = function(request)
		cfg = bot_config()
		if cfg == nil then
			return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
		end

		if
			cfg.url.paste_service == nil
			or cfg.commands.paste_body_name == nil
			or cfg.commands.paste_title_name == nil
		then
			return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
		end

		chatters = twitch_get_chatters()
		body = #chatters .. " chatters\r\n---------------------\r\n\r\n"

		for i = 1, #chatters, 1 do
			chatter = chatters[i]
			body = body .. chatter.login .. "\r\n"
		end

		time = time_format(time_current(), "%d.%m.%Y %H:%M:%S %z")

		response = net_post_multipart_with_headers(cfg.url.paste_service, {
			[cfg.commands.paste_body_name] = body,
			[cfg.commands.paste_title_name] = request.channel.alias_name .. "'s chatter list on " .. time,
		}, {
			Accept = "application/json",
		})

		if response.code ~= 201 and response.code ~= 200 then
			return l10n_custom_formatted_line_request(request, lines, "external_api_error", { response.code })
		end

		body = json_parse(response.text)

		link = json_get_value(body, cfg.commands.paste_path)
		if link == nil then
			return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
		end

		return l10n_custom_formatted_line_request(request, lines, "success", { link })
	end,
}
