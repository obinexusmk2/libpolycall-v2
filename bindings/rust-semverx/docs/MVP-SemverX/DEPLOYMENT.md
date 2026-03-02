# r.obinexus.org Registry Deployment Guide

## Overview

This guide covers deploying the OBINexus SemVerX Registry with:
- **Live Tier**: Production registry at r.obinexus.org/live
- **Local Tier**: Development server at r.obinexus.org/local or localhost:8080
- **Remote Tier**: Private enterprise server at r.obinexus.org/remote

---

## Prerequisites

### System Requirements
- **OS**: Ubuntu 24.04 LTS (or compatible Linux)
- **RAM**: Minimum 4GB, recommended 8GB
- **Storage**: Minimum 20GB SSD
- **CPU**: 2+ cores

### Software Requirements
```bash
# Rust toolchain
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
rustup default stable

# Node.js (v20+)
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt-get install -y nodejs

# Python (3.11+)
sudo apt-get install python3.11 python3.11-venv python3-pip

# CMake (3.25+)
sudo apt-get install cmake build-essential

# PostgreSQL (for package metadata)
sudo apt-get install postgresql postgresql-contrib

# Nginx (reverse proxy)
sudo apt-get install nginx

# SSL certificates (Let's Encrypt)
sudo apt-get install certbot python3-certbot-nginx
```

---

## Repository Structure

```
/opt/obinexus/
├── registry-server/         # Rust registry server
│   ├── src/
│   │   ├── main.rs
│   │   ├── registry_server.rs  # Core implementation
│   │   ├── dag_resolver.rs
│   │   └── observer_manager.rs
│   ├── Cargo.toml
│   └── config/
│       ├── live.toml
│       ├── local.toml
│       └── remote.toml
│
├── agha-dozie/              # Pattern gating + ODTS
│   ├── CMakeLists.txt
│   ├── gatogi_algorithm.c
│   ├── pattern_gating_odts.c
│   └── fault_tolerance_integration.c
│
├── bindings/
│   ├── python/
│   │   ├── pysemverx/
│   │   │   ├── __init__.py
│   │   │   └── registry.py
│   │   └── setup.py
│   │
│   ├── node/
│   │   ├── @obinexus/registry/
│   │   │   ├── index.ts
│   │   │   └── package.json
│   │   └── tsconfig.json
│   │
│   └── lua/
│       └── semverx.lua
│
└── web/                     # Website + API docs
    ├── about.html
    ├── api-docs/
    └── assets/
```

---

## Step 1: Database Setup

### PostgreSQL Configuration

```bash
# Create database user
sudo -u postgres createuser obinexus_registry --pwprompt

# Create databases for each tier
sudo -u postgres createdb registry_live --owner=obinexus_registry
sudo -u postgres createdb registry_local --owner=obinexus_registry
sudo -u postgres createdb registry_remote --owner=obinexus_registry

# Initialize schema
psql -U obinexus_registry -d registry_live -f /opt/obinexus/registry-server/schema.sql
```

### Schema (schema.sql):
```sql
-- Package metadata table
CREATE TABLE packages (
    id VARCHAR(255) PRIMARY KEY,
    version VARCHAR(100) NOT NULL,
    major INT NOT NULL,
    major_state VARCHAR(20) NOT NULL,
    minor INT NOT NULL,
    minor_state VARCHAR(20) NOT NULL,
    patch INT NOT NULL,
    patch_state VARCHAR(20) NOT NULL,
    
    name VARCHAR(255) NOT NULL,
    description TEXT,
    author VARCHAR(255),
    license VARCHAR(100),
    tarball_url TEXT NOT NULL,
    checksum VARCHAR(64) NOT NULL,
    
    access_tier VARCHAR(20) NOT NULL,
    access_level VARCHAR(20) NOT NULL,
    fault_state INT DEFAULT 0,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    UNIQUE(id, version)
);

-- Dependency edges table
CREATE TABLE dependencies (
    id SERIAL PRIMARY KEY,
    package_id VARCHAR(255) NOT NULL,
    package_version VARCHAR(100) NOT NULL,
    dependency_id VARCHAR(255) NOT NULL,
    version_range VARCHAR(100) NOT NULL,
    optional BOOLEAN DEFAULT FALSE,
    
    FOREIGN KEY (package_id, package_version) 
        REFERENCES packages(id, version) ON DELETE CASCADE
);

-- Observer subscriptions table
CREATE TABLE observers (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    package_id VARCHAR(255) NOT NULL,
    callback_url TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_notified TIMESTAMP
);

-- Indices for performance
CREATE INDEX idx_packages_version ON packages(major, minor, patch);
CREATE INDEX idx_packages_state ON packages(major_state, minor_state, patch_state);
CREATE INDEX idx_dependencies_package ON dependencies(package_id, package_version);
CREATE INDEX idx_observers_package ON observers(package_id);
```

