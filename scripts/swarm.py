#!/usr/bin/env python3
"""
Unified CLI for QScript swarms workflows.

Usage:
  python scripts/swarm.py spec <topic>       # Spec refinement (research -> draft -> review)
  python scripts/swarm.py example <text>     # Generate examples from spec snippet
  python scripts/swarm.py dx <error-text>    # Improve compiler error messages

Requires: pip install swarms
"""

import sys
import os

# Add orchestration to path
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ORCH_DIR = os.path.join(SCRIPT_DIR, "..", "orchestration")
sys.path.insert(0, os.path.abspath(ORCH_DIR))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        print("\nExamples:")
        print('  python scripts/swarm.py spec "quantum type system"')
        print('  python scripts/swarm.py example "fn main() -> unit { let x = 1 + 2; }"')
        print('  python scripts/swarm.py dx "undefined variable x"')
        sys.exit(1)

    cmd = sys.argv[1].lower()
    args = sys.argv[2:]

    if cmd == "spec":
        if not args:
            print("Usage: python scripts/swarm.py spec <topic>")
            sys.exit(1)
        from spec_swarm import run_spec_swarm
        topic = " ".join(args)
        output = run_spec_swarm(topic)
        print(output)

    elif cmd == "example":
        if not args:
            print("Usage: python scripts/swarm.py example <spec-snippet>")
            sys.exit(1)
        from example_swarm import run_example_swarm
        snippet = " ".join(args)
        output = run_example_swarm(snippet)
        print(output)

    elif cmd == "dx":
        if not args:
            print("Usage: python scripts/swarm.py dx <error-text>")
            sys.exit(1)
        from dx_swarm import run_dx_swarm
        text = " ".join(args)
        output = run_dx_swarm(text)
        print(output)

    else:
        print(f"Unknown command: {cmd}")
        print("Use: spec, example, or dx")
        sys.exit(1)


if __name__ == "__main__":
    main()
