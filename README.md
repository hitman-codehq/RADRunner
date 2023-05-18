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
DIR/K,EXECUTE/K,GET/K,PORT/K,SCRIPT/K,SEND/K,SERVER/S,SHUTDOWN/S,REMOTE
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
$ RADRunner \<server\> COMMAND
```

\<server\> is either the name of the server or its IP address.  COMMAND is one of the commands shown by the ? request.
A basic session at the command line, including commands typed and text output by RADRunner during development might be:

```shell
$ RADRunner vampire SEND Debug/MyL33tCode
send: Sending file "Debug/MyL33tCode"
send: Transferring file "Debug/MyL33tCode"
send: Transferred 39.828 Kilobytes in 0.3 seconds
$ RADRunner vampire SEND Debug/MyL33tCode
execute: Executing file "MyL33tCode"
```

On the server, the following would be ouput:

```shell
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

```shell
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

```shell
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

## DIR/K \<dirname\>

Lists the contents of the given directory on the remote server.  For example:

```shell
d:\Source> RADRunner vampire DIR RAM:
```

> Note: Using `DIR ""` will cause RADRunner to return the contents of the current remote directory (i.e. The directory
in which RADRunner is executing).  However, the Amiga OS versions of RADRunner use a wrapper around the dos.library
argument parsing functions, which don't allow "" to be passed in.  (they consider this to be a missing argument).  This
can be worked around by using `DIR """"`.  This applies to all commands that refer to the current directory.

## EXECUTE/K \<filename\>

Executes a file on the remote server.  Both the EXECUTE keyword and the filename must be specified.  If the filename
contains no path or a relative path then it is executed from the server's current directory.  If it contains a path
then it will be executed from that path, and the path can be absolute or relative.  Executables and script files can
be executed.

Arguments can be passed to the command by including both the filename and the arguments inside quotes.

```shell
d:\Source> RADRunner vampire EXECUTE "MyL33tCode.exe highres"
```

When the command is executed on the remote server, its output will be captured and displayed by the client.  Here is
an example of using RADRunner to build itself remotely.

