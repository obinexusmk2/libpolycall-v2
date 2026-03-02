# pysemverx/registry.py
"""
Python binding for r.obinexus.org Registry

Usage:
    from pysemverx import Registry, SemverX

    registry = Registry("https://r.obinexus.org")
    package = registry.fetch("@obinexus/core", "2.stable.*.stable.*.stable")
    
    def on_update(update):
        print(f"Package updated: {update}")
    
    registry.subscribe("@obinexus/core", on_update)
"""

import requests
import json
from typing import Optional, Callable, Dict, List
from dataclasses import dataclass
from enum import Enum
import hashlib

class VersionState(Enum):
    STABLE = "stable"
    LEGACY = "legacy"
    EXPERIMENTAL = "experimental"

@dataclass
class SemverX:
    """SemVerX version representation"""
    major: int
    major_state: VersionState
    minor: int
    minor_state: VersionState
    patch: int
    patch_state: VersionState
    
    def __str__(self):
        return f"{self.major}.{self.major_state.value}.{self.minor}.{self.minor_state.value}.{self.patch}.{self.patch_state.value}"
    
    @classmethod
    def parse(cls, version_str: str) -> 'SemverX':
        """Parse SemVerX string like '2.stable.1.stable.0.stable'"""
        parts = version_str.split('.')
        if len(parts) != 6:
            raise ValueError(f"Invalid SemVerX format: {version_str}")
        
        return cls(
            major=int(parts[0]),
            major_state=VersionState(parts[1]),
            minor=int(parts[2]),
            minor_state=VersionState(parts[3]),
            patch=int(parts[4]),
            patch_state=VersionState(parts[5])
        )

class FaultState(Enum):
    """Fault tolerance states (0-17)"""
    CLEAN = 0
    LOW_WARNING = 1
    MEDIUM_WARNING = 3
    HIGH_WARNING = 5
    LOW_DANGER = 6
    CRITICAL_DANGER = 11
    LOW_PANIC = 12
    SYSTEM_PANIC = 17

class AccessTier(Enum):
    LIVE = "live"
    LOCAL = "local"
    REMOTE = "remote"

class AccessLevel(Enum):
    PUBLIC = "public"
    PROTECTED = "protected"
    PRIVATE = "private"

@dataclass
class Package:
    """Package metadata"""
    id: str
    version: SemverX
    name: str
    description: str
    author: str
    license: str
    tarball_url: str
    dependencies: List[str]
    checksum: str
    fault_state: FaultState

@dataclass
class Update:
    """Package update notification"""
    package_id: str
    old_version: Optional[SemverX]
    new_version: SemverX
    update_type: str  # "opt-in" | "mandatory" | "stale-release"

class ResolutionStrategy(Enum):
    """DAG resolution strategies"""
    EULERIAN = "eulerian"
    HAMILTONIAN = "hamiltonian"
    ASTAR = "astar"
    HYBRID = "hybrid"

class ResolutionError(Exception):
    """Raised when dependency resolution fails"""
    pass

