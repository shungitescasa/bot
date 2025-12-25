local lines = {
	english = {
		["command_unavailable"] = "{sender.alias_name}: This command is not available.",
		["external_api_error"] = "{sender.alias_name}: Failed to post name history. Try again later. (%s)",
		["user_not_found"] = "{sender.alias_name}: User %s not found.",
		["user_no_changes"] = "{sender.alias_name}: %s hasn't changed their username.",
		["user_changes_one"] = "{sender.alias_name}: %s was previously known as %s (%s ago).",
		["user_changes_more"] = "{sender.alias_name}: %s was previously known as %s (%s ago) and has %s other changes. More: %s",
	},
	russian = {
		["command_unavailable"] = "{sender.alias_name}: Эта команда недоступна.",
		["external_api_error"] = "{sender.alias_name}: Не удалось отправить историю. Попробуйте позже. (%s)",
		["user_not_found"] = "{sender.alias_name}: Пользователь %s не найден",
		["user_no_changes"] = "{sender.alias_name}: %s не изменял своего никнейма.",
		["user_changes_one"] = "{sender.alias_name}: %s был известен как %s (%s назад).",
		["user_changes_more"] = "{sender.alias_name}: %s был известен как %s (%s назад), а также имеет ещё %s изменений. Больше: %s",
	},
}

local function remove_ms(str)
	if str[#str] == "Z" and str[#str - 4] == "." then
		str = string.sub(str, 1, #str - 4)
	end
	return str
end

return {
	name = "namehistory",
	summary = "Get user nickname history.",
	description = [[
The `!namehistory` command retrieves a list of previous usernames.
After collecting the username history, the bot returns a summary and a link from
[the Pastebin-like service](https://tnd.quest).

# Syntax

`!namehistory [username]`

+ `[username]` - Valid Twitch username or user ID. Leave blank to retrieve your history.

# Usage

+ `!namehistory`
+ `!namehistory xqc`

# Responses

+ `(You) was previously known as (NotMe) (2y ago) and has 23 other changes. More: https://tnd.quest/XXXXX.txt`
+ `xqc was previously known as xqcow (3y ago).`

# Important notes

+ Name history is taken from the third-party API service ["logs.zonian.dev"](https://logs.zonian.dev)
]],
	delay_sec = 5,
	options = {},
	subcommands = {},
	aliases = {"namechange"},
	minimal_rights = "user",
	handle = function(request)
		local cfg = bot_config()
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

		local rooturl = "https://logs.zonian.dev/namehistory"

		local name = request.sender.alias_name
		if request.message ~= nil then
			name = request.message
		end
		
		local response = net_get(rooturl .. "/" .. name)
		if response.code ~= 200 then
			response = net_get(rooturl .. "/login:" .. name)
			if response.code ~= 200 then
				return l10n_custom_formatted_line_request(request, lines, "user_not_found", { name })
			end
		end

		local usernames = json_parse(response.text)

		if #usernames == 0 then
			return l10n_custom_formatted_line_request(request, lines, "user_not_found", { name })
		elseif #usernames == 1 then
			return l10n_custom_formatted_line_request(request, lines, "user_no_changes", { usernames[1].user_login })
		end

		local previousname = usernames[#usernames - 1]
		local previoustimestamp = time_humanize(time_current() - time_parse(remove_ms(previousname.last_timestamp), "%Y-%m-%dT%H:%M:%S"))

		if #usernames == 2 then
			return l10n_custom_formatted_line_request(request, lines, "user_changes_one", { name, previousname.user_login, previoustimestamp })
		end

		local body = name .. " has changed their username " .. #usernames .. " times.\r\n"
		.. "This user is also known as:\r\n\r\n"

		for i = #usernames, 1, -1 do
			local user = usernames[i]
			local ts = time_parse(remove_ms(user.last_timestamp), "%Y-%m-%dT%H:%M:%S")
			local fts = time_format(ts, "%b %d, %Y at %H:%M:%S")
			local hts = time_humanize(ts)
		
			body = body .. tostring(#usernames - i + 1) .. ". " .. user.user_login .. " (last seen " .. hts .. " ago, on " .. fts .. ")\r\n"
		end

		local time = time_format(time_current(), "%d.%m.%Y %H:%M:%S %z")

		response = net_post_multipart_with_headers(cfg.url.paste_service, {
			[cfg.commands.paste_body_name] = body,
			[cfg.commands.paste_title_name] = name .. "'s name history as of " .. time,
		}, {
			Accept = "application/json",
		})

		if response.code ~= 201 and response.code ~= 200 then
			return l10n_custom_formatted_line_request(request, lines, "external_api_error", { response.code })
		end

		body = json_parse(response.text)

		local link = json_get_value(body, cfg.commands.paste_path)
		if link == nil then
			return l10n_custom_formatted_line_request(request, lines, "command_unavailable", {})
		end

		return l10n_custom_formatted_line_request(request, lines, "user_changes_more", { name, previousname.user_login, previoustimestamp, tostring(#usernames - 2), link })
	end,
}
