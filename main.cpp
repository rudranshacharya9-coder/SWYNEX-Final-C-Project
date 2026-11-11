#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

enum class Priority { Low, Medium, High };
enum class Status { Todo, InProgress, Done };

class Task {
public:
    int id{};
    std::string title;
    std::string category;
    std::string dueDate;
    Priority priority{Priority::Medium};
    Status status{Status::Todo};
    std::string notes;
};

const char* toString(Priority value) {
    switch (value) { case Priority::Low: return "Low"; case Priority::High: return "High"; default: return "Medium"; }
}
const char* toString(Status value) {
    switch (value) { case Status::InProgress: return "In progress"; case Status::Done: return "Done"; default: return "To do"; }
}
std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
std::string encode(const std::string& value) {
    std::string result;
    for (char c : value) { if (c == '\\' || c == '\t' || c == '\n' || c == '\r') result += '\\';
        if (c == '\t') result += 't'; else if (c == '\n') result += 'n'; else if (c == '\r') result += 'r'; else result += c; }
    return result;
}
std::string decode(const std::string& value) {
    std::string result; bool escaped = false;
    for (char c : value) { if (escaped) { result += c == 't' ? '\t' : c == 'n' ? '\n' : c == 'r' ? '\r' : c; escaped = false; }
        else if (c == '\\') escaped = true; else result += c; }
    if (escaped) result += '\\';
    return result;
}
std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> fields(1); bool escaped = false;
    for (char c : line) { if (escaped) { fields.back() += '\\'; fields.back() += c; escaped = false; }
        else if (c == '\\') escaped = true; else if (c == '\t') fields.emplace_back(); else fields.back() += c; }
    if (escaped) fields.back() += '\\';
    return fields;
}
bool validDate(const std::string& value) {
    if (value.empty()) return true;
    if (value.size() != 10 || value[4] != '-' || value[7] != '-') return false;
    for (std::size_t i = 0; i < value.size(); ++i) if (i != 4 && i != 7 && !std::isdigit(static_cast<unsigned char>(value[i]))) return false;
    int year = std::stoi(value.substr(0, 4)), month = std::stoi(value.substr(5, 2)), day = std::stoi(value.substr(8, 2));
    if (year < 1 || month < 1 || month > 12) return false;
    const int daysPerMonth[] = {31, (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)) ? 29 : 28,
                                31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return day >= 1 && day <= daysPerMonth[month - 1];
}

class TaskRepository {
public:
    explicit TaskRepository(std::string file) : file_(std::move(file)) {}

    std::vector<Task> load() const {
        std::vector<Task> tasks; std::ifstream in(file_); std::string line;
        while (std::getline(in, line)) {
            auto f = split(line); if (f.size() != 7) continue;
            try {
                Task t; t.id = std::stoi(f[0]); t.title = decode(f[1]); t.category = decode(f[2]); t.dueDate = decode(f[3]);
                t.priority = f[4] == "High" ? Priority::High : f[4] == "Low" ? Priority::Low : Priority::Medium;
                t.status = f[5] == "Done" ? Status::Done : f[5] == "In progress" ? Status::InProgress : Status::Todo;
                t.notes = decode(f[6]); tasks.push_back(std::move(t));
            } catch (...) { /* Ignore malformed records and keep the rest of the file. */ }
        }
        return tasks;
    }

    bool save(const std::vector<Task>& tasks) const {
        std::ofstream out(file_, std::ios::trunc);
        if (!out) return false;
        for (const auto& t : tasks) out << t.id << '\t' << encode(t.title) << '\t' << encode(t.category) << '\t'
            << encode(t.dueDate) << '\t' << toString(t.priority) << '\t' << toString(t.status) << '\t' << encode(t.notes) << '\n';
        return static_cast<bool>(out);
    }

private:
    std::string file_;
};

