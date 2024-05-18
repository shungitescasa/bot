# Minecraft server status check

The `!mcsrv` command allows you to quickly find out the status of Minecraft server.
This is a handy command that solves the problem of
logging into Minecraft and waiting for 20 seconds to load to check the server.

## Syntax
`!mcsrv [address]`

+ `[address]` - IP address or name address of the server.

## Usage

+ `!mcsrv mc.hypixel.net`
+ `!mcsrv 12.255.56.21`

## Responses

+ `✅ hypixel.net (209.222.114.115) | 36911/200000 | Hypixel Network [1.8-1.20]; HOLIDAYS EVENT | TRIPLE COINS AND EXP | 1.8.9`
+ `⛔ 12.255.56.21 (127.0.0.1)`

## The meanings of the parts of the message *(separated by |)*

+ Alphabetic and numeric IP addresses.
+ The number of people playing at the moment and the maximum number of players.
+ The MOTD of the server. Separated by **;** *(semicolon)*.
+ Server version.


## Important notes

+ The server status is taken from the third-party API ["mcsrvstat.us"](https://mcsrvstat.us).

## Error handling

+ [Not enough arguments](/wiki/errors#0)
+ [Not found](/wiki/errors#12)
+ [Something went wrong](/wiki/errors#127)
