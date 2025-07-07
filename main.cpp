#include <bits/stdc++.h>
#include "parser.h"

#define all(v) v.begin(), v.end()

using namespace std;

/** Global Constants **/
const string TRACE_MODE = "trace";
const string STATISTICS_MODE = "stats";
const string SCHEDULER_NAMES[9] = {"", "FCFS", "RR-", "SPN", "SRT", "HRRN", "FB-1", "FB-2i", "AGING"};

// Sorting helpers
bool sortByBurstTime(const tuple<string, int, int>& a, const tuple<string, int, int>& b) {
    return (get<2>(a) < get<2>(b));
}
bool sortByArrivalTime(const tuple<string, int, int>& a, const tuple<string, int, int>& b) {
    return (get<1>(a) < get<1>(b));
}
bool sortByResponseRatioDesc(const tuple<string, double, int>& a, const tuple<string, double, int>& b) {
    return get<1>(a) > get<1>(b);
}
bool sortByPriority(const tuple<int, int, int>& a, const tuple<int, int, int>& b) {
    if (get<0>(a) == get<0>(b))
        return get<2>(a) > get<2>(b);
    return get<0>(a) > get<0>(b);
}

// Timeline management
void clearScheduleTimeline() {
    for (int t = 0; t < totalTimeUnits; t++)
        for (int p = 0; p < totalProcesses; p++)
            scheduleTimeline[t][p] = ' ';
}

// Process tuple accessors
string getProcessID(tuple<string, int, int>& proc) { return get<0>(proc); }
int getArrival(tuple<string, int, int>& proc) { return get<1>(proc); }
int getBurst(tuple<string, int, int>& proc) { return get<2>(proc); }
int getPriority(tuple<string, int, int>& proc) { return get<2>(proc); }

// Response ratio calculation
double computeResponseRatio(int wait, int burst) {
    return (wait + burst) * 1.0 / burst;
}

// Fill in waiting periods in the timeline
void markWaitingPeriods() {
    for (int i = 0; i < totalProcesses; i++) {
        int arrival = getArrival(processList[i]);
        for (int t = arrival; t < completionTime[i]; t++) {
            if (scheduleTimeline[t][i] != '*')
                scheduleTimeline[t][i] = '.';
        }
    }
}

// First-Come-First-Serve
void runFCFS() {
    int currentTime = getArrival(processList[0]);
    for (int i = 0; i < totalProcesses; i++) {
        int procIdx = i;
        int arrival = getArrival(processList[i]);
        int burst = getBurst(processList[i]);

        completionTime[procIdx] = (currentTime + burst);
        turnaroundTime[procIdx] = (completionTime[procIdx] - arrival);
        normalizedTurnaround[procIdx] = (turnaroundTime[procIdx] * 1.0 / burst);

        for (int t = currentTime; t < completionTime[procIdx]; t++)
            scheduleTimeline[t][procIdx] = '*';
        for (int t = arrival; t < currentTime; t++)
            scheduleTimeline[t][procIdx] = '.';
        currentTime += burst;
    }
}

// Round Robin
void runRoundRobin(int quantum) {
    queue<pair<int, int>> rrQueue; // (procIdx, remainingBurst)
    int nextProc = 0;
    if (getArrival(processList[nextProc]) == 0) {
        rrQueue.push({nextProc, getBurst(processList[nextProc])});
        nextProc++;
    }
    int timeSlice = quantum;
    for (int t = 0; t < totalTimeUnits; t++) {
        if (!rrQueue.empty()) {
            int procIdx = rrQueue.front().first;
            rrQueue.front().second--;
            int remainingBurst = rrQueue.front().second;
            int arrival = getArrival(processList[procIdx]);
            int burst = getBurst(processList[procIdx]);
            timeSlice--;
            scheduleTimeline[t][procIdx] = '*';
            while (nextProc < totalProcesses && getArrival(processList[nextProc]) == t + 1) {
                rrQueue.push({nextProc, getBurst(processList[nextProc])});
                nextProc++;
            }

            if (timeSlice == 0 && remainingBurst == 0) {
                completionTime[procIdx] = t + 1;
                turnaroundTime[procIdx] = (completionTime[procIdx] - arrival);
                normalizedTurnaround[procIdx] = (turnaroundTime[procIdx] * 1.0 / burst);
                timeSlice = quantum;
                rrQueue.pop();
            } else if (timeSlice == 0 && remainingBurst != 0) {
                rrQueue.pop();
                rrQueue.push({procIdx, remainingBurst});
                timeSlice = quantum;
            } else if (timeSlice != 0 && remainingBurst == 0) {
                completionTime[procIdx] = t + 1;
                turnaroundTime[procIdx] = (completionTime[procIdx] - arrival);
                normalizedTurnaround[procIdx] = (turnaroundTime[procIdx] * 1.0 / burst);
                rrQueue.pop();
                timeSlice = quantum;
            }
        }
        while (nextProc < totalProcesses && getArrival(processList[nextProc]) == t + 1) {
            rrQueue.push({nextProc, getBurst(processList[nextProc])});
            nextProc++;
        }
    }
    markWaitingPeriods();
}