---

## Step 2: Build Registry Server

### Rust Server Build

```bash
cd /opt/obinexus/registry-server

# Build in release mode
cargo build --release

# Binary will be at target/release/registry-server
```

### Configuration (config/live.toml):
```toml
[server]
host = "0.0.0.0"
port = 8080
workers = 4

[database]
url = "postgresql://obinexus_registry:PASSWORD@localhost/registry_live"
max_connections = 10

[storage]
tarball_dir = "/var/lib/obinexus/tarballs"
cache_dir = "/var/cache/obinexus/registry"

[access]
tier = "live"
default_level = "public"

[observer]
max_updates_per_sec = 10
notification_timeout_ms = 5000

[fault_tolerance]
max_panic_level = 17
auto_rollback = true
```

---

## Step 3: Build Language Bindings

### Python Binding

```bash
cd /opt/obinexus/bindings/python

# Create virtual environment
python3 -m venv venv
source venv/bin/activate

# Install dependencies
pip install --upgrade pip
pip install requests setuptools wheel

# Build package
python setup.py sdist bdist_wheel

# Install locally for testing
pip install -e .

# Publish to PyPI (later)
# twine upload dist/*
```

### Node.js Binding

```bash
cd /opt/obinexus/bindings/node/@obinexus/registry

# Install dependencies
npm install

# Build TypeScript
npm run build

# Test locally
npm test

# Publish to npm (later)
# npm publish --access public
```

### Agha-Dozie (C/C++)

```bash
cd /opt/obinexus/agha-dozie

# Configure with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

# Build
cmake --build build --parallel

# Run tests
cd build && ctest --output-on-failure

# Install system-wide
sudo cmake --install build
```

---

## Step 4: Nginx Configuration

### r.obinexus.org Nginx Config

```nginx
# /etc/nginx/sites-available/r.obinexus.org

upstream registry_live {
    server 127.0.0.1:8080;
}

upstream registry_local {
    server 127.0.0.1:8081;
}

upstream registry_remote {
    server 127.0.0.1:8082;
}

# Redirect HTTP to HTTPS
server {
    listen 80;
    server_name r.obinexus.org;
    return 301 https://$server_name$request_uri;
}

# HTTPS Server
server {
    listen 443 ssl http2;
    server_name r.obinexus.org;

    ssl_certificate /etc/letsencrypt/live/r.obinexus.org/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/r.obinexus.org/privkey.pem;
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers HIGH:!aNULL:!MD5;

    # Security headers
    add_header Strict-Transport-Security "max-age=31536000; includeSubDomains" always;
    add_header X-Frame-Options "DENY" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header X-XSS-Protection "1; mode=block" always;

    # Live tier (public)
    location /live/ {
        proxy_pass http://registry_live/;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        
        # CORS for public API
        add_header Access-Control-Allow-Origin "*";
        add_header Access-Control-Allow-Methods "GET, POST, OPTIONS";
    }

    # Local tier (protected, requires auth)
    location /local/ {
        auth_request /auth;
        
        proxy_pass http://registry_local/;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }

    # Remote tier (private, SSH/OAuth2 only)
    location /remote/ {
        auth_request /oauth;
        
        proxy_pass http://registry_remote/;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }

    # Website
    location / {
        root /var/www/r.obinexus.org;
        index index.html;
        try_files $uri $uri/ =404;
    }

    # Auth endpoint (implement OAuth2 verification)
    location = /auth {
        internal;
        proxy_pass http://127.0.0.1:9000/verify;
    }

    location = /oauth {
        internal;
        proxy_pass http://127.0.0.1:9001/verify;
    }
}
```

```bash
# Enable site
sudo ln -s /etc/nginx/sites-available/r.obinexus.org /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

---

## Step 5: SSL Certificate

```bash
# Get Let's Encrypt certificate
sudo certbot --nginx -d r.obinexus.org

