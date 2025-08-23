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

function a(argc, argv)
	print(argc)
	for k,v in ipairs(argv) do
		print(string.format("%d %s", k, v))
	end
end

