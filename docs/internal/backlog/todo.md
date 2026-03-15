# Internal Backlog

## In Progress / Existing

- [ ] Add bus-level config for refresh coordination (`fullRefreshOnly` / wait for all transports to finish).
- [ ] Support non-reallocating settings alteration and expose common interfaces through composite buses (primary use-case: alter shader settings on the fly).
- [x] Expose access to the factory behind static `makeBus(...)` results (for example via `getFactory(makeBus(...))`) so callers can query buffer requirements (`getBufferSize()`) and allocate external backing storage before use.
- [ ] Add a bus path that is compile-time allocatable (no runtime heap requirement) for fixed-size/static-storage deployments.

## Dedicated Backlogs

- [ ] Pallets implementation plan: [pallets.md](pallets.md)
- [ ] Aggressive internal namespace reorganization plan: [namespace-reorg-aggressive-plan.md](namespace-reorg-aggressive-plan.md)
- [ ] ColorMath utility promotion plan: [color-math-refactors.md](color-math-refactors.md)
