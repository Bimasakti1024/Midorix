<p align="center">
  <img src="assets/images/Midorix Logo.png" alt="Midorix Logo" width="200"/>
</p>
<h1 align="center">Midorix</h1>
<p align="center">
  <em>Version 1.4.0-beta</em></br>
  <em>A simple, stateless, lightweight make-like tool written in C and powered by Lua.</em>
</p>



---

## Introduction

Midorix is a simple, stateless, lightweight make-like tool written in C and powered by Lua.

Configure your projects using `mdrxproject.lua` and automate builds, debugging, and execution using Midorix. Midorix is focused on simplicity and extensibility, making it easy for customization and managing projects.

Please note that Midorix is still under development. Bugs may occur; if you find any, please report them to the issue page.

### Interface Preview

Midorix provides a minimal yet verbose CLI interface.
Here are some screenshots to give you an overview of how it works:

• Launch
<p align=center><img src="assets/images/Launch Sample.png" alt="Sample Image 1, Launch." width=700></p>

• Command Execution
<p align=center><img src="assets/images/Execution Sample.png" alt="Sample Image 2, Command Execution." width=400></p>

<p align=center><img src="assets/images/Execution Sample2.png" alt="Sample Image 3, Another Command Execution." width=500></p>

• Midorix Doctor

<p align=center><img src="assets/images/Midorix Doctor.png" alt="Sample Image 3, Doctor." width=500></p>

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

