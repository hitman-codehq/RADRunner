# RADRunner

Software to assist with Amiga OS cross development.

## Introduction

RADRunner is an application to assist in cross development of Amiga software.  Its two main functions are the transfer
and execution of executable files, without the need to have any file system mounted.  The most basic use case is that
the Amiga acts as the server, running RADRunner in "server mode" and the computer being used to cross compile the Amiga
software under development acts as the client, running RADRunner in "client mode".

The Amiga then has RADRunner "listening" for incoming connections.  After cross compilation of a program on the development
computer, RADRunner then sends that program to the Amiga and instructs the Amiga to execute it.  After this, the programmer
repeats the edit/compile/execute on the development computer.

It is envisioned that normally the development computer would be a fast PC or Mac, running a more modern compiler than is
available on the Amiga, but really any combination is possible.  The development computer could easily be another Amiga
and the PC or Mac could also be running in server mode, with the Amiga sending the files and commands.  While the original
use case is to assist with cross development of Amiga software, RADRunner is quite generic and is useful in any situation
where file transfer without the assistance of a mounted file system is required.  There is nothing to prevent RADRunner
working over the Internet, provided a route is available to the target host.

In fact, one of the first truly useful uses of RADRunner in the early stages of its creation was to assist in bootstrapping
an Vampire system that contains nothing more than a TCP/IP stack, FTP client and basic operating system.  Once RADRunner
had been transferred via a remote FTP server, it was possible to use it to easily and directly transfer very large files,
very quickly, without the hassle of going via an FTP server in another country.

## What the hell does the name "RADRunner" mean?

Good question.  It seemed all very clever when I came up with it but now I can't remember the thinking behind it!  It's a
portmanteau of "radical" and "runner", with radical being shortened to RAD, which is a reference to the RAD: device on
Amiga OS.  Because in the old days of floppy disk based development, RAD: would be used during for writing executables to
speed up the linking proces and likewise RADRunner is used for sending executables to an Amiga during development.

Well it all made sense when I came up with it!  ;-)  Whatever, now the name has stuck so RADRunner it is.

## Supported Platforms

RADRunner should run on pretty much anything with a C++14 compiler.  Binaries for some of the most popular platforms are
provided in the releases section, but for others its DIY.  Sadly, the C++14 requirement means that it cannot be compiled
natively on Amiga OS 3 but then considering RADRunner's very existance is to support cross compiling mainly to that platform,
that shouldn't be a problem.  It might compile natively on OS4 with the latest adtools but otherwise it's the same as for OS3.

See the section "Compiling" for more information

### Wait, C++14 on Amiga OS 3?  Are you crazy?

Probably yes.  But it's 2021 and I want to combine a modern programming experience (writing C++14 with Visual Studio Code)
with the retro programming experience of running the result on Amiga OS.  While the venerable ADE with native GCC 2.95.3 has
served me well for two decades, I want to modernise my C++ codebase.  Hence the switch to cross compilers.

## Usage

RADRunner uses the Amiga ReadArgs() style command line processing, so it might be slightly strange to a non Amiga user.  To
find out what arguments are available, type "RADRunner ?" at the command line, on any support platform.  You should get something
like this:

```shell
$ RADRunner ?
REMOTE,EXECUTE/K,GET/K,SCRIPT/K,SEND/K,SERVER/S,SHUTDOWN/S
```

Arguments are not case sensitive but by convention on Amiga OS they are typed in upper case, at least in examples.  But
lower case will also work.

To start in server mode, simply type

```shell
$ RADRunner SERVER
```

That's it!  There's no mucking around with configuration files in /etc on Unix or Devs: on Amiga OS or c:\Windows on Windows.
One of the key features of RADRunner is that it is "zero configuration" and "just works".  This is what makes it useful for
bootstrapping brand new Vampires with only a TCP/IP stack installed!  The server is now listening for incoming connections
on port 80.

In client mode, usage tends to follow the pattern:

```shell
$ RADRunner \<server_address\> COMMAND
```

\<server_address\> is either the name of the server or its IP address.  COMMAND is one of the commands shown by the ? request.
A basic session at the command line, including commands typed and text output by RADRunner during development might be:

```
$ RADRunner vampire SEND Debug/MyL33tCode
send: Sending file "Debug/MyL33tCode"
send: Transferring file "Debug/MyL33tCode"
send: Transferred 39.828 Kilobytes in 0.3 seconds
$ RADRunner vampire SEND Debug/MyL33tCode
execute: Executing file "MyL33tCode"
```

On the server, the following would be ouput:

```script
$ RADRunner.exe server
Starting RADRunner server
Listening for a client connection... connected
Received request "version"
Received request "send"
send: Transferring file "MyL33tCode" of size 208064
send: Wrote 39.828 Kilobytes to file "MyL33tCode" in 0.3 seconds
Client disconnected, ending session
Listening for a client connection... connected
Received request "version"
Received request "execute"
execute: Executing command "MyL33tCode"
Damn, I'm gonna win the demo compo!
Client disconnected, ending session
Listening for a client connection...
```

If you compare the output of the two commands, it should be easy enough to match the server's output to the client's.
Output related to commands have the name of the command printed at the start, such as "send:" or "execute:".  General
output and output from the command being executed do not have any prefix.  It can also be seen that each execution of
RADRunner on the client results in a new socket connection being built up and torn down on the server.  Normally this
is not a problem, as in the use case above, as there is a delay between when the "send" command is sent and when the
"execute" command is set.  At the end of the send command, the server has to tear down its currently open socket and
start listening for a new connection.  If the above commands are automated in a script file or a makefile then it is
possible that connection problems can appear if the server is not yet ready after processing the send command, especially
if it is running on an Amiga, which is not the fastest.  RADRunner in server mode is only single threaded and accepts
only one client connection at at time - I didn't want to get too bogged down in making a fancy multithread multiclient
server architecture work on all supported platforms, including Amiga.

