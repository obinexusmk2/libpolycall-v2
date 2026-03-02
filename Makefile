# ============================================================================
# LibPolycall V2 - Top-Level Build Orchestrator
# OBINexus Computing - Polymorphic Core Library
#
# Usage:
#   make              Build core library + CLI + daemon
#   make core         Build libpolycall.so, libpolycall.a, polycall CLI
#   make daemon       Build polycalld daemon (links libpolycall)
#   make bindings     Build all language bindings
#   make test         Run core test suite
#   make install      Install to system (FHS-compliant)
#   make uninstall    Remove installed files
#   make clean        Remove build artifacts
#   make distclean    Remove all generated files including binding deps
# ============================================================================

VERSION   := $(shell cat VERSION 2>/dev/null || echo "2.0.0")
PREFIX    ?= /usr/local
ETCDIR    ?= /etc/polycall
VARDIR    ?= /var/lib/polycall
RUNDIR    ?= /var/run/polycall
LOGDIR    ?= /var/log/polycall

BUILD_DIR      := build
CORE_BUILD     := $(BUILD_DIR)/core
DAEMON_BUILD   := $(BUILD_DIR)/daemon

CC       ?= gcc
CFLAGS   ?= -Wall -Wextra -O2 -fPIC -DVERSION=\"$(VERSION)\"
LDFLAGS  ?=

CMAKE       ?= cmake
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=Release \
               -DBUILD_SHARED_LIBS=ON \
               -DCMAKE_INSTALL_PREFIX=$(PREFIX)

NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# ============================================================================
# Primary Targets
# ============================================================================

.PHONY: all core cli daemon bindings install uninstall clean distclean test help

all: core daemon
	@echo ""
	@echo "=== LibPolycall V2 $(VERSION) build complete ==="
	@echo "  Library: $(CORE_BUILD)/lib/"
	@echo "  CLI:     $(CORE_BUILD)/bin/polycall"
	@echo "  Daemon:  $(DAEMON_BUILD)/polycalld"
	@echo ""

help:
	@echo "LibPolycall V2 Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          Build core + daemon (default)"
	@echo "  core         Build libpolycall library and CLI"
	@echo "  daemon       Build polycalld daemon"
	@echo "  bindings     Build all language bindings"
	@echo "  test         Run tests"
	@echo "  install      Install to PREFIX=$(PREFIX)"
	@echo "  uninstall    Remove installed files"
	@echo "  clean        Remove build/"
	@echo "  distclean    Full clean including binding deps"
	@echo ""
	@echo "Variables:"
	@echo "  PREFIX=$(PREFIX)"
	@echo "  CC=$(CC)"
	@echo "  CFLAGS=$(CFLAGS)"

# ============================================================================
# Core Library (cmake-backed)
# ============================================================================

core:
	@echo "=== Building libpolycall core ==="
	@mkdir -p $(CORE_BUILD)
	cd $(CORE_BUILD) && $(CMAKE) $(CMAKE_FLAGS) ../../core && $(MAKE) -j$(NPROC)

cli: core

# ============================================================================
# Daemon
# ============================================================================

