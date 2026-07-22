---
name: document-cross-referencing
description: "Use when: updating cross-references, managing source document lists, fixing complement references, adding document links, verifying document references, or maintaining cross-document link consistency across the docs tree."
---

# Document Cross-Referencing

## Purpose

This skill ensures consistent cross-reference maintenance across all LumaLink documentation.
Every task list, design plan, and usage doc participates in a web of explicit references —
this skill defines the rules for keeping that web accurate, complete, and bidirectional.

## Cross-Reference Types

LumaLink docs use three cross-reference mechanisms:

| Mechanism | Where It Appears | Format |
|---|---|---|
| **Source Documents** | Task lists, design plans | Bullet list under `## Source Documents` |
| **This document complements** | Usage docs | Bullet list after `## Purpose` |
| **Inline references** | All doc types | Markdown links to specific sections or files |

## Rules

### 1. Task List → Plan Linkage

Every task list under `docs/backlog/task-lists/` must list its corresponding design plan
(if one exists) in its Source Documents section. If no plan exists yet, note that in the
Problem Statement.

### 2. Plan → Task List Linkage

Every design plan under `docs/backlog/plan/` must be referenced by at least one task list.
A plan without a path to implementation is aspirational, not actionable.

### 4. Alphabetical Sorting

Within each cross-reference list, sort entries alphabetically by filename within category.
Code files first, then docs. Within docs, sort by path.

### 5. File Existence Verification

Before adding a cross-reference, verify the target file exists in the repo.
Before removing or renaming a file, check all cross-reference lists for stale links.

### 6. Bidirectional Consistency

When doc A references doc B, doc B should reference doc A unless the relationship is
strictly one-directional (e.g., a task list references a plan, but the plan may predate
the task list). At minimum, verify that:

- Every plan is referenced by at least one task list.

- No doc references a file that does not exist.

## Update Procedure

When adding or changing a cross-reference:

1. Open the source document (the one containing the reference list).
2. Add or remove the entry in the appropriate section.
3. Verify the target file exists.
4. If adding a bidirectional relationship, update the target document as well.
5. Keep the list sorted alphabetically within category.

## Prohibitions

- Do not add a reference to a file that does not exist in the repo.
- Do not leave stale references when removing or renaming a document.
- Do not create a usage doc without cross-referencing at least one vision-plan doc.
