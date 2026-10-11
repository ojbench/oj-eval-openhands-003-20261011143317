#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;

// Enum for submission status
enum class Status {
    ACCEPTED,
    WRONG_ANSWER,
    RUNTIME_ERROR,
    TIME_LIMIT_EXCEED
};

// Convert string to Status
Status stringToStatus(const string& statusStr) {
    if (statusStr == "Accepted") return Status::ACCEPTED;
    if (statusStr == "Wrong_Answer") return Status::WRONG_ANSWER;
    if (statusStr == "Runtime_Error") return Status::RUNTIME_ERROR;
    if (statusStr == "Time_Limit_Exceed") return Status::TIME_LIMIT_EXCEED;
    return Status::WRONG_ANSWER; // default
}

// Convert Status to string
string statusToString(Status status) {
    switch (status) {
        case Status::ACCEPTED: return "Accepted";
        case Status::WRONG_ANSWER: return "Wrong_Answer";
        case Status::RUNTIME_ERROR: return "Runtime_Error";
        case Status::TIME_LIMIT_EXCEED: return "Time_Limit_Exceed";
        default: return "Unknown";
    }
}

// Problem status for a team
struct ProblemStatus {
    int incorrect_attempts = 0;
    int submissions_after_freeze = 0;
    int first_correct_time = -1;
    bool is_solved = false;
    bool is_frozen = false;
    
    ProblemStatus() = default;
};

// Submission record
struct Submission {
    string problem_name;
    string team_name;
    Status status;
    int time;
    
    Submission(const string& prob, const string& team, Status stat, int t)
        : problem_name(prob), team_name(team), status(stat), time(t) {}
};

// Team information
struct Team {
    string name;
    map<char, ProblemStatus> problems;
    vector<Submission> submissions;
    
    Team() = default;
    Team(const string& teamName) : name(teamName) {}
    
    int getSolvedCount() const {
        int count = 0;
        for (const auto& pair : problems) {
            if (pair.second.is_solved && !pair.second.is_frozen) {
                count++;
            }
        }
        return count;
    }
    
    int getTotalPenalty() const {
        int penalty = 0;
        for (const auto& pair : problems) {
            if (pair.second.is_solved && !pair.second.is_frozen) {
                penalty += 20 * pair.second.incorrect_attempts + pair.second.first_correct_time;
            }
        }
        return penalty;
    }
    
    vector<int> getSolveTimes() const {
        vector<int> times;
        times.reserve(26); // Max 26 problems
        for (const auto& pair : problems) {
            if (pair.second.is_solved && !pair.second.is_frozen) {
                times.push_back(pair.second.first_correct_time);
            }
        }
        sort(times.rbegin(), times.rend()); // descending order for comparison
        return times;
    }
};

