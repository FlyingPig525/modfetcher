# Mod Fetcher

This mod allows you to save and load your mods and configs between computers.

Two new buttons are added to the "Account" menu in "Settings", "Save Mods" and "Load Mods"

<img src="./resources/account.png">

If you have not created a user entry, clicking either button with show

<img src="./resources/user_entry.png">

Clicking create will create a user entry on the server using the password provided in the mod config.
It is highly recommended to change this value to something you can remember (a sha hash is practically random data),
**but do not use for anything else**.

Clicking save after creating a user entry will save your mods and configs to the server.

Clicking load will show a popup detailing the mods found on the server, red means you do not have the mod installed.
This list gets truncated after 8 elements.

<img src="./resources/load.png">

Once you load the mods you will be prompted to restart the game.