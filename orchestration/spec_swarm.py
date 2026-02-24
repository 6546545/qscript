"""
SpecSwarm: use swarms to assist with QScript spec work.

This script defines a simple SequentialWorkflow with:
- A Researcher agent that gathers information about a topic.
- A SpecWriter agent that proposes spec text.
- A Reviewer agent that suggests edits.

See the swarms project for more details:
https://github.com/kyegomez/swarms?tab=readme-ov-file
"""

from swarms import Agent, SequentialWorkflow


def build_spec_swarm() -> SequentialWorkflow:
    researcher = Agent(
        agent_name="Researcher",
        system_prompt=(
            "You are a research assistant for the QScript language. "
            "Given a topic, you find and summarize relevant computer science research "
            "with citations where possible."
        ),
        model_name="gpt-4o-mini",
    )

    spec_writer = Agent(
        agent_name="SpecWriter",
        system_prompt=(
            "You draft clear, concise specification text for the QScript language, "
            "based on the research summary you receive."
        ),
        model_name="gpt-4o-mini",
    )

    reviewer = Agent(
        agent_name="Reviewer",
        system_prompt=(
            "You review QScript spec drafts for clarity, consistency, and correctness, "
            "and propose improvements."
        ),
        model_name="gpt-4o-mini",
    )

    workflow = SequentialWorkflow(agents=[researcher, spec_writer, reviewer])
    return workflow


def run_spec_swarm(topic: str) -> str:
    swarm = build_spec_swarm()
    result = swarm.run(topic)
    return str(result)


if __name__ == "__main__":
    import sys

    if len(sys.argv) < 2:
        print("Usage: python spec_swarm.py <topic>")
        raise SystemExit(1)

    topic = " ".join(sys.argv[1:])
    output = run_spec_swarm(topic)
    print(output)

