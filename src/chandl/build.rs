use cc;

fn main() {
    cc::Build::new()
        .file("../engine/engine.c")
        .compile("mdrx_engine");
}
