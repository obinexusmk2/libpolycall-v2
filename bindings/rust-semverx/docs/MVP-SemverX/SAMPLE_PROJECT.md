# Sample Project Using r.obinexus.org Registry

## package.json (Node.js Project)

```json
{
  "name": "my-obinexus-app",
  "version": "1.0.0",
  "description": "Sample application using OBINexus Registry",
  "main": "dist/index.js",
  "scripts": {
    "start": "node dist/index.js",
    "build": "tsc",
    "install": "obinexus-install",
    "update": "obinexus-update --strategy=hybrid"
  },
  "dependencies": {
    "@obinexus/core": "semverx:2.stable.1.stable.0.stable",
    "@obinexus/utils": "semverx:1.*.*.stable",
    "@obinexus/http": "semverx:3.stable.*.*.{stable|experimental}"
  },
  "devDependencies": {
    "@obinexus/registry": "^1.0.0",
    "typescript": "^5.3.2"
  },
  "semverx": {
    "registry": "https://r.obinexus.org",
    "access_tier": "live",
    "policy": {
      "allow_experimental": false,
      "allow_legacy": true,
      "auto_update": "opt-in",
      "max_update_rate": 10
    },
    "resolution_strategy": "hybrid",
    "fault_tolerance": {
      "max_panic_level": 11,
      "auto_rollback": true,
      "retry_on_failure": 3
    },
    "observers": {
      "@obinexus/core": {
        "on_update": "npm run rebuild",
        "notify_method": "webhook",
        "webhook_url": "https://my-app.com/hooks/package-update"
      }
    }
  },
  "engines": {
    "node": ">=20.0.0"
  }
}
```

## pyproject.toml (Python Project)

```toml
[project]
name = "my-obinexus-app"
version = "1.0.0"
description = "Sample Python application using OBINexus Registry"
requires-python = ">=3.11"
dependencies = [
    "obinexus-core==semverx:2.stable.1.stable.0.stable",
    "obinexus-utils==semverx:1.*.*.stable",
    "pysemverx>=1.0.0"
]

[tool.semverx]
registry = "https://r.obinexus.org"
access_tier = "live"

[tool.semverx.policy]
allow_experimental = false
auto_update = "opt-in"
max_update_rate = 10

[tool.semverx.resolution]
strategy = "hybrid"

[tool.semverx.fault_tolerance]
max_panic_level = 11
auto_rollback = true

[tool.semverx.observers]
"obinexus-core" = { on_update = "pip install --upgrade", notify_method = "callback" }
```

## Cargo.toml (Rust Project)

```toml
[package]
name = "my-obinexus-app"
version = "1.0.0"
edition = "2021"

[dependencies]
obinexus-core = { registry = "semverx", version = "2.stable.1.stable.0.stable" }
obinexus-utils = { registry = "semverx", version = "1.*.*.stable" }
semverx = "1.0"

[semverx]
registry = "https://r.obinexus.org"
access_tier = "live"

[semverx.policy]
allow_experimental = false
auto_update = "opt-in"
max_update_rate = 10

[semverx.resolution]
strategy = "hybrid"

[semverx.fault_tolerance]
max_panic_level = 11
auto_rollback = true
```

---

## Usage Examples

### Node.js Example

```javascript
// src/index.ts
import { Registry, SemverX } from '@obinexus/registry';
import core from '@obinexus/core';

async function main() {
  // Initialize registry client
  const registry = new Registry({
    endpoint: 'https://r.obinexus.org',
    accessTier: 'live'
  });

  // Subscribe to package updates
  registry.subscribe('@obinexus/core', (update) => {
    console.log(`Package update available: ${update.newVersion}`);
    
    if (update.updateType === 'mandatory') {
      console.log('Critical update - auto-installing...');
      registry.install('@obinexus/core', update.newVersion);
    } else {
      console.log('Optional update - run `npm update` to install');
    }
  });

  // Use the package
  const result = core.doSomething();
  console.log('Result:', result);
}

main().catch(console.error);
```

### Python Example

```python
# main.py
from pysemverx import Registry, SemverX
import obinexus_core

def main():
    # Initialize registry client
    registry = Registry(
        endpoint="https://r.obinexus.org",
        access_tier="live"
    )
    
    # Subscribe to package updates
    def on_update(update):
        print(f"Package update available: {update.new_version}")
        
        if update.update_type == "mandatory":
            print("Critical update - auto-installing...")
            registry.install("obinexus-core", str(update.new_version))
        else:
            print("Optional update - run `pip install --upgrade` to update")
    
    registry.subscribe("obinexus-core", on_update)
    
    # Use the package
    result = obinexus_core.do_something()
    print(f"Result: {result}")

if __name__ == "__main__":
    main()
```

