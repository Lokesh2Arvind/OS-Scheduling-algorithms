#include<bits/stdc++.h>
using namespace std;

// Process class
class Process {
public:
    int pid;
    int arrival;
    int burst;
    int burst_remain;
    int priority;
    int start_time = -1;
    int completion = 0;
    int turnaround = 0;
    int waiting = 0;
    int response = -1;
    int queue_level = 0; // For MLQ/MLFQ

    Process(int id, int a, int b, int p){
        pid=id;
        arrival=a;
        burst=b;
        burst_remain=b;
        priority=p;
    }
};

struct GanttEntry {
    int pid;
    int start;
    int end;
    bool context_switch;
};

class Scheduler {
    int context_switch_overhead;
    vector<Process> processes;
    vector<GanttEntry> gantt;

public:
    Scheduler(vector<Process> plist, int cs_overhead) {
        processes=plist; 
        context_switch_overhead=cs_overhead;
    }

    void resetProcesses() {
        for (auto& p : processes) {
            p.burst_remain = p.burst;
            p.start_time = -1;
            p.completion = 0;
            p.turnaround = 0;
            p.waiting = 0;
            p.response = -1;
            p.queue_level = 0;
        }
        gantt.clear();
    }

    // FCFS
    void runFCFS() {
        resetProcesses();
        vector<Process> plist = processes;
        sort(plist.begin(), plist.end(), [](const Process& a, const Process& b) {
            return a.arrival < b.arrival;
        });

        int time = 0, prev_pid = -1;
        for (auto& p : plist) {
            if (time < p.arrival) 
                time = p.arrival;
            if (prev_pid != -1 && prev_pid != p.pid)
                time += context_switch_overhead;
            p.start_time = time;
            p.response = time - p.arrival;
            time += p.burst;
            p.completion = time;
            p.turnaround = p.completion - p.arrival;
            p.waiting = p.turnaround - p.burst;
            gantt.push_back({p.pid, p.start_time, p.completion, prev_pid != -1 && prev_pid != p.pid});
            prev_pid = p.pid;
        }
        printResults("FCFS", plist);
    }

    // SJF Non-Preemptive
    void runSJF_NP() {
        resetProcesses();
        vector<Process> plist = processes;
        int n = plist.size(), completed = 0, time = 0, prev_pid = -1;
        vector<bool> done(n, false);

        while (completed < n) {
            int idx = -1, min_burst = INT_MAX;
            int min_time=0;
            for (int i = 0; i < n; ++i) {
                if (!done[i] && plist[i].arrival <= time && plist[i].burst < min_burst) {
                    min_burst = plist[i].burst;
                    idx = i;
                }else if(!done[i] ){
                    min_time=min(min_time,plist[i].arrival);
                }
            }
            if (idx == -1) {
                time++; continue;
            }
            if (prev_pid != -1 && prev_pid != plist[idx].pid)
                time += context_switch_overhead;
            plist[idx].start_time = time;
            plist[idx].response = time - plist[idx].arrival;
            time += plist[idx].burst;
            plist[idx].completion = time;
            plist[idx].turnaround = plist[idx].completion - plist[idx].arrival;
            plist[idx].waiting = plist[idx].turnaround - plist[idx].burst;
            done[idx] = true;
            completed++;
            gantt.push_back({plist[idx].pid, plist[idx].start_time, plist[idx].completion, prev_pid != -1 && prev_pid != plist[idx].pid});
            prev_pid = plist[idx].pid;
        }
        printResults("SJF (NP)", plist);
    }