3. Build and Install Midorix. Before building and installing make sure you have [cJSON](https://github.com/DaveGamble/cJSON), [Lua](https://lua.org/), and [linenoise](https://github.com/antirez/linenoise) installed on your build machine.

   Midorix depends on [cJSON](https://github.com/DaveGamble/cJSON) for configuration parsing, [Lua](https://lua.org/) for customization, and [linenoise](https://github.com/antirez/linenoise) for efficient CLI input handling.

   Now, you can build and install with 1 command:

   ```sh
   make build install
   ```

   Now Midorix is installed on your machine!

---

## Using Midorix

### Quickstart

To start Midorix, run this command:

``` sh
midorix
```

Now you are in Midorix CLI. To see the list of available commands, run `.help`. Midorix CLI has a system call fallback, so you don't need to exit and enter again to do a system call.

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
| doctor     | doctr |

This documentation won't use the alias of a command.

#### Using the project manager

To use the project manager you should first add a `mdrxproject.lua` file in your working directory, here is the example of `mdrxproject.lua`:

```lua
project = {
	name = "Hello",
	version = "1-Stable",
	mode = {
		-- the mode for build, this allow to give different arguments when build
		release = {
			["C"] = { flags = "-O2 -s -Wall" },
			["Rust"] = { flags = "-O -C opt-level=2 -C overflow-cheks=yes" }
		},
		debug = {
			["C"] = { flags = "-O0 -g -Wall -fsanitize=address" },
			["Rust"] = { flags = "-C debuginfo=2 -C overflow-checks=yes" }
		}
	},
	-- the build configuration
	build_config = {
		-- language index
		languages = {
			-- The C Language
			["C"] = {
			executor = "gcc",	-- the executor (compiler)
			action = {
				["compile"] = {
					flags = "-O2 -Wall"	-- flags to use when using the "compile" action
				}
			}
			},
			-- Rust language
			["Rust"] = {
			executor = "rustc",
			action = {
				["compile"] = {
					flags = "-C debuginfo=2"
				}
			}
			}
		},
		-- build target
		target = {
			-- build target HelloC
			["HelloC"] = {
				[1] = {
					source = "main.c",
					language = "C";
					action = "compile",
					args = "-o HelloWorld"
				}
			},
			-- build target HelloRust
			["HelloRust"] = {
				[1] = {
					source = "main.rs",
					language = "Rust",
					action = "compile",
					args = "-o HelloWorld"
				}
			}
		}
	}
}

-- example of custom command (relative to project)
function list_targets()
	print("Project Targets:")
	for target,_ in pairs(project.build_config.target) do
		print("-", target)
	end
end

```

After that, You can initiate the project by using the `proman`(project manager) command, to initiate project, use this command:

>```
>.proman init
>```

Now your project should be initialized, after that, you can build the project, to build, run `proman build MODE TARGET`.

The mode is used to configures your project build, use `debug` mode for easier debugging and `release` mode for optimized builds. The mode is customizable and you can add your own mode.

The target is the build target (what do you want to build), if the target is empty Midorix will combine all build target automatically, but if not, it will just build the target you selected.

With that configuration, if you want to build `HelloRust` in debug mode you can execute:

>```
>.proman build debug HelloRust
>```

Or if you want to build all targets:

> ```
> .proman build debug

Full release build flow (assuming the final output file name is `HelloC` and the mode is release):

> ```
> .proman init
> .proman build release HelloC
> ./HelloC
> ```

Dot-prefixed commands do not conflict with built-in commands unless the name overlap.

Currently `proman` is in  minimal state but already offers a flexible feature set, You can extend it via custom rules:

>```
>.proman custom <rule name> [arguments]
>```

you can deinitialize the loaded `mdrxproject.lua` project configuration by executing this command:

>```
>.proman deinit
>```

### Configuring Midorix

To configure Midorix, you can edit the configuration file located at `~/.config/midorix/`. the configuration file name is `config.json`, it uses the JSON format for configuration, Currently, the configuration cannot be reloaded without restarting Midorix.

Here are the full configuration parameters:

| Name             | Data Type | Description                                                  |
| ---------------- | --------- | ------------------------------------------------------------ |
| welcome_msg      | string    | The message to print on launch.                              |
| prefix           | string    | The prefix to execute custom/built-in commands on Midorix.   |
| max_history      | integer   | Maximum command count to save to history.                    |
| editor           | string    | Editor for the `edit` command.                               |
| executor         | string    | "Executor" (supposed to be an interpreter or an compiler) for the `exec` command. |
| debugger         | string    | Debugger for the `debug` command.                            |
| memanalyzer      | string    | Memory Analyzer (valgrind or  something else) for the `memanalyze` command. |
| editor_arg       | string    | Automated argument for the `editor` command.                 |
| executor_arg     | string    | Automated argument for the `executor` command.               |
| debugger_arg     | string    | Automated argument for the `debugger` command.               |
| memanalyzer_arg  | string    | Automated argument for the `memanalyzer` command.            |
| prompt           | string    | Midorix CLI Prompt for commands.                             |
| autostart        | array     | Auto-start commands.                                         |
| autoinit_project | boolean   | Auto initiate the `mdrxproject.lua` file.                    |

This is the default configuration for Midorix:

```json
{
	"welcome_msg": "Welcome to Midorix!\n",
	"prompt": "[Midorix]> ",
	"prefix": ".",
	"max_history": 100,
	"autostart": [],
	"autoinit_project": true,

	"editor": "nvim",

	"executor": "gcc",
	"executor_arg": "-O2 -Wall",

	"debugger": "gdb",
	"debugger_arg": "--tui",

	"memanalyzer": "valgrind",
	"memanalyzer_arg": "--leak-check=full --leak-resolution=med --show-leak-kinds=all"
}
```

If you add an irrelevant or unknown parameter in the configuration it will still be loaded but will have no effect.

### Universal Midorix Custom Command

You can create a universal custom command for Midorix. The custom command is written in Lua and stored in the `~/.config/midorix/custom_command` directory. The file name is the custom command name, each script should define a function called `main(argc, argv)` where `argc` is the argument count and `argv` is the argument value.

When called, Midorix will invoke the `main` function with these two parameters.

---
## Third-Party Libraries
Midorix uses the following third-party libraries:

1. [cJSON](https://github.com/DaveGamble/cJSON) 
   Licensed under the MIT License. See [`third_party/cJSON.LICENSE`](third_party/cJSON.LICENSE) for details.

2. [linenoise](https://github.com/antirez/linenoise) 
   Licensed under the BSD-2 Clause License. See [`third_party/linenoise.LICENSE`](third_party/linenoise.LICENSE) for details.

3. [Lua](https://lua.org/)
Licensed under the MIT License. See [`third_party/Lua.LICENSE`](third_party/Lua.LICENSE) for details.

---

## License

Midorix is licensed under the MIT License, [`click here`](LICENSE) for details.
