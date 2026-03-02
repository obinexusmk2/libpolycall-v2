# Rust Setup Guide for MSYS/MinGW on Windows

## Complete Installation & Build Instructions

### Step 1: Install Rust via rustup (MSYS Compatible)

```bash
# Open MSYS2 MinGW 64-bit terminal

# Option A: Install rustup directly
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Option B: If behind firewall, download installer manually
# Visit: https://www.rust-lang.org/tools/install
# Download: rustup-init.exe
# Run in MSYS terminal: ./rustup-init.exe

# During installation, select:
# 1) Proceed with installation (default)
# Choose toolchain: stable-x86_64-pc-windows-gnu  (for MSYS/MinGW)
```

### Step 2: Configure Environment

```bash
# Add Rust to PATH (add to ~/.bashrc or ~/.bash_profile)
echo 'export PATH="$HOME/.cargo/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc

# Verify installation
rustc --version   # Should show: rustc 1.x.x
cargo --version   # Should show: cargo 1.x.x

# Set default toolchain to GNU (MinGW-compatible)
rustup default stable-x86_64-pc-windows-gnu

# Install additional components
rustup component add rustfmt clippy rust-src
```

### Step 3: Install MSYS Dependencies

```bash
# Update MSYS package manager
pacman -Syu

# Install required build tools
pacman -S --needed \
    base-devel \
    mingw-w64-x86_64-toolchain \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-pkg-config \
    git

# For C/C++ FFI (needed for ODTS integration)
pacman -S --needed \
    mingw-w64-x86_64-gcc \
    mingw-w64-x86_64-make
```

### Step 4: Clone and Build rust-semverx

```bash
# Navigate to workspace
cd ~/obinexus/workspace

# If not already cloned
git clone https://github.com/obinexus/rust-semverx.git
cd rust-semverx

# Create backup of existing Cargo.toml
cp Cargo.toml Cargo.toml.backup

# Build project
cargo build

# Run tests
cargo test

# Build with release optimizations
cargo build --release
```

### Step 5: Cargo.toml Configuration for MSYS

```toml
[package]
name = "rust-semverx"
version = "0.1.0"
edition = "2021"
authors = ["OBINexus <obinexus@proton.me>"]

[lib]
name = "semverx"
path = "src/lib.rs"
crate-type = ["lib", "cdylib", "staticlib"]  # For polyglot FFI

[[bin]]
name = "semverx"
path = "src/bin/semverx/main.rs"

[dependencies]
petgraph = "0.6"           # DAG resolution
serde = { version = "1.0", features = ["derive"] }
toml = "0.8"
sha2 = "0.10"              # AuraSeal crypto
regex = "1.10"

[target.'cfg(windows)'.dependencies]
winapi = { version = "0.3", features = ["winuser", "wincon"] }

[dev-dependencies]
quickcheck = "1.0"         # Property testing
proptest = "1.4"
criterion = "0.5"          # Benchmarking

[profile.release]
opt-level = 3
lto = true                 # Link-time optimization
codegen-units = 1          # Better optimization

[features]
default = ["unicode-normalizer"]
unicode-normalizer = []
polyglot-ffi = []
hot-swap = []
filterflash = []
```

### Step 6: Fix Common MSYS Build Issues

#### Issue 1: Linker Errors

```bash
# If you see: "error: linking with `cc` failed"
# Solution: Explicitly set linker

# Add to .cargo/config.toml (create if doesn't exist)
mkdir -p ~/.cargo
cat > ~/.cargo/config.toml << 'EOF'
[target.x86_64-pc-windows-gnu]
linker = "x86_64-w64-mingw32-gcc"
ar = "x86_64-w64-mingw32-ar"

[build]
target = "x86_64-pc-windows-gnu"
EOF
```

#### Issue 2: CMake Integration (for ODTS/aghe-dozie)

```bash
# Your project has C components in docs/MVP-SemverX/aghe-dozie/
cd docs/MVP-SemverX/aghe-dozie

# Build C components
mkdir -p build && cd build
cmake -G "MinGW Makefiles" ..
cmake --build .

# This creates libraries that Rust can link against
```

#### Issue 3: Path Issues

```bash
# MSYS converts paths like C:\Users -> /c/Users
# For Cargo, use native Windows paths:

# If build.rs needs absolute paths:
export CARGO_MANIFEST_DIR=$(pwd -W)  # Windows-style path
```

### Step 7: Build Verification

```bash
# Clean build
cargo clean
cargo build --verbose

# Expected output:
#    Compiling semverx v0.1.0 (.../rust-semverx)
#    Finished dev [unoptimized + debuginfo] target(s) in X.XXs

# Run binary
./target/debug/semverx.exe --help

# Run tests with output
cargo test -- --nocapture
```

