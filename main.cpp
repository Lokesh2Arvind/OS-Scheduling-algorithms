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
            for (int i = 0; i < n; ++i) {
                if (!done[i] && plist[i].arrival <= time && plist[i].burst < min_burst) {
                    min_burst = plist[i].burst;
                    idx = i;
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

    // Priority Non-Preemptive
    void runPriority_NP() {
        resetProcesses();
        vector<Process> plist = processes;
        int n = plist.size(), completed = 0, time = 0, prev_pid = -1;
        vector<bool> done(n, false);

        while (completed < n) {
            int idx = -1, min_pri = INT_MAX;
            for (int i = 0; i < n; ++i) {
                if (!done[i] && plist[i].arrival <= time && plist[i].priority < min_pri) {
                    min_pri = plist[i].priority;
                    idx = i;
                }
            }
            if (idx == -1) { time++; continue; }
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
        printResults("Priority (NP)", plist);
    }

    // Priority Preemptive
    void runPriority_P() {
        resetProcesses();
        vector<Process> plist = processes;
        int n = plist.size(), completed = 0, time = 0, prev_pid = -1;
        vector<bool> done(n, false);

        while (completed < n) {
            int idx = -1, min_pri = INT_MAX;
            for (int i = 0; i < n; ++i) {
                if (!done[i] && plist[i].arrival <= time && plist[i].priority < min_pri && plist[i].burst_remain > 0) {
                    min_pri = plist[i].priority;
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
        printResults("Priority (P)", plist);
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
            for (int i = 0; i < n; ++i)
                if (!in_queue[i] && plist[i].arrival <= time && plist[i].burst_remain > 0)
                    ready.push(i), in_queue[i] = true;
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

    // MLQ
    void runMLQ() {
        resetProcesses();
        vector<Process> q1, q2, q3;
        for (auto p : processes) {
            if (p.priority >= 7) q1.push_back(p);      // Highest
            else if (p.priority >= 4) q2.push_back(p); // Medium
            else q3.push_back(p);                      // Lowest
        }

        int time = 0, prev_pid = -1;
        vector<GanttEntry> mlq_gantt;
        int total = q1.size() + q2.size() + q3.size();

        int completed = runQueueRR(q1, 4, time, prev_pid, mlq_gantt);
        completed += runQueuePriorityNP(q2, time, prev_pid, mlq_gantt);
        completed += runQueueFCFS(q3, time, prev_pid, mlq_gantt);

        vector<Process> all;
        all.insert(all.end(), q1.begin(), q1.end());
        all.insert(all.end(), q2.begin(), q2.end());
        all.insert(all.end(), q3.begin(), q3.end());

        gantt = mlq_gantt;
        printResults("MLQ", all);
    }

    // MLFQ
    void runMLFQ() {
        resetProcesses();
        vector<Process> plist = processes;
        int n = plist.size(), completed = 0, time = 0, prev_pid = -1;
        queue<int> q1, q2, q3;
        vector<int> level(n, 0);
        vector<bool> in_queue(n, false);

        const int q1_quantum = 8, q2_quantum = 16;
        while (completed < n) {
            for (int i = 0; i < n; ++i)
                if (!in_queue[i] && plist[i].arrival <= time && plist[i].burst_remain > 0)
                    q1.push(i), in_queue[i] = true, level[i] = 0;
            int idx = -1, exec = 0, queue_used = 0;
            if (!q1.empty()) { idx = q1.front(); q1.pop(); exec = min(q1_quantum, plist[idx].burst_remain); queue_used = 1; }
            else if (!q2.empty()) { idx = q2.front(); q2.pop(); exec = min(q2_quantum, plist[idx].burst_remain); queue_used = 2; }
            else if (!q3.empty()) { idx = q3.front(); q3.pop(); exec = plist[idx].burst_remain; queue_used = 3; }
            else { time++; continue; }
            if (prev_pid != -1 && prev_pid != plist[idx].pid)
                time += context_switch_overhead;
            if (plist[idx].start_time == -1)
                plist[idx].start_time = time, plist[idx].response = time - plist[idx].arrival;
            gantt.push_back({plist[idx].pid, time, time + exec, prev_pid != -1 && prev_pid != plist[idx].pid});
            time += exec;
            plist[idx].burst_remain -= exec;
            for (int i = 0; i < n; ++i)
                if (!in_queue[i] && plist[i].arrival <= time && plist[i].burst_remain > 0)
                    q1.push(i), in_queue[i] = true, level[i] = 0;
            if (plist[idx].burst_remain == 0) {
                plist[idx].completion = time;
                plist[idx].turnaround = plist[idx].completion - plist[idx].arrival;
                plist[idx].waiting = plist[idx].turnaround - plist[idx].burst;
                completed++;
            } else {
                if (queue_used == 1) { q2.push(idx); level[idx] = 1; }
                else if (queue_used == 2) { q3.push(idx); level[idx] = 2; }
                else { q3.push(idx); }
            }
            prev_pid = plist[idx].pid;
        }
        printResults("MLFQ", plist);
    }

    // Helpers for MLQ
    int runQueueRR(vector<Process>& q, int quantum, int& time, int& prev_pid, vector<GanttEntry>& local_gantt) {
        int finished = 0, n = q.size();
        queue<int> ready;
        vector<bool> in_queue(n, false);
        while (finished < n) {
            for (int i = 0; i < n; ++i)
                if (!in_queue[i] && q[i].arrival <= time && q[i].burst_remain > 0)
                    ready.push(i), in_queue[i] = true;
            if (ready.empty()) { time++; continue; }
            int idx = ready.front(); ready.pop();
            if (prev_pid != -1 && prev_pid != q[idx].pid)
                time += context_switch_overhead;
            if (q[idx].start_time == -1)
                q[idx].start_time = time, q[idx].response = time - q[idx].arrival;
            int exec = min(quantum, q[idx].burst_remain);
            local_gantt.push_back({q[idx].pid, time, time + exec, prev_pid != -1 && prev_pid != q[idx].pid});
            time += exec;
            q[idx].burst_remain -= exec;
            for (int i = 0; i < n; ++i)
                if (!in_queue[i] && q[i].arrival <= time && q[i].burst_remain > 0)
                    ready.push(i), in_queue[i] = true;
            if (q[idx].burst_remain == 0) {
                q[idx].completion = time;
                q[idx].turnaround = q[idx].completion - q[idx].arrival;
                q[idx].waiting = q[idx].turnaround - q[idx].burst;
                finished++;
            } else {
                ready.push(idx);
            }
            prev_pid = q[idx].pid;
        }
        return finished;
    }
    int runQueuePriorityNP(vector<Process>& q, int& time, int& prev_pid, vector<GanttEntry>& local_gantt) {
        int finished = 0, n = q.size();
        vector<bool> done(n, false);
        while (finished < n) {
            int idx = -1, min_pri = INT_MAX;
            for (int i = 0; i < n; ++i)
                if (!done[i] && q[i].arrival <= time && q[i].priority < min_pri)
                    min_pri = q[i].priority, idx = i;
            if (idx == -1) { time++; continue; }
            if (prev_pid != -1 && prev_pid != q[idx].pid)
                time += context_switch_overhead;
            q[idx].start_time = time;
            q[idx].response = time - q[idx].arrival;
            local_gantt.push_back({q[idx].pid, time, time + q[idx].burst, prev_pid != -1 && prev_pid != q[idx].pid});
            time += q[idx].burst;
            q[idx].completion = time;
            q[idx].turnaround = q[idx].completion - q[idx].arrival;
            q[idx].waiting = q[idx].turnaround - q[idx].burst;
            done[idx] = true;
            finished++;
            prev_pid = q[idx].pid;
        }
        return finished;
    }
    int runQueueFCFS(vector<Process>& q, int& time, int& prev_pid, vector<GanttEntry>& local_gantt) {
        int finished = 0;
        sort(q.begin(), q.end(), [](const Process& a, const Process& b) { return a.arrival < b.arrival; });
        for (auto& p : q) {
            if (time < p.arrival) time = p.arrival;
            if (prev_pid != -1 && prev_pid != p.pid)
                time += context_switch_overhead;
            p.start_time = time;
            p.response = time - p.arrival;
            local_gantt.push_back({p.pid, time, time + p.burst, prev_pid != -1 && prev_pid != p.pid});
            time += p.burst;
            p.completion = time;
            p.turnaround = p.completion - p.arrival;
            p.waiting = p.turnaround - p.burst;
            finished++;
            prev_pid = p.pid;
        }
        return finished;
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
    sched.runPriority_NP();
    sched.runPriority_P();
    sched.runRR(4); // Example quantum
    sched.runMLQ();
    sched.runMLFQ();

    return 0;
}
