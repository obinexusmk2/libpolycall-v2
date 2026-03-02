# r.obinexus.org Registry Schema

## System Overview

The OBINexus Registry is a **fault-tolerant, polyglot package registry** using SemVerX versioning, DAG-based dependency resolution, and bidirectional AVL tree storage.

```
┌─────────────────────────────────────────────────────────────┐
│                   r.obinexus.org Registry                   │
│                                                             │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐             │
│  │  LIVE    │───▶│  LOCAL   │───▶│  REMOTE  │             │
│  │ (public) │    │(protected│    │ (private)│             │
│  └──────────┘    └──────────┘    └──────────┘             │
│                                                             │
│  Consumer-Observer Pattern: Max 5-10 active updates/sec   │
└─────────────────────────────────────────────────────────────┘
```

## Access Tier Architecture

### 1. LIVE Tier (Public)
- **Endpoint**: `https://r.obinexus.org/live`
- **Access**: Public read-only
- **Purpose**: Production-ready packages
- **Versioning**: `major.stable.minor.stable.patch.stable`
- **Update Frequency**: Manual releases only

### 2. LOCAL Tier (Protected)
- **Endpoint**: `https://r.obinexus.org/local` or `localhost:8080`
- **Access**: Authenticated developers
- **Purpose**: Development and testing
- **Versioning**: `major.{stable|experimental}.minor.{stable|experimental}.patch.{stable|experimental}`
- **Update Frequency**: Real-time via agha-dozie gating

### 3. REMOTE Tier (Private)
- **Endpoint**: `https://r.obinexus.org/remote`
- **Access**: SSH/OAuth2 with encryption
- **Purpose**: Private enterprise packages
- **Versioning**: Full SemVerX with custom states
- **Update Frequency**: Controlled by organization policy

---

## AVL Tree Storage Schema

```
                     Root Package Node
                    /               \
              [left]                 [right]
            major.x.y.z            major.a.b.c
           /          \           /           \
    stable branch   exp branch  stable     legacy
```

### Node Structure:
```rust
pub struct RegistryNode {
    pub package_id: String,
    pub version: SemverX,           // major.state.minor.state.patch.state
    pub left: Option<Box<RegistryNode>>,
    pub right: Option<Box<RegistryNode>>,
    pub height: usize,              // AVL balance factor
    
    // DAG Resolution
    pub dependencies: Vec<DependencyEdge>,
    pub dependents: Vec<String>,    // Reverse edges
    
    // Access Control
    pub access_tier: AccessTier,    // LIVE | LOCAL | REMOTE
    pub access_level: AccessLevel,  // PUBLIC | PROTECTED | PRIVATE
    
    // Consumer-Observer
    pub observers: Vec<ObserverId>,
    pub last_update: Timestamp,
    pub update_count: u32,          // Max 5-10 per sec
    
    // Fault Tolerance
    pub fault_state: FaultState,    // ERROR | WARNING | PANIC
    pub checksum: String,           // Integrity verification
}
```

### Operations (O(log n)):
- **Insert**: Add new package version
- **Search**: Find compatible version
- **Update**: Hot-swap with observer notification
- **Delete**: Deprecate/archive version
- **Balance**: Maintain AVL invariants

---

## DAG Dependency Resolution

### Eulerian Resolution (Edge-based)
**Purpose**: Verify all dependencies connect without conflicts

```python
def eulerian_resolve(package: Package) -> ResolutionResult:
    """
    Check if dependency graph has Eulerian path (all edges connected)
    - Even degree nodes: Safe hot-swap
    - Odd degree nodes: Warning - may break
    """
    graph = build_dependency_graph(package)
    
    odd_degree_nodes = [n for n in graph.nodes if degree(n) % 2 != 0]
    
    if len(odd_degree_nodes) == 0:
        return ResolutionResult.SAFE_SWAP
    elif len(odd_degree_nodes) == 2:
        return ResolutionResult.WARNING
    else:
        return ResolutionResult.PANIC  # Disconnected graph
```

### Hamiltonian Resolution (Node-based)
**Purpose**: Visit all dependencies to ensure complete compatibility

```python
def hamiltonian_resolve(package: Package) -> ResolutionResult:
    """
    Find Hamiltonian path (visit all nodes once)
    - Path exists: All dependencies resolvable
    - No path: Circular dependency or missing package
    """
    visited = set()
    path = []
    
    if hamiltonian_path_dfs(package.root_dep, visited, path):
        return ResolutionResult.SAFE_SWAP
    else:
        return ResolutionResult.PANIC  # Unresolvable dependency
```