### Step 8: Integrate DAG Resolution Strategies

```bash
# Copy complete strategies implementation
# (Use strategies_complete.rs provided earlier)

cp /path/to/strategies_complete.rs src/resolver/strategies.rs

# Update src/resolver/mod.rs
cat >> src/resolver/mod.rs << 'EOF'

// Export strategy functions
pub use strategies::{
    is_eulerian,
    find_hamiltonian_path,
    astar_resolve,
    resolve_hybrid,
};
EOF

# Rebuild with new strategies
cargo build
```

### Step 9: Performance Verification (O(log n) Index)

```bash
# Add benchmark (create benches/dag_resolution.rs)
mkdir -p benches
cat > benches/dag_resolution.rs << 'EOF'
use criterion::{black_box, criterion_group, criterion_main, Criterion};
use semverx::resolver::DependencyGraph;

fn benchmark_astar(c: &mut Criterion) {
    c.bench_function("astar_resolve_100_nodes", |b| {
        let mut graph = DependencyGraph::new();
        // Create graph with 100 nodes
        let nodes: Vec<_> = (0..100)
            .map(|i| graph.add_node(format!("pkg-{}", i)))
            .collect();
        
        // Add edges
        for i in 0..99 {
            graph.add_edge(&nodes[i], &nodes[i + 1]);
        }
        
        b.iter(|| {
            semverx::resolver::astar_resolve(
                black_box(&graph),
                black_box(nodes[0].clone()),
                black_box(nodes[99].clone()),
            )
        });
    });
}

criterion_group!(benches, benchmark_astar);
criterion_main!(benches);
EOF

# Run benchmarks
cargo bench
```

### Step 10: FFI Integration (for PolyCall)

```bash
# Your existing FFI is in src/ffi/
# Add C header generation

# Install cbindgen
cargo install cbindgen

# Generate C headers
cbindgen --config cbindgen.toml --crate semverx --output semverx.h

# Build as shared library
cargo build --release --lib

# Output: target/release/semverx.dll (Windows)
#         target/release/libsemverx.a (static)
```

### Troubleshooting

#### Problem: "failed to run custom build command"
```bash
# Solution: Ensure CMake is in PATH
which cmake  # Should show /mingw64/bin/cmake

# If not found:
export PATH="/mingw64/bin:$PATH"
```

#### Problem: "error: could not find native static library"
```bash
# Solution: Link against correct runtime
# Add to Cargo.toml:
[target.'cfg(windows)'.dependencies]
windows-sys = "0.52"
```

#### Problem: Test failures
```bash
# Run specific test with backtrace
RUST_BACKTRACE=1 cargo test test_eulerian_detection -- --exact

# Run with logging
RUST_LOG=debug cargo test
```

### Next Steps After Setup

1. **Implement missing modules:**
   ```bash
   # Create FilterFlash module
   mkdir -p src/filterflash
   touch src/filterflash/{mod.rs,scorer.rs,canonicalizer.rs}
   
   # Create Observer Gate module  
   mkdir -p src/observer_gate
   touch src/observer_gate/{mod.rs,adjudicator.rs,fault_taxonomy.rs}
   
   # Create BiDAG module
   mkdir -p src/bidag
   touch src/bidag/{mod.rs,topology.rs,sync.rs}
   ```

2. **Set up CI/CD:**
   ```bash
   mkdir -p .github/workflows
   # Add CI workflow for Windows/MSYS builds
   ```

3. **Integrate with semverx repo:**
   ```bash
   # Your other repo at github.com/obinexus/semverx
   # Sync implementations between repositories
   ```

## Quick Reference Commands

```bash
# Daily workflow
cargo check                    # Fast syntax check
cargo build                    # Dev build
cargo test                     # Run all tests
cargo clippy                   # Linting
cargo fmt                      # Format code

# Release workflow  
cargo build --release          # Optimized build
cargo test --release          # Test release build
cargo doc --open              # Generate and view docs

# Debugging
RUST_BACKTRACE=full cargo test
cargo tree                    # Show dependency tree
cargo clean && cargo build -vv  # Verbose rebuild
```

## Common MSYS-specific Issues & Solutions

| Issue | Solution |
|-------|----------|
| `link.exe` not found | Use `x86_64-pc-windows-gnu` toolchain |
| Path separators | Use forward slashes even on Windows |
| Permission denied | Run MSYS terminal as Administrator |
| `cc` not found | Install `mingw-w64-x86_64-toolchain` |
| CMake errors | Use `-G "MinGW Makefiles"` |

---

**You're now ready to build rust-semverx on MSYS!**

Run these commands to verify everything works:
```bash
cd ~/obinexus/workspace/rust-semverx
cargo clean
cargo build
cargo test
echo "✅ Rust SemVerX setup complete!"
```
