#include <jni.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <android/log.h>
#include <sys/auxv.h>
#define LOG_TAG "NativeBenchmark"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

using namespace std;

// Helper function to read a single line from a file
string readFile(const string &path)
{
    ifstream file(path);
    if(!file.is_open())
        return "";
    string line;
    getline(file, line);
    return line;
}

// ============================================================================
// CACHE DETECTION METHODS - Multiple approaches for ARM and x86 compatibility
// ============================================================================

// Method 1: Try sysfs (works on x86 emulators, rarely on ARM devices)
string getCacheFromSysfs() {
    stringstream ss;
    bool foundAny = false;

    for (int i = 0; i < 4; ++i) {
        string base_path = "/sys/devices/system/cpu/cpu0/cache/index" + to_string(i);

        ifstream check(base_path + "/size");
        if (!check.good()) {
            continue;
        }

        foundAny = true;
        string level = readFile(base_path + "/level");
        string size = readFile(base_path + "/size");
        string type = readFile(base_path + "/type");
        string coherency = readFile(base_path + "/coherency_line_size");

        if (!level.empty() && !size.empty()) {
            ss << "L" << level << " Cache";
            if (!type.empty()) {
                ss << " (" << type << ")";
            }
            ss << ": " << size;
            if (!coherency.empty()) {
                ss << " (Line size: " << coherency << " bytes)";
            }
            ss << "\n";
        }
    }

    if (foundAny) {
        LOGI("Cache detection: sysfs method succeeded");
    }

    return ss.str();
}

// Method 2: Parse /proc/cpuinfo (works on many ARM devices)
string getCacheFromCpuinfo() {
    stringstream ss;
    ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        return "";
    }

    string line;
    map<string, string> cacheInfo; // Use map to deduplicate

    while (getline(cpuinfo, line)) {
        // Standard "cache size" field
        if (line.find("cache size") != string::npos) {
            size_t colonPos = line.find(":");
            if (colonPos != string::npos) {
                string value = line.substr(colonPos + 2);
                cacheInfo["Cache size"] = value;
            }
        }

        // ARM-specific cache fields
        if (line.find("cache_size") != string::npos) {
            size_t colonPos = line.find(":");
            if (colonPos != string::npos) {
                string value = line.substr(colonPos + 2);
                cacheInfo["Cache size"] = value;
            }
        }

        // Some ARM devices report separate I/D cache
        if (line.find("I cache") != string::npos) {
            size_t colonPos = line.find(":");
            if (colonPos != string::npos) {
                cacheInfo["L1 Instruction Cache"] = line.substr(colonPos + 2);
            }
        }
        if (line.find("D cache") != string::npos) {
            size_t colonPos = line.find(":");
            if (colonPos != string::npos) {
                cacheInfo["L1 Data Cache"] = line.substr(colonPos + 2);
            }
        }
    }

    if (!cacheInfo.empty()) {
        LOGI("Cache detection: /proc/cpuinfo method succeeded");
        for (const auto& entry : cacheInfo) {
            ss << entry.first << ": " << entry.second << "\n";
        }
    }

    return ss.str();
}

// Method 3: Use getauxval for ARM64-specific cache line sizes
string getCacheFromAuxval() {
    stringstream ss;

#ifdef __aarch64__
    // ARM64-specific: Get cache line size from auxiliary vector
    unsigned long dcache_line_size = getauxval(AT_DCACHEBSIZE);
    unsigned long icache_line_size = getauxval(AT_ICACHEBSIZE);

    if (dcache_line_size > 0 || icache_line_size > 0) {
        LOGI("Cache detection: getauxval method succeeded (ARM64)");
        if (dcache_line_size > 0) {
            ss << "D-Cache Line Size: " << dcache_line_size << " bytes\n";
        }
        if (icache_line_size > 0) {
            ss << "I-Cache Line Size: " << icache_line_size << " bytes\n";
        }
        return ss.str();
    }
#elif defined(__arm__)
    // ARM32
    unsigned long dcache_line_size = getauxval(AT_DCACHEBSIZE);
    unsigned long icache_line_size = getauxval(AT_ICACHEBSIZE);

    if (dcache_line_size > 0 || icache_line_size > 0) {
        LOGI("Cache detection: getauxval method succeeded (ARM32)");
        if (dcache_line_size > 0) {
            ss << "D-Cache Line Size: " << dcache_line_size << " bytes\n";
        }
        if (icache_line_size > 0) {
            ss << "I-Cache Line Size: " << icache_line_size << " bytes\n";
        }
        return ss.str();
    }
#endif

    return "";
}

