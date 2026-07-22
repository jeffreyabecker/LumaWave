---
name: usage-doc-authoring
description: "Use when: writing a usage doc, creating a contract document, writing an authoring guide, documenting a pattern, creating a rule document, defining implementation guidance, or establishing conventions for a specific domain."
---

# Usage Doc Authoring

## Purpose

This skill ensures consistent documentation creation for the project.
Docs should describe the current or target implemented state, not planned work.

## Canonical Examples

Review existing documentation in the codebase for patterns and conventions.

## Required Sections

Every usage doc must include these sections:

1. **Title** — `# <Topic>`
2. **Purpose** — A `## Purpose` section describing what the document covers and who it is for.
   Be explicit about scope: which subsystem, which role (author, reviewer, integrator).
3. **Cross-References** — A "This document complements:" bullet list linking to related
   vision-plan docs and other usage docs. Every usage doc must cross-reference at least
   one vision-plan doc that provides the architectural context.
4. **Rules** — Numbered or named sections with concrete, enforceable rules. Each rule must:
   - State what to do (or not do).
   - Explain the rationale briefly.
   - Provide a concrete example (code, markup, or document fragment).
5. **Anti-Patterns** (where applicable) — A section listing patterns that have proven
   harmful and must not be reintroduced. Each anti-pattern should include what it looks
   like and why it is harmful.

## Non-Backlog Intent

- Docs describe the *current* or *target implemented* state. They are reference
  material for authors, not planning artifacts.
- If you find yourself writing implementation tasks or status tracking in a doc,
  move that content to a task list under `docs/backlog/task-lists/`.
- If you find yourself describing a future architecture that does not yet exist, write
  a design plan under `docs/backlog/plan/` first, then update the doc when the
  plan is implemented.

## Cross-Reference Format

```markdown
This document complements:

- [other-doc.md](./other-doc.md) — what it covers
```

- List entries alphabetically.
- Each entry must include a brief parenthetical describing what the referenced doc covers.
- Verify that every referenced file exists at the time of writing.

## Rule Style

- Prefer imperative, testable rules: "Do X" or "Do not do Y."
- Each rule section should be short enough to scan quickly. If a rule needs more than
  ~15 lines of explanation, consider whether it should be split into sub-rules.
- Concrete examples are mandatory. A rule without an example is not enforceable.

## Prohibitions

- Do not include backlog status tracking, task IDs, or phase tables in a usage doc.
  Those belong in task lists.
- Do not create a usage doc without cross-referencing at least one vision-plan doc.
- Do not use aspirational language ("we should consider", "ideally we would").
  Usage docs state what IS the rule, not what might become the rule.
