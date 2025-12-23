#include <jni.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <cctype>
#include <algorithm> // Necesar pentru transform
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <android/log.h>
#include <sys/auxv.h>

#define LOG_TAG "NativeBenchmark"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

using namespace std;

struct CpuCacheSpecs {
    string humanName;
    string l1;
    string l2;
    string l3;
};

map<string, CpuCacheSpecs> socDatabase = {
        // === QUALCOMM SNAPDRAGON ===
        {"sm8650",    {"Snapdragon 8 Gen 3", "96KB(I)/48KB(D)", "2MB(P)/512KB(E)", "12MB Shared"}},
        {"pineapple", {"Snapdragon 8 Gen 3", "96KB(I)/48KB(D)", "2MB(P)/512KB(E)", "12MB Shared"}},
        {"sm8550",    {"Snapdragon 8 Gen 2", "64KB", "1MB(P)/512KB(G)", "8MB Shared"}},
        {"kalama",    {"Snapdragon 8 Gen 2", "64KB", "1MB(P)/512KB(G)", "8MB Shared"}},
        {"sm8450",    {"Snapdragon 8 Gen 1", "64KB", "512KB", "6MB Shared"}},
        {"taro",      {"Snapdragon 8 Gen 1", "64KB", "512KB", "6MB Shared"}},
        {"sm8350",    {"Snapdragon 888",     "64KB", "512KB", "4MB Shared"}},
        {"lahaina",   {"Snapdragon 888",     "64KB", "512KB", "4MB Shared"}},
        {"sm8250",    {"Snapdragon 865/870", "64KB", "512KB", "4MB Shared"}},
        {"kona",      {"Snapdragon 865/870", "64KB", "512KB", "4MB Shared"}},
        {"sm8150",    {"Snapdragon 855",     "64KB", "256KB", "2MB Shared"}},
        {"sdm845",    {"Snapdragon 845",     "64KB", "256KB", "2MB Shared"}},
        {"sm7325",    {"Snapdragon 778G",    "64KB", "256KB", "2MB"}},

        // === GOOGLE TENSOR ===
        {"zuma",      {"Google Tensor G3",   "64KB", "512KB", "4MB(SLC)+8MB(L3)"}},
        {"gs201",     {"Google Tensor G2",   "64KB", "256KB/512KB", "4MB(SLC)+8MB(L3)"}},
        {"cheetah",   {"Google Tensor G2",   "64KB", "256KB/512KB", "4MB(SLC)+8MB(L3)"}},
        {"panther",   {"Google Tensor G2",   "64KB", "256KB/512KB", "4MB(SLC)+8MB(L3)"}},
        {"gs101",     {"Google Tensor (G1)", "64KB", "256KB/512KB", "4MB(SLC)+8MB(L3)"}},
        {"oriole",    {"Google Tensor (G1)", "64KB", "256KB/512KB", "4MB(SLC)+8MB(L3)"}},
        {"raven",     {"Google Tensor (G1)", "64KB", "256KB/512KB", "4MB(SLC)+8MB(L3)"}},

        // === SAMSUNG EXYNOS ===
        {"s5e9945",   {"Exynos 2400", "64KB", "1MB", "8MB Shared"}},
        {"exynos2400",{"Exynos 2400", "64KB", "1MB", "8MB Shared"}},
        {"s5e9925",   {"Exynos 2200", "64KB", "512KB", "4MB Shared"}},
        {"exynos2200",{"Exynos 2200", "64KB", "512KB", "4MB Shared"}},
        {"exynos2100",{"Exynos 2100", "64KB", "512KB", "4MB Shared"}},

        // === MEDIATEK ===
        {"mt6989",    {"Dimensity 9300", "64KB", "512KB", "10MB+ SLC"}},
        {"mt6985",    {"Dimensity 9200", "64KB", "512KB", "8MB"}},
        {"mt6983",    {"Dimensity 9000", "64KB", "512KB", "8MB"}},
        {"mt6893",    {"Dimensity 1200", "64KB", "512KB", "2MB"}},
};
// Helper: Convert JString to C++ String
string jstringToString(JNIEnv *env, jstring jStr) {
    if (!jStr) return "";
    const char *chars = env->GetStringUTFChars(jStr, NULL);
    string str = chars;
    env->ReleaseStringUTFChars(jStr, chars);
    return str;
}

