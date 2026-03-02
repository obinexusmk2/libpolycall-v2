// registry_server.rs
//! r.obinexus.org Registry Server
//! 
//! Fault-tolerant package registry with:
//! - AVL tree storage (O(log n) operations)
//! - DAG dependency resolution
//! - Consumer-observer pattern
//! - Multi-language bindings

use std::collections::HashMap;
use std::sync::{Arc, RwLock};
use std::time::{Duration, Instant};

// ==================== Core Data Structures ====================

/// SemVerX version representation
/// Format: major.{stable|legacy|experimental}.minor.{...}.patch.{...}
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct SemverX {
    pub major: u32,
    pub major_state: VersionState,
    pub minor: u32,
    pub minor_state: VersionState,
    pub patch: u32,
    pub patch_state: VersionState,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum VersionState {
    Stable,
    Legacy,
    Experimental,
}

impl VersionState {
    fn cost(&self) -> u32 {
        match self {
            VersionState::Stable => 0,
            VersionState::Experimental => 5,
            VersionState::Legacy => 10,
        }
    }
}

/// Access tier for registry endpoints
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AccessTier {
    Live,      // https://r.obinexus.org/live
    Local,     // https://r.obinexus.org/local or localhost:8080
    Remote,    // https://r.obinexus.org/remote (SSH/OAuth2)
}

/// Access level for packages
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AccessLevel {
    Public,
    Protected,
    Private,
}

/// Fault tolerance states (0-17)
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
#[repr(u8)]
pub enum FaultState {
    Clean = 0,
    LowWarning = 1,
    LowMediumWarning = 2,
    MediumWarning = 3,
    MediumHighWarning = 4,
    HighWarning = 5,
    LowDanger = 6,
    LowMediumDanger = 7,
    MediumDanger = 8,
    MediumHighDanger = 9,
    HighDanger = 10,
    CriticalDanger = 11,
    LowPanic = 12,
    LowMediumPanic = 13,
    MediumPanic = 14,
    MediumHighPanic = 15,
    HighPanic = 16,
    SystemPanic = 17,
}

/// Recovery actions based on fault state
#[derive(Debug, Clone, Copy)]
pub enum RecoveryAction {
    NoAction,
    NotifyObservers,
    RequestManualReview,
    FreezeUpdates,
    RollbackToStable,
    SystemReset,
}

/// AVL Tree node for package storage
pub struct RegistryNode {
    pub package_id: String,
    pub version: SemverX,
    pub metadata: PackageMetadata,
    
    // AVL tree structure
    pub left: Option<Box<RegistryNode>>,
    pub right: Option<Box<RegistryNode>>,
    pub height: usize,
    
    // DAG edges
    pub dependencies: Vec<DependencyEdge>,
    pub dependents: Vec<String>,
    
    // Access control
    pub access_tier: AccessTier,
    pub access_level: AccessLevel,
    
    // Consumer-Observer
    pub observers: Vec<ObserverId>,
    pub last_update: Instant,
    pub update_count: u32,
    
    // Fault tolerance
    pub fault_state: FaultState,
    pub checksum: String,
}

#[derive(Debug, Clone)]
pub struct PackageMetadata {
    pub name: String,
    pub description: String,
    pub author: String,
    pub license: String,
    pub tarball_url: String,
    pub install_script: Option<String>,
}

#[derive(Debug, Clone)]
pub struct DependencyEdge {
    pub target_id: String,
    pub version_range: VersionRange,
    pub optional: bool,
}

#[derive(Debug, Clone)]
pub struct VersionRange {
    pub min: SemverX,
    pub max: Option<SemverX>,
    pub allowed_states: Vec<VersionState>,
}

pub type ObserverId = uuid::Uuid;

// ==================== AVL Tree Implementation ====================

impl RegistryNode {
    fn new(
        package_id: String,
        version: SemverX,
        metadata: PackageMetadata,
        access_tier: AccessTier,
        access_level: AccessLevel,
    ) -> Self {
        Self {
            package_id,
            version,
            metadata,
            left: None,
            right: None,
            height: 1,
            dependencies: Vec::new(),
            dependents: Vec::new(),
            access_tier,
            access_level,
            observers: Vec::new(),
            last_update: Instant::now(),
            update_count: 0,
            fault_state: FaultState::Clean,
            checksum: String::new(),
        }
    }
    
    fn height_of(node: &Option<Box<RegistryNode>>) -> usize {
        node.as_ref().map(|n| n.height).unwrap_or(0)
    }
    
    fn balance_factor(&self) -> i32 {
        Self::height_of(&self.left) as i32 - Self::height_of(&self.right) as i32
    }
    
    fn update_height(&mut self) {
        self.height = 1 + std::cmp::max(
            Self::height_of(&self.left),
            Self::height_of(&self.right)
        );
    }
    
    fn rotate_right(mut self: Box<Self>) -> Box<RegistryNode> {
        let mut new_root = self.left.take().unwrap();
        self.left = new_root.right.take();
        self.update_height();
        new_root.right = Some(self);
        new_root.update_height();
        new_root
    }
    
    fn rotate_left(mut self: Box<Self>) -> Box<RegistryNode> {
        let mut new_root = self.right.take().unwrap();
        self.right = new_root.left.take();
        self.update_height();
        new_root.left = Some(self);
        new_root.update_height();
        new_root
    }
    
    fn rebalance(mut self: Box<Self>) -> Box<RegistryNode> {
        self.update_height();
        let balance = self.balance_factor();
        
        if balance > 1 {
            let left = self.left.as_mut().unwrap();
            if left.balance_factor() < 0 {
                self.left = Some(left.as_ref().clone().rotate_left());
            }
            return self.rotate_right();
        }
        
        if balance < -1 {
            let right = self.right.as_mut().unwrap();
            if right.balance_factor() > 0 {
                self.right = Some(right.as_ref().clone().rotate_right());
            }
            return self.rotate_left();
        }
        
        self
    }
}

// ==================== DAG Resolution ====================

pub struct DagResolver {
    resolution_cache: HashMap<String, ResolutionResult>,
}

#[derive(Debug, Clone)]
pub enum ResolutionStrategy {
    Eulerian,    // Check all edges connect
    Hamiltonian, // Visit all nodes once
    AStar,       // Optimal path with heuristic
    Hybrid,      // Combine strategies
}

#[derive(Debug, Clone)]
pub struct ResolutionResult {
    pub package: Option<RegistryNode>,
    pub path: Vec<String>,
    pub fault_state: FaultState,
    pub error_message: Option<String>,
}

impl DagResolver {
    pub fn new() -> Self {
        Self {
            resolution_cache: HashMap::new(),
        }
    }
    
    /// Eulerian resolution: Verify all dependency edges connect
    pub fn eulerian_resolve(&self, package_id: &str, graph: &DependencyGraph) 
        -> ResolutionResult 
    {
        let odd_degree_nodes = graph.nodes
            .iter()
            .filter(|(_, node)| self.degree(node, graph) % 2 != 0)
            .count();
        
        match odd_degree_nodes {
            0 => ResolutionResult {
                package: graph.nodes.get(package_id).cloned(),
                path: vec![package_id.to_string()],
                fault_state: FaultState::Clean,
                error_message: None,
            },
            2 => ResolutionResult {
                package: graph.nodes.get(package_id).cloned(),
                path: vec![package_id.to_string()],
                fault_state: FaultState::MediumWarning,
                error_message: Some("Semi-Eulerian path detected".to_string()),
            },
            _ => ResolutionResult {
                package: None,
                path: vec![],
                fault_state: FaultState::SystemPanic,
                error_message: Some("Disconnected dependency graph".to_string()),
            },
        }
    }
    
    /// Hamiltonian resolution: Visit all dependency nodes once
    pub fn hamiltonian_resolve(&self, package_id: &str, graph: &DependencyGraph) 
        -> ResolutionResult 
    {
        let mut visited = std::collections::HashSet::new();
        let mut path = Vec::new();
        
        if self.hamiltonian_dfs(package_id, graph, &mut visited, &mut path) {
            ResolutionResult {
                package: graph.nodes.get(package_id).cloned(),
                path,
                fault_state: FaultState::Clean,
                error_message: None,
            }
        } else {
            ResolutionResult {
                package: None,
                path: vec![],
                fault_state: FaultState::HighPanic,
                error_message: Some("No Hamiltonian path found".to_string()),
            }
        }
    }
    
    /// A* resolution: Find optimal update path
    pub fn astar_resolve(&self, current: &SemverX, target: &SemverX, graph: &DependencyGraph) 
        -> ResolutionResult 
    {
        use std::collections::BinaryHeap;
        use std::cmp::Ordering;
        
        #[derive(Eq, PartialEq)]
        struct State {
            cost: u32,
            version: SemverX,
            path: Vec<String>,
        }
        
        impl Ord for State {
            fn cmp(&self, other: &Self) -> Ordering {
                other.cost.cmp(&self.cost)
            }
        }
        
        impl PartialOrd for State {
            fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
                Some(self.cmp(other))
            }
        }
        
        let heuristic = |v: &SemverX| -> u32 {
            (target.major as i32 - v.major as i32).abs() as u32 * 100 +
            (target.minor as i32 - v.minor as i32).abs() as u32 * 10 +
            (target.patch as i32 - v.patch as i32).abs() as u32 +
            v.major_state.cost() + v.minor_state.cost() + v.patch_state.cost()
        };
        
        let mut open_set = BinaryHeap::new();
        open_set.push(State {
            cost: heuristic(current),
            version: current.clone(),
            path: vec![format!("{:?}", current)],
        });
        
        while let Some(State { cost: _, version, path }) = open_set.pop() {
            if &version == target {
                return ResolutionResult {
                    package: None, // Would fetch actual package here
                    path,
                    fault_state: FaultState::Clean,
                    error_message: None,
                };
            }
            
            // Explore neighbors (compatible versions)
            for neighbor in self.get_compatible_versions(&version, graph) {
                let g_score = self.calculate_cost(&version, &neighbor);
                let f_score = g_score + heuristic(&neighbor);
                
                let mut new_path = path.clone();
                new_path.push(format!("{:?}", neighbor));
                
                open_set.push(State {
                    cost: f_score,
                    version: neighbor,
                    path: new_path,
                });
            }
        }
        
        ResolutionResult {
            package: None,
            path: vec![],
            fault_state: FaultState::MediumPanic,
            error_message: Some("No safe update path found".to_string()),
        }
    }
    
    fn degree(&self, node: &RegistryNode, graph: &DependencyGraph) -> usize {
        node.dependencies.len() + node.dependents.len()
    }
    
    fn hamiltonian_dfs(
        &self,
        current: &str,
        graph: &DependencyGraph,
        visited: &mut std::collections::HashSet<String>,
        path: &mut Vec<String>,
    ) -> bool {
        visited.insert(current.to_string());
        path.push(current.to_string());
        
        if visited.len() == graph.nodes.len() {
            return true;
        }
        
        if let Some(node) = graph.nodes.get(current) {
            for dep in &node.dependencies {
                if !visited.contains(&dep.target_id) {
                    if self.hamiltonian_dfs(&dep.target_id, graph, visited, path) {
                        return true;
                    }
                }
            }
        }
        
        visited.remove(current);
        path.pop();
        false
    }
    
    fn get_compatible_versions(&self, version: &SemverX, graph: &DependencyGraph) 
        -> Vec<SemverX> 
    {
        // Simplified: return versions within ±1 of current
        vec![
            SemverX {
                major: version.major,
                major_state: version.major_state,
                minor: version.minor + 1,
                minor_state: version.minor_state,
                patch: version.patch,
                patch_state: version.patch_state,
            },
            SemverX {
                major: version.major,
                major_state: version.major_state,
                minor: version.minor,
                minor_state: version.minor_state,
                patch: version.patch + 1,
                patch_state: version.patch_state,
            },
        ]
    }
    
    fn calculate_cost(&self, from: &SemverX, to: &SemverX) -> u32 {
        (to.major as i32 - from.major as i32).abs() as u32 * 100 +
        (to.minor as i32 - from.minor as i32).abs() as u32 * 10 +
        (to.patch as i32 - from.patch as i32).abs() as u32
    }
}

