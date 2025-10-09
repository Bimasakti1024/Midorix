// src/chandl/rscmd_chandl.rs
use std::{ffi::{c_char, c_int, CStr, CString}, fs, rc::Rc, cell::RefCell};
use serde_json::Value;

// Midorix Core
#[link(name = "mdrx_engine", kind = "static")]
unsafe extern "C" {
    fn execute(command: *const c_char);
}

// Global Variable
static mut SHORTCUTS: Value = Value::Null;

// Command Struct
struct Command {
    name: &'static str,
    alias: &'static str,
    desc: &'static str,
    func: Box<dyn Fn(i32, Vec<String>)>,
}

// Initiate
#[unsafe(no_mangle)]
pub extern "C" fn rscmd_init_shortcut(shortcutfn: *const c_char) -> c_int {
    // Get shortcut_path
    let shortcut_path = own_cstr(shortcutfn);

    // Read shortcut configuration and save it as a
    // string
    match fs::read_to_string(&shortcut_path) {
        // Read success
        Ok(c) => unsafe {
            // Parse shortcut configuration
            match serde_json::from_str(&c) {
                // Parse success
                Ok(parsed) => SHORTCUTS = parsed,

                // Parse failed
                Err(e) => {
                    // Debug print
                    eprintln!("Failed to parse {}: {}", shortcut_path, e);
                    return 10;
                }
            }
        },

        // Read failed
        Err(e) => {
            // Debug print
            eprintln!("Cannot read {}: {}", shortcut_path, e);
            return 9;
        }
    };
    return 0;
}

// Utility: convert C string to Rust String
fn own_cstr(cstr: *const c_char) -> String {
    unsafe {
        // Get the string from pointer
        CStr::from_ptr(cstr)

            // Convert to string and if there
            // is a "weird data" will change it
            // to a placeholder symbol to
            // keep it a valid UTF-8
            .to_string_lossy()

            // Force own the string
            .into_owned()
    }
}

// Utility: convert C argv to Vec<String>
fn own_c_argv(argc: c_int, argv: *const *const c_char) -> Vec<String> {
    (0..argc)
        .map(|i| unsafe { own_cstr(*argv.add(i as usize)) })
        .collect()
}

// Alias command

// Help function for shortcut
fn sc_help(_argc: i32, _argv: Vec<String>, cmd_index: &RefCell<Vec<Command>>) {
    println!("NAME        ALIAS      DESCRIPTION");
    for c in cmd_index.borrow().iter() {
        println!("{:9}   {:5}      {:7}", c.name, c.shortcut, c.desc);
    }
}

// Show function for shortcut
fn sc_show(_argc: i32, _argv: Vec<String>, _cmd_index: &RefCell<Vec<Command>>) {
    unsafe {
    if let Value::Object(ref shortcuts) = SHORTCUTS {
        println!    ("ALIAS      COMMAND");
        for (alias, val) in shortcuts {
            println!("{:5}      {}", shortcut, val.as_str().unwrap_or("<Error: Not a string.>"));
        }
    }}
}

fn sc_run(argc: i32, argv: Vec<String>, _cmd_index: &RefCell<Vec<Command>>) {
    if argc < 2 {
        eprintln!("Expected shortcut name.");
        return;
    }

    let shortcutnm = &argv[1];
    unsafe {
        if let Value::Object(ref shortcuts) = SHORTCUTS {
            // Iterate each shortcut
            for (shortcut, val) in shortcuts {
                // Check if the shortcut name match
                if shortcut == shortcutnm {
                    // Set up argument
                    let mut fcmd = val.as_str().unwrap_or("").to_string();
                    if argc > 2 {
                        for i in 2..argc {
                            fcmd.push_str(&(String::from(" ") + &argv[i as usize]));
                        }
                    }

                    // Execute
                    execute(CString::new(fcmd).expect("CString failed.").as_ptr());

                    return;
                }
            }
        }
    }

    // Debug print
    eprintln!("Alias {} not found.", shortcutnm);
}

// Main shortcut function
#[unsafe(no_mangle)]
pub extern "C" fn cmd_shortcut(argc: c_int, argv: *const *const c_char) {
    if argc < 2 {
        eprintln!("Expected a subcommand, use \"help\" subcommand to get a list of all subcommand.");
        return;
    }

    // Get the arguments and the subcommands
    let args = own_c_argv(argc, argv);
    let subcommand = args[1].clone();

    // Command list
    let cmd_index: Rc<RefCell<Vec<Command>>> = Rc::new(RefCell::new(Vec::new()));

    // Setup subcommand index

    // Push help subcommand
    let help_cindexc = Rc::clone(&cmd_index);
    cmd_index.borrow_mut().push(Command {
            name: "help",
            alias: "h",
            desc: "Show help.",
            func: Box::new(move |argc, argv| sc_help(argc, argv, &help_cindexc)),
        }
    );

    // Push show subcommand
    let show_cindexc = Rc::clone(&cmd_index);
    cmd_index.borrow_mut().push(Command {
            name: "show",
            alias: "s",
            desc: "Show all shortcut.",
            func: Box::new(move |argc, argv| sc_show(argc, argv, &show_cindexc)),
        }
    );

    // Push run subcommand
    let run_cindexc = Rc::clone(&cmd_index);
    cmd_index.borrow_mut().push(Command {
            name: "run",
            alias: "r",
            desc: "Run an shortcut.",
            func: Box::new(move |argc, argv| sc_run(argc, argv, &run_cindexc)),
        }
    );

    // Iterate commands
    for c in cmd_index.borrow().iter() {
        if subcommand == c.name || subcommand == c.shortcut {
            let cargv = args[1..].to_vec();
            (c.func)(argc - 1, cargv);
            return;
        }
    }

    eprintln!("Subcommand {} does not exist.", subcommand);
}

