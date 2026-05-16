extern crate cbindgen;

use std::env;
use std::fs;
use std::path::PathBuf;

fn main() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();

    let mut output_file = PathBuf::from(&crate_dir);
    output_file.pop();
    output_file.push("src");
    output_file.push("core");

    if !output_file.exists() {
        fs::create_dir_all(&output_file).expect("failed to create include directory");
    }

    output_file.push("GFCoreRust.h");

    let config = cbindgen::Config::from_root_or_default(&crate_dir);

    cbindgen::Builder::new()
        .with_crate(crate_dir)
        .with_config(config)
        .with_language(cbindgen::Language::Cxx)
        .generate()
        .expect("failed to generate binding file")
        .write_to_file(output_file);
}
