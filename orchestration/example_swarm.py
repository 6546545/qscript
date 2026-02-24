"""
ExampleSwarm: generate example QScript programs and tests from spec text.
"""

from swarms import Agent, SequentialWorkflow


def build_example_swarm() -> SequentialWorkflow:
    reader = Agent(
        agent_name="SpecReader",
        system_prompt=(
            "You read QScript specification text and extract key language features "
            "and constraints that should be demonstrated with examples."
        ),
        model_name="gpt-4o-mini",
    )

    example_writer = Agent(
        agent_name="ExampleWriter",
        system_prompt=(
            "Given a description of QScript language features, you write small, "
            "self-contained example programs in QScript syntax that illustrate those features."
        ),
        model_name="gpt-4o-mini",
    )

    reviewer = Agent(
        agent_name="ExampleReviewer",
        system_prompt=(
            "You review QScript example programs for clarity and alignment with the spec, "
            "and suggest corrections if needed."
        ),
        model_name="gpt-4o-mini",
    )

    workflow = SequentialWorkflow(agents=[reader, example_writer, reviewer])
    return workflow


def run_example_swarm(spec_snippet: str) -> str:
    swarm = build_example_swarm()
    result = swarm.run(spec_snippet)
    return str(result)


if __name__ == "__main__":
    import sys

    if len(sys.argv) < 2:
        print("Usage: python example_swarm.py <spec-snippet-text>")
        raise SystemExit(1)

    snippet = " ".join(sys.argv[1:])
    output = run_example_swarm(snippet)
    print(output)

