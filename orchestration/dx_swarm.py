"""
DXSwarm: evaluate and improve QScript compiler diagnostics.
"""

from swarms import Agent, SequentialWorkflow


def build_dx_swarm() -> SequentialWorkflow:
    reader = Agent(
        agent_name="ErrorReader",
        system_prompt=(
            "You read QScript compiler error messages and minimal code snippets, "
            "and identify which parts are confusing or unhelpful."
        ),
        model_name="gpt-4o-mini",
    )

    improver = Agent(
        agent_name="ErrorImprover",
        system_prompt=(
            "You rewrite QScript compiler error messages to be clearer and more actionable, "
            "keeping them concise but friendly."
        ),
        model_name="gpt-4o-mini",
    )

    workflow = SequentialWorkflow(agents=[reader, improver])
    return workflow


def run_dx_swarm(error_text: str) -> str:
    swarm = build_dx_swarm()
    result = swarm.run(error_text)
    return str(result)


if __name__ == "__main__":
    import sys

    if len(sys.argv) < 2:
        print("Usage: python dx_swarm.py <error-text>")
        raise SystemExit(1)

    text = " ".join(sys.argv[1:])
    output = run_dx_swarm(text)
    print(output)

