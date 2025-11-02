#include <jni.h>
#include <string>
#include <sstream> //
#include <fstream>
#include <vector>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <android/log.h> //ca sa scriu in logul androidului

#define LOG_TAG "NativeBenchmark"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

using namespace std;
string readFile(const string &path)
{
    ifstream file(path);
    if(!file.is_open())
        return "N/A";
    string line;
    getline(file, line);
    return line;
}

string getCPUInfo() {
    stringstream ss;
    ss << "CPU INFO \n";

    ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        ss << "Eroare: Nu am putut deschide /proc/cpuinfo\n";
        return ss.str();
    }

    string line;
    string hardware, model;
    bool cacheInfoFound = false;

    // Citim linie cu linie întregul fișier /proc/cpuinfo
    while (getline(cpuinfo, line)) {
        // Căutăm informațiile generale o singură dată
        if (hardware.empty() && line.find("Hardware") != string::npos) {
            hardware = line.substr(line.find(":") + 2);
        } else if (model.empty() && line.find("model name") != string::npos) {
            model = line.substr(line.find(":") + 2);
        }

        if (line.find("L1d cache") != string::npos ||
            line.find("L1i cache") != string::npos ||
            line.find("L2 cache") != string::npos ||
            line.find("L3 cache") != string::npos)
        {
            if (!cacheInfoFound) {
                ss << "\n--- CACHE INFO (din /proc/cpuinfo) ---\n";
                cacheInfoFound = true;
            }

            ss << line << "\n";
        }
    }

    stringstream final_report;
    final_report << "CPU INFO \n";
    final_report << "Cores: " << sysconf(_SC_NPROCESSORS_ONLN) << "\n";

    if (!hardware.empty())
        final_report << "Hardware: " << hardware << "\n";
    if (!model.empty())
        final_report << "Model: " << model << "\n";

    struct utsname unameData;
    if(uname(&unameData) == 0) {
        final_report << "Architecture: " << unameData.machine << "\n";
    }

    // Adăugăm la sfârșit secțiunea de cache, dacă a fost găsită
    if (cacheInfoFound) {
        final_report << ss.str().substr(ss.str().find("--- CACHE INFO"));
    } else {
        final_report << "\nCache Info: N/A (informații indisponibile în /proc/cpuinfo)\n";
    }

    return final_report.str();
}
string getMemoryInfo()
{
    stringstream ss;
    ss << "MEMORY INFO \n";
    struct sysinfo info;
    if(sysinfo(&info) == 0)
    {
        long totalRam = info.totalram * info.mem_unit;
        long freeRam = info.freeram * info.mem_unit;
        long usedRam = totalRam - freeRam;

        ss<< "Total RAM: " << totalRam / (1024 * 1024) << " MB\n";
        ss<< "Used RAM: " << usedRam / (1024 * 1024) << " MB\n";
        ss<< "Free RAM: " << freeRam / (1024 * 1024) << " MB\n";
    }
    return ss.str();
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_DeviceInfo_getDeviceInfoFromJNI(JNIEnv *env, jobject /* this */) {
    string cpuInfo = getCPUInfo();
    string memInfo = getMemoryInfo();


    string finalReport = cpuInfo + "\n" + memInfo + "\n" ;
    LOGI("%s", finalReport.c_str());
    return env->NewStringUTF(finalReport.c_str());
}