class TaskManager {
public:
    explicit TaskManager(std::string file) : repository_(std::move(file)), tasks_(repository_.load()) {
        for (const auto& task : tasks_) nextId_ = std::max(nextId_, task.id + 1);
    }
    void run() {
        std::cout << "\n  Welcome to TaskFlow\n  Your work, in clear view.\n";
        while (true) {
            std::cout << "\n  [1] View tasks   [2] Add task   [3] Update status\n"
                         "  [4] Edit task    [5] Remove task [6] Search\n"
                         "  [7] Summary      [0] Save & exit\n\n  Choose: ";
            std::string choice; std::getline(std::cin, choice);
            if (!std::cin) { save(); std::cout << "\n  See you soon!\n"; return; }
            if (choice == "0") { save(); std::cout << "\n  See you soon!\n"; return; }
            if (choice == "1") viewTasks(); else if (choice == "2") addTask(); else if (choice == "3") updateStatus();
            else if (choice == "4") editTask(); else if (choice == "5") removeTask(); else if (choice == "6") search();
            else if (choice == "7") summary(); else std::cout << "\n  Please choose one of the menu options.\n";
        }
    }
private:
    TaskRepository repository_;
    std::vector<Task> tasks_;
    int nextId_{1};

    void save() const {
        if (!repository_.save(tasks_)) std::cout << "  Could not save the task file. Check the folder permissions.\n";
    }
    std::string prompt(const std::string& label, const std::string& current = {}) const {
        std::cout << "  " << label; if (!current.empty()) std::cout << " [" << current << "]"; std::cout << ": ";
        std::string value; std::getline(std::cin, value); return value;
    }
    int readId() const {
        auto raw = prompt("Task ID"); try { std::size_t used{}; int id = std::stoi(raw, &used); return used == raw.size() ? id : -1; }
        catch (...) { return -1; }
    }
    Task* findTask(int id) { auto it = std::find_if(tasks_.begin(), tasks_.end(), [id](const Task& t) { return t.id == id; }); return it == tasks_.end() ? nullptr : &*it; }
    Priority choosePriority(Priority current = Priority::Medium) const {
        const char* defaultValue = current == Priority::High ? "3" : current == Priority::Low ? "1" : "2";
        while (true) { auto value = lower(prompt("Priority (1 low, 2 medium, 3 high)", defaultValue));
            if (value.empty()) return current;
            if (value == "1" || value == "low") return Priority::Low; if (value == "3" || value == "high") return Priority::High;
            if (value == "2" || value == "medium") return Priority::Medium; std::cout << "  Enter 1, 2, or 3.\n"; }
    }
    void print(const std::vector<const Task*>& list) const {
        if (list.empty()) { std::cout << "\n  No tasks to show. Add one to get started.\n"; return; }
        std::cout << "\n  " << std::left << std::setw(5) << "ID" << std::setw(29) << "TASK" << std::setw(16) << "CATEGORY"
                  << std::setw(13) << "DUE DATE" << std::setw(11) << "PRIORITY" << "STATUS\n  " << std::string(86, '-') << '\n';
        for (const auto* t : list) { auto title = t->title.size() > 26 ? t->title.substr(0, 23) + "..." : t->title;
            auto category = t->category.size() > 13 ? t->category.substr(0, 10) + "..." : t->category;
            std::cout << "  " << std::left << std::setw(5) << t->id << std::setw(29) << title << std::setw(16) << category
                      << std::setw(13) << (t->dueDate.empty() ? "—" : t->dueDate) << std::setw(11) << toString(t->priority) << toString(t->status) << '\n'; }
    }
    std::vector<const Task*> all() const { std::vector<const Task*> result; for (const auto& t : tasks_) result.push_back(&t); return result; }
    void viewTasks() {
        std::cout << "\n  View: [1] all  [2] to do  [3] in progress  [4] done\n  Choose: "; std::string filter; std::getline(std::cin, filter);
        std::vector<const Task*> result; for (const auto& t : tasks_) if (filter == "1" || (filter == "2" && t.status == Status::Todo) ||
            (filter == "3" && t.status == Status::InProgress) || (filter == "4" && t.status == Status::Done)) result.push_back(&t);
        std::stable_sort(result.begin(), result.end(), [](const Task* a, const Task* b) { return a->dueDate.empty() ? false : b->dueDate.empty() || a->dueDate < b->dueDate; }); print(result);
    }
    void addTask() {
        Task t; t.id = nextId_++; t.title = prompt("Title"); if (t.title.empty()) { --nextId_; std::cout << "  A title is required.\n"; return; }
        t.category = prompt("Category", "General"); if (t.category.empty()) t.category = "General";
        while (true) { t.dueDate = prompt("Due date (YYYY-MM-DD, optional)"); if (validDate(t.dueDate)) break; std::cout << "  Use YYYY-MM-DD, or leave it blank.\n"; }
        t.priority = choosePriority(); t.notes = prompt("Notes (optional)"); tasks_.push_back(std::move(t)); save(); std::cout << "  Task added.\n";
    }
    void updateStatus() {
        int id = readId(); auto* t = findTask(id); if (!t) { std::cout << "  Task not found.\n"; return; }
        std::cout << "  [1] To do  [2] In progress  [3] Done\n"; auto v = prompt("New status");
        if (v == "1") t->status = Status::Todo; else if (v == "2") t->status = Status::InProgress; else if (v == "3") t->status = Status::Done;
        else { std::cout << "  Status unchanged.\n"; return; } save(); std::cout << "  Status updated.\n";
    }
    void editTask() {
        int id = readId(); auto* t = findTask(id); if (!t) { std::cout << "  Task not found.\n"; return; }
        auto v = prompt("Title", t->title); if (!v.empty()) t->title = v;
        v = prompt("Category", t->category); if (!v.empty()) t->category = v;
        while (true) { v = prompt("Due date (YYYY-MM-DD, blank clears)", t->dueDate); if (v.empty() || validDate(v)) { t->dueDate = v; break; } std::cout << "  Use YYYY-MM-DD.\n"; }
        t->priority = choosePriority(t->priority); v = prompt("Notes", t->notes); if (!v.empty()) t->notes = v;
        save(); std::cout << "  Task updated.\n";
    }
    void removeTask() {
        int id = readId(); auto it = std::find_if(tasks_.begin(), tasks_.end(), [id](const Task& t) { return t.id == id; });
        if (it == tasks_.end()) { std::cout << "  Task not found.\n"; return; }
        std::cout << "  Remove \"" << it->title << "\"? [y/N] "; std::string confirm; std::getline(std::cin, confirm);
        if (lower(confirm) == "y" || lower(confirm) == "yes") { tasks_.erase(it); save(); std::cout << "  Task removed.\n"; } else std::cout << "  Kept the task.\n";
    }
    void search() const {
        auto query = lower(prompt("Search title, category, or notes")); if (query.empty()) { std::cout << "  Enter a search phrase.\n"; return; }
        std::vector<const Task*> matches; for (const auto& t : tasks_) if (lower(t.title + " " + t.category + " " + t.notes).find(query) != std::string::npos) matches.push_back(&t); print(matches);
    }
    void summary() const {
        int todo = 0, doing = 0, done = 0, high = 0; for (const auto& t : tasks_) { todo += t.status == Status::Todo; doing += t.status == Status::InProgress; done += t.status == Status::Done; high += t.priority == Priority::High && t.status != Status::Done; }
        int total = static_cast<int>(tasks_.size()); int percent = total ? done * 100 / total : 0;
        std::cout << "\n  YOUR OVERVIEW\n  " << std::string(30, '-') << "\n"
                  << "  Total tasks       " << total << "\n  To do             " << todo << "\n  In progress       " << doing
                  << "\n  Completed         " << done << "\n  High priority open " << high << "\n\n  Progress          [";
        int bars = percent / 5; for (int i = 0; i < 20; ++i) std::cout << (i < bars ? '#' : '-'); std::cout << "] " << percent << "%\n";
    }
};

} // namespace

int main(int argc, char* argv[]) {
    std::string dataFile = argc > 1 ? argv[1] : "tasks.tsv";
    try { TaskManager manager(dataFile); manager.run(); }
    catch (const std::exception& e) { std::cerr << "TaskFlow could not start: " << e.what() << '\n'; return 1; }
    return 0;
}
