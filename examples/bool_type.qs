fn main() -> unit {
    let a: bool = true;
    let b: bool = false;
    if (a) {
        print_int(1);
    }
    if (!b) {
        print_int(2);
    }
    let c: bool = 1 < 2;
    if (c && !b) {
        print_int(3);
    }
    let d: bool = 5 == 5;
    print_int(d);
}
