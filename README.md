# ilotterytea's redpilled bot

A feature-rich chatbot for Twitch. It is written in C++17 and uses the Twitch API/IRC custom library.

> The bot in real action can be seen at [https://twitch.tv/teabot](https://twitch.tv/teabot)

## Features
+ Notifications when a stream starts or ends
+ Massping chatters :trollface:
+ Timer
+ Custom commands
+ Listening for commit updates in GitHub repositories
+ Markov-generated responses?!?!?!?

## Prerequisites

+ C++ compiler
+ PostgreSQL

## Dependencies

### Bot

+ `ixwebsocket` for Twitch connections
+ `pqxx` for databases
+ `cpr` for HTTP requests
+ `nlohmann/json` for JSON data deserialization

### Web

+ `CrowCpp` for creating a simple web server **(requires `asio` library)**
+ `Maddy` for MarkDown parsing

## Installation Guide

### 1. Clone the Git repository

```bash
git clone https://github.com/ilotterytea/bot.git
cd bot
```

### 2. Run the SQL migrations

All SQL migrations are located in the corresponding `/migrations` folder.

You can run all `up.sql` in sequence yourself or you can use [a special program created for this purpose](https://github.com/ilotterytea/sql_migrator) and run the related command:

`sqlm run --db-name DB_NAME --db-user DB_USER --db-pass DB_PASS`

### 3. Build the project

```bash
mkdir build
cd build
cmake -DUSE_TLS=1 ..
make
```

### 4. Create the configuration file

The configuration file is in `KEY=VALUE` format. \
Here's example of `.env` file with required parameters. This file should be along with compiled executable.

```env
db_name=DB_NAME
db_user=DB_USER
db_pass=DB_PASS
twitch.client_id=CLIENT_ID
twitch.token=TOKEN
```

### 5. Launch

+ Launch bot: `./redpilledbot`
+ Start the web server: `./redpilledweb`