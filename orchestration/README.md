# QScript Swarms Orchestration

Optional AI-assisted workflows for spec drafting, example generation, and developer experience improvements.

## Setup

```bash
pip install swarms
```

## Commands

Run from the project root:

```bash
# Spec refinement: research -> draft -> review
python scripts/swarm.py spec "quantum type system"

# Generate examples from spec text
python scripts/swarm.py example "fn main() -> unit { let x: i32 = 1 + 2; }"

# Improve compiler error messages
python scripts/swarm.py dx "undefined variable x"
```

Or run the orchestration scripts directly:

```bash
python orchestration/spec_swarm.py "quantum type system"
python orchestration/example_swarm.py "fn main() -> unit { ... }"
python orchestration/dx_swarm.py "error message text"
```

## Workflows

| Script | Purpose |
|--------|---------|
| `spec_swarm.py` | Researcher → SpecWriter → Reviewer for spec topics |
| `example_swarm.py` | SpecReader → ExampleWriter → ExampleReviewer |
| `dx_swarm.py` | ErrorReader → ErrorImprover for clearer diagnostics |

## Integration with Compiler

The swarms are standalone Python workflows. The QScript compiler (`qlangc`) does not invoke them directly. Use `scripts/swarm.py` as the unified entry point.
