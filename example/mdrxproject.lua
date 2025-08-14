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

