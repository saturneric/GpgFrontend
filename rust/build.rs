extern crate cbindgen;

use std::env;
use std::fs;
use std::path::PathBuf;
use std::process::Command;

/// Key dependencies surfaced in the "About" dialog. Each name must match a
/// `[[package]]` entry in `Cargo.lock`; the resolved version is exported as the
/// `GFR_DEP_<UPPERCASE_NAME>` compile-time environment variable.
const TRACKED_DEPS: &[&str] = &[
    "pgp",
    "rsa",
    "rand",
    "zeroize",
    "anyhow",
    "tar",
    "env_logger",
    "log",
    "once_cell",
    "tempfile",
];

/// Capture the build environment (rustc version, target, profile) and the
/// resolved versions of the tracked dependencies, exporting each as a
/// compile-time environment variable consumed via `option_env!` in the crate.
fn emit_build_info(crate_dir: &str) {
    let rustc = env::var("RUSTC").unwrap_or_else(|_| "rustc".to_string());
    let rustc_version = Command::new(rustc)
        .arg("--version")
        .output()
        .ok()
        .filter(|o| o.status.success())
        .map(|o| String::from_utf8_lossy(&o.stdout).trim().to_string())
        .unwrap_or_default();
    println!("cargo:rustc-env=GFR_RUSTC_VERSION={rustc_version}");

    println!(
        "cargo:rustc-env=GFR_TARGET={}",
        env::var("TARGET").unwrap_or_default()
    );
    println!(
        "cargo:rustc-env=GFR_PROFILE={}",
        env::var("PROFILE").unwrap_or_default()
    );

    let deps = resolve_locked_versions(crate_dir);
    for name in TRACKED_DEPS {
        let version = deps.get(*name).map(String::as_str).unwrap_or("");
        let key = name.to_uppercase().replace('-', "_");
        println!("cargo:rustc-env=GFR_DEP_{key}={version}");
    }
}

/// Parse `Cargo.lock` for the resolved versions of the tracked dependencies.
/// Returns a map of crate name to version string; absent crates are omitted.
fn resolve_locked_versions(crate_dir: &str) -> std::collections::HashMap<String, String> {
    let mut versions = std::collections::HashMap::new();

    let lock_path = PathBuf::from(crate_dir).join("Cargo.lock");
    println!("cargo:rerun-if-changed={}", lock_path.display());

    let Ok(content) = fs::read_to_string(&lock_path) else {
        return versions;
    };

    let mut current_name: Option<String> = None;
    for line in content.lines() {
        let line = line.trim();
        if let Some(rest) = line.strip_prefix("name = \"") {
            current_name = rest.strip_suffix('"').map(str::to_string);
        } else if let Some(rest) = line.strip_prefix("version = \"") {
            if let Some(name) = current_name.take() {
                if TRACKED_DEPS.contains(&name.as_str()) {
                    if let Some(version) = rest.strip_suffix('"') {
                        versions.insert(name, version.to_string());
                    }
                }
            }
        }
    }

    versions
}

fn main() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();

    emit_build_info(&crate_dir);

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