pub struct DependencyGraph {
    nodes: HashMap<String, RegistryNode>,
}

// ==================== Consumer-Observer Pattern ====================

pub struct ObserverManager {
    max_updates_per_sec: u32,
    active_observers: Arc<RwLock<HashMap<String, Vec<Observer>>>>,
}

pub struct Observer {
    pub id: ObserverId,
    pub callback: Box<dyn Fn(&Update) + Send + Sync>,
    pub last_notified: Instant,
}

#[derive(Debug, Clone)]
pub struct Update {
    pub package_id: String,
    pub old_version: Option<SemverX>,
    pub new_version: SemverX,
    pub update_type: UpdateType,
}

#[derive(Debug, Clone, Copy)]
pub enum UpdateType {
    OptIn,      // User must explicitly upgrade
    Mandatory,  // Automatic security update
    StaleRelease, // Package marked outdated
}

impl ObserverManager {
    pub fn new(max_updates_per_sec: u32) -> Self {
        Self {
            max_updates_per_sec,
            active_observers: Arc::new(RwLock::new(HashMap::new())),
        }
    }
    
    pub fn register(&self, package_id: &str, observer: Observer) -> Result<(), String> {
        let mut observers = self.active_observers.write().unwrap();
        let package_observers = observers.entry(package_id.to_string()).or_insert_with(Vec::new);
        
        if package_observers.len() >= 100 {
            return Err("Too many observers".to_string());
        }
        
        package_observers.push(observer);
        Ok(())
    }
    
