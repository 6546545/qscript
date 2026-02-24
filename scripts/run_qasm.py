#!/usr/bin/env python3
"""
Run an OpenQASM 2.0 file on a local simulator.
Usage: python run_qasm.py <file.qasm>
Requires: pip install qiskit
"""
import sys

def main():
    if len(sys.argv) < 2:
        print("Usage: python run_qasm.py <file.qasm>", file=sys.stderr)
        sys.exit(1)
    path = sys.argv[1]
    try:
        from qiskit import QuantumCircuit
        from qiskit_aer import AerSimulator
    except ImportError:
        print("Error: qiskit not installed. Run: pip install qiskit qiskit-aer", file=sys.stderr)
        sys.exit(1)

    try:
        try:
            from qiskit.qasm2 import load as load_qasm
            qc = load_qasm(path)
        except ImportError:
            qc = QuantumCircuit.from_qasm_file(path)
    except Exception as e:
        print(f"Error loading QASM: {e}", file=sys.stderr)
        sys.exit(1)

    sim = AerSimulator()
    job = sim.run(qc, shots=1024)
    result = job.result()
    counts = result.get_counts()

    print("Measurement counts (Bell pair: expect ~50% 00, ~50% 11):")
    for outcome, count in sorted(counts.items()):
        pct = 100.0 * count / 1024
        print(f"  {outcome}: {count} ({pct:.1f}%)")

if __name__ == "__main__":
    main()
