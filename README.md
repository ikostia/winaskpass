# winaskpass
An askpass script for requesting additional user input on Windows.
This script is intended to be used with programs whose STDIN/STDOUT were
redirected, but which still need to be able to request some user input.

Currently, just a console version of askpass functionality is supported,
which just directly prints the prompt to `CONOUT$` and reads the answer
from `CONIN$`. This obviously assumes, that `askpass.exe` has an actual
console attached, e.g. it means that while the parent's program STDIN/STDOUT
might've been redirected, it was still run from the console. It wouldn't work
if the parent program was ran form GUI.

It is also possible (and planned) to implement a GUI version of this script,
which would read the answer in a graphical input box.

# Usage
The client program should call `askpass.exe` and provide the prompt text via
STDIN of `askpass`. `askpass` will display the prompt, get the user's answer
and print it into its STDOUT. Use `-p` command line argument if you want
console echoing to be disabled while the users types his/her answer (desired
if the answer is a password).

# Building
Just run `cl.exe askpass.c` and use produced `askpass.exe`.
