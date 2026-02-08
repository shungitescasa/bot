local lines = {
    english = {
        ["no_message"] = "{sender.alias_name}: No phrase provided.",
        ["empty"] = "{sender.alias_name}: No results.",
        ["success"] = "{sender.alias_name}: %s (%s)",
        ["hebrew"] = "Jewish: %s",
        ["english"] = "English: %s",
        ["simple"] = "Simple: %s"
    },
    russian = {
        ["no_message"] = "{sender.alias_name}: Предоставьте фразу.",
        ["empty"] = "{sender.alias_name}: Нет результатов.",
        ["success"] = "{sender.alias_name}: %s (%s)",
        ["hebrew"] = "Иврит: %s",
        ["english"] = "Англ.: %s",
        ["simple"] = "Простой (англ.): %s"
    },
}

local letters = {
    hebrew = {
        A = 1,
        B = 2,
        C = 3,
        D = 4,
        E = 5,
        F = 6,
        G = 7,
        H = 8,
        I = 9,
        K = 10,
        L = 20,
        M = 30,
        N = 40,
        O = 50,
        P = 60,
        Q = 70,
        R = 80,
        S = 90,
        T = 100,
        U = 200,
        X = 300,
        Y = 400,
        Z = 500,
        J = 600,
        V = 700,
        W = 900
    },
    english = {
        A = 6,
        B = 12,
        C = 18,
        D = 24,
        E = 30,
        F = 36,
        G = 42,
        H = 48,
        I = 54,
        J = 60,
        K = 66,
        L = 72,
        M = 78,
        N = 84,
        O = 90,
        P = 96,
        Q = 102,
        R = 108,
        S = 114,
        T = 120,
        U = 126,
        V = 132,
        W = 138,
        X = 144,
        Y = 150,
        Z = 156
    },
    simple = {
        A = 1,
        B = 2,
        C = 3,
        D = 4,
        E = 5,
        F = 6,
        G = 7,
        H = 8,
        I = 9,
        J = 10,
        K = 11,
        L = 12,
        M = 13,
        N = 14,
        O = 15,
        P = 16,
        Q = 17,
        R = 18,
        S = 19,
        T = 20,
        U = 21,
        V = 22,
        W = 23,
        X = 24,
        Y = 25,
        Z = 26
    }
}

local function encode(text, type)
    local result = 0
    local map = letters[type]

    if not map then
        return nil
    end

    text = text:upper()

    for i = 1, #text, 1 do
        local c = text:sub(i, i)
        if map[c] then
            result = result + map[c]
        end
    end

    return result
end

return {
    name = "gematria",
    summary = "Read the phrase as a number",
    description = [[
In numerology, `!gematria` is the practice of assigning a numerical value to a name, word, or phrase by reading it as a number.

# Syntax

`!gematria [phrase...]`

+ `[phrase...]` - Phrase.

# Usage

+ `!gematria hello`
+ `!gematria this is my phrase`

# Responses

+ `hello (Jewish: 103, English: 312, Simple: 52)`
+ `this is my phrase (Jewish: 980, English: 1134, Simple: 189)`
]],
    delay_sec = 2,
    options = {},
    subcommands = {},
    aliases = {},
    minimal_rights = "user",
    handle = function(request)
        local text = request.message
        if text == nil and request.reply ~= nil then
            text = request.reply.message
        end

        if text == nil then
            return l10n_custom_formatted_line_request(request, lines, "no_message", {})
        end

        local results = {}

        for index, _ in pairs(letters) do
            local result = encode(text, index)
            if result ~= nil then
                results[index] = result
            end
        end

        if next(results) == nil then
            return l10n_custom_formatted_line_request(request, lines, "empty", {})
        end

        local parts = {}

        for index, value in pairs(results) do
            local part = l10n_custom_formatted_line_request(request, lines, index, { value })
            table.insert(parts, part)
        end

        return l10n_custom_formatted_line_request(request, lines, "success", { text, table.concat(parts, ", ") })
    end
}