// Competition management system
class ICPCManagement {
private:
    map<string, Team> teams;
    int duration_time = 0;
    int problem_count = 0;
    bool competition_started = false;
    bool is_frozen = false;
    int freeze_time = -1;
    vector<string> last_flush_ranking;
    
public:
    void addTeam(const string& teamName) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        if (teams.find(teamName) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }
        teams[teamName] = Team(teamName);
        cout << "[Info]Add successfully.\n";
    }
    
    void startCompetition(int duration, int problems) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }
        duration_time = duration;
        problem_count = problems;
        competition_started = true;
        
        // Initialize problem status for all teams
        for (auto& pair : teams) {
            for (char c = 'A'; c < 'A' + problem_count; c++) {
                pair.second.problems[c] = ProblemStatus();
            }
        }
        
        cout << "[Info]Competition starts.\n";
    }
    
    void submitProblem(const string& problemName, const string& teamName, 
                      const string& statusStr, int time) {
        if (!competition_started) return;
        
        current_time = time;
        Status status = stringToStatus(statusStr);
        char problem = problemName[0];
        
        // Record submission
        Submission submission(problemName, teamName, status, time);
        teams[teamName].submissions.push_back(submission);
        
        // Update problem status
        ProblemStatus& probStatus = teams[teamName].problems[problem];
        
        if (is_frozen && !probStatus.is_solved) {
            probStatus.submissions_after_freeze++;
            if (!probStatus.is_frozen) {
                probStatus.is_frozen = true;
            }
            // Also count incorrect attempts during freeze
            if (status != Status::ACCEPTED) {
                probStatus.incorrect_attempts++;
            }
        } else if (!probStatus.is_solved) {
            if (status == Status::ACCEPTED) {
                probStatus.is_solved = true;
                probStatus.first_correct_time = time;
            } else {
                probStatus.incorrect_attempts++;
            }
        }
    }
    
    void flushScoreboard() {
        cout << "[Info]Flush scoreboard.\n";
        updateRanking();
    }
    
    void freezeScoreboard() {
        if (is_frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }
        is_frozen = true;
        freeze_time = current_time;
        cout << "[Info]Freeze scoreboard.\n";
    }
    
    void scrollScoreboard() {
        if (!is_frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }
        
        cout << "[Info]Scroll scoreboard.\n";
        flushScoreboard();
        
        // Display scoreboard before scrolling
        displayScoreboard();
        
        // Implement scroll logic more efficiently
        vector<string> rankingChanges;
        
        // Collect all frozen problems first
        vector<pair<pair<string, char>, bool>> frozenProblems; // team, problem, solvedDuringFreeze
        
        for (auto& teamPair : teams) {
            const string& teamName = teamPair.first;
            Team& team = teamPair.second;
            
            for (char c = 'A'; c < 'A' + problem_count; c++) {
                if (team.problems[c].is_frozen) {
                    // Check if solved during freeze
                    bool solvedDuringFreeze = false;
                    for (const auto& submission : team.submissions) {
                        if (submission.problem_name[0] == c && 
                            submission.time > freeze_time && 
                            submission.status == Status::ACCEPTED &&
                            !team.problems[c].is_solved) {
                            solvedDuringFreeze = true;
                            break;
                        }
                    }
                    frozenProblems.push_back({{teamName, c}, solvedDuringFreeze});
                }
            }
        }
        
        // Sort frozen problems by ranking (lowest first) then problem number
        sort(frozenProblems.begin(), frozenProblems.end(), [this](const auto& a, const auto& b) {
            const string& teamA = a.first.first;
            const string& teamB = b.first.first;
            
            auto posA = find(last_flush_ranking.begin(), last_flush_ranking.end(), teamA);
            auto posB = find(last_flush_ranking.begin(), last_flush_ranking.end(), teamB);
            
            int rankA = (posA != last_flush_ranking.end()) ? (posA - last_flush_ranking.begin()) : last_flush_ranking.size();
            int rankB = (posB != last_flush_ranking.end()) ? (posB - last_flush_ranking.begin()) : last_flush_ranking.size();
            
            if (rankA != rankB) return rankA > rankB; // Lower rank = higher number
            return a.first.second < b.first.second; // Smaller problem first
        });
        
        // Unfreeze problems one by one
        for (auto& frozenProb : frozenProblems) {
            string teamName = frozenProb.first.first;
            char problem = frozenProb.first.second;
            bool solvedDuringFreeze = frozenProb.second;
            
            Team& team = teams[teamName];
            ProblemStatus& probStatus = team.problems[problem];
            
            // Unfreeze
            probStatus.is_frozen = false;
            
            // Apply solve if it happened during freeze
            if (solvedDuringFreeze && !probStatus.is_solved) {
                for (const auto& submission : team.submissions) {
                    if (submission.problem_name[0] == problem && 
                        submission.time > freeze_time && 
                        submission.status == Status::ACCEPTED) {
                        probStatus.is_solved = true;
                        probStatus.first_correct_time = submission.time;
                        break;
                    }
                }
            }
            
            // Check ranking change
            if (solvedDuringFreeze) {
                vector<string> oldRanking = last_flush_ranking;
                updateRanking();
                
                auto oldPos = find(oldRanking.begin(), oldRanking.end(), teamName);
                auto newPos = find(last_flush_ranking.begin(), last_flush_ranking.end(), teamName);
                
                if (oldPos != oldRanking.end() && newPos != last_flush_ranking.end()) {
                    size_t oldIndex = oldPos - oldRanking.begin();
                    size_t newIndex = newPos - last_flush_ranking.begin();
                    
                    if (newIndex < oldIndex) {
                        int newSolved = team.getSolvedCount();
                        int newPenalty = team.getTotalPenalty();
                        
                        // Find who got replaced
                        if (newIndex + 1 < last_flush_ranking.size()) {
                            string replacedTeam = last_flush_ranking[newIndex + 1];
                            rankingChanges.push_back(teamName + " " + replacedTeam + " " + 
                                                   to_string(newSolved) + " " + to_string(newPenalty));
                        }
                    }
                }
            }
        }
        
        // Final ranking update
        updateRanking();
        
        // Output ranking changes
        for (const string& change : rankingChanges) {
            cout << change << "\n";
        }
        
        is_frozen = false;
        
        // Display scoreboard after scrolling
        displayScoreboard();
    }
    
    void queryRanking(const string& teamName) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query ranking.\n";
        if (is_frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }
        
        updateRanking();
        auto it = find(last_flush_ranking.begin(), last_flush_ranking.end(), teamName);
        int ranking = (it != last_flush_ranking.end()) ? (it - last_flush_ranking.begin() + 1) : -1;
        cout << "[" << teamName << "] NOW AT RANKING [" << ranking << "]\n";
    }
    
    void querySubmission(const string& teamName, const string& problemName, const string& statusStr) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query submission.\n";
        
        const auto& submissions = teams[teamName].submissions;
        const Submission* result = nullptr;
        
        for (auto it = submissions.rbegin(); it != submissions.rend(); ++it) {
            bool problemMatch = (problemName == "ALL" || it->problem_name == problemName);
            bool statusMatch = (statusStr == "ALL" || statusToString(it->status) == statusStr);
            
            if (problemMatch && statusMatch) {
                result = &(*it);
                break;
            }
        }
        
        if (result) {
            cout << "[" << result->team_name << "] [" << result->problem_name << "] [" 
                 << statusToString(result->status) << "] [" << result->time << "]\n";
        } else {
            cout << "Cannot find any submission.\n";
        }
    }
    
    void endCompetition() {
        cout << "[Info]Competition ends.\n";
    }
    
