---
name: design-plan-authoring
description: "Use when: writing a design plan, making an architecture decision, creating a refactor plan, describing a recommended end state, performing problem analysis, or documenting a recommended design for a subsystem."
---

# Design Plan Authoring

## Purpose

This skill ensures consistent design-plan document creation under `docs/backlog/plan/`.
Design plans describe *what* should change and *why*, at the architecture level —
they do not contain implementation task sequences (those belong in task lists under
`docs/backlog/task-lists/`).

## Canonical Example

`docs/backlog/plan/rendering-manager-state-residency-refactor.md`

## Required Sections

Every design plan must include these sections in order:

1. **Title** — `# <Topic>` (no "Backlog" suffix; that belongs on task lists).
2. **Purpose** — One sentence starting with `Purpose:` describing the scope of the plan.
3. **Status legend** — Same five values as task lists: `todo`, `doing`, `done`, `blocked`, `deferred`.
4. **Source Documents** — Every code file, config file, and doc that informed the plan.
   Include concrete file paths and, where useful, specific class or function names.
   Sorted alphabetically within category.
5. **Current State** — A `## Current State` section describing the as-implemented behavior,
   data shapes, class relationships, and pain points. Use concrete code references:
   file paths, class names, method signatures. Include code snippets where they clarify
   the current shape.
6. **Problem Statement** — A `## Problem Statement` section explaining what is wrong with
   the current state and why it must change. Be specific about costs: RAM, CPU, coupling,
   maintainability, or correctness risks.
7. **Design Goals** — A `## Design Goals` section with a numbered list of properties the
   target design must have. Each goal should be falsifiable.
8. **Non-Goals** — A `## Non-Goals` section explicitly listing what the plan does *not*
   attempt to change. This prevents scope creep and clarifies boundaries.
9. **Recommended End State** — A `## Recommended End State` section describing the target
   architecture. Use concrete type names, data shapes, and ownership boundaries.
   Include diagrams (ASCII or Mermaid) where helpful.
10. **Open Decisions** — A `## Open Decisions` table with columns `ID`, `Status`, `Decision`,
    `Notes`. Decision IDs use the pattern `PREFIX-DEC-N`. Resolved decisions stay in the
    table with `done` status and the resolution in Notes.

## Relationship to Task Lists

- Every design plan must be referenced by at least one task list's Source Documents section.
- A task list under `docs/backlog/task-lists/` should break the plan into concrete,
  sequenced implementation tasks. The plan describes the destination; the task list
  describes the path.
- When a plan and its task list share a topic, use the same prefix for decision IDs
  and task IDs (e.g., `RMR-DEC-1` in the plan, `RMR-01` in the task list).

## Concrete References Rule

- Plans must include concrete code references when describing current state.
  Prefer specific file paths (`src/RenderingManager.h`) and symbol names (`outputsConfig_`)
  over vague descriptions ("the output configuration").
- This ensures plans remain grounded in the actual codebase and can be validated
  against the repo at any time.

## Prohibitions

- Do not include implementation task sequences in a design plan. Tasks go in task lists.
- Do not create a plan without a corresponding or planned task list. Every plan must have
  a path to implementation.
- Do not skip the Non-Goals section. Explicit scope boundaries prevent misunderstanding.