// Method 4: Try device-tree on ARM
string getCacheFromDeviceTree() {
    stringstream ss;
    bool foundAny = false;

    // Common device tree paths for cache information
    vector<string> dt_paths = {
            "/proc/device-tree/cpus/cpu@0/d-cache-size",
            "/proc/device-tree/cpus/cpu@0/i-cache-size",
            "/proc/device-tree/cpus/cpu@0/cache-size",
            "/proc/device-tree/cpus/cpu@0/l2-cache-size"
    };

    for (const auto& path : dt_paths) {
        ifstream file(path, ios::binary);
        if (file.is_open()) {
            // Device tree values are 32-bit big-endian integers
            uint32_t value = 0;
            file.read(reinterpret_cast<char*>(&value), sizeof(value));

            if (file.gcount() == sizeof(value) && value > 0) {
                // Convert from big-endian to host byte order
                value = __builtin_bswap32(value);

                // Determine cache type from path
                string cache_type = "Cache";
                if (path.find("d-cache") != string::npos) {
                    cache_type = "L1 D-Cache";
                } else if (path.find("i-cache") != string::npos) {
                    cache_type = "L1 I-Cache";
                } else if (path.find("l2-cache") != string::npos) {
                    cache_type = "L2 Cache";
                }

                ss << cache_type << ": " << (value / 1024) << " KB\n";
                foundAny = true;
            }
            file.close();
        }
    }

    if (foundAny) {
        LOGI("Cache detection: device-tree method succeeded");
    }

    return ss.str();
}

// Method 5: Provide typical values for known ARM processors
string getCacheFromKnownCpus(const string& hardware) {
    stringstream ss;

    // Map of known CPU families to their typical cache configurations
    map<string, string> knownCaches = {
            {"Qualcomm", "Typical: L1: 64KB/core (32KB I + 32KB D), L2: 256KB-512KB/cluster, L3: 2-4MB"},
            {"Snapdragon", "Typical: L1: 64KB/core, L2: 256KB-512KB/cluster, L3: 2-4MB shared"},
            {"Exynos", "Typical: L1: 64KB/core, L2: 512KB-2MB/cluster, L3: 2-4MB"},
            {"Kirin", "Typical: L1: 64KB/core, L2: 256KB-512KB, L3: 4MB"},
            {"MediaTek", "Typical: L1: 32-64KB/core, L2: 256KB-512KB, L3: 1-2MB"},
            {"Tensor", "Typical: L1: 64KB/core, L2: 512KB-1MB, L3: 8MB"},
            {"Dimensity", "Typical: L1: 64KB/core, L2: 512KB/cluster, L3: 2-4MB"}
    };

    for (const auto& entry : knownCaches) {
        if (hardware.find(entry.first) != string::npos) {
            LOGI("Cache detection: using typical values for %s", entry.first.c_str());
            ss << entry.second << "\n";
            ss << "(Note: Exact values vary by specific model)\n";
            return ss.str();
        }
    }

    return "";
}

// Main cache info function that tries all methods
string getCacheInfo(const string& hardware = "") {
    stringstream ss;
    ss << "\n=== CACHE INFO ===\n";

    string result;
    bool foundPrecise = false;

    // Method 1: Try sysfs first (most detailed when available)
    result = getCacheFromSysfs();
    if (!result.empty()) {
        ss << result;
        return ss.str();
    }

    // Method 2: Try /proc/cpuinfo parsing
    result = getCacheFromCpuinfo();
    if (!result.empty()) {
        ss << result;
        foundPrecise = true;
    }

    // Method 3: Try ARM-specific auxiliary vector
    result = getCacheFromAuxval();
    if (!result.empty()) {
        if (foundPrecise) {
            ss << "\nAdditional info:\n";
        }
        ss << result;
        foundPrecise = true;
    }

    // Method 4: Try device tree
    result = getCacheFromDeviceTree();
    if (!result.empty()) {
        if (foundPrecise) {
            ss << "\nFrom device tree:\n";
        }
        ss << result;
        foundPrecise = true;
    }

    // If we found some precise info, add typical values as reference
    if (foundPrecise && !hardware.empty()) {
        string typical = getCacheFromKnownCpus(hardware);
        if (!typical.empty()) {
            ss << "\nReference values:\n" << typical;
        }
        return ss.str();
    }

    // Method 5: Last resort - use typical values for known CPUs
    if (!hardware.empty()) {
        result = getCacheFromKnownCpus(hardware);
        if (!result.empty()) {
            ss << "Kernel does not expose cache details for this device.\n";
            ss << result;
            return ss.str();
        }
    }

    // Nothing worked at all
    ss << "Cache information not available.\n";
    ss << "This is common on ARM devices where manufacturers don't expose\n";
    ss << "cache hierarchy through standard kernel interfaces.\n";

    LOGI("Cache detection: all methods failed");

    return ss.str();
}

