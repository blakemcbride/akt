APL Key Translation for use with GNU APL
========================================

This program, `akt`, is intended as a lightweight tool to generate
keyboard input suitable for GNU APL. You should pipe `akt` to `apl`,
like this:

```
$ akt | apl
```

When you invoke `apl` this way, you will be able to type APL glyphs
using the Alt key.

`akt`'s input must be a terminal.

`akt` takes advantage of the fact that many terminal emulators are
capable of generating Emacs-compatible character sequences, in which
`Alt-char` (or, using Emacs terminology: `M-char`) emits the sequence
`ESC char`.

Both `gnome-terminal` and `xterm` on my Fedora boxes are (or can be)
configured to send "meta as escape".

Note that some terminal emulators also use the Alt key to access the
terminal program's menus. This conflicts with `akt`'s use of the Alt
key as a shift for APL characters; find a way to disable the keyboard
menu access.

Is there a catch? Of course. There always is... Most modern terminal
emulators send cursor keys as a sequence of characters beginning with
a "control sequence introducer" (CSI): the characters `ESC [`.

GNU APL's keyboard mapping uses `Alt [` to produce the `←` character.
Yes, this is exactly the same as the CSI. `akt` attempts to
disambiguate the two cases by looking at the timing of the next
character to arrive. If 50 ms passes with no further input after the
`Alt [`, `akt` assumes that the sequence should translate to `←`. If
`akt` receives another character within 50 ms of `Alt [`, it assumes
that the next character is really following a CSI; `akt` then passes
the CSI and subsequent characters to the output untranslated.

This timing-based approach works well for the most part. The approach
can fall apart, though, if you attempt to autorepeat the `←`
character. If you really need a long line of `←`s (and let's face it:
you're doing this for purely decorative reasons) then type them one at
a time.

Because `akt` starts the key timer on receipt of the ESC character
(without which you wouldn't be able to type a bare ESC), you'll see
a similar problem if you autorepeat the ESC key.

Finally, be aware that you can't type APL characters by pressing the
ESC key followed by another character. Again, this is because of
timing considerations. Use the Alt key on your terminal emulator.

I have no intention to "fix" issues around the key timer; it would
require a much more sophisticated state machine than is warranted by
the nature of the "problem".

`akt` has an atypical exit protocol: when piped into another program,
`akt` quits when its output pipe disappears or the receiving program
disappears.

When `akt`'s output is piped, a Ctrl-C received by `akt` sends a
SIGINT to the process on the receiving end of the pipe; the Ctrl-C
does *not* terminate `akt`.

Some programs disable the SIGINT signal, expecting to receive a typed
Ctrl-C character. To use `atk` with these programs pass the `-n` ("no
signal") option to `akt`. For example:

```
$ akt -n | nano
```

`akt` compiles and runs on my Fedora 20 Linux boxes. It is also
reported to have compiled and run on a LinuxMint x64 box. I make no
promises that `akt` will compile or run on any other system, but will
happily accept patches to make it work on other Unix-like platforms.

Compile `akt` using the command:

```
$ make akt
```

Put the executable somewhere on your shell's PATH, then invoke `akt`
and GNU APL with the command:

```
$ akt | apl
```
