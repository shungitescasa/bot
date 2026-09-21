# ![](icon.png) Project Tinybot _(formerly ilotterytea's redpilled bot)_

A feature-rich bot for IRC (and Twitch) chat rooms. It is written in C++23 and uses Boost for connections.

## Features

- Variety of notifications _(Twitch/Kick streams, Telegram/Twitter posts and more)_.
- Moderator tools, such as the ability to ping all chatters and spam a certain message.
- Emote management _(7TV supported)_.
- Timed messages and custom commands.
- Lua scripting.

## Prerequisites

- CMake
- Ninja
- C++23 compiler
- PostgreSQL

## Dependencies

- `boost` for IRC connections.
- `openssl` for secure IRC connections.
- `rpclib` for IPC connections between `singlebot` and `scriptvm`.
- `sol2` for bot commands (`lua` is already bundled).
- `pqxx` for databases.
- `cpr` for HTTP requests.
- `nlohmann/json` for JSON data deserialization.
- `emotespp` for third-party Twitch emotes.
- `pugixml` for parsing RSS feeds.

## Installation Guide

### 1. Clone the Git repository

```bash
git clone https://github.com/shungitescasa/bot.git
cd bot
```

### 2. Run the SQL migrations

All SQL migrations are located in the corresponding `/migrations` folder.

You can run all `up.sql` in sequence yourself or you can use [a special program created for this purpose](https://github.com/ilotterytea/sql_migrator) and run the related command:

`sqlm run --driver postgres --host localhost --port 5432 --db-name DB_NAME --db-user DB_USER --db-pass DB_PASS`

### 3. Build the project

```bash
cmake -G Ninja -DUSE_POSTGRES=1 -DCMAKE_BUILD_TYPE=Release -DUSE_TLS=1 -S . -B build
ninja -C build
```

This will create a `build` folder in the project's root directory. `ninja` will compile two executables: `singlebot` (in `build/singlebot/bin`) and `scriptvm` (in `build/scriptvm/bin`).

### 4. Create the configuration file

The configuration file is in `KEY=VALUE` format. \
Here's example of `tinybot.properties` file with required parameters. This file should be along with compiled executable.

<details>
<summary>Full configuration file</summary>

```properties
instance.name=Tinybot
instance.user_agent=tinybot/vX.X.X (compatible; https://wiki.shungites.casa/doku.php?id=bot:tinybot)
instance.supernicks=OWNER_USERNAME THE_SECOND_OWNER

# used in !help, where the command is appended to the URL using the pattern HOST/!command
instance.host=https://bot.alright.party

irc.host=IRC_SERVER
irc.port=6697
irc.nick=BOT_USERNAME
irc.pass=BOT_PASSWORD

database.user=DB_USER
database.password=DB_PASS
database.name=DB_NAME
database.port=5432

# allow chat joins
join.allow_from_chat=true
join.allow_other_origins=false

# scriptVM-related settings
script.loader=lua
script.directory=ABSOLUTE_PATH_TO_LUASCRIPTS
script.timeout=0
script.allow_arbitrary_scripts=false
script.url_whitelist=https://example.com https://pastebin.com/raw/XXXXXXXX

# insert valid anonbin upload endpoint for long responses
anonbin.url=XXXXXXXXXXXXXXXXX
anonbin.contents=contents
anonbin.subject=subject
anonbin.path=data.urls.download_url

# insert valid anonupload URL for !randompost
anonupload.url=XXXXXXXXXXXXXXXXX
anonupload.base64_contents=base64
anonupload.path=data.urls.download_url

# insert valid 7TV API key for !7tv command
7tv.key=XXXXXXXXXXXXXXXXX

# insert valid RSS-Bridge instance for !events
rss.url=XXXXXXXXXXXXXXXXX
rss.timeout=30

# insert a valid officialchadrankings.com API url for !mog
thirdparty.mogranks=XXXXXXXXXXXXXXXXX

# insert a valid shungitescasa for !ecount
thirdparty.stats=XXXXXXXXXXXXXXXXX
```

If you want to use this bot in a Twitch chat, change these values:

```properties
irc.host=irc.chat.twitch.tv
irc.port=6697
irc.nick=BOT_USERNAME
irc.pass=oauth2:TWITCH_TOKEN
twitch.token=TWITCH_TOKEN
```

</details>

### 5. Launch

- Launch scriptVM: `./build/scriptvm/bin/scriptvm`
- Launch bot: `./build/singlebot/bin/singlebot`

## Why C++?

I explained this unusual decision in detail on my blog. You can read about it [here!](https://tea.shungites.casa/blog/teabot-intro/)