A solution to this problem is that RADRunner contains basic built in script support, as detailed below.

## Script Support

As shown in the examples above, each command results in a new connection being made to the server.  From the start, it
was envisaged that multiple commands could be sent using a single connection.  This can be done by specifying multiple
command arguments on the command line (not yet implemented) or by specifying a script file.  Commands in the script
file are in the exact same format as commands on the command file, except that the server name does not need to be
specified, as a connection is already established at the time a script is executed.  An example script will look
something like this:

```shell
# Send my l33t code to the Amiga and run it
send Debug/WinningEntry.exe
execute WinningEntry.exe
shutdown
```

To execute this script, from the command line or inside a makefile, the following command is used:

```script
$ RADRunner vampire SCRIPT SendAndRun.rad
```

At this point, all output will be almost identical to the output from the manual example above.  The script itself
can be named anything, although the extension .rad is used by convention.  # at the start of a line is a comment and
anything else is a command, with arguments following.  There is no flow control and the parser is super simple, so
don't throw too much at it!  It should handle white space and missing arguments to commands though, but not much else.

The "execute" command can accept arguments.  To pass these, simply type the arguments after the executable name in the
script, with or without quotes:

```shell
execute WinningEntry.exe fullscreen
execute "WinningEntry.exe fullscreen"
```

The scripting system also has very basic variable support, which is used for passing arguments from the command line.
For instance, consider the following script line:

```shell
execute WinningEntry.exe $1
```

To use this script, one would type the following on the command line:

```script
$ RADRunner vampire SCRIPT "SendAndRun.rad fullscreen"
```

"fullscreen" would then be passed in as script argument $1.  It is possible to use argument numbers between $1 and
$9.  But as stated, this support is very basic, to allow reuseable scripts which otherwise would have to be copied
for tiny changes to be made, such as the name of the executable to be sent and executed.  The whole thing is really
a bit of a workaround for the fact that in server mode, the application only accepts one connection at a time, and
this causes timing related problems if you wrap multiple calls to RADRunner in a Bash script, for instance, thus
making automation of multiple commands difficult.

Perhaps in the future RADRunner will receive a more sophisticated server mode, which can accept multiple connections
at a time and scripts won't be necessary, or perhaps the script system will be extended to use a proper parser.

# Commands

This is a list of the commands currently supported by RADRunner.

## EXECUTE/K \<filename\>

Executes a file on the remote server.  Both the EXECUTE keyword and the filename must be specified.  If the filename
contains no path or a relative path then it is executed from the server's current directory.  If it contains a path
then it will be executed from that path, and the path can be absolute or relative.  Executables and script files can
be executed.

Arguments can be passed to the command by including both the filename and the arguments inside quotes.

```script
d:\Source> RADRunner vampire EXECUTE "MyL33tCode.exe highres"
```

When the command is executed on the remote server, its output will be captured and displayed by the client.  Here is
an example of using RADRunner to build itself remotely.

```script
$ RADRunner fastpc EXECUTE "make DEBUG=1"
execute: Executing file "make DEBUG=1"
Compiling RADRunner.cpp...
Linking Debug/RADRunner...
execute: Command complete
```

Note that currently, output will only be captured if the RADRunner server is running on a Windows PC.  If it is running
on Amiga OS or UNIX then it will still be executed, but no output will be sent back to the client (yet).

## GET/K \<filename\>

Downloads a file from the remote server.  The same rules for path handling apply as for the EXECUTE command.  When
received, the file will be written into the current directory and its timestamp will be set to match the timestamp
on the original file.  Protection bits such as the script bit (on Amiga OS) are not currently preserved.

## SEND/K \<filename\>

Uploads a file to the remote server.  The filename can specify an absolute or relative path on the client computer, or
it can be in the current directory.  However, any path component specified is stripped upon sending and only the filename
itself is sent to the server, resulting in it being written to the server's current directory.

The logic behind this is that the directory structure on the client (development) computer doesn't make sense on the
server.  For example if you are cross compiling using Windows then you might type:

```script
d:\Source> RADRunner vampire SEND d:\Output\Debug\MyL33tCode.exe
```

It would make no sense to write this to \Output\Debug\MyL33tCode.exe and in fact, during early RADRunner development,
this is exactly what happened, except that the file ended up being called "\Output\Debug\MyL33tCode.exe" on the Amiga
with the directories and backslashes as a part of the filename!  Hence the decision to strip the path.  For backup
software it would make sense to keep the path but RADRunner is not backup software.

If you really have a need to write to a path on the target computer then use the SEND command followed by the EXECUTE
command to execute a script that is already on the target computer, which can move the file to its final location.

Like the GET command, the timestamp is preserved during the transfer, but protection bits are not.

## SCRIPT/K \<filename\>

Executes a number of commands inside the RADRunner script file specified by \<filename\>.  See the section "Script Support"
for further information.

## SHUTDOWN

Shuts down the remote instance of RADRunner, when it's 3 am and time to go to bed.

## SERVER

Starts RADRunner in server mode, causing it to listen to incoming connections from other instances of RADRunner.

# More advanced uses

# Compiling

# Future

The creation of RADRunner was prompted by my purchase of a wonderful Vampire 4 Standalone system and the desire to
program it in a more modern way than what was provided natively on an OS3 based Amiga.  Some ideas for the future are:

- Add support for preserving protection bits such as the Amiga script bit
- Allow multiple commands to be specified on the command line
- Capture stdin, stdout and stderr when running the EXECUTE command on Amiga OS and UNIX
- Add the ability for multiple clients to connect simultaneously
- Improve the script parser