daemon: core
	@echo "=== Building polycalld daemon ==="
	@mkdir -p $(DAEMON_BUILD)
	$(CC) $(CFLAGS) \
		-I core/include \
		-I core/include/libpolycall \
		-I daemon/include \
		daemon/src/polycalld.c \
		daemon/src/daemonize.c \
		-L $(CORE_BUILD)/lib -L $(CORE_BUILD)/*/lib \
		-lpthread \
		-o $(DAEMON_BUILD)/polycalld
	@echo "  Built: $(DAEMON_BUILD)/polycalld"

# ============================================================================
# Language Bindings
# ============================================================================

.PHONY: bindings bindings-node bindings-go bindings-java bindings-rust

bindings: bindings-node bindings-go bindings-java bindings-rust

bindings-node:
	@echo "=== Node.js binding ==="
	@if [ -f bindings/node/package.json ]; then \
		cd bindings/node && npm install --production 2>/dev/null || true; \
	fi

bindings-go:
	@echo "=== Go binding ==="
	@if [ -f bindings/go/go.mod ]; then \
		cd bindings/go && go build ./... 2>/dev/null || true; \
	fi

bindings-java:
	@echo "=== Java binding ==="
	@if [ -f bindings/java/pom.xml ]; then \
		cd bindings/java && mvn package -DskipTests -q 2>/dev/null || true; \
	fi

bindings-rust:
	@echo "=== Rust SemVerX binding ==="
	@if [ -f bindings/rust-semverx/Cargo.toml ]; then \
		cd bindings/rust-semverx && cargo build --release 2>/dev/null || true; \
	fi

# ============================================================================
# Testing
# ============================================================================

test: core
	@echo "=== Running tests ==="
	@if [ -d core/test ]; then \
		for t in core/test/test_*.c; do \
			name=$$(basename "$$t" .c); \
			echo "  Compiling $$name..."; \
			$(CC) $(CFLAGS) -I core/include -I core/include/libpolycall \
				"$$t" \
				-L $(CORE_BUILD)/lib -L $(CORE_BUILD)/*/lib \
				-lpthread \
				-o $(BUILD_DIR)/$$name 2>/dev/null && \
			echo "  Running $$name..." && \
			LD_LIBRARY_PATH=$(CORE_BUILD)/lib:$(CORE_BUILD)/*/lib \
				$(BUILD_DIR)/$$name || \
			echo "  WARN: $$name failed"; \
		done; \
	fi

# ============================================================================
# Installation (FHS-compliant)
# ============================================================================

install: all
	@echo "=== Installing LibPolycall V2 to $(DESTDIR)$(PREFIX) ==="
	# Libraries
	install -d $(DESTDIR)$(PREFIX)/lib
	find $(CORE_BUILD) -name 'libpolycall.a' -exec install -m 644 {} $(DESTDIR)$(PREFIX)/lib/ \; 2>/dev/null || true
	find $(CORE_BUILD) -name 'libpolycall.so*' -exec install -m 755 {} $(DESTDIR)$(PREFIX)/lib/ \; 2>/dev/null || true
	ldconfig 2>/dev/null || true
	# Headers
	install -d $(DESTDIR)$(PREFIX)/include/libpolycall
	cp -r core/include/libpolycall/* $(DESTDIR)$(PREFIX)/include/libpolycall/
	# CLI binary
	install -d $(DESTDIR)$(PREFIX)/bin
	find $(CORE_BUILD) -name 'polycall' -type f -executable -exec install -m 755 {} $(DESTDIR)$(PREFIX)/bin/polycall \; 2>/dev/null || true
	# Daemon binary
	install -d $(DESTDIR)$(PREFIX)/sbin
	install -m 755 $(DAEMON_BUILD)/polycalld $(DESTDIR)$(PREFIX)/sbin/polycalld
	# Configuration
	install -d $(DESTDIR)$(ETCDIR)
	install -m 644 config/config.polycall $(DESTDIR)$(ETCDIR)/config.polycall
	test -f config/.polycallrc && install -m 644 config/.polycallrc $(DESTDIR)$(ETCDIR)/polycallrc.example || true
	# nginx
	install -d $(DESTDIR)/etc/nginx/conf.d
	install -m 644 config/nginx/polycall.conf $(DESTDIR)/etc/nginx/conf.d/polycall.conf
	# systemd
	install -d $(DESTDIR)/etc/systemd/system
	install -m 644 config/systemd/polycalld.service $(DESTDIR)/etc/systemd/system/
	test -f config/systemd/polycalld@.service && \
		install -m 644 config/systemd/polycalld@.service $(DESTDIR)/etc/systemd/system/ || true
	# Runtime directories
	install -d $(DESTDIR)$(VARDIR)
	install -d $(DESTDIR)$(RUNDIR)
	install -d $(DESTDIR)$(LOGDIR)
	# Man pages
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install -d $(DESTDIR)$(PREFIX)/share/man/man5
	install -d $(DESTDIR)$(PREFIX)/share/man/man8
	test -f docs/man/polycall.1 && install -m 644 docs/man/polycall.1 $(DESTDIR)$(PREFIX)/share/man/man1/ || true
	test -f docs/man/config.polycall.5 && install -m 644 docs/man/config.polycall.5 $(DESTDIR)$(PREFIX)/share/man/man5/ || true
	test -f docs/man/polycalld.8 && install -m 644 docs/man/polycalld.8 $(DESTDIR)$(PREFIX)/share/man/man8/ || true
	@echo "=== Installation complete ==="

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/polycall
	rm -f $(DESTDIR)$(PREFIX)/sbin/polycalld
	rm -f $(DESTDIR)$(PREFIX)/lib/libpolycall.*
	rm -rf $(DESTDIR)$(PREFIX)/include/libpolycall
	rm -rf $(DESTDIR)$(ETCDIR)
	rm -f $(DESTDIR)/etc/systemd/system/polycalld.service
	rm -f $(DESTDIR)/etc/systemd/system/polycalld@.service
	rm -f $(DESTDIR)/etc/nginx/conf.d/polycall.conf
	rm -f $(DESTDIR)$(PREFIX)/share/man/man1/polycall.1
	rm -f $(DESTDIR)$(PREFIX)/share/man/man5/config.polycall.5
	rm -f $(DESTDIR)$(PREFIX)/share/man/man8/polycalld.8

# ============================================================================
# Cleanup
# ============================================================================

clean:
	rm -rf $(BUILD_DIR)

distclean: clean
	cd bindings/node && rm -rf node_modules 2>/dev/null || true
	cd bindings/rust-semverx && cargo clean 2>/dev/null || true
	cd bindings/java && mvn clean -q 2>/dev/null || true
