# Godtify
Godtify is a lightweight application that adds global hotkeys to Spotify.

When Spotify isn't running Godtify will keep trying to find it until it is running.
Once Godtify has found Spotify it will send the commands generated by the hotkeys to Spotify.

## Installation
To install, download the latest .exe from the releases tab and place it wherever you would like.

Godtify will generate a godtify.ini file in the same folder that the .exe is placed. 
The .ini file will contain the possible keybindings with a few defaults set.

Godtify will add itself to the tray, if you would like to close it, rightclick the icon (light blue spotify logo) and select exit.

## Troubleshooting
If for some odd reason the hotkeys aren't working it could be because of one of the two issues below. 
* Godtify couldn't register the hotkeys (another program already has these registered or the .ini file is broken).
* Godtify couldn't find Spotify even though it was open.

To fix the first issue, close any applications that also register these hotkeys or have a good look at the .ini file and make sure it follows the rules in the [Keybindings](#keybindings) section below.

To fix the second issue, clicking on Spotify itself can suddenly fix Godtify not finding Spotify.

## Keybindings
Keybindings are located in the godtify.ini that will be generated next to godtify.exe.

The way that the keybinds are set up is done in a similar fashion to Emacs.

You can bind multiple modifiers to a single keybind, these modifiers are:
* C: Control
* S: Shift
* M: Alt

For regular keys you would write down the actual key i.e. `m` for the key `m`.

Regarding special keys like enter or the right arrow key there are is a [list](#special-keys) below.

If you would like to combine multiple modifiers you can chain them by using a dash (`-`).
<br><sub>If you would like to bind dash(`-`) itself to a hotkey, you can't for now due to limitations with how the keybinds are resolved.</sub>

Summarizing all of this information with a few examples:
* To have spotify skip to the next track with Control and the right arrow key, you would have:
	```
	SPOTIFY_NEXT=C-right
	```
* Or to mute spotify with Control + Shift + m:
	```
	SPOTIFY_MUTE=C-S-m
	```

## Special keys
* Backspace: `bck`
* Spacebar: `spc`
* Tab: `tab`
* Return/Enter: `ret`
* Escape: `esc`
* Delete: `del`
* Left arrow: `left`
* Right arrow: `right`
* Up arrow: `up`
* Down arrow: `down`
* F1 through F24: `F1...F24`