### A* Resolution (Optimal Path)
**Purpose**: Find fastest safe update path with scoring

```python
def astar_resolve(current: SemverX, target: SemverX) -> UpdatePath:
    """
    A* scoring for optimal update path:
    - g(n): Cost from current to node n
    - h(n): Heuristic estimate to target
    - f(n) = g(n) + h(n)
    """
    def heuristic(version: SemverX) -> int:
        # Prefer stable > experimental > legacy
        state_cost = {
            'stable': 0,
            'experimental': 5,
            'legacy': 10
        }
        return (
            abs(target.major - version.major) * 100 +
            abs(target.minor - version.minor) * 10 +
            abs(target.patch - version.patch) +
            state_cost[version.state]
        )
    
    open_set = PriorityQueue()
    open_set.put((0, current))
    
    while not open_set.empty():
        _, current = open_set.get()
        
        if current == target:
            return reconstruct_path(current)
        
        for neighbor in get_compatible_versions(current):
            g_score = calculate_cost(current, neighbor)
            f_score = g_score + heuristic(neighbor)
            open_set.put((f_score, neighbor))
    
    return UpdatePath.NO_SAFE_PATH
```

---

## Consumer-Observer Pattern

### Observer Registration
```rust
pub struct ObserverManager {
    pub max_updates_per_sec: u32,  // Default: 5-10
    pub active_observers: HashMap<PackageId, Vec<Observer>>,
}

impl ObserverManager {
    pub fn register(&mut self, package_id: &str, observer: Observer) -> Result<(), Error> {
        let observers = self.active_observers.entry(package_id).or_insert(vec![]);
        
        if observers.len() >= 100 {
            return Err(Error::TooManyObservers);
        }
        
        observers.push(observer);
        Ok(())
    }
    
    pub fn notify(&self, package_id: &str, update: Update) -> Result<(), Error> {
        let observers = self.active_observers.get(package_id).ok_or(Error::NotFound)?;
        
        // Rate limiting: Max 5-10 updates/sec
        let now = Instant::now();
        let recent_updates = observers.iter()
            .filter(|o| now.duration_since(o.last_notified) < Duration::from_secs(1))
            .count();
        
        if recent_updates >= self.max_updates_per_sec {
            return Err(Error::RateLimitExceeded);
        }
        
        for observer in observers {
            observer.on_update(&update)?;
        }
        
        Ok(())
    }
}
```

### Update Types
1. **Opt-in**: User must explicitly upgrade
   - `major.*.*.experimental` → `major.*.*.stable`
2. **Mandatory**: Automatic security/critical updates
   - CVE patches → Force upgrade all dependents
3. **Stale Release**: Package marked outdated after N days
   - Notify observers to migrate

---

## Language Binding Schema

### Python Binding (`pysemverx`)
```python
# pysemverx/__init__.py
from semverx import SemverX, Resolver

class PyRegistry:
    def __init__(self, endpoint: str = "https://r.obinexus.org"):
        self.endpoint = endpoint
        self.resolver = Resolver()
    
    def fetch(self, package: str, version_range: str) -> Package:
        """Fetch package with SemVerX range"""
        result = self.resolver.resolve(package, version_range)
        
        if result.fault_state == FaultState.PANIC:
            raise ResolutionError(result.error_message)
        
        return result.package
    
    def subscribe(self, package: str, callback):
        """Observer pattern for updates"""
        observer = Observer(callback)
        self.register_observer(package, observer)
```

### Java Binding (`java-semverx`)
```java
// com/obinexus/semverx/Registry.java
public class Registry {
    private final String endpoint;
    private final Resolver resolver;
    
    public Registry(String endpoint) {
        this.endpoint = endpoint;
        this.resolver = new Resolver();
    }
    
    public Package fetch(String packageId, String versionRange) 
        throws ResolutionException {
        
        ResolutionResult result = resolver.resolve(packageId, versionRange);
        
        if (result.getFaultState() == FaultState.PANIC) {
            throw new ResolutionException(result.getErrorMessage());
        }
        
        return result.getPackage();
    }
    
    public void subscribe(String packageId, Consumer<Update> callback) {
        Observer observer = new Observer(callback);
        registerObserver(packageId, observer);
    }
}
```

