---
name: backlog-task-authoring
description: "Use when: creating a backlog, adding a task, writing a task list, creating a new task, updating task status, defining phases, writing definitions of done, or structuring implementation work into tracked work items."
---

# Backlog Task Authoring

## Purpose

This skill ensures consistent backlog task-list creation and maintenance across the LumaLink project.
Every task list follows the same structural conventions so that status is machine-scannable,
dependencies are traceable, and the format stays uniform across all contributors.

## Canonical Example

`docs/backlog/task-lists/003-rendering-manager-state-residency-refactor-backlog.md`

## Required Sections

Every task list document must include these sections in order:

1. **Title** — `# <Topic> Backlog`
2. **Purpose** — One sentence starting with `Purpose:` describing what the backlog breaks down and why.
3. **Status legend** — A bullet list of the five status values:
   - `todo`: not started
   - `doing`: active
   - `done`: completed
   - `blocked`: waiting on dependency
   - `deferred`: intentionally postponed
4. **Source Documents** — A bullet list of every file (code, doc, config) that informed or is affected by this work.
   Sorted alphabetically within category (code files first, then docs).
5. **Problem Statement** — A `## Problem Statement` section describing the current state,
   what is wrong with it, and why the work is needed. Be concrete; reference specific files,
   class names, and data shapes.
6. **Open Decisions** — A `## Open Decisions` table with columns `ID`, `Status`, `Decision`, `Notes`.
   Decision IDs use the pattern `PREFIX-DEC-N` (e.g., `RMR-DEC-1`, `CS-DEC-1`).
   Status values are the same as the task status legend.
   Resolved decisions stay in the table with `done` status and the resolution recorded in Notes.
7. **Phased Task Tables** — One `## Phase N - <Name>` section per phase.
   Each contains a table with columns: `ID`, `Status`, `Task`, `Depends On`, `Definition of Done`.

## Task ID Conventions

- Task IDs use the pattern `<PREFIX>-<NN>` where the prefix is a 2-4 letter acronym for the backlog topic
  (e.g., `RMR-01`, `CS-02`).
- Number tasks sequentially within the backlog; skip tens between phases (e.g., 01-03, 10-12, 20-24)
  so new tasks can be inserted without renumbering.
- `Depends On` must only reference tasks that appear earlier in the same document or are explicitly
  noted as external dependencies. No forward references to uncreated tasks.

## Phase Conventions

- Each phase must produce a working or testable state. No phase should depend on a future phase
  being complete.
- Phases are ordered by dependency: earlier phases unblock later phases.
- The final phase is always validation, documentation, or cleanup.

## Numeric Prefix Convention

- New task lists under `docs/backlog/task-lists/` must use the lowest available numeric prefix
  (`NNN-`) in that directory. Do not jump to high placeholder numbers.
- Check the directory for existing files to find the next available number.

## Status Updates

- When updating status, change only the row(s) affected. Do not reorder or renumber tasks
  as a side effect of a status change.
- Mark a task `doing` before starting work on it. Only one task should be `doing` at a time
  unless tasks are explicitly independent.
- Mark a task `done` immediately after its Definition of Done is met.
- Use `blocked` when work cannot proceed due to an unresolved dependency; note the blocking
  task ID in the Definition of Done or a separate notes column.

## Prohibitions

- Do not add new backlog files under `docs/backlog/task-lists/` without a numeric prefix.
- Do not add task lists that lack a Problem Statement section. Every backlog must justify
  why the work is needed.
