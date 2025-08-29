project = {
  name = "Hello World!",
  version = "1-Stable",
  mode = {
    release = {
      ["C"] = { flags = "-O2 -s -Wall" }
      },
    debug = {
      ["C"] = { flags = "-O0 -g -Wall -fsanitize=address" }
    }
  },
  build_config = {
	languages = {
	  ["C"] = {
		executor = "gcc",
		action = {
					["compile"] = {
						flags = "-O2 -Wall"
					}
				}
		}
	},
	target = {
	  [1] = {
		source = "main.c",
		language = "C",
		action = "compile",
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

