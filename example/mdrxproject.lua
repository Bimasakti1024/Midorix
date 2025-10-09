project = {
	name = "Hello",
	version = "1-Stable",
	mode = {
		-- the mode for build, this allow to give different arguments when build
		release = {
			["C"] = { flags = "-O2 -s -Wall" },
			["Rust"] = { flags = "-O -C opt-level=2 -C overflow-checks=no" }
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
					flags = ""
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
					language = "C",
					action = "compile",
					args = "-o HelloC"
				}
			},
			-- build target HelloRust
			["HelloRust"] = {
				[1] = {
					source = "main.rs",
					language = "Rust",
					action = "compile",
					args = "-o HelloRust",
					output = "HelloRust"
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

