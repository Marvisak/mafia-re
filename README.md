# Mafia-RE

Project that aims to completely decompile of Mafia: The City of Lost Heaven.

## Progress

The project is in a very early stage, the only fully decompiled part is `rw_data.dll`, which should be able to replace the original one

There are 2 big tasks ahead, the engine `LS3DF.dll` and the game itself `Game.exe`, both of these are quite big and it will take a long time to reverse them. There is also the various `Setup.exe` files (several because different languages have completely different executables compiled years after the original executable, sometimes you can find 2 versions in the same game files) but those are fairly unimportant at the moment.

Right now the goal is to replace every function one by one using the replacer located in `injector/`, until we have the full game decompiled

## Contributing

Any contributions are welcome, all informations for contributors are in [CONTRIBUTING.md](CONTRIBUTING.md)