// Helper: Lowercase string
string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return tolower(c); });
    return s;
}

// Helper: Read single line
string readFile(const string &path) {
    ifstream file(path);
    if(!file.is_open()) return "";
    string line;
    getline(file, line);
    return line;
}

// Method 1: Sysfs
string getCacheFromSysfs() {
    stringstream ss;
    bool foundAny = false;

    for (int i = 0; i < 4; ++i) {
        string base_path = "/sys/devices/system/cpu/cpu0/cache/index" + to_string(i);
        ifstream check(base_path + "/size");
        if (!check.good()) continue;

        foundAny = true;
        string level = readFile(base_path + "/level");
        string size = readFile(base_path + "/size");
        string type = readFile(base_path + "/type");

        if (!level.empty() && !size.empty()) {
            ss << "L" << level << " Cache";
            if (!type.empty()) ss << " (" << type << ")";
            ss << ": " << size << "\n";
        }
    }
    if (foundAny) LOGI("Cache detection: sysfs method succeeded");
    return ss.str();
}

// Method 2: /proc/cpuinfo
string getCacheFromCpuinfo() {
    stringstream ss;
    ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) return "";

    string line;
    map<string, string> cacheInfo;

    while (getline(cpuinfo, line)) {
        if (line.find("cache size") != string::npos || line.find("cache_size") != string::npos) {
            size_t colonPos = line.find(":");
            if (colonPos != string::npos) cacheInfo["Cache size"] = line.substr(colonPos + 2);
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

// Method 3: Auxval (ARM64)
string getCacheFromAuxval() {
    stringstream ss;
#ifdef __aarch64__
    unsigned long dcache = getauxval(AT_DCACHEBSIZE);
    unsigned long icache = getauxval(AT_ICACHEBSIZE);
    if (dcache > 0 || icache > 0) {
        LOGI("Cache detection: getauxval method succeeded");
        if (dcache > 0) ss << "D-Cache Line Size: " << dcache << " bytes\n";
        if (icache > 0) ss << "I-Cache Line Size: " << icache << " bytes\n";
        return ss.str();
    }
#endif
    return "";
}

// Method 4: Device Tree
string getCacheFromDeviceTree() {
    stringstream ss;
    bool foundAny = false;
    vector<string> dt_paths = {
            "/proc/device-tree/cpus/cpu@0/d-cache-size",
            "/proc/device-tree/cpus/cpu@0/i-cache-size",
            "/proc/device-tree/cpus/cpu@0/l2-cache-size"
    };

    for (const auto& path : dt_paths) {
        ifstream file(path, ios::binary);
        if (file.is_open()) {
            uint32_t value = 0;
            file.read(reinterpret_cast<char*>(&value), sizeof(value));
            if (file.gcount() == sizeof(value) && value > 0) {
                value = __builtin_bswap32(value); // Big Endian to Host
                string name = (path.find("d-cache") != string::npos) ? "L1 D-Cache" :
                              (path.find("i-cache") != string::npos) ? "L1 I-Cache" : "L2 Cache";
                ss << name << ": " << (value / 1024) << " KB\n";
                foundAny = true;
            }
        }
    }
    if (foundAny) LOGI("Cache detection: device-tree method succeeded");
    return ss.str();
}

// Method 5: Database Lookup
// Board is optional (default empty) to allow calling with just hardware
string lookupCacheInfo(string hardware, string board = "") {
    string hwLower = toLower(hardware);
    string bdLower = toLower(board);

    stringstream ss;
    for (auto const& [key, val] : socDatabase) {
        // Check if key exists in either hardware OR board string
        if ((!bdLower.empty() && bdLower.find(key) != string::npos) ||
            (!hwLower.empty() && hwLower.find(key) != string::npos)) {

            ss << ">> DATABASE MATCH <<\n";
            ss << "SoC: " << val.humanName << "\n";
            ss << "L1: " << val.l1 << " | L2: " << val.l2 << " | L3: " << val.l3 << "\n";
            return ss.str();
        }
    }
    return "";
}

// Main aggregator
string getCacheInfo(const string& hardware) {
    stringstream ss;
    ss << "\n=== CACHE INFO ===\n";

    string res = getCacheFromSysfs();
    if (res.empty()) res = getCacheFromCpuinfo();

    ss << res;
    ss << getCacheFromAuxval();
    ss << getCacheFromDeviceTree();

    // Always try database look up as supplement if hardware name is known
    if (!hardware.empty()) {
        string dbRes = lookupCacheInfo(hardware, "");
        if (!dbRes.empty()) ss << "\n" << dbRes;
    }

    return ss.str();
}

string getCPUInfo(string& outHardware) {
    stringstream ss;
    ss << "=== CPU INFO ===\n";

    ifstream cpuinfo("/proc/cpuinfo");
    string line, model;

    while (getline(cpuinfo, line)) {
        if (outHardware.empty() && line.find("Hardware") != string::npos) {
            size_t pos = line.find(":");
            if(pos != string::npos) outHardware = line.substr(pos + 2);
        }
        if (model.empty() && line.find("model name") != string::npos) {
            size_t pos = line.find(":");
            if(pos != string::npos) model = line.substr(pos + 2);
        }
    }

    ss << "Hardware: " << outHardware << "\n";
    if(!model.empty()) ss << "Model: " << model << "\n";
    ss << "Cores: " << sysconf(_SC_NPROCESSORS_ONLN) << "\n";

    return ss.str();
}

string getMemoryInfo() {
    stringstream ss;
    ss << "\n=== MEMORY INFO ===\n";
    ifstream meminfo("/proc/meminfo");
    string line;
    long total = 0, avail = 0;

    while(getline(meminfo, line)) {
        if(line.find("MemTotal:") == 0) sscanf(line.c_str(), "MemTotal: %ld kB", &total);
        else if(line.find("MemAvailable:") == 0) sscanf(line.c_str(), "MemAvailable: %ld kB", &avail);
    }
    ss << "Total RAM: " << total/1024 << " MB\nAvailable: " << avail/1024 << " MB\n";
    return ss.str();
}

// JNI EXPORT 1: Get All Info
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_DeviceInfo_getDeviceInfoFromJNI(JNIEnv *env, jobject) {
    string hardware; // Will be filled by getCPUInfo
    string report = getCPUInfo(hardware);
    report += getMemoryInfo();
    report += getCacheInfo(hardware);

    LOGI("%s", report.c_str());
    return env->NewStringUTF(report.c_str());
}
// Helper to parse a single token like "48KB(D)" or "10MB" into bytes
long parseSimpleToken(string token) {
    long val = 0;
    string unit = "";

    // Scan for digits
    size_t i = 0;
    while(i < token.length() && !isdigit(token[i])) i++; // Skip non-digits
    if (i == token.length()) return 0;

    char* endPtr;
    val = strtol(token.c_str() + i, &endPtr, 10);

    // Check multiplier in the rest of the string
    string suffix = string(endPtr);
    if (suffix.find("MB") != string::npos || suffix.find("M") != string::npos) {
        val *= 1024 * 1024;
    } else if (suffix.find("KB") != string::npos || suffix.find("K") != string::npos) {
        val *= 1024;
    }
    return val;
}

// MAIN PARSER
long parseSmart(string raw, int level) {
    if (raw.empty()) return 0;

    // 1. Replace separators '/' and '+' with spaces for easier splitting
    replace(raw.begin(), raw.end(), '/', ' ');
    replace(raw.begin(), raw.end(), '+', ' ');

    stringstream ss(raw);
    string segment;

    long bestValue = 0;
    bool foundSpecific = false;

    while (ss >> segment) {
        long val = parseSimpleToken(segment);
        if (val == 0) continue;

        // DECISION LOGIC BASED ON LEVEL

        if (level == 1) {
            // === L1 CACHE STRATEGY ===
            // We want "D" (Data). Ignore "I" (Instruction).
            if (segment.find("(D)") != string::npos || segment.find("D") != string::npos) {
                return val; // Found explicitly marked Data cache, return immediately
            }
            if (segment.find("(I)") != string::npos || segment.find("I") != string::npos) {
                continue; // Ignore instruction cache
            }
            // If neither I nor D is marked, keep the value (e.g., "64KB")
            bestValue = val;
        }
        else if (level == 2) {
            // === L2 CACHE STRATEGY ===
            // We want "(P)" (Performance core) or simply the LARGEST value.
            // Example: "1MB(P)/512KB(E)" -> We want 1MB.
            if (segment.find("(P)") != string::npos) {
                return val; // Found explicit Performance cache
            }
            // Keep track of the largest number found so far
            if (val > bestValue) bestValue = val;
        }
        else if (level == 3) {
            // === L3 CACHE STRATEGY ===
            // We want "L3". We usually ignore "SLC" if L3 is present.
            // Example: "4MB(SLC)+8MB(L3)" -> We want 8MB.
            if (segment.find("L3") != string::npos) {
                // If we explicitly see L3, this is definitely the one.
                bestValue = val;
                foundSpecific = true;
            } else if (!foundSpecific) {
                // If we haven't found explicit "L3" yet, take the largest number
                // (This handles "8MB Shared" or "10MB")
                if (val > bestValue) bestValue = val;
            }
        }
    }
    return bestValue;
}
// JNI EXPORT 2: Manual Database Lookup
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_DeviceInfo_sentInfoToC(JNIEnv *env, jobject, jstring jHardware, jstring jBoard) {
    string hardware = jstringToString(env, jHardware);
    string board = jstringToString(env, jBoard);

    string result = lookupCacheInfo(hardware, board);

    if (result.empty()) result = "No database match for this device.";
    LOGI("Manual Lookup Result: %s", result.c_str());

    return env->NewStringUTF(result.c_str());
}
extern "C" JNIEXPORT jlongArray JNICALL
Java_com_example_myapplication_MemoryPerformanceActivity_getCacheSizeBytes(JNIEnv *env, jobject, jstring jHardware, jstring jBoard)
{
    const char *boardCons = env->GetStringUTFChars(jBoard,0);
    const char *hardwareCons = env->GetStringUTFChars(jHardware,0);

    string board = toLower(string(boardCons));
    string hardware = toLower(string(hardwareCons));

    env->ReleaseStringUTFChars(jBoard,boardCons);
    env->ReleaseStringUTFChars(jHardware,hardwareCons);

    long l1Size = 0, l2Size = 0, l3Size = 0;
    bool found = false;

    // Iterate through Global Database
    for (auto const& [key, val] : socDatabase) {
        // Check if key is inside board or hardware string
        if (board.find(key) != string::npos || hardware.find(key) != string::npos) {
            l1Size = parseSmart(val.l1, 1);
            l2Size = parseSmart(val.l2, 2);
            l3Size = parseSmart(val.l3, 3);
            found = true;
            break;
        }
    }

    if (!found) {
        // Defaults if not in database
        l1Size = 32 * 1024;
        l2Size = 512 * 1024; // Fixed 513 typo
        l3Size = 2 * 1024 * 1024;
    }

    jlongArray res = env->NewLongArray(3);
    if(res == NULL) return NULL;

    jlong tempBuffer[3];
    tempBuffer[0] = l1Size;
    tempBuffer[1] = l2Size;
    tempBuffer[2] = l3Size;

    env->SetLongArrayRegion(res, 0, 3, tempBuffer);
    return res;
}