private:
    int current_time = 0;
    
    void updateRanking() {
        last_flush_ranking.clear();
        last_flush_ranking.reserve(teams.size());
        
        vector<pair<string, Team*>> teamList;
        teamList.reserve(teams.size());
        for (auto& pair : teams) {
            teamList.emplace_back(pair.first, &pair.second);
        }
        
        sort(teamList.begin(), teamList.end(), [](const auto& a, const auto& b) {
            const Team* teamA = a.second;
            const Team* teamB = b.second;
            
            // First compare solved problems
            int solvedA = teamA->getSolvedCount();
            int solvedB = teamB->getSolvedCount();
            if (solvedA != solvedB) return solvedA > solvedB;
            
            // Then compare penalty time
            int penaltyA = teamA->getTotalPenalty();
            int penaltyB = teamB->getTotalPenalty();
            if (penaltyA != penaltyB) return penaltyA < penaltyB;
            
            // Then compare solve times
            vector<int> timesA = teamA->getSolveTimes();
            vector<int> timesB = teamB->getSolveTimes();
            size_t minSize = min(timesA.size(), timesB.size());
            for (size_t i = 0; i < minSize; i++) {
                if (timesA[i] != timesB[i]) return timesA[i] < timesB[i];
            }
            if (timesA.size() != timesB.size()) return timesA.size() < timesB.size();
            
            // Finally compare team names lexicographically
            return a.first < b.first;
        });
        
        for (const auto& pair : teamList) {
            last_flush_ranking.push_back(pair.first);
        }
    }
    
    void displayScoreboard() {
        updateRanking();
        
        for (size_t i = 0; i < last_flush_ranking.size(); i++) {
            const string& teamName = last_flush_ranking[i];
            const Team& team = teams[teamName];
            int solved = team.getSolvedCount();
            int penalty = team.getTotalPenalty();
            
            cout << teamName << " " << (i + 1) << " " << solved << " " << penalty;
            
            // Display problem status
            for (char c = 'A'; c < 'A' + problem_count; c++) {
                auto it = team.problems.find(c);
                if (it != team.problems.end()) {
                    const ProblemStatus& status = it->second;
                    if (status.is_frozen) {
                        if (status.incorrect_attempts == 0) {
                            cout << " 0/" << status.submissions_after_freeze;
                        } else {
                            cout << " " << status.incorrect_attempts << "/" << status.submissions_after_freeze;
                        }
                    } else if (status.is_solved) {
                        if (status.incorrect_attempts == 0) {
                            cout << " +";
                        } else {
                            cout << " +" << status.incorrect_attempts;
                        }
                    } else {
                        if (status.incorrect_attempts == 0) {
                            cout << " .";
                        } else {
                            cout << " -" << status.incorrect_attempts;
                        }
                    }
                } else {
                    cout << " .";
                }
            }
            cout << "\n";
        }
    }
    
    int getCurrentTime() {
        return current_time;
    }
};

int main() {
    ICPCManagement system;
    string line;
    
    while (getline(cin, line)) {
        istringstream iss(line);
        string command;
        iss >> command;
        
        if (command == "ADDTEAM") {
            string teamName;
            iss >> teamName;
            system.addTeam(teamName);
        } else if (command == "START") {
            string durationStr, problemStr;
            int duration, problems;
            iss >> durationStr >> duration >> problemStr >> problems;
            system.startCompetition(duration, problems);
        } else if (command == "SUBMIT") {
            string problemName, byStr, teamName, withStr, statusStr, atStr;
            int time;
            iss >> problemName >> byStr >> teamName >> withStr >> statusStr >> atStr >> time;
            system.submitProblem(problemName, teamName, statusStr, time);
        } else if (command == "FLUSH") {
            system.flushScoreboard();
        } else if (command == "FREEZE") {
            system.freezeScoreboard();
        } else if (command == "SCROLL") {
            system.scrollScoreboard();
        } else if (command == "QUERY_RANKING") {
            string teamName;
            iss >> teamName;
            system.queryRanking(teamName);
        } else if (command == "QUERY_SUBMISSION") {
            string teamName, whereStr, problemStr, problemName, andStr, statusStr, status;
            iss >> teamName >> whereStr >> problemStr >> problemName >> andStr >> statusStr >> status;
            system.querySubmission(teamName, problemName, status);
        } else if (command == "END") {
            system.endCompetition();
            break;
        }
    }
    
    return 0;
}