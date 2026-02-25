// Example: function return values (-> i32)
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

fn main() -> unit {
    let x: i32 = add(1, 2);
    if x < 5 {
        print("x is less than 5");
    } else {
        print("x is 5 or more");
    }
}