// Shortest Process Next (Non-preemptive)
void runSPN() {
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq; // (burst, procIdx)
    int nextProc = 0;
    for (int t = 0; t < totalTimeUnits; t++) {
        while (nextProc < totalProcesses && getArrival(processList[nextProc]) <= t) {
            pq.push({getBurst(processList[nextProc]), nextProc});
            nextProc++;
        }
        if (!pq.empty()) {
            int procIdx = pq.top().second;
            int arrival = getArrival(processList[procIdx]);
            int burst = getBurst(processList[procIdx]);
            pq.pop();

            int temp = arrival;
            for (; temp < t; temp++)
                scheduleTimeline[temp][procIdx] = '.';
            temp = t;
            for (; temp < t + burst; temp++)
                scheduleTimeline[temp][procIdx] = '*';

            completionTime[procIdx] = (t + burst);
            turnaroundTime[procIdx] = (completionTime[procIdx] - arrival);
            normalizedTurnaround[procIdx] = (turnaroundTime[procIdx] * 1.0 / burst);
            t = temp - 1;
        }
    }
}

// Shortest Remaining Time (Preemptive)
void runSRT() {
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq; // (remaining, procIdx)
    int nextProc = 0;
    for (int t = 0; t < totalTimeUnits; t++) {
        while (nextProc < totalProcesses && getArrival(processList[nextProc]) == t) {
            pq.push({getBurst(processList[nextProc]), nextProc});
            nextProc++;
        }
        if (!pq.empty()) {
            int procIdx = pq.top().second;
            int remaining = pq.top().first;
            pq.pop();
            int burst = getBurst(processList[procIdx]);
            int arrival = getArrival(processList[procIdx]);
            scheduleTimeline[t][procIdx] = '*';

            if (remaining == 1) {
                completionTime[procIdx] = t + 1;
                turnaroundTime[procIdx] = (completionTime[procIdx] - arrival);
                normalizedTurnaround[procIdx] = (turnaroundTime[procIdx] * 1.0 / burst);
            } else {
                pq.push({remaining - 1, procIdx});
            }
        }
    }
    markWaitingPeriods();
}

// Highest Response Ratio Next
void runHRRN() {
    vector<tuple<string, double, int>> readyQueue; // (procID, responseRatio, timeServed)
    int nextProc = 0;
    for (int t = 0; t < totalTimeUnits; t++) {
        while (nextProc < totalProcesses && getArrival(processList[nextProc]) <= t) {
            readyQueue.push_back({getProcessID(processList[nextProc]), 1.0, 0});
            nextProc++;
        }
        for (auto& proc : readyQueue) {
            string procID = get<0>(proc);
            int procIdx = processIDtoIndex[procID];
            int wait = t - getArrival(processList[procIdx]);
            int burst = getBurst(processList[procIdx]);
            get<1>(proc) = computeResponseRatio(wait, burst);
        }
        sort(all(readyQueue), sortByResponseRatioDesc);

        if (!readyQueue.empty()) {
            int procIdx = processIDtoIndex[get<0>(readyQueue[0])];
            while (t < totalTimeUnits && get<2>(readyQueue[0]) != getBurst(processList[procIdx])) {
                scheduleTimeline[t][procIdx] = '*';
                t++;
                get<2>(readyQueue[0])++;
            }
            t--;
            readyQueue.erase(readyQueue.begin());
            completionTime[procIdx] = t + 1;
            turnaroundTime[procIdx] = completionTime[procIdx] - getArrival(processList[procIdx]);
            normalizedTurnaround[procIdx] = (turnaroundTime[procIdx] * 1.0 / getBurst(processList[procIdx]));
        }
    }
    markWaitingPeriods();
}

