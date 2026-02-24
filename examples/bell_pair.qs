fn main() -> unit {
    bell_pair();
}

fn bell_pair() -> unit {
    quantum {
        let q: qreg<2> = alloc_qreg<2>();
        h(q[0]);
        cx(q[0], q[1]);
        let result = measure_all(q);
        print_bits(result);
    }
}