class Registry:
    """r.obinexus.org Registry Client"""
    
    def __init__(
        self,
        endpoint: str = "https://r.obinexus.org",
        access_tier: AccessTier = AccessTier.LIVE,
        auth_token: Optional[str] = None
    ):
        self.endpoint = endpoint
        self.access_tier = access_tier
        self.auth_token = auth_token
        self.observers: Dict[str, List[Callable]] = {}
        self.session = requests.Session()
        
        if auth_token:
            self.session.headers.update({
                "Authorization": f"Bearer {auth_token}"
            })
    
    def fetch(
        self,
        package_id: str,
        version_range: str,
        strategy: ResolutionStrategy = ResolutionStrategy.HYBRID
    ) -> Package:
        """
        Fetch package with SemVerX version range
        
        Args:
            package_id: Package identifier (e.g., "@obinexus/core")
            version_range: SemVerX range (e.g., "2.stable.*.stable.*.stable")
            strategy: DAG resolution strategy
            
        Returns:
            Package object
            
        Raises:
            ResolutionError: If dependency resolution fails
        """
        url = f"{self.endpoint}/{self.access_tier.value}/packages/{package_id}"
        params = {
            "version": version_range,
            "strategy": strategy.value
        }
        
        response = self.session.get(url, params=params)
        
        if response.status_code == 404:
            raise ResolutionError(f"Package not found: {package_id}")
        
        if response.status_code != 200:
            raise ResolutionError(f"Failed to fetch package: {response.text}")
        
        data = response.json()
        
        # Check fault state
        fault_state = FaultState(data.get('fault_state', 0))
        if fault_state.value >= FaultState.SYSTEM_PANIC.value:
            raise ResolutionError(
                f"Package in PANIC state: {data.get('error_message', 'Unknown error')}"
            )
        
        # Parse response
        return Package(
            id=data['id'],
            version=SemverX.parse(data['version']),
            name=data['name'],
            description=data['description'],
            author=data['author'],
            license=data['license'],
            tarball_url=data['tarball_url'],
            dependencies=data.get('dependencies', []),
            checksum=data['checksum'],
            fault_state=fault_state
        )
    
    def install(
        self,
        package_id: str,
        version_range: str,
        verify_checksum: bool = True
    ) -> Package:
        """
        Install package with dependency resolution
        
        Args:
            package_id: Package identifier
            version_range: SemVerX version range
            verify_checksum: Verify package integrity
            
        Returns:
            Installed package
        """
        package = self.fetch(package_id, version_range)
        
        if verify_checksum:
            self._verify_checksum(package)
        
        # Download tarball
        tarball_response = self.session.get(package.tarball_url)
        tarball_response.raise_for_status()
        
        # Extract and install (simplified)
        print(f"Installing {package.name} v{package.version}...")
        
        # Resolve dependencies recursively
        for dep in package.dependencies:
            dep_id, dep_version = dep.split('@')
            self.install(dep_id, dep_version, verify_checksum=verify_checksum)
        
        return package
    
    def subscribe(
        self,
        package_id: str,
        callback: Callable[[Update], None]
    ) -> str:
        """
        Subscribe to package updates (Consumer-Observer pattern)
        
        Args:
            package_id: Package to observe
            callback: Function called on update
            
        Returns:
            Observer ID
        """
        if package_id not in self.observers:
            self.observers[package_id] = []
        
        self.observers[package_id].append(callback)
        
        # Register with server
        url = f"{self.endpoint}/{self.access_tier.value}/subscribe"
        response = self.session.post(url, json={
            "package_id": package_id
        })
        
        if response.status_code != 200:
            raise Exception(f"Failed to subscribe: {response.text}")
        
        observer_id = response.json()['observer_id']
        return observer_id
    
    def unsubscribe(self, package_id: str, observer_id: str):
        """Unsubscribe from package updates"""
        url = f"{self.endpoint}/{self.access_tier.value}/unsubscribe/{observer_id}"
        self.session.delete(url)
        
        if package_id in self.observers:
            del self.observers[package_id]
    
    def _verify_checksum(self, package: Package):
        """Verify package integrity"""
        tarball_response = self.session.get(package.tarball_url)
        tarball_data = tarball_response.content
        
        calculated_checksum = hashlib.sha256(tarball_data).hexdigest()
        
        if calculated_checksum != package.checksum:
            raise ResolutionError(
                f"Checksum mismatch for {package.id}: "
                f"expected {package.checksum}, got {calculated_checksum}"
            )
    
    def resolve_dag(
        self,
        package_id: str,
        strategy: ResolutionStrategy = ResolutionStrategy.HYBRID
    ) -> Dict:
        """
        Resolve dependency DAG
        
        Returns:
            DAG resolution result with path and fault state
        """
        url = f"{self.endpoint}/{self.access_tier.value}/resolve"
        response = self.session.post(url, json={
            "package_id": package_id,
            "strategy": strategy.value
        })
        
        response.raise_for_status()
        return response.json()

# ==================== CLI Tool ====================

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="r.obinexus.org Registry CLI")
    parser.add_argument("command", choices=["fetch", "install", "resolve"])
    parser.add_argument("package", help="Package ID")
    parser.add_argument("--version", default="*.*.*", help="Version range")
    parser.add_argument("--endpoint", default="https://r.obinexus.org")
    parser.add_argument("--tier", choices=["live", "local", "remote"], default="live")
    
    args = parser.parse_args()
    
    registry = Registry(
        endpoint=args.endpoint,
        access_tier=AccessTier(args.tier)
    )
    
    if args.command == "fetch":
        package = registry.fetch(args.package, args.version)
        print(f"Fetched: {package.name} v{package.version}")
        print(f"Fault state: {package.fault_state.name}")
    
    elif args.command == "install":
        package = registry.install(args.package, args.version)
        print(f"Installed: {package.name} v{package.version}")
    
    elif args.command == "resolve":
        result = registry.resolve_dag(args.package)
        print(f"Resolution path: {' -> '.join(result['path'])}")
        print(f"Fault state: {result['fault_state']}")
