// @obinexus/registry/index.ts
/**
 * r.obinexus.org Registry Client for Node.js
 * 
 * Usage:
 *   import { Registry, SemverX } from '@obinexus/registry';
 * 
 *   const registry = new Registry('https://r.obinexus.org');
 *   const pkg = await registry.fetch('@obinexus/core', '2.stable.*.stable.*.stable');
 *   
 *   registry.subscribe('@obinexus/core', (update) => {
 *     console.log('Package updated:', update);
 *   });
 */

import axios, { AxiosInstance } from 'axios';
import crypto from 'crypto';
import { EventEmitter } from 'events';

// ==================== Types ====================

export enum VersionState {
  Stable = 'stable',
  Legacy = 'legacy',
  Experimental = 'experimental'
}

export interface SemverX {
  major: number;
  majorState: VersionState;
  minor: number;
  minorState: VersionState;
  patch: number;
  patchState: VersionState;
}

export enum FaultState {
  Clean = 0,
  LowWarning = 1,
  MediumWarning = 3,
  HighWarning = 5,
  LowDanger = 6,
  CriticalDanger = 11,
  LowPanic = 12,
  SystemPanic = 17
}

export enum AccessTier {
  Live = 'live',
  Local = 'local',
  Remote = 'remote'
}

export enum AccessLevel {
  Public = 'public',
  Protected = 'protected',
  Private = 'private'
}

export interface Package {
  id: string;
  version: SemverX;
  name: string;
  description: string;
  author: string;
  license: string;
  tarballUrl: string;
  dependencies: string[];
  checksum: string;
  faultState: FaultState;
}

export interface Update {
  packageId: string;
  oldVersion?: SemverX;
  newVersion: SemverX;
  updateType: 'opt-in' | 'mandatory' | 'stale-release';
}

export enum ResolutionStrategy {
  Eulerian = 'eulerian',
  Hamiltonian = 'hamiltonian',
  AStar = 'astar',
  Hybrid = 'hybrid'
}

export interface ResolutionResult {
  package?: Package;
  path: string[];
  faultState: FaultState;
  errorMessage?: string;
}

// ==================== SemverX Utilities ====================

export class SemverXParser {
  static parse(versionStr: string): SemverX {
    const parts = versionStr.split('.');
    if (parts.length !== 6) {
      throw new Error(`Invalid SemVerX format: ${versionStr}`);
    }

    return {
      major: parseInt(parts[0], 10),
      majorState: parts[1] as VersionState,
      minor: parseInt(parts[2], 10),
      minorState: parts[3] as VersionState,
      patch: parseInt(parts[4], 10),
      patchState: parts[5] as VersionState
    };
  }

  static stringify(version: SemverX): string {
    return `${version.major}.${version.majorState}.${version.minor}.${version.minorState}.${version.patch}.${version.patchState}`;
  }

  static compare(v1: SemverX, v2: SemverX): number {
    if (v1.major !== v2.major) return v1.major - v2.major;
    if (v1.minor !== v2.minor) return v1.minor - v2.minor;
    if (v1.patch !== v2.patch) return v1.patch - v2.patch;
    return 0;
  }

  static stateCost(state: VersionState): number {
    switch (state) {
      case VersionState.Stable: return 0;
      case VersionState.Experimental: return 5;
      case VersionState.Legacy: return 10;
    }
  }
}

// ==================== Registry Client ====================

export class ResolutionError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'ResolutionError';
  }
}

export interface RegistryOptions {
  endpoint?: string;
  accessTier?: AccessTier;
  authToken?: string;
  maxUpdatesPerSec?: number;
}

export class Registry extends EventEmitter {
  private endpoint: string;
  private accessTier: AccessTier;
  private client: AxiosInstance;
  private observers: Map<string, Set<(update: Update) => void>>;
  private maxUpdatesPerSec: number;
  private updateCounts: Map<string, number[]>;

  constructor(options: RegistryOptions = {}) {
    super();
    
    this.endpoint = options.endpoint || 'https://r.obinexus.org';
    this.accessTier = options.accessTier || AccessTier.Live;
    this.maxUpdatesPerSec = options.maxUpdatesPerSec || 10;
    this.observers = new Map();
    this.updateCounts = new Map();

    this.client = axios.create({
      baseURL: `${this.endpoint}/${this.accessTier}`,
      headers: options.authToken ? {
        'Authorization': `Bearer ${options.authToken}`
      } : {}
    });
  }

  // ==================== Core Methods ====================

  async fetch(
    packageId: string,
    versionRange: string,
    strategy: ResolutionStrategy = ResolutionStrategy.Hybrid
  ): Promise<Package> {
    try {
      const response = await this.client.get(`/packages/${packageId}`, {
        params: {
          version: versionRange,
          strategy
        }
      });

      const data = response.data;

      // Check fault state
      if (data.faultState >= FaultState.SystemPanic) {
        throw new ResolutionError(
          `Package in PANIC state: ${data.errorMessage || 'Unknown error'}`
        );
      }

      return {
        id: data.id,
        version: SemverXParser.parse(data.version),
        name: data.name,
        description: data.description,
        author: data.author,
        license: data.license,
        tarballUrl: data.tarballUrl,
        dependencies: data.dependencies || [],
        checksum: data.checksum,
        faultState: data.faultState
      };
    } catch (error) {
      if (axios.isAxiosError(error) && error.response?.status === 404) {
        throw new ResolutionError(`Package not found: ${packageId}`);
      }
      throw error;
    }
  }