// Feedback Queue FB-1
void runFB1() {
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq; // (priority, procIdx)
    unordered_map<int, int> remainingBurst;
    int nextProc = 0;
    if (getArrival(processList[0]) == 0) {
        pq.push({0, nextProc});
        remainingBurst[nextProc] = getBurst(processList[nextProc]);
        nextProc++;
    }
    for (int t = 0; t < totalTimeUnits; t++) {
        if (!pq.empty()) {
            int priority = pq.top().first;
            int procIdx = pq.top().second;
            int arrival = getArrival(processList[procIdx]);
            int burst = getBurst(processList[procIdx]);
            pq.pop();
            while (nextProc < totalProcesses && getArrival(processList[nextProc]) == t + 1) {
                pq.push({0, nextProc});
                remainingBurst[nextProc] = getBurst(processList[nextProc]);
                nextProc++;
            }
            remainingBurst[procIdx]--;
            scheduleTimeline[t][procIdx] = '*';
            if (remainingBurst[procIdx] == 0) {
                completionTime[procIdx] = t + 1;
                turnaroundTime[procIdx] = (completionTime[procIdx] - arrival);
                normalizedTurnaround[procIdx] = (turnaroundTime[procIdx] * 1.0 / burst);
            } else {
                if (pq.size() >= 1)
                    pq.push({priority + 1, procIdx});
                else
                    pq.push({priority, procIdx});
            }
        }
        while (nextProc < totalProcesses && getArrival(processList[nextProc]) == t + 1) {
            pq.push({0, nextProc});
            remainingBurst[nextProc] = getBurst(processList[nextProc]);
            nextProc++;
        }
    }
    markWaitingPeriods();
}

// Feedback Queue FB-2i
void runFB2i() {
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq; // (priority, procIdx)
    unordered_map<int, int> remainingBurst;
    int nextProc = 0;
    if (getArrival(processList[0]) == 0) {
        pq.push({0, nextProc});
        remainingBurst[nextProc] = getBurst(processList[nextProc]);
        nextProc++;
    }
    for (int t = 0; t < totalTimeUnits; t++) {
        if (!pq.empty()) {
            int priority = pq.top().first;
            int procIdx = pq.top().second;
            int arrival = getArrival(processList[procIdx]);
            int burst = getBurst(processList[procIdx]);
            pq.pop();
            while (nextProc < totalProcesses && getArrival(processList[nextProc]) <= t + 1) {
                pq.push({0, nextProc});
                remainingBurst[nextProc] = getBurst(processList[nextProc]);
                nextProc++;
            }
            int quantum = pow(2, priority);
            int temp = t;
            while (quantum && remainingBurst[procIdx]) {
                quantum--;
                remainingBurst[procIdx]--;
                scheduleTimeline[temp++][procIdx] = '*';
            }
            if (remainingBurst[procIdx] == 0) {
                completionTime[procIdx] = temp;
                turnaroundTime[procIdx] = (completionTime[procIdx] - arrival);
                normalizedTurnaround[procIdx] = (turnaroundTime[procIdx] * 1.0 / burst);
            } else {
                if (pq.size() >= 1)
                    pq.push({priority + 1, procIdx});
                else
                    pq.push({priority, procIdx});
            }
            t = temp - 1;
        }
        while (nextProc < totalProcesses && getArrival(processList[nextProc]) <= t + 1) {
            pq.push({0, nextProc});
            remainingBurst[nextProc] = getBurst(processList[nextProc]);
            nextProc++;
        }
    }
    markWaitingPeriods();
}

// Aging Scheduler
void runAging(int quantum) {
    vector<tuple<int, int, int>> procQueue; // (priority, procIdx, waitingTime)
    int nextProc = 0, currentProc = -1;
    for (int t = 0; t < totalTimeUnits; t++) {
        while (nextProc < totalProcesses && getArrival(processList[nextProc]) <= t) {
            procQueue.push_back({getPriority(processList[nextProc]), nextProc, 0});
            nextProc++;
        }
        for (int i = 0; i < procQueue.size(); i++) {
            if (get<1>(procQueue[i]) == currentProc) {
                get<2>(procQueue[i]) = 0;
                get<0>(procQueue[i]) = getPriority(processList[currentProc]);
            } else {
                get<0>(procQueue[i])++;
                get<2>(procQueue[i])++;
            }
        }
        sort(procQueue.begin(), procQueue.end(), sortByPriority);
        currentProc = get<1>(procQueue[0]);
        int timeSlice = quantum;
        while (timeSlice-- && t < totalTimeUnits) {
            scheduleTimeline[t][currentProc] = '*';
            t++;
        }
        t--;
    }
    markWaitingPeriods();
}

