## Volition -- first public release

![Volition about logo](https://raw.githubusercontent.com/Subsentient/volition/master/src/control/icons/ctlabout.png)


Welcome to Volition.

Volition is software to administrate, configure, and monitor vast amounts of PCs simultaneously.
**The end goal is to create something significantly easier to use and more flexible than Puppet or Ansible**, while allowing a single command to perform a similar action on thousands of nodes running vastly different operating systems.

A proprietary "enterprise" edition is not off the table, but this repository here will always be GPLv3 licensed and provide all the core functionality required for systems administration purposes.

From mid-2016 onwards, I've been fiddling with this pile of code of mine,
adding and removing features, the idea of what exactly it should be and how it should behave shifting like sand.

It seems to be a perpetual pet project of mine, and I personally think a damned interesting and useful one. :^)

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
* GTK 3 based control program for administration
* Custom binary protocol for network traffic, Conation.
* Lua-based scripting capabilities for nodes with true multithreading and optional LuaJIT-style FFI.
* Unlimited simultaneous jobs/tasks, including unlimited node Lua scripts
* Unlimited simultaneous nodes
* Authentication tokens to add another layer of security
* Vault, to allow nodes to store files on the server and retrieve them as needed, with proper permissions
* Routines, automatic jobs to send nodes at regular intervals
* Regular file copy operations, popen commands, etc.

Right now, Volition is lacking many enterprise-desired features and some other niceties, but I intend to fix that with time.
For now, it's probably useful as a way to administrate your home network all at once and whatnot.

## How to compile

Volition's dependencies are:
* OpenSSL
* SQLite (server)
* Lua, 5.3 or higher recommended (node)
* libcurl (node, optional)
* GTK 3 (control)
* CMake
* GCC or clang or MinGW
* pkg-config
* pthreads on POSIX, win32 threads are used on Windows
* A POSIX shell

Once you have the -devel packages of all these installed, you can use the example compile script.

```
./example_compile.sh
```
This script, by default, compiles Volition in debug mode, which enables considerably more chatter at stdout for the three programs.

To create a password for the admin, add a global config key AdminLogins and set it to e.g. User1,Pass1|User2,Pass2 via Control.

You will need to add the authentication token that the script generates for the node into the server. You can do that from the control program in the Server menu >> Authorization tokens >> add token. Right now, enter 0 for the permissions when prompted, as they're not utilized.

Have fun experimenting with Volition!

## Contributing

If you wish to contribute to Volition, here are the requirements for your code to be seriously considered for merging.

* You must surrender the copyright to me, because as stated earlier, there may be an enterprise version of Volition in the future that is not GPLv3 licensed. In order to make that legal, I must have copyright control.
* You must write your code with the same styling as the rest of the codebase.
* You must use tabs, not spaces.
* You must know C++ **well**.
* I will not accept fugly code. Volition's codebase must remain high quality. I've been burned before with other projects.
* Do not reinvent the wheel. If Volition already has facilities for something, and you are told to use them, use them.
* Do not break portability! Most developers are probably on Linux, but that does not mean it's okay to break support for other platforms, especially Windows!
* Use VLScopedPtr and VLString whenever possible! We want to minimize the chance of memory leaks and buffer overflows/corruption.


