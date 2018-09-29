## Volition -- first public release

![Volition about logo](https://raw.githubusercontent.com/Subsentient/volition/master/src/control/icons/ctlabout.png)

Welcome to Volition. From mid-2016 until now (September 29th, 2018), I've been fiddling with this pile of code of mine,
adding and removing features, the idea of what exactly it should be and how it should behave shifting like sand.

At last, I've reached a point in its development where I'm comfortable exposing it to the public.

Volition is written in C++14, but when you observe the style, you'll probably get a strong feeling that 'this guy came from C first',
which of course is correct.

It's around 22,000 lines of code at the time of writing.

Volition is:
* Very multithreaded
* Portable
* Highly extensible
* Currently somewhat user-unfriendly, being fixed.
* TLS-enabled, mandatory.
* GPLv3 licensed

Some of Volition's features include:
* GTK 2 or 3 based control program for administration
* Custom binary protocol for network traffic, Conation.
* LuaJIT-based scripting capabilities for nodes
* Unlimited simultanious jobs/tasks, including unlimited node Lua scripts
* Unlimited simultanious nodes
* Authentication tokens to add another layer of security
* Vault, to allow nodes to store files on the server and retrieve them as needed, with proper permissions
* Routines, automatic jobs to send nodes at regular intervals
* Regular file copy operations, popen commands, etc.

Right now, Volition is lacking many enterprise-desired features and some other niceties, but I intend to fix that with time.
For now, it's probably useful as a way to administrate your home network all at once and whatnot.

## How to compile

Volition's dependencies are:
* OpenSSL
* SQLite
* LuaJIT
* libcurl
* GTK 2 or 3
* GNU make
* GCC or clang or MinGW
* pkg-config
* pthreads on POSIX, win32 threads are used on Windows
* A POSIX shell

Once you have the -devel packages of all these installed, you can use the example compile script.

```
./example_compile.sh
```
This script, by default, compiles Volition in debug mode, which enables considerably more chatter at stdout for the three programs.

Have fun experimenting with Volition!