    // SJF Preemptive (SRTF)
    void runSJF_P() {
        resetProcesses();
        vector<Process> plist = processes;
        int n = plist.size(), completed = 0, time = 0, prev_pid = -1;
        vector<bool> done(n, false);

        while (completed < n) {
            int idx = -1, min_burst = INT_MAX;
            for (int i = 0; i < n; ++i) {
                if (!done[i] && plist[i].arrival <= time && plist[i].burst_remain < min_burst && plist[i].burst_remain > 0) {
                    min_burst = plist[i].burst_remain;
                    idx = i;
                }
            }
            if (idx == -1) { time++; continue; }
            if (prev_pid != -1 && prev_pid != plist[idx].pid)
                time += context_switch_overhead;
            if (plist[idx].start_time == -1)
                plist[idx].start_time = time, plist[idx].response = time - plist[idx].arrival;
            int next_time = time + 1;
            plist[idx].burst_remain--;
            if (plist[idx].burst_remain == 0) {
                plist[idx].completion = next_time;
                plist[idx].turnaround = plist[idx].completion - plist[idx].arrival;
                plist[idx].waiting = plist[idx].turnaround - plist[idx].burst;
                done[idx] = true;
                completed++;
            }
            gantt.push_back({plist[idx].pid, time, next_time, prev_pid != -1 && prev_pid != plist[idx].pid});
            prev_pid = plist[idx].pid;
            time = next_time;
        }
        printResults("SJF (P)", plist);
    }

   

    // Round Robin
    void runRR(int quantum) {
        resetProcesses();
        vector<Process> plist = processes;
        int n = plist.size(), completed = 0, time = 0, prev_pid = -1;
        queue<int> ready;
        vector<bool> in_queue(n, false);

        while (completed < n) {
            for (int i = 0; i < n; ++i)
                if (!in_queue[i] && plist[i].arrival <= time && plist[i].burst_remain > 0)
                    ready.push(i), in_queue[i] = true;
            if (ready.empty()) { time++; continue; }
            int idx = ready.front(); ready.pop();
            if (prev_pid != -1 && prev_pid != plist[idx].pid)
                time += context_switch_overhead;
            if (plist[idx].start_time == -1)
                plist[idx].start_time = time, plist[idx].response = time - plist[idx].arrival;
            int exec = min(quantum, plist[idx].burst_remain);
            gantt.push_back({plist[idx].pid, time, time + exec, prev_pid != -1 && prev_pid != plist[idx].pid});
            time += exec;
            plist[idx].burst_remain -= exec;
            
            if (plist[idx].burst_remain == 0) {
                plist[idx].completion = time;
                plist[idx].turnaround = plist[idx].completion - plist[idx].arrival;
                plist[idx].waiting = plist[idx].turnaround - plist[idx].burst;
                completed++;
            } else {
                ready.push(idx);
            }
            prev_pid = plist[idx].pid;
        }
        printResults("RR", plist);
    }

    
    // Highest Response Ratio Next (Non-preemptive)
    void runHRRN() {
        resetProcesses();
        vector<Process> plist = processes;
        int n = plist.size(), completed = 0, time = 0, prev_pid = -1;
        vector<bool> done(n, false);

        while (completed < n) {
            int idx = -1;
            double best_ratio = -1.0;

            // Find the process with the highest response ratio
            for (int i = 0; i < n; i++) {
                if (!done[i] && plist[i].arrival <= time) {
                    int waiting = time - plist[i].arrival;
                    double ratio = (double)(waiting + plist[i].burst) / plist[i].burst;
                    if (ratio > best_ratio) {
                        best_ratio = ratio;
                        idx = i;
                    }
                }
            }

            if (idx == -1) { 
                time++; 
                continue; 
            }

            if (prev_pid != -1 && prev_pid != plist[idx].pid)
                time += context_switch_overhead;

            plist[idx].start_time = time;
            plist[idx].response = time - plist[idx].arrival;
            time += plist[idx].burst;
            plist[idx].completion = time;
            plist[idx].turnaround = plist[idx].completion - plist[idx].arrival;
            plist[idx].waiting = plist[idx].turnaround - plist[idx].burst;

            done[idx] = true;
            completed++;

            gantt.push_back({plist[idx].pid, plist[idx].start_time, plist[idx].completion, prev_pid != -1 && prev_pid != plist[idx].pid});
            prev_pid = plist[idx].pid;
        }

        printResults("HRRN", plist);
    }