# Auto-renewal is configured automatically
sudo certbot renew --dry-run
```

---

## Step 6: Systemd Services

### Registry Server Service

```ini
# /etc/systemd/system/registry-live.service

[Unit]
Description=OBINexus Registry (Live Tier)
After=network.target postgresql.service

[Service]
Type=simple
User=obinexus
WorkingDirectory=/opt/obinexus/registry-server
Environment="RUST_LOG=info"
ExecStart=/opt/obinexus/registry-server/target/release/registry-server \
    --config=/opt/obinexus/registry-server/config/live.toml
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

```bash
# Create similar services for local and remote tiers
sudo systemctl daemon-reload
sudo systemctl enable registry-live
sudo systemctl start registry-live
sudo systemctl status registry-live
```

---

## Step 7: Testing

### Test Live Endpoint

```bash
# Health check
curl https://r.obinexus.org/live/health

# Fetch package
curl https://r.obinexus.org/live/packages/@obinexus/core?version=2.stable.*.*.*

# Resolve dependencies
curl -X POST https://r.obinexus.org/live/resolve \
  -H "Content-Type: application/json" \
  -d '{"package_id": "@obinexus/core", "strategy": "hybrid"}'
```

### Test Python Binding

```python
from pysemverx import Registry

registry = Registry("https://r.obinexus.org")
package = registry.fetch("@obinexus/core", "2.stable.*.stable.*.stable")
print(f"Fetched: {package.name} v{package.version}")
```

### Test Node.js Binding

```javascript
import { Registry } from '@obinexus/registry';

const registry = new Registry({
  endpoint: 'https://r.obinexus.org'
});

const pkg = await registry.fetch('@obinexus/core', '2.stable.*.stable.*.stable');
console.log(`Fetched: ${pkg.name} v${pkg.version}`);
```

---

## Step 8: Monitoring

### Prometheus Metrics

```bash
# Install Prometheus
sudo apt-get install prometheus

# Configure scraping
# /etc/prometheus/prometheus.yml
scrape_configs:
  - job_name: 'registry'
    static_configs:
      - targets: ['localhost:8080', 'localhost:8081', 'localhost:8082']
```

### Grafana Dashboards

```bash
# Install Grafana
sudo apt-get install grafana

# Start Grafana
sudo systemctl enable grafana-server
sudo systemctl start grafana-server

# Access at http://localhost:3000
# Default login: admin/admin
```

---

## Step 9: Backup Strategy

```bash
# Automated PostgreSQL backups
sudo crontab -e

# Daily backup at 2 AM
0 2 * * * pg_dump -U obinexus_registry registry_live | gzip > /backup/registry_live_$(date +\%Y\%m\%d).sql.gz

# Tarball storage backup (weekly)
0 3 * * 0 rsync -av /var/lib/obinexus/tarballs/ /backup/tarballs/
```

---

## Step 10: Security Hardening

### Firewall Rules

```bash
# Allow only necessary ports
sudo ufw default deny incoming
sudo ufw default allow outgoing
sudo ufw allow ssh
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw enable
```

### Fail2Ban

```bash
# Install fail2ban
sudo apt-get install fail2ban

# Configure for nginx
sudo cp /etc/fail2ban/jail.conf /etc/fail2ban/jail.local
# Edit jail.local to enable nginx protections
sudo systemctl enable fail2ban
sudo systemctl start fail2ban
```

---

## Summary

✅ **Live Tier**: Production registry at https://r.obinexus.org/live  
✅ **Local Tier**: Development at https://r.obinexus.org/local  
✅ **Remote Tier**: Private at https://r.obinexus.org/remote  
✅ **Language Bindings**: Python, Node.js, Lua  
✅ **DAG Resolution**: Eulerian, Hamiltonian, A*  
✅ **Fault Tolerance**: 0-17 error levels  
✅ **Observer Pattern**: Max 5-10 updates/sec  
✅ **SSL/TLS**: Let's Encrypt certificates  
✅ **Monitoring**: Prometheus + Grafana

---

## Next Steps

1. **Publish Bindings**: Upload to PyPI and npm
2. **Community Testing**: Beta test with OBINexus developers
3. **Documentation**: Complete API docs at r.obinexus.org/docs
4. **CI/CD**: GitHub Actions for automated deployment
5. **Scaling**: Add load balancing and Redis caching

---

**OBINexus Registry** is now live! 🚀