// Printing functions
void printSchedulerName(int schedulerIdx) {
    int schedulerID = schedulerConfigs[schedulerIdx].first - '0';
    if (schedulerID == 2)
        cout << SCHEDULER_NAMES[schedulerID] << schedulerConfigs[schedulerIdx].second << endl;
    else
        cout << SCHEDULER_NAMES[schedulerID] << endl;
}
void printProcessIDs() {
    cout << "Process    ";
    for (int i = 0; i < totalProcesses; i++)
        cout << "|  " << getProcessID(processList[i]) << "  ";
    cout << "|\n";
}
void printArrivals() {
    cout << "Arrival    ";
    for (int i = 0; i < totalProcesses; i++)
        printf("|%3d  ", getArrival(processList[i]));
    cout << "|\n";
}
void printBursts() {
    cout << "Burst      |";
    for (int i = 0; i < totalProcesses; i++)
        printf("%3d  |", getBurst(processList[i]));
    cout << " Mean|\n";
}
void printCompletions() {
    cout << "Finish     ";
    for (int i = 0; i < totalProcesses; i++)
        printf("|%3d  ", completionTime[i]);
    cout << "|-----|\n";
}
void printTurnarounds() {
    cout << "Turnaround |";
    int sum = 0;
    for (int i = 0; i < totalProcesses; i++) {
        printf("%3d  |", turnaroundTime[i]);
        sum += turnaroundTime[i];
    }
    if ((1.0 * sum / turnaroundTime.size()) >= 10)
        printf("%2.2f|\n", (1.0 * sum / turnaroundTime.size()));
    else
        printf(" %2.2f|\n", (1.0 * sum / turnaroundTime.size()));
}
void printNormalizedTurnarounds() {
    cout << "NormTurn   |";
    float sum = 0;
    for (int i = 0; i < totalProcesses; i++) {
        if (normalizedTurnaround[i] >= 10)
            printf("%2.2f|", normalizedTurnaround[i]);
        else
            printf(" %2.2f|", normalizedTurnaround[i]);
        sum += normalizedTurnaround[i];
    }
    if ((1.0 * sum / normalizedTurnaround.size()) >= 10)
        printf("%2.2f|\n", (1.0 * sum / normalizedTurnaround.size()));
    else
        printf(" %2.2f|\n", (1.0 * sum / normalizedTurnaround.size()));
}
void printStatistics(int schedulerIdx) {
    printSchedulerName(schedulerIdx);
    printProcessIDs();
    printArrivals();
    printBursts();
    printCompletions();
    printTurnarounds();
    printNormalizedTurnarounds();
}
void printScheduleTimeline(int schedulerIdx) {
    for (int t = 0; t <= totalTimeUnits; t++)
        cout << t % 10 << " ";
    cout << "\n";
    cout << "------------------------------------------------\n";
    for (int i = 0; i < totalProcesses; i++) {
        cout << getProcessID(processList[i]) << "     |";
        for (int t = 0; t < totalTimeUnits; t++) {
            cout << scheduleTimeline[t][i] << "|";
        }
        cout << " \n";
    }
    cout << "------------------------------------------------\n";
}

// Scheduler dispatcher
void runScheduler(char schedulerID, int quantum, string mode) {
    switch (schedulerID) {
        case '1':
            if (mode == TRACE_MODE) cout << "FCFS  ";
            runFCFS();
            break;
        case '2':
            if (mode == TRACE_MODE) cout << "RR-" << quantum << "  ";
            runRoundRobin(quantum);
            break;
        case '3':
            if (mode == TRACE_MODE) cout << "SPN   ";
            runSPN();
            break;
        case '4':
            if (mode == TRACE_MODE) cout << "SRT   ";
            runSRT();
            break;
        case '5':
            if (mode == TRACE_MODE) cout << "HRRN  ";
            runHRRN();
            break;
        case '6':
            if (mode == TRACE_MODE) cout << "FB-1  ";
            runFB1();
            break;
        case '7':
            if (mode == TRACE_MODE) cout << "FB-2i ";
            runFB2i();
            break;
        case '8':
            if (mode == TRACE_MODE) cout << "Aging ";
            runAging(quantum);
            break;
        default:
            break;
    }
}

int main() {
    parse();
    for (int idx = 0; idx < (int)schedulerConfigs.size(); idx++) {
        clearScheduleTimeline();
        runScheduler(schedulerConfigs[idx].first, schedulerConfigs[idx].second, runMode);
        if (runMode == TRACE_MODE)
            printScheduleTimeline(idx);
        else if (runMode == STATISTICS_MODE)
            printStatistics(idx);
        cout << "\n";
    }
    return 0;
}