  async install(
    packageId: string,
    versionRange: string,
    verifyChecksum: boolean = true
  ): Promise<Package> {
    const pkg = await this.fetch(packageId, versionRange);

    if (verifyChecksum) {
      await this.verifyChecksum(pkg);
    }

    // Download tarball
    const tarballResponse = await axios.get(pkg.tarballUrl, {
      responseType: 'arraybuffer'
    });

    console.log(`Installing ${pkg.name} v${SemverXParser.stringify(pkg.version)}...`);

    // Resolve dependencies recursively
    for (const dep of pkg.dependencies) {
      const [depId, depVersion] = dep.split('@');
      await this.install(depId, depVersion, verifyChecksum);
    }

    return pkg;
  }

  // ==================== Observer Pattern ====================

  subscribe(
    packageId: string,
    callback: (update: Update) => void
  ): string {
    if (!this.observers.has(packageId)) {
      this.observers.set(packageId, new Set());
    }

    this.observers.get(packageId)!.add(callback);

    // Register with server
    this.client.post('/subscribe', { packageId })
      .then(response => {
        const observerId = response.data.observerId;
        this.emit('subscribed', { packageId, observerId });
        return observerId;
      })
      .catch(error => {
        console.error('Failed to subscribe:', error);
        throw new Error(`Subscription failed: ${error.message}`);
      });

    return crypto.randomUUID();
  }

  unsubscribe(packageId: string, observerId: string): void {
    this.client.delete(`/unsubscribe/${observerId}`)
      .catch(error => {
        console.error('Failed to unsubscribe:', error);
      });

    this.observers.delete(packageId);
  }

  private notifyObservers(packageId: string, update: Update): void {
    // Rate limiting: Max 5-10 updates/sec
    const now = Date.now();
    const timestamps = this.updateCounts.get(packageId) || [];
    
    // Keep only timestamps from last second
    const recentTimestamps = timestamps.filter(t => now - t < 1000);
    
    if (recentTimestamps.length >= this.maxUpdatesPerSec) {
      console.warn(`Rate limit exceeded for ${packageId}`);
      return;
    }

    recentTimestamps.push(now);
    this.updateCounts.set(packageId, recentTimestamps);

    // Notify all observers
    const callbacks = this.observers.get(packageId);
    if (callbacks) {
      callbacks.forEach(cb => cb(update));
    }

    this.emit('update', update);
  }

  // ==================== DAG Resolution ====================

  async resolveDAG(
    packageId: string,
    strategy: ResolutionStrategy = ResolutionStrategy.Hybrid
  ): Promise<ResolutionResult> {
    const response = await this.client.post('/resolve', {
      packageId,
      strategy
    });

    return response.data;
  }

  // ==================== Utilities ====================

  private async verifyChecksum(pkg: Package): Promise<void> {
    const tarballResponse = await axios.get(pkg.tarballUrl, {
      responseType: 'arraybuffer'
    });

    const tarballData = Buffer.from(tarballResponse.data);
    const calculatedChecksum = crypto
      .createHash('sha256')
      .update(tarballData)
      .digest('hex');

    if (calculatedChecksum !== pkg.checksum) {
      throw new ResolutionError(
        `Checksum mismatch for ${pkg.id}: expected ${pkg.checksum}, got ${calculatedChecksum}`
      );
    }
  }

  async health(): Promise<{ status: string; version: string }> {
    const response = await this.client.get('/health');
    return response.data;
  }
}

// ==================== CLI Support ====================

export async function main() {
  const args = process.argv.slice(2);
  
  if (args.length < 2) {
    console.log('Usage: npx @obinexus/registry <command> <package> [--version=<range>]');
    console.log('Commands: fetch, install, resolve');
    process.exit(1);
  }

  const [command, packageId] = args;
  const versionArg = args.find(a => a.startsWith('--version='));
  const version = versionArg ? versionArg.split('=')[1] : '*.*.*';

  const registry = new Registry();

  switch (command) {
    case 'fetch': {
      const pkg = await registry.fetch(packageId, version);
      console.log(`Fetched: ${pkg.name} v${SemverXParser.stringify(pkg.version)}`);
      console.log(`Fault state: ${FaultState[pkg.faultState]}`);
      break;
    }

    case 'install': {
      const pkg = await registry.install(packageId, version);
      console.log(`Installed: ${pkg.name} v${SemverXParser.stringify(pkg.version)}`);
      break;
    }

    case 'resolve': {
      const result = await registry.resolveDAG(packageId);
      console.log(`Resolution path: ${result.path.join(' -> ')}`);
      console.log(`Fault state: ${FaultState[result.faultState]}`);
      break;
    }

    default:
      console.error(`Unknown command: ${command}`);
      process.exit(1);
  }
}

// Run CLI if executed directly
if (require.main === module) {
  main().catch(error => {
    console.error('Error:', error.message);
    process.exit(1);
  });
}

// ==================== Exports ====================

export default Registry;
