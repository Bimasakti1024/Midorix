use clap::Parser;
use std::ffi::{CString, CStr};
use std::os::raw::c_char;

extern crate clap;

// Flag Parser

#[derive(Parser, Debug)]
#[command(
    name = "midorix",
    author, version, about
    )]
struct CliArgs {
	/// Ignore initiate error
	#[arg(long)]
	ignore_init_error: bool,

	/// Does not initiate
	#[arg(short, long, default_value_t = false)]
	no_init: bool,

	/// Configuration path
	#[arg(long)]
	config_path: Option<String>,

	/// Enable CLI
	#[arg(short, long)]
	cli: bool,

	/// Direct command execution
	#[arg(short, long)]
	execute: Option<String>,
}

/// FFI Struct for Midorix Launch Parameter
#[repr(C)]
pub struct LaunchParams {
	pub ignore_init_error: bool,
	pub no_init: bool,
	pub config_path: *const c_char,
	pub cli: bool,
	pub execute: *const c_char,
}

// Midorix Launch Parameter parser
#[unsafe(no_mangle)]
pub extern "C" fn parse_mdrxparam(argc: i32, argv: *const *const c_char) -> LaunchParams {
	// Convert argv from C to Vec<String>
	let args: Vec<String> = unsafe {
		(0..argc)
			.map(|i| {
				let ptr = *argv.add(i as usize);
				CStr::from_ptr(ptr).to_string_lossy().into_owned()
			})
			.collect()
	};

	// Parse with clap
	let cli = CliArgs::parse_from(args);

	// Convert to LaunchParams (C-compatible)
	LaunchParams {
        ignore_init_error: cli.ignore_init_error,
		no_init: cli.no_init,
		config_path: cli.config_path
			.map(|s| CString::new(s).unwrap().into_raw() as *const c_char)
			.unwrap_or(std::ptr::null()),
		cli: cli.cli,
		execute: cli.execute
			.map(|s| CString::new(s).unwrap().into_raw() as *const c_char)
			.unwrap_or(std::ptr::null()),
	}
}

// A function to free cstring form C
#[unsafe(no_mangle)]
pub extern "C" fn free_cstring(s: *mut c_char) {
	if !s.is_null() {
		unsafe {
			// Get the ownership and drop it
            drop(CString::from_raw(s));
		}
	}
}

