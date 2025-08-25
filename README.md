<p align="center">
  <img src="assets/images/Midorix Logo.png" alt="Midorix Logo" width="200"/>
</p>

<h1 align="center">Midorix</h1>
<p align="center">
  <em>A simple, stateless, lightweight make-like tool written in C powered by Lua.</em>
</p>

---

## Introduction

Midorix is a simple, stateless, lightweight, make-like tool made in C. It uses an `mdrxproject.lua` file that acts as a project configuration, the `mdrxproject.lua` is written in Lua, it will contain project information. Midorix can automatically build a project once a `mdrxproject.lua` file has been initiated.

Midorix is focused on simplicity and extensibility, making it easy for customization and managing projects.

Please note that Midorix is still under development, Bugs may occur, if you find any, please report them to the issue page.

### Interface Preview

Midorix provides a minimal yet verbose CLI interface.
Here are some screenshots to give you a feel of how it works:

• Launch
<p align=center><img src=assets/images/sample1.png alt="Sample Image 1, Launch." width=700></p>
• System Call
<p align-center><img src=assets/images/sample2.png alt="Sample Image 2, System Call." width=400></p>
• Built-In Command Execution
<p align=center><img src=assets/images/sample3.png alt="Sample Image 3, Built-In Commands execution." width=500></p>

## Installation

To install Midorix, you can follow these steps:

1. Download source code. To get the source code, you can download the source code or use `git` command:

     ```sh
     git clone https://github.com/Bimasakti1024/Midorix.git
     ```

2. Navigate the cloned repository directory.

    ```sh
    cd Midorix
    ```

3. Build and Install Midorix. Before building and installing make sure you have [cJSON](https://github.com/DaveGamble/cJSON), [Lua](https://lua.org/), and [linenoise](https://github.com/antirez/linenoise) installed on your build machine. Now you can build and install by running:

    ```sh
   sudo make build install
   ```
   
   Now Midorix is installed on your machine!

---

## Using Midorix

### Quickstart

To start Midorix, run this command:

``` sh
midorix
```

Now you are in Midorix CLI, To see the list of available commands, run `.help`. Midorix CLI has a system call fallback, so you don't need to exit and enter again to do a system call.

You can see what configuration is loaded in your Midorix, to see run the `.showcfg` command.

You will notice that there is an "ALIAS" column in the list of commands, the alias is used to shorten the commands, so you can type less when executing a long named command by just typing their alias. Here is a list of built in commands and its aliases:

| Name       | Alias |
| ---------- | ----- |
| helloWorld | hw    |
| help       | h     |
| showcfg    | scfg  |
| quit       | q     |
| edit       | e     |
| exec       | x     |
| debug      | d     |
| memanalyze | mema  |
| proman     | pm    |

This documentation won't use the alias of a command.

#### Using the project manager

To use the project manager you should first add a `mdrxproject.lua` file in your working directory, here is the example of `mdrxproject.lua`:

```lua
project = {
  name = "Hello World!",
  version = "1-Stable",
  build_config = {
    languages = {
      ["C"] = {
        executor = "gcc",
        flags = "-Wall -g -O2"
      }
    },
    target = {
      [1] = {
        source = "main.c",
        language = "C",
        args = "-o HelloWorld",
    }
  }
  },
  run = {
    source = "HelloWorld"
  }
}

function test(argc, argv)
	print(argc)
	for k,v in ipairs(argv) do
		print(string.format("%d %s", k, v))
	end
end

```

After that, You can initiate the project by using the `proman`(project manager) command, to initiate project, use this command:

>```
>.proman init
>```

Now your project should be initialized, after that, you can build the project, to build, run `proman build`.

Full build flow (assuming the final output file name is `HelloWorld`):

> ```
> .proman init
> .proman build
> ./HelloWorld
> ```

Even if the prefix is a dot, it won't have conflict with the built-in commands, **as long there is no name conflict**.

Currently `proman` is in  minimal state but already offers a flexible feature set, You can extend it via custom rules:

>```
>.proman custom <rule name> [arguments]
>```

you can deinitialize to clean the loaded `mdrxproject.lua` project configuration by executing this command:

>```
>.proman deinit
>```

### Configuring Midorix

To configure Midorix, you can edit the configuration file located at ~/.config/midorix/. the configuration file name is `config.json`, it uses the JSON format for configuration, currently you cannot reload configuration without relaunching Midorix.

Here are the full configuration parameters:

| Name        | Data Type | Description                                                  |
| ----------- | --------- | ------------------------------------------------------------ |
| welcome_msg | string    | The message to print on launch.                              |
| prefix      | string    | The prefix to execute custom/built-in commands on Midorix.   |
| max_history | integer   | Maximum command count to save to history.                    |
| editor      | string    | Editor for the `edit` command.                               |
| executor    | string    | "Executor"(supposed to be an interpreter or an compiler) for the `exec` command. |
| debugger    | string    | Debugger for the `debug` command.                            |
| memanalyzer | string    | Memory Analyzer(valgrind or  something else) for the `memanalyze` command. |
| prompt      | string    | Midorix CLI Prompt for commands.                             |
| autostart   | array     | Auto-start commands.                                         |

This is the default configuration for Midorix:

```json
{
    "welcome_msg": "Welcome to Midorix!\n",
    "prompt": "[Midorix]> ",
    "prefix": ".",
    "max_history": 100,
    "editor": "nvim",
	"executor": "gcc",
	"debugger": "gdb",
	"memanalyzer": "valgrind",
	"autostart": []
}
```




---
## Third-Party Libraries
Midorix uses the following third-party libraries:

1. [cJSON](https://github.com/DaveGamble/cJSON) 
   Licensed under the MIT License. See [`third_party/cJSON.LICENSE`](third_party/cJSON.LICENSE) for details. Used for configuration.

2. [linenoise](https://github.com/antirez/linenoise) 
   Licensed under the BSD-2 Clause License. See [`third_party/linenoise.LICENSE`](third_party/linenoise.LICENSE) for details. Used for user input and command history.

3. [Lua](https://lua.org/)
Licensed under the MIT License. See [`third_party/Lua.LICENSE`](third_party/Lua.LICENSE) for details. Used for configuration, custom rules and custom commands.

---

## License

Midorix is licensed under the MIT License, [`click here`](LICENSE) for details.
