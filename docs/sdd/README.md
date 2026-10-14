# Lightweight specification-driven development

Use **one Markdown file per meaningful change**, named `NNNN-short-name.md`. Copy [TEMPLATE.md](TEMPLATE.md).
Do not maintain parallel requirements/design/tasks documents for small features.

1. **Specify:** describe the problem, observable behavior, non-goals, design boundary and testable acceptance criteria.
2. **Align:** record the owner's implementation request or accepted plan. Resolve significant scope decisions;
   do not ask for authorization again when the request already supplies it.
3. **Implement:** work through a small checklist. Update the spec when a decision changes, before hiding it in code.
4. **Verify:** record actual commands/results and remaining limitations. Distinguish native review from automated checks.
5. **Close:** mark complete when acceptance criteria pass; update architecture/roadmap only when the contract changed.

Statuses: Draft -> Ready -> In progress -> Complete. A small fix may update an existing spec; obvious documentation,
formatting or build corrections need no new spec. A substantial new architectural decision can live in the spec's decision
section; create an ADR only when it needs a durable reference independent of the feature.

Tests should exercise useful behavior and failure cases, not duplicate implementation. Use focused checks during work,
then task check for code/tooling acceptance. Prose-only changes get link/consistency checks.
For UI changes, record any native visual/input review still required; offscreen tests do not establish usability.

The roadmap describes intent, the architecture describes boundaries, and feature SDDs describe concrete accepted changes.
Keep each fact in one authoritative place and link to it elsewhere.
