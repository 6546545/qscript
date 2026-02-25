// Example: arithmetic in let initializers and return
fn main() -> unit {
    let x: i32 = 1 + 2;
    if x < 5 {
        print("x is less than 5");
        return;
    }
    print("x is not less than 5");
}
