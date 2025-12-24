local lines = {
	english = {
		["command_unavailable"] = "{sender.alias_name}: This command is not available.",
		["external_api_error"] = "{sender.alias_name}: Failed to get followlist for %s. Try again later. (%s)",
		["success"] = "{sender.alias_name}: %s",
	},
	russian = {
		["command_unavailable"] = "{sender.alias_name}: Эта команда недоступна.",
		["external_api_error"] = "{sender.alias_name}: Не удалось получить фолловлист %s. Попробуйте позже. (%s)",
		["success"] = "{sender.alias_name}: %s",
	},
}

return {
	name = "followlist",
	summary = "Get user's followlist.",
	description = [[
Read the user's follows as plain text.
After collecting the list of chatters, the bot returns a link to the paste from
[the Pastebin-like service](https://tnd.quest).

# Syntax

`!followlist <username>`

+ `<username>` - Twitch username *(optional)*.

# Responses
+ `https://tnd.quest/XXXXX.txt`
]],
	delay_sec = 5,
	options = {},
	subcommands = {},
	aliases = { "flist", "follows" },
	minimal_rights = "user",
	handle = function(request)
		cfg = bot_config()
		if cfg == nil then
			return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
		end

		if
			cfg.url.paste_service == nil
			or cfg.commands.paste_path == nil
			or cfg.commands.paste_body_name == nil
			or cfg.commands.paste_title_name == nil
		then
			return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
		end

		username = request.sender.alias_name

		if request.message ~= nil then
			username = request.message
		end

		response =
			net_get_with_headers("https://tools.2807.eu/api/getfollows/" .. username, { Accept = "application/json" })

		if response.code ~= 200 then
			return l10n_custom_formatted_line_request(request, lines, "external_api_error", { username, response.code })
		end

		follows = json_parse(response.text)

		body = #follows .. " channels\r\n---------------------\r\n\r\n"

		for i = 1, #follows, 1 do
			follow = follows[i]
			follow_timestamp = time_parse(follow.followedAt, "%Y-%m-%dT%H:%M:%SZ")
			follow_diff = time_humanize(time_current() - follow_timestamp)
			body = body .. follow.login .. " (" .. follow_diff .. " ago)" .. "\r\n"
		end

		time = time_format(time_current(), "%d.%m.%Y %H:%M:%S %z")

		response = net_post_multipart_with_headers(cfg.url.paste_service, {
			[cfg.commands.paste_body_name] = body,
			[cfg.commands.paste_title_name] = username .. "'s followlist on " .. time,
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
