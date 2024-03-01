#![deny(missing_docs)]
#![no_std]

//! # Framed
//!
//! Rust bindings for `framed.h`.
//!
//! # No std
//!
//! This library does not use the standard library.
//!
//! # Examples
//!
//! ## Manual
//!
//! Zones can be manually set up by calling [`zone_begin`] and [`zone_end`];
//!
//! ```
//! use framed;
//! use std::ffi::CString;
//!
//! framed::init(true);
//! framed::zone_begin(&CString::new("Test").unwrap());
//! framed::zone_end();
//! framed::flush();
//! ```
//!
//! ## [`framed_zone`] attribute
//!
//! Using [`framed_zone`], functions can automatically be made into a zone. The attribute
//! will add calls to [`zone_begin`] on entry and [`zone_end`] on return;
//!
//! ```
//! use framed::framed_zone;
//!
//! #[framed_zone(name = "custom name")]
//! fn foo(x: i32) { /* ... */ }
//!
//! #[framed_zone]
//! fn zone_name(y: i32) { /* ... */ }
//! ```
//!
//! Note how the name can be made explicit by specifying it in the attribute, otherwise it
//! is taken to be that of the function.
//!
//! ### Note
//!
//! This feature is still a work-in-progress and does not for example support adding zones to
//! methods (ie `foo(self, ..)` signatures) or functions with variadic arguments.

/// Raw bindings
pub mod ffi;

pub use framed_macro::framed_zone;

/// Initialize a tcp connection to the framed server.
///
/// # Warning
///
/// The connection is managed through a global variable that is not handled by Rust.
/// It is undefined behaviour to call this function more than once per application.
pub fn init(wait_for_connection: bool) -> () {
    let wait_for_connection = wait_for_connection as crate::ffi::FramedB32;
    unsafe {
        // SAFE: global memory managed by framed
        crate::ffi::framed_init_(wait_for_connection);
    }
}

/// Flushes unsent packages.
pub fn flush() -> () {
    unsafe {
        // SAFE: global memory managed by framed
        crate::ffi::framed_flush_();
    }
}

/// Mark start of frame
pub fn mark_frame_start() -> () {
    unsafe {
        // SAFE: global memory managed by framed
        crate::ffi::framed_mark_frame_start_();
    }
}
/// Mark the start of a zone
pub fn zone_begin(name: &::core::ffi::CStr) -> () {
    unsafe {
        // SAFE:
        //  * The string is not freed or reallocated in framed_zone_begin_ despite the absence of
        //    the const specifier.
        //  * global memory managed by framed

        let c_str_ptr = name.as_ptr() as *mut i8;
        crate::ffi::framed_zone_begin_(c_str_ptr);
    }
}

/// Mark the end of a zone
pub fn zone_end() -> () {
    unsafe {
        // SAFE: global memory managed by framed
        crate::ffi::framed_zone_end_();
    }
}
