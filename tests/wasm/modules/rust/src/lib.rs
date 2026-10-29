#![no_std]

static mut DATA_WORD: i32 = 4321;

extern "C" fn thrice(x: i32) -> i32 {
    unsafe { x * 3 + core::ptr::read_volatile(&raw const DATA_WORD) }
}

static mut SLOT: extern "C" fn(i32) -> i32 = thrice;

#[no_mangle]
pub extern "C" fn module_data() -> *mut i32 {
    &raw mut DATA_WORD
}

#[no_mangle]
pub extern "C" fn module_slot() -> *mut extern "C" fn(i32) -> i32 {
    &raw mut SLOT
}

#[panic_handler]
fn panic(_: &core::panic::PanicInfo) -> ! {
    loop {}
}