// ============================================================================
// CPU INFORMATION
// ============================================================================

string getCPUInfo() {
    stringstream ss;
    ss << "=== CPU INFO ===\n";

    ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        ss << "Error: Could not open /proc/cpuinfo\n";
        return ss.str();
    }

    string line;
    string hardware, model;
    vector<string> cache_lines;

    while (getline(cpuinfo, line)) {
        if (hardware.empty() && line.find("Hardware") != string::npos) {
            hardware = line.substr(line.find(":") + 2);
        } else if (model.empty() && line.find("model name") != string::npos) {
            model = line.substr(line.find(":") + 2);
        }

        // Collect cache-related lines for backup display
        if (line.find("cache") != string::npos || line.find("Cache") != string::npos) {
            if(line.find(":") != string::npos) {
                cache_lines.push_back(line);
            }
        }
    }

    // Build report
    ss << "Cores: " << sysconf(_SC_NPROCESSORS_ONLN) << "\n";

    if (!hardware.empty()) {
        ss << "Hardware: " << hardware << "\n";
    }
    if (!model.empty()) {
        ss << "Model: " << model << "\n";
    }

    struct utsname unameData;
    if (uname(&unameData) == 0) {
        ss << "Architecture: " << unameData.machine << "\n";
    }

    return ss.str();
}

// ============================================================================
// MEMORY INFORMATION
// ============================================================================

string getMemoryInfo()
{
    stringstream ss;
    ss << "\n=== MEMORY INFO ===\n";

    ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) {
        ss << "Error: Could not open /proc/meminfo\n";
        return ss.str();
    }

    string line;
    long totalRam = 0, freeRam = 0, availableRam = 0, buffers = 0, cached = 0;

    while(getline(meminfo, line))
    {
        if(line.find("MemTotal:") == 0)
            sscanf(line.c_str(), "MemTotal: %ld kB", &totalRam);
        else if(line.find("MemFree:") == 0)
            sscanf(line.c_str(), "MemFree: %ld kB", &freeRam);
        else if(line.find("MemAvailable:") == 0)
            sscanf(line.c_str(), "MemAvailable: %ld kB", &availableRam);
        else if(line.find("Buffers:") == 0)
            sscanf(line.c_str(), "Buffers: %ld kB", &buffers);
        else if(line.find("Cached:") == 0)
            sscanf(line.c_str(), "Cached: %ld kB", &cached);
    }
    meminfo.close();

    // Convert from kB to MB
    totalRam /= 1024;
    freeRam /= 1024;
    availableRam /= 1024;
    long usedRam = totalRam - availableRam;

    ss << "Total RAM: " << totalRam << " MB\n";
    ss << "Used RAM: " << usedRam << " MB\n";
    ss << "Available RAM: " << availableRam << " MB\n";
    ss << "Free RAM (actual): " << freeRam << " MB\n";

    return ss.str();
}

// ============================================================================
// JNI EXPORT FUNCTION
// ============================================================================

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_DeviceInfo_getDeviceInfoFromJNI(JNIEnv *env, jobject /* this */) {

    // Get CPU info first (contains hardware name)
    string cpuInfo = getCPUInfo();

    // Extract hardware name for enhanced cache detection
    string hardware;
    ifstream cpuinfo("/proc/cpuinfo");
    string line;
    while (getline(cpuinfo, line)) {
        if (line.find("Hardware") != string::npos) {
            size_t colonPos = line.find(":");
            if (colonPos != string::npos) {
                hardware = line.substr(colonPos + 2);
            }
            break;
        }
    }
    cpuinfo.close();

    // Get memory info
    string memInfo = getMemoryInfo();

    // Get cache info with hardware name for fallback detection
    string cacheInfo = getCacheInfo(hardware);

    // Combine all information
    string finalReport = cpuInfo + "\n" + memInfo + "\n" + cacheInfo;

    // Log to Android logcat for debugging
    LOGI("=== Device Info Report ===");
    LOGI("%s", finalReport.c_str());

    // Return to Java/Kotlin
    return env->NewStringUTF(finalReport.c_str());
}