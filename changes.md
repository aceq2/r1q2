My R1Q2 Clone Changes
--------------------
---
- [General] - Converted readme, TODO, and r1q2.txt files to markdown format.
- [General] - Added MSVC 2010 Solution and Project files. (Game project currently isn't building; using custom .cmd file instead.)
<br><br>
- [Client] - Removed checking for singleplayer maps on new game.
- [Client] - Added hard+ difficulty selection from game menu.
- [Client] - Added drawing of the missing menu banners: Start Server, Player Setup, and Customize Controls.
<br><br>
- [Game] - Enabled unique death messages to display in singleplayer. (Previously only displayed when playing deathmatch or co-op.)
- [Game] - Allow "Player blew up." death message for attacking an explosive.
- [Game] - Added unique death messages for death by monster (Only ones in the demo so far).
- [Game] - Added function pointer error checking patch from http://www.btinternet.com/~anthonyj/projects/FunctionPtrs/ (Saves from different builds will still most likely crash).
<br><br>
- *[Game] - FIXED WARNINGS*