### Rust Example

```rust
// src/main.rs
use obinexus_core;
use semverx::{Registry, SemverX};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Initialize registry client
    let registry = Registry::new("https://r.obinexus.org", AccessTier::Live)?;
    
    // Subscribe to package updates
    registry.subscribe("obinexus-core", |update| {
        println!("Package update available: {:?}", update.new_version);
        
        if update.update_type == UpdateType::Mandatory {
            println!("Critical update - auto-installing...");
            // Auto-install logic here
        } else {
            println!("Optional update - run `cargo update` to install");
        }
    }).await?;
    
    // Use the package
    let result = obinexus_core::do_something();
    println!("Result: {:?}", result);
    
    Ok(())
}
```

---

## Custom Update Policies

### Opt-in Updates (Default)
```json
{
  "semverx": {
    "policy": {
      "auto_update": "opt-in"
    }
  }
}
```

User must explicitly run `npm update` or equivalent.

### Mandatory Critical Updates
```json
{
  "semverx": {
    "policy": {
      "auto_update": "mandatory",
      "mandatory_rules": {
        "security": true,
        "fault_state_threshold": 11
      }
    }
  }
}
```

Critical security updates and high-danger faults auto-install.

### Stale Release Notifications
```json
{
  "semverx": {
    "policy": {
      "stale_warning_days": 90,
      "stale_action": "notify"
    }
  }
}
```

Warn users if dependencies haven't been updated in 90 days.

---

## DAG Resolution Strategies

### Eulerian (Edge-based)
Best for: Verifying all dependencies connect properly

```json
{
  "semverx": {
    "resolution_strategy": "eulerian"
  }
}
```

### Hamiltonian (Node-based)
Best for: Ensuring all packages are reachable

```json
{
  "semverx": {
    "resolution_strategy": "hamiltonian"
  }
}
```

### A* (Optimal Path)
Best for: Finding fastest safe update path

```json
{
  "semverx": {
    "resolution_strategy": "astar"
  }
}
```

### Hybrid (Recommended)
Combines all strategies for best results

```json
{
  "semverx": {
    "resolution_strategy": "hybrid"
  }
}
```

---

## Fault Tolerance Configuration

### Error Levels
```json
{
  "semverx": {
    "fault_tolerance": {
      "max_panic_level": 11,
      "auto_rollback": true,
      "retry_on_failure": 3,
      "rollback_strategy": "last_stable"
    }
  }
}
```

- **0-5**: Warnings (continue operation)
- **6-11**: Danger (freeze updates, manual review)
- **12-17**: Panic (auto-rollback to last stable)

---

## Access Tiers

### Live (Production)
```javascript
const registry = new Registry({
  endpoint: 'https://r.obinexus.org',
  accessTier: 'live'  // Public, read-only
});
```

### Local (Development)
```javascript
const registry = new Registry({
  endpoint: 'https://r.obinexus.org',
  accessTier: 'local',  // Protected, requires auth
  authToken: process.env.OBINEXUS_TOKEN
});
```

### Remote (Private Enterprise)
```javascript
const registry = new Registry({
  endpoint: 'https://r.obinexus.org',
  accessTier: 'remote',  // Private, SSH/OAuth2
  authToken: process.env.OBINEXUS_ENTERPRISE_TOKEN
});
```

---

## Monitoring & Observability

### Health Check
```bash
curl https://r.obinexus.org/live/health
```

### Package Search
```bash
curl "https://r.obinexus.org/live/search?q=obinexus-core"
```

### Dependency Graph
```bash
curl "https://r.obinexus.org/live/graph/@obinexus/core"
```

### Observer Dashboard
```bash
curl "https://r.obinexus.org/live/observers/@obinexus/core"
```

---

## Summary

The **r.obinexus.org Registry** provides:

✅ SemVerX versioning with state-based resolution  
✅ DAG dependency resolution (Eulerian, Hamiltonian, A*)  
✅ Consumer-Observer pattern with rate limiting  
✅ Multi-tier access control (Live, Local, Remote)  
✅ Fault-tolerant updates (0-17 error levels)  
✅ Polyglot bindings (Node.js, Python, Rust, Lua)  
✅ Anti-hallucination pattern gating (agha-dozie)  
✅ ODTS integration for mathematical soundness

For more information, visit https://r.obinexus.org
