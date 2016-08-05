APL Keyboard Translator
=======================

The `akt` program generates keyboard input suitable for GNU APL and for
editing APL code using a Unicode-capable text editor (e.g. vim).

Compile `akt` using the command:

```
$ make akt
```

Invoke `akt` with a command and its arguments (if any) like this:

```
$ akt apl
```

When you invoke `apl` this way, you will be able to type APL glyphs using
the `Alt` key. The keyboard mapping is illustrated in a file included with
`akt`.

`akt`'s input must be a terminal. Your system's locale must use UTF-8
encoding.

`akt` runs the specified command attached to a pty. `akt` is the master
and the specified command is the slave. The slave's terminal size
automatically adjusts to conform to the master's terminal size.

Use the `-z` option to suppress the action of the suspend character.

`akt` depends upon your terminal emulator sending the two-character
sequence `ESC <char>` when you hold the `Alt` key and type `<char>`.
The ability for a terminal to do so may be a configurable option, often
named something like "Alt sends meta".

Note that some terminal emulators use the `Alt` key to access the
terminal program's menus. This conflicts with `akt`'s use of the `Alt`
key as a shift for APL characters. If your terminal does this, there's
probably a configuration option to disable using `Alt` for menu access.