```shell
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

## PORT/K \<port\>

This command is meant to be used in conjunction with other commands, to direct RADRunner to use a different port to the
default, which is port 80.  This is useful when that port is being used for something else or is blocked.

For example, to start a server on port 68000 (every Amiga user's favourite number):

```shell
$ RADRunner SERVER PORT 68000
```

And to use that from a remote computer:

```shell
d:\Source> RADRunner vampire PORT 68000 DIR RAM:
```

## SCRIPT/K \<filename\>

Executes a number of commands inside the RADRunner script file specified by \<filename\>.  See the section "Script Support"
for further information.

## SEND/K \<filename\>

Uploads a file to the remote server.  The filename can specify an absolute or relative path on the client computer, or
it can be in the current directory.  However, any path component specified is stripped upon sending and only the filename
itself is sent to the server, resulting in it being written to the server's current directory.

The logic behind this is that the directory structure on the client (development) computer doesn't make sense on the
server.  For example if you are cross compiling using Windows then you might type:

```shell
d:\Source> RADRunner vampire SEND d:\Output\Debug\MyL33tCode.exe
```

It would make no sense to write this to \Output\Debug\MyL33tCode.exe and in fact, during early RADRunner development,
this is exactly what happened, except that the file ended up being called "\Output\Debug\MyL33tCode.exe" on the Amiga
with the directories and backslashes as a part of the filename!  Hence the decision to strip the path.  For backup
software it would make sense to keep the path but RADRunner is not backup software.

If you really have a need to write to a path on the target computer then use the SEND command followed by the EXECUTE
command to execute a script that is already on the target computer, which can move the file to its final location.

Like the GET command, the timestamp is preserved during the transfer, but protection bits are not.

## SERVER

Starts RADRunner in server mode, causing it to listen to incoming connections from other instances of RADRunner.

## SHUTDOWN

Shuts down the remote instance of RADRunner, when it's 3 am and time to go to bed.

# More advanced uses

You may have noticed that some examples use "$ RADRunner" and some use "d:\Source> RADRunner".  This is to show that
RADRunner is truly a cross-platform utility and can be used on Amiga OS, Linux, Mac OS or Windows, with any of these
platforms acting as a server and any platform acting as a client.

This allows some imaginative uses of RADRunner, to suit the way that _you_ want to work.  Here are my (Colin Ward AKA
Hitman/Code HQ) two favourites.

## Favourite 1: Remote integrated development from Visual Studio Code on a PC or Mac

In this scenario, we wish to use Visual Studio Code and its wonderful code lookup features to edit and compile our code,
but we need to run it on either a real Amiga or an emulator.

To begin with, you need to install the VS Code extension for building using make.  Click the "Extensions" icon in the
Activity Bar and search for "Makefile Tools" by Microsoft.  It's ironic that we are using an extension by Microsoft in
a Microsoft IDE to program our beloved Amigas!  :-)  To build using this extension, you need to create a settings.json
file in the .vscode folder.  This instructs the extension how to build using make and how to launch the project once it
has been built.  The settings.json file that is used for building RADRunner itself has been included in the _Examples_
directory.  Copy this file to the .vscode directory and open it to view in VS Studio.

Note that the paths are configured for the author's computer (files such as settings.json in the .vscode directory are
not really meant to be shared), so you will have to tweak them for your system for the example to work.  In particular,
the "cwd" (current working directory) and "binaryPath" properties need to be changed to suit your system, as shown
below:

```json
"makefile.launchConfigurations": [
    {
        "cwd": "d:\\Source\\RADRunner",
        "binaryPath": "d:\\Utils\\Windows\\RADRunner.exe",
        "binaryArgs": [
            "vampire",
            "script",
            "Examples\\SendAndExecute.rad"
        ]
    }
```

You also need to configure the Makefile Tools extension to tell it which build and launch configuration to use.  To do
this, click the extension's icon in the Activity Bar and in the window pane that appears, click the pencil item and
choose the following:

```
Configuration: OS3 Debug
Build target: all
Launch target: ..\..\Utils\Windows\RADRunner.exe...
```

The "Launch target" will be a long string that changes on different systems (the Makefile Tools extension is a little
bit quirky) - the important thing is that the path to your local RADRunner executable should be included in the name you
select.

You may also need to configure the version of make to use, depending on your system.

Of course, for this to work, it assumes that you have an Amiga cross compiler installed and in your path.  At Code HQ,
we use [Bebbo's GCC](https://github.com/bebbo/amiga-gcc) for OS3 and [ADtools](https://github.com/sba1/adtools) for OS4.

By default, the makefile will use any native version of GCC that it finds on the system, so in order to cross compile
for Amiga OS3 or OS4, you must export the _PREFIX_ environment variable.  Choose one of the two exports, depending on
your desired target:

```shell
export PREFIX=m68k-amigaos- # For OS3
export PREFIX=ppc-amigaos- # For OS4
```

Or for Windows:

```shell
set PREFIX=m68k-amigaos- # For OS3
set PREFIX=ppc-amigaos- # For OS4
```

This needs to be accessible to VS Code, so it's usually best to set the variable from a shell and then launch VS Code
from that same shell, rather than launching VS Code through the Start Menu or similar.

The _PREFIX_ environment variable will be picked up by the makefile and used to choose the correct compiler.  If you are
interested in writing a makefile that can be used to compile for OS3, OS4 and native Linux and Mac OS, take a look at
RADRunner's makefile.

Now for the interesting part: Remotely launching the binary you have just built!  Take a look at the settings.json file
again.  You can see that there is some magic in the "makefile.launchConfigurations" section.  Normally, the "binaryPath"
key would be set to the name of the file that was just built by the makefile extension.  In our case, this won't work,
as we have cross compiled for Amiga OS.  So instead, we tell the extension to launch the native (Windows, Linux, Mac OS)
version of RADRunner, using this to send the Amiga OS binary to the Amiga and to execute it.  For instance:

```json
"makefile.launchConfigurations": [
    {
        "cwd": "d:\\Source\\RADRunner",
        "binaryPath": "d:\\Utils\\Windows\\RADRunner.exe",
        "binaryArgs": [
            "vampire",
            "script",
            "Examples\\SendAndExecute.rad"
        ]
    }
```

You can see in the "binaryArgs" array that we pass in the arguments "vampire script Examples\\SendAndExecute.rad".
"vampire" is the name of the host Amiga that we want to communicate with.  The author's Amiga is a Vampire 4 Standalone
that is called "vampire" on his home network, so change this to the name or IP address of your Amiga or WinUAE emulator.
Of course, this Amiga needs to be running RADRunner in server mode:

```shell
RADRunner server
```

The second argument, SCRIPT, tells RADRunner to run a script file, and the third argument, "Examples\\SendAndExecute.rad"
is the name of the script to run.  This script enables you to perform multiple steps in a single execution of RADRunner.
First, it will transfer the recently built binary to the remote Amiga system and then, it will instruct the remote system
to execute it and to return the output via stdout.  To run the launch configuration inside VS Code, select __View -> Command Palette...__
and start typing __Makefile: Run the selected binary target in the terminal__.  Select this, and it will build the
executable and run the _Launch target_ you set previously.  If all goes well, the Amiga executable will be remotely
launched and its stdout will be displayed on the remote Amiga's shell and also in the terminal of VS Code.  In this case,
you should see something like the following:

```shell
D:\Source\RADRunner>"d:\Utils\Windows\RADRunner.exe" vampire script SendAndExecute.rad
Parsing script "SendAndExecute.rad"...
Sending request "send"
send: Sending file "Debug/RADRunner"
send: Transferring file "Debug/RADRunner"
send: Transferred 235.424 Kilobytes in 0.47 seconds
Sending request "execute"
execute: Executing file "RADRunner"
Error: REMOTE argument must be specified
execute: Command complete
```

If you get the output "Executing file "RADRunner"" and "REMOTE argument must be specified" then the native version of
RADRunner that was just built is being executed successfully.  It is not actually useful, as RADRunner can't be used to
develop itself, but it shows how the end-to-end system works, so you can copy these configuration files to your own
project's directory and use them there.

As a bonus, RADRunner can also be built with CMake, which makes it possible to hit \<f7\> to build the local
Windows/Linux/Mac OS version and \<f5\> to run it, and then you can bind \<shift-f7\> and \<shift-f5\> to do the same
thing for the Amiga OS build.  The author finds that this works quite seamlessly!  So please feel free to have a look at
how the RADRunner build works in VS Code and try to build it yourself, to get some ideas of how to configure your own
multiplatform project.

As an extra bonus, take a look in the _.github/workflows_ directory to see how RADRunner is built for Amiga OS3, OS4,
Windows, Linux and Mac OS on GitHub using Code HQ's Amiga OS3 and OS4 builder actions, and feel free to use them in
your own projects!

And as an extra extra bonus, the author's _c_cpp_properties.json_ file is included in the Examples directory.  This file
enables you to easily get syntax highlighting, code lookup and code completion working in Visual Studio Code, when
developing for Amiga OS.  This allows you to type in an Amiga OS type or function name and to hit \<f12\> to see its
definition.  You will need to copy this file to your .vscode directory and tweak the compiler paths to suit your system.
It can also be used when building the same project for other targets, such as Windows, and can be adapted easily for
Linux and Mac OS.  To activate Amiga OS as a target, simply open a .cpp file in your project, open the _command palette_,
type __"C/C++: Select a Configuration..."__ and select "AmigaOS3", or activate it from the "Select Language Mode" button on
the bottom right of the VS Code window.

## Favourite 2: Native development on Amiga OS with remote compilation

In this scenario, we wish to develop on the Amiga itself, using our favourite text editor, with the source code on a
remote PC or Mac.

To begin with, you need to mount the PC or Mac drive remotely, so that you can edit the source remotely on the Amiga.
This is sadly not easy on Amiga OS, as modern networking is not the Amiga's strong point.  Some options are Samba and
FTPMount.  The author of RADRunner has used both of these methods on OS3 and OS4.

Once the PC or Mac drive is mounted and you've edited some files, you can execute the commands below to perform a remote
compilation from the Amiga.

On the PC, start a shell and ensure that you are in the directory that contains the source code that you wish to
compile, and start RADRunner in server mode:

```shell
RADRunner server
```

On the Amiga, request the PC to perform a compilation:

```shell
RADRunner <server> execute make
```

At this point, you will see the results of the compilation displayed on the Amiga's shell, as though the compiler was
running natively on the Amiga itself (that's the beauty of this system).  Now, you can fetch the executable that was
just built:

```shell
RADRunner <server> get <executable_name>
```

### Wrap it all up

This can be automated so that you don't have to type multiple commands every time.  Simply add the EXECUTE and GET
commands in a script that you can execute with RADRunner's SCRIPT command.  Then, create a script file that can be run
on the Amiga called "make", that just calls RADRunner with the SCRIPT command.  Ensure that the s bit is set, and then
you can build on the Amiga as follows:

```shell
9.Work:NewStuff> make
Parsing script "BuildAndGet.rad"...
Sending request "execute"
execute: Executing file "make DEBUG=1"
Compiling ClientCommands.cpp...
Compiling Execute.cpp...
Compiling RADRunner.cpp...
RADRunner.cpp:488:6: warning: unused variable 'unusedVariable' [-Wunused-variable]
  int unusedVariable;
      ^~~~~~~~~~~~~~
Compiling ServerCommands.cpp...
Linking Debug/RADRunner...
execute: Command complete
Sending request "get"
get: Requesting file "Debug/RADRunner"
get: Transferring file "RADRunner" of size 241064
get: Wrote 235.424 Kilobytes to file "RADRunner" in 0.165 seconds
```

The output above is cut and pasted from a Vampire 4 Standalone running Amiga OS 3.1, performing a remote build of
RADRunner.  You can see the steps of requesting the build, the remote PC performing the build, the compiler outputting a
warning (of course this has been added for demonstration purposes - RADRunner compiles with zero warnings on warning
level 4!) and finally fetching the executable for local testing.  Once you get used to saving changes in your text
editor, switching to the shell and typing "make", it "feels" very much as though the compilation is actually being done
on the Amiga - which is the whole point of RADRunner!

Take a look at the "make" and "BuildAndGet.rad" files in the [Examples](https://github.com/hitman-codehq/RADRunner/blob/master/Examples)
directory.  They work with RADRunner by default, but you edit and use in your own projects.  Just change \<server\> to
the name or IP address of your PC and \<executable_name\> to the name of the executable that is built.  Remember to type
"protect make +s" on the Amiga to make the "make" script executable, as this can't be saved by git!

# Compiling

To build RADRunner, you need a cross-platform framework called "The Framework" (yes, a very imaginitive name!), which
is available on GitHub [here](https://github.com/hitman-codehq/StdFuncs).  Clone both repos to the same directory, like
this:

```shell
git clone https://github.com/hitman-codehq/StdFuncs.git
git clone https://github.com/hitman-codehq/RADRunner.git
```

When building RADRunner, it will depend on The Framework headers and libraries being found in ../StdFuncs, so first,
you should build StdFuncs, and then you should build RADRunner.  See the documentation in The Framework's GitHub
project [here](https://github.com/hitman-codehq/StdFuncs#compiling)
for information on how to build for all platforms.  Once built, come back to the RADRunner directory and build RADRunner using the same mechanism.

If you are building RADRunner on Windows, simply load the RADRunner.sln solution into Visual Studio and The Framework
will be automatically built as a dependency when RADRunner is built.

# Future

The creation of RADRunner was prompted by my purchase of a wonderful Vampire 4 Standalone system and the desire to
program it in a more modern way than what was provided natively on an OS3 based Amiga.  Some ideas for the future are:

- Add support for preserving protection bits such as the Amiga script bit
- Allow multiple commands to be specified on the command line
- Add the ability for multiple clients to connect simultaneously
- Improve the script parser
