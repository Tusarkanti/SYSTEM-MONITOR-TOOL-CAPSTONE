#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// Terminal Colors
#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define RED     "\033[31m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"
#define CLEAR   "\033[2J\033[1;1H"

// Format progress bar
string bar(float value){
    int width = 30;
    int filled = (value/100.0f)*width;
    string b = "[";
    for(int i=0;i<width;i++) b += (i<filled ? "#" : "-");
    b += "]";
    return b;
}

// CPU Usage
float getCPU() {
    static long long lastTotal=0, lastIdle=0;
    ifstream file("/proc/stat");
    string cpu;
    long long user, nice, system, idle, iowait, irq, softirq;
    file >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
    long long idleTime = idle + iowait;
    long long totalTime = user + nice + system + idle + iowait + irq + softirq;
    long long diffIdle = idleTime - lastIdle;
    long long diffTotal = totalTime - lastTotal;
    lastIdle = idleTime; lastTotal = totalTime;
    return (diffTotal - diffIdle) * 100.0f / diffTotal;
}

// Memory Usage
float getMemory() {
    ifstream file("/proc/meminfo");
    string key; long long total=0, free=0;
    while(file >> key) {
        if(key=="MemTotal:") file >> total;
        if(key=="MemAvailable:") { file >> free; break; }
        file.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    return (float)(total-free)*100.0f/(float)total;
}

// Process Info
struct Proc { int pid; float usage; string name; };

float procCPU(int pid){
    string path="/proc/"+to_string(pid)+"/stat";
    ifstream f(path);
    if(!f) return 0;
    string ignore; long long utime, stime;
    for(int i=0;i<13;i++) f>>ignore;
    f >> utime >> stime;
    return (utime+stime)/100.0f;
}

vector<Proc> getProcs(){
    vector<Proc> list;
    for(auto &entry: fs::directory_iterator("/proc")){
        if(entry.is_directory()){
            string d=entry.path().filename().string();
            if(all_of(d.begin(), d.end(), ::isdigit)){
                int pid=stoi(d);
                ifstream nameFile("/proc/"+d+"/comm");
                string name; getline(nameFile, name);
                float usage=procCPU(pid);
                if(usage>0) list.push_back({pid,usage,name});
            }
        }
    }
    sort(list.begin(), list.end(), [](auto &a,auto &b){return a.usage>b.usage;});
    if(list.size()>8) list.resize(8);
    return list;
}

int main(){
    while(true){
        system("clear");

        float cpu = getCPU();
        float mem = getMemory();
        auto procs = getProcs();

        cout << CYAN << BOLD << "\n=== REAL-TIME SYSTEM MONITOR DASHBOARD ===\n" << RESET;

        cout << BOLD << "\nCPU Usage:   " << RESET << cpu << "% " << bar(cpu) << "\n";
        cout << BOLD << "Memory Usage:" << RESET << mem << "% " << bar(mem) << "\n\n";

        cout << YELLOW << BOLD << "Top Processes (PID | CPU% | NAME)\n" << RESET;
        cout << "------------------------------------\n";
        for(auto &p: procs)
            cout << p.pid << "\t" << p.usage << "\t" << p.name << "\n";

        cout << "\n" << CYAN << "Updating every 1 second..." << RESET << "\n";
        this_thread::sleep_for(chrono::seconds(1));
    }
    return 0;
}
