# ModdingEx

An Unreal Engine 5 plugin with various tools helping you in the modding process.
Mainly for usage with [UE4SS](https://github.com/UE4SS-RE/RE-UE4SS)

## Features
- Handle cooking and packaging for mods automatically
- Packaging happens on a per mod basis, all mod specific files are being tracked without the need to play around with chunks
- Start the game from the editor
- Optionally build mods before starting the game
- Build mods in the correct structure and convention to the game dir
- Sanity checks like checking the hash before and after build to make sure content has changed
- Kill processes to allow for pak overwrite (list is configurable)
- Mod creator: Create mods with the click of a button. It will create a "ModActor" blueprint in the correct folder structure. The Blueprint will have all lua interop events and info properties initialized.
- Configurable (e.g. add custom commonly used events to the mod creation process)
- Zip your mod for release

## Installation

**Using prebuilt binaries**
1) Go to [Releases](https://github.com/ToniMacaroni/ModdingEx/releases) and download the latest one.
2) Drop the contents of the zip into `your_uneal_project/Plugins/` (create the `Plugin` folder if it doesn't exist).

**Building from source**
1) Open the terminal.
1) Cd to your `your_uneal_project/Plugins` folder.
2) Execute `git clone https://github.com/ToniMacaroni/ModdingEx`.
3) Regenerate the solution for your Unreal project.
4) Open it in your favorite IDE and build the project.