### Lua Binding (`lua-semverx`)
```lua
-- semverx.lua
local ffi = require("ffi")
local semverx_lib = ffi.load("libsemverx")

local Registry = {}
Registry.__index = Registry

function Registry.new(endpoint)
    local self = setmetatable({}, Registry)
    self.endpoint = endpoint or "https://r.obinexus.org"
    self.resolver = semverx_lib.semverx_resolver_new()
    return self
end

function Registry:fetch(package_id, version_range)
    local result = semverx_lib.semverx_resolve(
        self.resolver,
        package_id,
        version_range
    )
    
    if result.fault_state == FaultState.PANIC then
        error(result.error_message)
    end
    
    return result.package
end

return Registry
```

---

## Fault Tolerance States

### Error Levels (0-17):
```
0:       CLEAN         - No issues
1-5:     WARNING       - Low to high warnings
6-11:    DANGER        - Critical but recoverable
12-17:   PANIC         - System failure, rollback required
```

### Fault Handling:
```rust
pub enum FaultState {
    Clean = 0,
    LowWarning = 1,
    MediumWarning = 3,
    HighWarning = 5,
    LowDanger = 6,
    CriticalDanger = 11,
    LowPanic = 12,
    SystemPanic = 17,
}

impl RegistryNode {
    pub fn handle_fault(&mut self, fault: FaultState) -> RecoveryAction {
        match fault {
            FaultState::Clean => RecoveryAction::NoAction,
            
            FaultState::LowWarning..=FaultState::HighWarning => {
                self.log_warning();
                RecoveryAction::NotifyObservers
            }
            
            FaultState::LowDanger..=FaultState::CriticalDanger => {
                self.freeze_updates();
                RecoveryAction::RequestManualReview
            }
            
            FaultState::LowPanic..=FaultState::SystemPanic => {
                self.rollback_to_last_stable();
                RecoveryAction::SystemReset
            }
        }
    }
}
```

---

## Package.json Resolution Integration

### Example: Node.js Project
```json
{
  "name": "my-app",
  "version": "1.0.0",
  "dependencies": {
    "@obinexus/core": "semverx:2.stable.1.stable.0.stable",
    "@obinexus/utils": "semverx:1.*.*.stable"
  },
  "semverx": {
    "registry": "https://r.obinexus.org",
    "policy": {
      "allow_experimental": false,
      "auto_update": "opt-in",
      "max_update_rate": 5
    }
  }
}
```

### Resolution Process:
1. Parse `semverx:` prefix in dependency
2. Query `r.obinexus.org` with version range
3. Run DAG resolution (Eulerian/Hamiltonian/A*)
4. Verify checksum and fault state
5. Install package or surface error

---

## Plugin Extension System

### Plugin Schema:
```rust
pub trait RegistryPlugin {
    fn on_install(&self, package: &Package) -> Result<(), Error>;
    fn on_update(&self, old: &Package, new: &Package) -> Result<(), Error>;
    fn on_remove(&self, package: &Package) -> Result<(), Error>;
}

// Example: Security Scanner Plugin
pub struct SecurityScanner;

impl RegistryPlugin for SecurityScanner {
    fn on_install(&self, package: &Package) -> Result<(), Error> {
        // Scan for CVEs
        if self.has_vulnerabilities(package) {
            Err(Error::SecurityViolation)
        } else {
            Ok(())
        }
    }
    
    fn on_update(&self, old: &Package, new: &Package) -> Result<(), Error> {
        // Verify update fixes known issues
        if self.fixes_vulnerabilities(old, new) {
            Ok(())
        } else {
            Err(Error::UpdateDoesNotFixVulnerabilities)
        }
    }
    
    fn on_remove(&self, package: &Package) -> Result<(), Error> {
        // Check for orphaned dependencies
        Ok(())
    }
}
```

---

## Summary

The **r.obinexus.org Registry** provides:

✅ **AVL Tree Storage**: O(log n) operations  
✅ **DAG Resolution**: Eulerian, Hamiltonian, A* strategies  
✅ **Fault Tolerance**: 0-17 error levels with auto-recovery  
✅ **Access Tiers**: LIVE/LOCAL/REMOTE with public/protected/private  
✅ **Consumer-Observer**: Max 5-10 updates/sec rate limiting  
✅ **Polyglot Bindings**: Python, Java, Lua, Node.js, Rust  
✅ **Plugin System**: Extensible with custom validation logic  
✅ **Anti-Hallucination**: Agha-Dozie pattern gating prevents false resolutions
