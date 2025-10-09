# Midorix Command Documentation

---

## Table Of Contents

- [helloWorld | hw](#helloWorld)

- [help | h](#help)

- [showcfg | scfg](#scfg)

- [quit | q](#quit)

- [edit | e](#edit)

- [exec | x](#exec)

- [debug | d](#debug)

- [memanalyze | mema](#memanalyze)

- [proman | pm](#proman)

- [doctor | doctr](#doctor)

- [shortcut | s](#shortcut)

---

## helloWorld

This command prints out "Hello, World!" (alias: "hw").

---

## help

This command shows all built-in commands and the description of it (alias: "h").

---

## quit

This command is used to exit the CLI (alias: "q"), You can also give custom exit code by using the first argument as the exit code, example:

```
.quit 1
```

That command will make the CLI exit with exit code 1.

---

## edit

This command is used to edit a file (alias: "e"), The editor can be configured in the configuration file `~/.config/midorix/config.json` at the `editor` field, this command can automatically use a configured argument, the argument is configured at the `editor_arg` field.

Lets assume the `editor` field value is "vim" and the `editor_arg` field value is "-y", when using that configuration, when you use the edit command, the command you will be running is "vim -y ...".

---

## exec

This command is used to compile or interpret a file/program (alias: "x"), The field for exec command is `executor`.

---

## debug

This command is used to debug a program (alias: "d"), The field for this command configuration is `debugger`.

---

## memanalyzer

This command is used to analyze the memory of a program using `valgrind` by default (alias: "mema"), And the configuration field for this command is `memanalyzer`.

---

## proman

proman (project manager) is a command to manage project (alias: "pm"). proman uses subcommand for the execution, here are the list of the proman subcommands:

| Name   | Alias | Description              |
| ------ | ----- | ------------------------ |
| init   | i     | Initiate project.        |
| deinit | di    | Deinitiate project.      |
| build  | b     | Build initiated project. |
| custom | c     | Execute a custom rule.   |
| show   | s     | Show configuration.      |
| help   | h     | Show help.               |

Here is the detailed information for each subcommand:

### init

This subcommand is used to initiate a project, To initiate a project, You should have a `mdrxproject.lua` file in your working directory, proman will use the file to initiate the project you are working on, The `mdrxproject.lua` file contain the project data, This include: source file, output, targets, and other information.

### deinit

This subcommand is used to deinitiate a project, The example usage of this command is when you want to reload the project configuration.

### custom

This subcommand is used to execute a custom rule on a loaded configuration.

### show

This subcommand is used to show the project configuration that are loaded.

### help

This subcommand will show you all the subcommand that are available.

---

## doctor

This command is used to check Midorix "health" (alias: "doctr"), This command will do a test on your Midorix installed on your system and Your loaded project.

This command will check if:

- Configuration file exist.
- Configuration file can be parsed
- `build_config` field on the loaded project configuration.
- `languages` field on the project configuration.
- Language `executor`.
- Build targets.
- Build modes.
- Custom command directory existence.
- Custom command syntax.
- Custom command can be executed by Midorix.

---

## shortcut

This command is used to manage shortcut (alias: "l"), This command is the same like proman, It uses a subcommand, Here are the list of the subcommands for alias command:

| Name | Alias | Description         |
| ---- | ----- | ------------------- |
| help | h     | Show help.          |
| show | s     | Show all shortcuts. |
| run  | r     | Run a shortcut.     |

When you run an alias,  You can join a optional argument, For example: Lets assume that you have a shortcut named "cc" and the command for the "cc" alias is `gcc -O2 -g -Wall`, You can run the shortcut with additional argument by executing this command:

```
.shortcut run cc main.c
```

With that command, The command that will be executed is:

```sh
gcc -O2 -g -Wall main.c
```

alias if effective when you want to execute a long command multiple times.
