fn main() -> unit {
    let x = 5;
    let y = 10;
    if (x > 0 && y < 20) {
        print_int(1);
    } else {
        print_int(0);
    }
    if (x < 0 || y > 5) {
        print_int(2);
    } else {
        print_int(3);
    }
    if (!(x < 0)) {
        print_int(4);
    }
    let z = !0;
    print_int(z);
}
