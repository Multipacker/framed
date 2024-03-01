extern crate cc;

use std::error::Error;

fn main() -> Result<(), Box<dyn Error>> {
    println!("cargo:rerun-if-changed=./src/framed.c");

    let gcc_no_unused_variable = "-Wno-unused-variable";
    let gcc_no_unused_parameter = "-Wno-unused-parameter";
    cc::Build::new()
        .file("./src/framed.c")
        .define("FRAMED_IMPLEMENTATION", None)
        .flag_if_supported(gcc_no_unused_variable)
        .flag_if_supported(gcc_no_unused_parameter)
        .compile("framed");

    println!("cargo:rustc-link-lib=framed");
    Ok(())
}
