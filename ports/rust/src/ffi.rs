/// Binding to `FRAMED_B32` typedef'.
pub type FramedB32 = u32;

extern "C" {
    /// Binding to `FRAMED_DEF void framed_init_(Framed_B32 wait_for_connection)`.
    pub fn framed_init_(wait_for_connection: FramedB32) -> ();
    /// Binding to `FRAMED_DEF void framed_flush_(void)`.
    pub fn framed_flush_() -> ();
    /// Binding to `FRAMED_DEF void framed_mark_frame_start_(void)`.
    pub fn framed_mark_frame_start_() -> ();
    /// Binding to `FRAMED_DEF void framed_zone_begin_(char *name)`.
    pub fn framed_zone_begin_(name: *mut i8) -> ();
    /// Binding to `FRAMED_DEF void framed_zone_end_(void)`.
    pub fn framed_zone_end_() -> ();
}
