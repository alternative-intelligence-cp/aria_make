# aria_make Tasks

**Last Updated**: 2025-12-26

This file tracks available work for contributors. See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

## ⚠️ ECOSYSTEM INTEGRATION REQUIREMENTS

**CRITICAL**: Before implementing any build system feature, check for integration dependencies:

📋 **Master Integration Map**: `../aria_ecosystem/INTEGRATION_MAP.md`

**Required Ecosystem Components for aria_make**:

1. **ecosystem/01_GlobEngine** (CRITICAL - Source Discovery)
   - Calls `ariac_glob_expand()` from aria compiler
   - Resolves `src/**/*.aria` patterns to file lists
   - **⚠️ CONFLICT**: Local spec exists at `tasks/task_02_globbing_engine.txt` (220 lines)
   - **RESOLUTION**: Ecosystem spec is authoritative (560 lines) for FFI layer
   - Use local spec for build-specific extensions only
   - Spec: `../aria_ecosystem/.internal/research/responses/01_GlobEngine.txt`
   - **Status**: NOT IMPLEMENTED

2. **ecosystem/02_StateManager** (CRITICAL - Incremental Compilation)
   - Tracks file timestamps and content hashes
   - API: `is_dirty(target)` determines rebuild necessity
   - **⚠️ VERIFY**: Implementation exists (19/19 tests passing!)
   - **ACTION REQUIRED**: Check if implementation matches ecosystem spec
   - May need to add content-addressable hashing (currently only timestamps?)
   - Spec: `../aria_ecosystem/.internal/research/responses/02_StateManager.txt`
   - **Status**: IMPLEMENTED - needs ecosystem spec verification

3. **ecosystem/03_DependencyGraph** (CRITICAL - Build Order)
   - Topological sort of compilation units using Kahn's algorithm
   - Must call aria compiler's import extraction API
   - **⚠️ VERIFY**: Dependency graph exists with cycle detection
   - **ACTION REQUIRED**: Check if it calls compiler API or duplicates parsing logic
   - Spec: `../aria_ecosystem/.internal/research/responses/03_DependencyGraph.txt`
   - **Status**: Implemented - integration method unclear

**Immediate Verification Needed**:
1. Does StateManager use content hashes or only timestamps?
2. Does DependencyGraph call compiler API or parse files itself?

**Integration Rules**:
- ✅ Must use canonical sort for reproducible builds
- ✅ Must call compiler API, NOT reimplement parsing
- ✅ Content hashing required for cache correctness
- ❌ Never use locale-dependent sorting
- ❌ Never duplicate compiler's parsing logic

See `INTEGRATION_MAP.md` for complete details.

---

## Task Format

Each task includes:
- **ID**: Unique identifier
- **Status**: AVAILABLE, CLAIMED, IN_PROGRESS, COMPLETED
- **Claimed By**: GitHub username (if claimed)
- **Spec**: Reference to specification document
- **Complexity**: LOW, MEDIUM, HIGH
- **Tier**: 1 (first-time), 2 (proven), 3 (core team)
- **Description**: What needs to be done
- **Acceptance Criteria**: How we know it's complete
- **Files**: Affected files

---

## Available Tasks

### Task Format Example

```
ID: AM-001
Status: AVAILABLE
Spec: aria_ecosystem/specs/BUILD_SYSTEM.md, Section 2.3
Complexity: MEDIUM
Tier: 2
Description: Implement basic project initialization
Acceptance Criteria:
  - Creates project directory structure
  - Generates build.aria manifest
  - Initializes git repository (optional)
  - Includes comprehensive tests
Files: src/init/init.aria, tests/test_init.aria
```

---

## Tier 1 Tasks (First-Time Contributors)

*No tasks yet. Will be populated as the project develops.*

---

## Tier 2 Tasks (Proven Contributors)

*No tasks yet. Will be populated as the project develops.*

---

## Tier 3 Tasks (Core Team Only)

*No tasks yet. Will be populated as the project develops.*

---

## Completed Tasks

### Task 2: StateManager Implementation (Claude)
**Completed**: 2025-12-24  
**Spec**: aria_ecosystem/specs/BUILD_SYSTEM.md, Section 3  
**Files**: 
- include/state/artifact_record.hpp
- include/state/state_manager.hpp
- src/state/state_manager.cpp
- tests/test_state_manager.cpp (19/19 tests passing)

**Implementation**:
- Content-addressable incremental builds
- Thread-safe operations (std::shared_mutex)
- FNV-1a + SHA-256 hashing
- 8 DirtyReason types for comprehensive change detection
- JSON state persistence

### Task 7: Build Orchestrator + StateManager Integration (Claude)
**Completed**: 2025-12-24  
**Spec**: aria_ecosystem/specs/BUILD_SYSTEM.md, Sections 4-6  
**Files**:
- include/core/build_orchestrator.hpp (346 lines)
- src/core/build_orchestrator.cpp (991 lines)
- src/main.cpp (406 lines)
- test_project/ (multi-target test case)

**Implementation**:
- INI-style build.abc parser
- Dependency graph with topological sort
- Cycle detection with full path reporting
- Parallel compilation via thread pool
- StateManager integration for incremental builds
- DOT graph export for visualization
- Progress reporting with build phases
- 6 CLI commands with comprehensive options

**Status**: Foundation complete, ready for real-world testing

---

## How to Claim a Task

1. Comment on this file in a PR or GitHub Discussion
2. State your approach and estimated timeline
3. Wait for maintainer acknowledgment
4. Create feature branch and start work
5. Reference task ID in commits and PR

---

*Tasks will be added as the project roadmap is broken down into actionable items.*
