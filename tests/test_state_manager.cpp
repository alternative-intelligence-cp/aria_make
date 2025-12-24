// test_state_manager.cpp - Tests for StateManager
// Part of aria_make - Aria Build System

#include "state/state_manager.hpp"
#include "state/artifact_record.hpp"

#include <iostream>
#include <fstream>
#include <cassert>
#include <filesystem>
#include <thread>
#include <vector>
#include <atomic>

namespace fs = std::filesystem;
using namespace aria::make;

// Test counters
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        std::cout << "  Testing: " << #name << "... "; \
        try { \
            test_##name(); \
            tests_passed++; \
            std::cout << "PASS\n"; \
        } catch (const std::exception& e) { \
            std::cout << "FAIL: " << e.what() << "\n"; \
        } \
    } while(0)

#define ASSERT(cond) \
    if (!(cond)) { \
        throw std::runtime_error("Assertion failed: " #cond); \
    }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        throw std::runtime_error("Assertion failed: " #a " == " #b); \
    }

// =============================================================================
// Test Fixtures
// =============================================================================

class TestFixture {
public:
    fs::path test_dir;
    fs::path source_file;
    fs::path output_file;

    TestFixture() {
        // Create temporary test directory
        test_dir = fs::temp_directory_path() / "aria_make_test";
        fs::create_directories(test_dir);

        // Create a test source file
        source_file = test_dir / "test.aria";
        std::ofstream src(source_file);
        src << "func:main = int8() { pass(0); };\n";
        src.close();

        // Create a fake output file
        output_file = test_dir / "test.o";
        std::ofstream out(output_file);
        out << "fake object file\n";
        out.close();
    }

    ~TestFixture() {
        // Cleanup
        std::error_code ec;
        fs::remove_all(test_dir, ec);
    }
};

static std::unique_ptr<TestFixture> fixture;

// =============================================================================
// ArtifactRecord Tests
// =============================================================================

void test_artifact_record_default() {
    ArtifactRecord record;
    ASSERT(!record.is_valid());
    ASSERT_EQ(record.command_hash, 0ULL);
    ASSERT_EQ(record.source_timestamp, 0ULL);
}

void test_artifact_record_valid() {
    ArtifactRecord record;
    record.target_name = "src/main.aria";
    record.source_hash = "fnv1a:123456";
    ASSERT(record.is_valid());
}

void test_dependency_info() {
    DependencyInfo dep("src/utils.aria", "fnv1a:abcdef");
    ASSERT_EQ(dep.path, "src/utils.aria");
    ASSERT_EQ(dep.hash, "fnv1a:abcdef");

    DependencyInfo dep2("src/utils.aria", "fnv1a:abcdef");
    ASSERT(dep == dep2);

    DependencyInfo dep3("src/other.aria", "fnv1a:abcdef");
    ASSERT(!(dep == dep3));
}

void test_dirty_reason_to_string() {
    ASSERT_EQ(std::string(dirty_reason_to_string(DirtyReason::CLEAN)), "clean");
    ASSERT_EQ(std::string(dirty_reason_to_string(DirtyReason::SOURCE_CHANGED)), "source_changed");
    ASSERT_EQ(std::string(dirty_reason_to_string(DirtyReason::FLAGS_CHANGED)), "flags_changed");
}

void test_build_stats() {
    BuildStats stats;
    ASSERT_EQ(stats.cache_hit_rate(), 0.0);

    stats.total_targets = 10;
    stats.cached_targets = 7;
    ASSERT(stats.cache_hit_rate() > 0.69 && stats.cache_hit_rate() < 0.71);
}

// =============================================================================
// StateManager Tests
// =============================================================================

void test_state_manager_construction() {
    StateManager mgr(fixture->test_dir);
    ASSERT(!mgr.has_state());
    ASSERT_EQ(mgr.target_count(), 0ULL);
}

void test_state_manager_load_empty() {
    StateManager mgr(fixture->test_dir);
    ASSERT(mgr.load());  // Should succeed even with no file
    ASSERT(!mgr.has_state());
}

void test_state_manager_save_load() {
    // Create and save state
    {
        StateManager mgr(fixture->test_dir);
        mgr.set_toolchain(ToolchainInfo("v0.0.7", "hash123"));

        std::vector<std::string> sources = { fixture->source_file.string() };
        std::vector<std::string> flags = { "-O2", "-DNDEBUG" };
        std::vector<DependencyInfo> deps;
        std::vector<std::string> impl_deps;

        mgr.update_record(
            "test.aria",
            fixture->output_file,
            sources,
            deps,
            impl_deps,
            flags,
            100);

        ASSERT(mgr.save());
    }

    // Load in new instance
    {
        StateManager mgr(fixture->test_dir);
        ASSERT(mgr.load());
        ASSERT(mgr.has_state());
        ASSERT_EQ(mgr.target_count(), 1ULL);

        auto record = mgr.get_record("test.aria");
        ASSERT(record.has_value());
        ASSERT_EQ(record->target_name, "test.aria");
    }
}

void test_state_manager_hash_flags() {
    std::vector<std::string> flags1 = { "-O2", "-DNDEBUG" };
    std::vector<std::string> flags2 = { "-O2", "-DNDEBUG" };
    std::vector<std::string> flags3 = { "-O0", "-DDEBUG" };

    uint64_t hash1 = StateManager::hash_flags(flags1);
    uint64_t hash2 = StateManager::hash_flags(flags2);
    uint64_t hash3 = StateManager::hash_flags(flags3);

    ASSERT_EQ(hash1, hash2);  // Same flags = same hash
    ASSERT(hash1 != hash3);   // Different flags = different hash
}

void test_state_manager_file_hash() {
    StateManager mgr(fixture->test_dir);

    std::string hash1 = mgr.hash_file(fixture->source_file);
    std::string hash2 = mgr.hash_file(fixture->source_file);

    ASSERT(!hash1.empty());
    ASSERT_EQ(hash1, hash2);  // Same file = same hash

    // Modify file
    {
        std::ofstream f(fixture->source_file, std::ios::app);
        f << "// modified\n";
    }

    // Invalidate cache since file was modified externally
    mgr.invalidate_hash_cache(fixture->source_file);

    std::string hash3 = mgr.hash_file(fixture->source_file);
    ASSERT(hash1 != hash3);  // Modified file = different hash

    // Restore file to original state for other tests
    {
        std::ofstream f(fixture->source_file, std::ios::trunc);
        f << "func:main = int8() { pass(0); };\n";
    }
}

void test_state_manager_dirty_missing_artifact() {
    StateManager mgr(fixture->test_dir);
    mgr.load();

    fs::path nonexistent = fixture->test_dir / "nonexistent.o";
    std::vector<std::string> sources = { fixture->source_file.string() };
    std::vector<std::string> flags = { "-O2" };

    DirtyReason reason = mgr.check_dirty("test", nonexistent, sources, flags);
    ASSERT_EQ(reason, DirtyReason::MISSING_ARTIFACT);
}

void test_state_manager_dirty_missing_record() {
    StateManager mgr(fixture->test_dir);
    mgr.load();

    std::vector<std::string> sources = { fixture->source_file.string() };
    std::vector<std::string> flags = { "-O2" };

    DirtyReason reason = mgr.check_dirty("unknown", fixture->output_file, sources, flags);
    ASSERT_EQ(reason, DirtyReason::MISSING_RECORD);
}

void test_state_manager_dirty_flags_changed() {
    StateManager mgr(fixture->test_dir);
    mgr.set_toolchain(ToolchainInfo("v0.0.7"));

    std::vector<std::string> sources = { fixture->source_file.string() };
    std::vector<std::string> flags1 = { "-O2" };
    std::vector<std::string> flags2 = { "-O0" };
    std::vector<DependencyInfo> deps;
    std::vector<std::string> impl_deps;

    // Build with flags1
    mgr.update_record("test", fixture->output_file, sources, deps, impl_deps, flags1, 0);

    // Check with same flags - should be clean
    DirtyReason reason = mgr.check_dirty("test", fixture->output_file, sources, flags1);
    ASSERT_EQ(reason, DirtyReason::CLEAN);

    // Check with different flags - should be dirty
    reason = mgr.check_dirty("test", fixture->output_file, sources, flags2);
    ASSERT_EQ(reason, DirtyReason::FLAGS_CHANGED);
}

void test_state_manager_invalidate() {
    StateManager mgr(fixture->test_dir);
    mgr.set_toolchain(ToolchainInfo("v0.0.7"));

    std::vector<std::string> sources = { fixture->source_file.string() };
    std::vector<std::string> flags = { "-O2" };
    std::vector<DependencyInfo> deps;
    std::vector<std::string> impl_deps;

    mgr.update_record("test", fixture->output_file, sources, deps, impl_deps, flags, 0);
    ASSERT(mgr.get_record("test").has_value());

    mgr.invalidate("test");
    ASSERT(!mgr.get_record("test").has_value());
}

void test_state_manager_clear() {
    StateManager mgr(fixture->test_dir);
    mgr.set_toolchain(ToolchainInfo("v0.0.7"));

    std::vector<std::string> sources = { fixture->source_file.string() };
    std::vector<std::string> flags = { "-O2" };
    std::vector<DependencyInfo> deps;
    std::vector<std::string> impl_deps;

    mgr.update_record("test1", fixture->output_file, sources, deps, impl_deps, flags, 0);
    mgr.update_record("test2", fixture->output_file, sources, deps, impl_deps, flags, 0);
    ASSERT_EQ(mgr.target_count(), 2ULL);

    mgr.clear();
    ASSERT_EQ(mgr.target_count(), 0ULL);
    ASSERT(!mgr.has_state());
}

void test_state_manager_toolchain() {
    StateManager mgr(fixture->test_dir);

    ToolchainInfo tc("v0.0.7", "abc123");
    mgr.set_toolchain(tc);

    ToolchainInfo retrieved = mgr.get_toolchain();
    ASSERT_EQ(retrieved.compiler_version, "v0.0.7");
    ASSERT_EQ(retrieved.compiler_hash, "abc123");
}

void test_state_manager_stats() {
    StateManager mgr(fixture->test_dir);

    BuildStats stats = mgr.get_stats();
    ASSERT_EQ(stats.total_targets, 0ULL);
    ASSERT_EQ(stats.rebuilt_targets, 0ULL);

    std::vector<std::string> sources = { fixture->source_file.string() };
    std::vector<std::string> flags = { "-O2" };
    std::vector<DependencyInfo> deps;
    std::vector<std::string> impl_deps;

    mgr.update_record("test", fixture->output_file, sources, deps, impl_deps, flags, 100);

    stats = mgr.get_stats();
    ASSERT_EQ(stats.rebuilt_targets, 1ULL);

    mgr.reset_stats();
    stats = mgr.get_stats();
    ASSERT_EQ(stats.rebuilt_targets, 0ULL);
}

// =============================================================================
// Thread Safety Tests
// =============================================================================

void test_state_manager_concurrent_reads() {
    StateManager mgr(fixture->test_dir);
    mgr.set_toolchain(ToolchainInfo("v0.0.7"));

    std::vector<std::string> sources = { fixture->source_file.string() };
    std::vector<std::string> flags = { "-O2" };
    std::vector<DependencyInfo> deps;
    std::vector<std::string> impl_deps;

    mgr.update_record("test", fixture->output_file, sources, deps, impl_deps, flags, 0);

    // Spawn multiple reader threads
    std::vector<std::thread> threads;
    std::atomic<int> read_count{0};

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&mgr, &sources, &flags, &read_count]() {
            for (int j = 0; j < 100; ++j) {
                auto record = mgr.get_record("test");
                if (record.has_value()) {
                    read_count++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(read_count.load(), 1000);
}

void test_state_manager_concurrent_write_read() {
    StateManager mgr(fixture->test_dir);
    mgr.set_toolchain(ToolchainInfo("v0.0.7"));

    std::vector<std::string> sources = { fixture->source_file.string() };
    std::vector<std::string> flags = { "-O2" };
    std::vector<DependencyInfo> deps;
    std::vector<std::string> impl_deps;

    std::atomic<bool> stop{false};
    std::atomic<int> writes{0};
    std::atomic<int> reads{0};

    // Writer thread
    std::thread writer([&]() {
        for (int i = 0; i < 50 && !stop; ++i) {
            std::string name = "target_" + std::to_string(i);
            mgr.update_record(name, fixture->output_file, sources, deps, impl_deps, flags, 0);
            writes++;
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    // Reader threads
    std::vector<std::thread> readers;
    for (int i = 0; i < 5; ++i) {
        readers.emplace_back([&mgr, &stop, &reads]() {
            while (!stop) {
                size_t count = mgr.target_count();
                reads++;
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        });
    }

    writer.join();
    stop = true;

    for (auto& t : readers) {
        t.join();
    }

    ASSERT(writes.load() >= 50);
    ASSERT(reads.load() > 0);
    ASSERT(mgr.target_count() >= 50);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== StateManager Test Suite ===\n\n";

    // Setup
    fixture = std::make_unique<TestFixture>();

    std::cout << "ArtifactRecord Tests:\n";
    TEST(artifact_record_default);
    TEST(artifact_record_valid);
    TEST(dependency_info);
    TEST(dirty_reason_to_string);
    TEST(build_stats);

    std::cout << "\nStateManager Core Tests:\n";
    TEST(state_manager_construction);
    TEST(state_manager_load_empty);
    TEST(state_manager_save_load);
    TEST(state_manager_hash_flags);
    TEST(state_manager_file_hash);

    std::cout << "\nDirty Detection Tests:\n";
    TEST(state_manager_dirty_missing_artifact);
    TEST(state_manager_dirty_missing_record);
    TEST(state_manager_dirty_flags_changed);

    std::cout << "\nState Management Tests:\n";
    TEST(state_manager_invalidate);
    TEST(state_manager_clear);
    TEST(state_manager_toolchain);
    TEST(state_manager_stats);

    std::cout << "\nThread Safety Tests:\n";
    TEST(state_manager_concurrent_reads);
    TEST(state_manager_concurrent_write_read);

    // Cleanup
    fixture.reset();

    std::cout << "\n=== Results ===\n";
    std::cout << "Passed: " << tests_passed << "/" << tests_run << "\n";

    return (tests_passed == tests_run) ? 0 : 1;
}
