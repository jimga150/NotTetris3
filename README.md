# NotTetris3
Not Tetris 3 is a re-implementation of [Not Tetris 2 by StabYourself](https://stabyourself.net/nottetris2/), which i assume is the sequel to Not Tetris, in C++ using the [Qt framework](https://www.qt.io/).
Not Tetris 2 is implemented in Lua using the [LÖVE framework](https://love2d.org/), which is a library that wraps OpenGL rendering and the [Box2D library](https://box2d.org/) (an open-source rigid-body 2d physics simulator) together and lets you quickly write games. However, Not Tetris 2 has two issues:

* It is a 32-bit application right now, so some mac users can't run it. I recompiled it with some effort into a 64-bit version for macos Catalina with some hacky source editing, but the new version exacerbates the below issue. I've also tried and failed to update the original source to a newer version of LÖVE that natively supports 64-bit deployment, but the interface also changed significantly since Not Tetris 2's development
* It's not slow, at least not enough to be an issue, but it takes up a lot of processing power to run. My macbook pro 2015 (i7) exports logic and final cut pro projects for breakfast, and its fans are screaming when this game so much as looks at it.
* It doesn't support high-DPI monitors, such as Apple's retina display.
* The original developers are no longer supporting this and the other projects they made, and the code they wrote is extremely difficult to understand. And i would like to see this project in particular live for a bit longer.

I work professionally as a developer. I know, code rewrites are rarely a good idea. But I tried both understanding the old code and porting it in the ways i know possible. Not Tetris 2 is not long for this world. Anyways, this is my own time and i do what i want. So i reimplemented Not Tetris 2 in C++ using Qt. The hope is that with code that is well organized and decently self-documenting (as well as actually documented where it's warrented), this project will be much more maintainable in the future. Plus, it runs like a dream.

Known bugs:
* Score sometimes is off by one from the original game
* Tetris pieces not always flush with wall (one pixel off depending on scaling)

Todo:
* Keep optimizing for many shapes
* Add multiplayer