    pub fn notify(&self, package_id: &str, update: &Update) -> Result<(), String> {
        let observers = self.active_observers.read().unwrap();
        let package_observers = observers.get(package_id).ok_or("Package not found")?;
        
        // Rate limiting: Max 5-10 updates/sec
        let now = Instant::now();
        let recent_updates = package_observers.iter()
            .filter(|o| now.duration_since(o.last_notified) < Duration::from_secs(1))
            .count();
        
        if recent_updates >= self.max_updates_per_sec as usize {
            return Err("Rate limit exceeded".to_string());
        }
        
        for observer in package_observers.iter() {
            (observer.callback)(update);
        }
        
        Ok(())
    }
}

// ==================== Registry Server ====================

pub struct RegistryServer {
    root: Option<Box<RegistryNode>>,
    resolver: DagResolver,
    observer_manager: ObserverManager,
}

impl RegistryServer {
    pub fn new() -> Self {
        Self {
            root: None,
            resolver: DagResolver::new(),
            observer_manager: ObserverManager::new(10), // Max 10 updates/sec
        }
    }
    
    pub fn insert(&mut self, node: RegistryNode) {
        // Insert with AVL balancing
        self.root = Some(self.insert_recursive(self.root.take(), node));
    }
    
    fn insert_recursive(
        &self,
        node: Option<Box<RegistryNode>>,
        new_node: RegistryNode,
    ) -> Box<RegistryNode> {
        match node {
            None => Box::new(new_node),
            Some(mut current) => {
                if new_node.package_id < current.package_id {
                    current.left = Some(self.insert_recursive(current.left.take(), new_node));
                } else {
                    current.right = Some(self.insert_recursive(current.right.take(), new_node));
                }
                current.rebalance()
            }
        }
    }
    