    // Priority Scheduling with Aging (Prevents starvation)
    void runPriorityAging(int aging_rate = 1) {
        resetProcesses();
        vector<Process> plist = processes;
        int n = plist.size(), completed = 0, time = 0, prev_pid = -1;
        vector<bool> done(n, false);

        while (completed < n) {
            int idx = -1, best_priority = INT_MAX;

            // Find the process with best (lowest) priority
            for (int i = 0; i < n; i++) {
                if (!done[i] && plist[i].arrival <= time) {
                    if (plist[i].priority < best_priority) {
                        best_priority = plist[i].priority;
                        idx = i;
                    }
                }
            }

            if (idx == -1) { 
                time++; 
                continue; 
            }

            // Aging: all waiting processes improve priority
            for (int i = 0; i < n; i++) {
                if (!done[i] && plist[i].arrival <= time && i != idx) {
                    plist[i].priority = max(1, plist[i].priority - aging_rate);
                }
            }

            if (prev_pid != -1 && prev_pid != plist[idx].pid)
                time += context_switch_overhead;

            plist[idx].start_time = time;
            plist[idx].response = time - plist[idx].arrival;
            time += plist[idx].burst;
            plist[idx].completion = time;
            plist[idx].turnaround = plist[idx].completion - plist[idx].arrival;
            plist[idx].waiting = plist[idx].turnaround - plist[idx].burst;

            done[idx] = true;
            completed++;

            gantt.push_back({plist[idx].pid, plist[idx].start_time, plist[idx].completion, prev_pid != -1 && prev_pid != plist[idx].pid});
            prev_pid = plist[idx].pid;
        }

        printResults("Priority + Aging", plist);
    }

    // Print Gantt chart and process table
    void printResults(const string& algo, const vector<Process>& plist) {
        cout << "\n===== " << algo << " =====\n";
        cout << "Gantt Chart:\n|";
        for (auto& g : gantt)
            cout << " P" << g.pid << " |";
        cout << "\n0";
        for (auto& g : gantt)
            cout << setw(5) << g.end;
        cout << "\n";
        cout << "\nPID\tAT\tBT\tPRI\tCT\tTAT\tWT\tRT\n";
        double sum_tat = 0, sum_wt = 0, sum_rt = 0;
        for (const auto& p : plist) {
            cout << "P" << p.pid << "\t" << p.arrival << "\t" << p.burst << "\t"
                 << p.priority << "\t" << p.completion << "\t" << p.turnaround << "\t"
                 << p.waiting << "\t" << p.response << "\n";
            sum_tat += p.turnaround;
            sum_wt += p.waiting;
            sum_rt += p.response;
        }
        int n = plist.size();
        cout << fixed << setprecision(2);
        cout << "Avg TAT: " << sum_tat / n << ", Avg WT: " << sum_wt / n << ", Avg RT: " << sum_rt / n << "\n";
        gantt.clear();
    }
};

void generateRandomProcesses(vector<Process>& plist, int n, int seed = 42) {
    mt19937 gen(seed);
    uniform_int_distribution<> arrival_dist(0, 10), burst_dist(2, 20), pri_dist(1, 9);
    for (int i = 0; i < n; ++i) {
        int at = arrival_dist(gen);
        int bt = burst_dist(gen);
        int pr = pri_dist(gen);
        plist.emplace_back(i, at, bt, pr);
    }
}

void userInputProcesses(vector<Process>& plist, int n) {
    for (int i = 0; i < n; ++i) {
        int at, bt, pr;
        cout << "Enter arrival, burst, priority for P" << i << ": ";
        cin >> at >> bt >> pr;
        plist.emplace_back(i, at, bt, pr);
    }
}

int main() {
    int n, cs_overhead = 1, input_mode;
    cout << "Number of processes: "; cin >> n;
    cout << "Context switch overhead (default 1): "; cin >> cs_overhead;
    cout << "Enter 1 for random input, 2 for manual: "; cin >> input_mode;
    vector<Process> plist;
    if (input_mode == 1) generateRandomProcesses(plist, n);
    else userInputProcesses(plist, n);

    cout << "\nProcess List:\nPID\tAT\tBT\tPRI\n";
    for (const auto& p : plist)
        cout << "P" << p.pid << "\t" << p.arrival << "\t" << p.burst << "\t" << p.priority << "\n";

    Scheduler sched(plist, cs_overhead);
    sched.runFCFS();
    sched.runSJF_NP();
    sched.runSJF_P();
    sched.runRR(4);
    sched.runHRRN();
    sched.runPriorityAging();
    // end of the program
    return 0;
}
