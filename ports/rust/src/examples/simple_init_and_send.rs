use framed;
use std::ffi::CString;

fn main() {
    framed::init(true);
    framed::zone_begin(&CString::new("Test").unwrap());
    framed::zone_end();
    framed::flush();
}