    pub fn search(&self, package_id: &str) -> Option<&RegistryNode> {
        self.search_recursive(self.root.as_ref(), package_id)
    }
    
    fn search_recursive<'a>(
        &self,
        node: Option<&'a Box<RegistryNode>>,
        package_id: &str,
    ) -> Option<&'a RegistryNode> {
        match node {
            None => None,
            Some(current) => {
                if package_id == current.package_id {
                    Some(current.as_ref())
                } else if package_id < &current.package_id {
                    self.search_recursive(current.left.as_ref(), package_id)
                } else {
                    self.search_recursive(current.right.as_ref(), package_id)
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_avl_insertion() {
        let mut server = RegistryServer::new();
        
        let node1 = RegistryNode::new(
            "package-a".to_string(),
            SemverX {
                major: 1,
                major_state: VersionState::Stable,
                minor: 0,
                minor_state: VersionState::Stable,
                patch: 0,
                patch_state: VersionState::Stable,
            },
            PackageMetadata {
                name: "Package A".to_string(),
                description: "Test package".to_string(),
                author: "OBINexus".to_string(),
                license: "MIT".to_string(),
                tarball_url: "https://r.obinexus.org/tarballs/package-a-1.0.0.tar.gz".to_string(),
                install_script: None,
            },
            AccessTier::Live,
            AccessLevel::Public,
        );
        
        server.insert(node1);
        
        assert!(server.search("package-a").is_some());
        assert!(server.search("nonexistent").is_none());
    }
    
    #[test]
    fn test_semverx_comparison() {
        let v1 = SemverX {
            major: 1,
            major_state: VersionState::Stable,
            minor: 0,
            minor_state: VersionState::Stable,
            patch: 0,
            patch_state: VersionState::Stable,
        };
        
        let v2 = v1.clone();
        assert_eq!(v1, v2);
    }
}
