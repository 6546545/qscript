// Example: mutable assignment in a loop
fn main() -> unit {
    let i: i32 = 0;
    loop {
        if i >= 3 {
            break;
        }
        print("iteration");
        i = i + 1;
    }
    print("done");
}
