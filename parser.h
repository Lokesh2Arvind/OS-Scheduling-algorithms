#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <bits/stdc++.h>
using namespace std;

/** This file handles parsing the data we are going to work with **/
/** It also holds all the global variables we parse into         **/

// Global variables for scheduler configuration and process data
string runMode;
int totalTimeUnits, totalProcesses;
vector<pair<char, int>> schedulerConfigs; // (algorithm_id, quantum if any)
vector<tuple<string, int, int>> processList; // (processID, arrivalTime, burstTime)
vector<vector<char>> scheduleTimeline; // [time][process] for trace output
unordered_map<string, int> processIDtoIndex;

// Results
vector<int> completionTime;
vector<int> turnaroundTime;
vector<float> normalizedTurnaround;

// Parse scheduler algorithms from input string
void parseSchedulers(string scheduler_chunk) {
    stringstream stream(scheduler_chunk);
    while (stream.good()) {
        string temp_str;
        getline(stream, temp_str, ',');
        stringstream ss(temp_str);
        getline(ss, temp_str, '-');
        char scheduler_id = temp_str[0];
        getline(ss, temp_str, '-');
        int quantum = temp_str.size() >= 1 ? stoi(temp_str) : -1;
        schedulerConfigs.push_back(make_pair(scheduler_id, quantum));
    }
}

// Parse process list from input
void parseProcesses() {
    string process_chunk, processID;
    int arrivalTime, burstTime;
    for (int i = 0; i < totalProcesses; i++) {
        cin >> process_chunk;
        stringstream stream(process_chunk);
        string temp_str;
        getline(stream, temp_str, ',');
        processID = temp_str;
        getline(stream, temp_str, ',');
        arrivalTime = stoi(temp_str);
        getline(stream, temp_str, ',');
        burstTime = stoi(temp_str);

        processList.push_back(make_tuple(processID, arrivalTime, burstTime));
        processIDtoIndex[processID] = i;
    }
}

// Main parse function
void parse() {
    string scheduler_chunk;
    cin >> runMode >> scheduler_chunk >> totalTimeUnits >> totalProcesses;
    parseSchedulers(scheduler_chunk);
    parseProcesses();
    completionTime.resize(totalProcesses);
    turnaroundTime.resize(totalProcesses);
    normalizedTurnaround.resize(totalProcesses);
    scheduleTimeline.resize(totalTimeUnits);
    for (int t = 0; t < totalTimeUnits; t++)
        for (int p = 0; p < totalProcesses; p++)
            scheduleTimeline[t].push_back(' ');
}

#endif // PARSER_H_INCLUDED
