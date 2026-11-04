
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <cctype>

using namespace std;

struct Student {
    int id = 0;
    string name, roll, department, expiry, status;
    bool inside = false;
};

class SmartIDGuardian {
private:
    vector<Student> students;
    int nextID = 1001;

    const string databaseFile = "students.db";
    const string auditFile = "audit.log";
    const string csvFile = "students_export.csv";

    bool running = true;

    string today() {
        time_t now = time(nullptr);
        tm* local = localtime(&now);

        stringstream ss;
        ss << put_time(local, "%Y-%m-%d");
        return ss.str();
    }

    string input(const string& prompt) {
        string value;
        cout << prompt;

        if (!getline(cin, value)) {
            running = false;
            return "";
        }

        return value;
    }

    int menuChoice() {
        string value = input("\nEnter choice (0-11): ");

        if (!running) return 0;

        try {
            size_t pos = 0;
            int n = stoi(value, &pos);

            if (pos == value.size() && n >= 0 && n <= 11)
                return n;
        }
        catch (...) {}

        return -1;
    }

    bool validDate(const string& date) {
        if (date.size() != 10 ||
            date[4] != '-' || date[7] != '-')
            return false;

        for (int i = 0; i < 10; i++) {
            if (i == 4 || i == 7) continue;

            if (!isdigit(static_cast<unsigned char>(date[i])))
                return false;
        }

        int y, m, d;

        try {
            y = stoi(date.substr(0, 4));
            m = stoi(date.substr(5, 2));
            d = stoi(date.substr(8, 2));
        }
        catch (...) {
            return false;
        }

        if (y < 1900 || m < 1 || m > 12 || d < 1)
            return false;

        int days[] = {
            31,28,31,30,31,30,
            31,31,30,31,30,31
        };

        bool leap = (y % 400 == 0) ||
                    (y % 4 == 0 && y % 100 != 0);

        if (leap) days[1] = 29;

        return d <= days[m - 1];
    }

    bool isExpired(const Student& s) {
        return s.expiry < today();
    }

    Student* findRoll(const string& roll) {
        for (auto& s : students) {
            if (s.roll == roll)
                return &s;
        }

        return nullptr;
    }

    void logAction(const string& action) {
        ofstream file(auditFile, ios::app);

        if (file)
            file << today() << " | " << action << '\n';
    }

    bool safeField(const string& value) {
        return value.find('|') == string::npos &&
               value.find('\n') == string::npos &&
               value.find('\r') == string::npos;
    }

    void saveDatabase() {
        ofstream file(databaseFile);

        if (!file) {
            cout << "WARNING: Could not save database.\n";
            return;
        }

        for (const auto& s : students) {
            file << s.id << '|'
                 << s.name << '|'
                 << s.roll << '|'
                 << s.department << '|'
                 << s.expiry << '|'
                 << s.status << '|'
                 << (s.inside ? 1 : 0) << '\n';
        }
    }

    void loadDatabase() {
        ifstream file(databaseFile);

        if (!file) return;

        string line;

        while (getline(file, line)) {
            stringstream ss(line);
            vector<string> fields;
            string field;

            while (getline(ss, field, '|'))
                fields.push_back(field);

            if (fields.size() != 7)
                continue;

            try {
                Student s;

                s.id = stoi(fields[0]);
                s.name = fields[1];
                s.roll = fields[2];
                s.department = fields[3];
                s.expiry = fields[4];
                s.status = fields[5];
                s.inside = (fields[6] == "1");

                if (s.id < 1 || !validDate(s.expiry))
                    continue;

                if (findRoll(s.roll))
                    continue;

                students.push_back(s);
                nextID = max(nextID, s.id + 1);
            }
            catch (...) {
                continue;
            }
        }
    }

    string csvEscape(const string& value) {
        string result = "\"";

        for (char c : value) {
            if (c == '"')
                result += "\"\"";
            else
                result += c;
        }

        return result + "\"";
    }

    void showStudent(const Student& s) {
        cout << "\n----------------------------------------\n";
        cout << "             STUDENT RECORD\n";
        cout << "----------------------------------------\n";
        cout << "Student ID   : " << s.id << '\n';
        cout << "Name         : " << s.name << '\n';
        cout << "Roll Number  : " << s.roll << '\n';
        cout << "Department   : " << s.department << '\n';
        cout << "Expiry Date  : " << s.expiry << '\n';
        cout << "Card Status  : " << s.status << '\n';
        cout << "Campus Status: "
             << (s.inside ? "INSIDE" : "OUTSIDE") << '\n';
        cout << "----------------------------------------\n";
    }

    bool canAccess(Student& s) {
        if (s.status == "REVOKED" || s.status == "LOST")
            return false;

        if (isExpired(s)) {
            s.status = "EXPIRED";
            s.inside = false;
            saveDatabase();
            return false;
        }

        if (s.status == "EXPIRED")
            s.status = "ACTIVE";

        return s.status == "ACTIVE";
    }

    void registerStudent() {
        Student s;

        cout << "\n--- STUDENT REGISTRATION ---\n";

        s.name = input("Full name: ");
        if (!running) return;

        s.roll = input("Roll number: ");
        if (!running) return;

        s.department = input("Department: ");
        if (!running) return;

        s.expiry = input("Expiry date (YYYY-MM-DD): ");
        if (!running) return;

        if (s.name.empty() || s.roll.empty() ||
            s.department.empty()) {
            cout << "ERROR: All fields are required.\n";
            return;
        }

        if (!safeField(s.name) || !safeField(s.roll) ||
            !safeField(s.department)) {
            cout << "ERROR: Invalid character in input.\n";
            return;
        }

        if (findRoll(s.roll)) {
            cout << "ERROR: Duplicate roll number detected!\n";
            return;
        }

        if (!validDate(s.expiry)) {
            cout << "ERROR: Invalid expiry date.\n";
            return;
        }

        s.id = nextID++;
        s.status = isExpired(s) ? "EXPIRED" : "ACTIVE";

        students.push_back(s);

        saveDatabase();
        logAction("Registered student: " + s.roll);

        cout << "\nStudent registered successfully!\n";
        cout << "Generated Student ID: " << s.id << '\n';
    }

    void verifyStudent() {
        string roll = input("\nEnter roll number: ");
        if (!running) return;

        Student* s = findRoll(roll);

        if (!s) {
            cout << "VERIFICATION FAILED: Student not found.\n";
            return;
        }

        if (!canAccess(*s)) {
            cout << "VERIFICATION FAILED: Card is "
                 << s->status << ".\n";

            logAction("Failed verification: " + roll);
            return;
        }

        cout << "\nVERIFICATION SUCCESSFUL!\n";
        showStudent(*s);

        logAction("Successful verification: " + roll);
    }

    void reportLostCard() {
        string roll = input("\nEnter roll number: ");
        if (!running) return;

        Student* s = findRoll(roll);

        if (!s) {
            cout << "Student not found.\n";
            return;
        }

        if (s->status == "REVOKED") {
            cout << "This card is already revoked.\n";
            return;
        }

        if (s->status == "LOST") {
            cout << "This card is already marked as LOST.\n";
            return;
        }

        s->status = "LOST";
        s->inside = false;

        saveDatabase();
        logAction("Card reported LOST: " + roll);

        cout << "Card marked as LOST. Verification blocked.\n";
    }

    void revokeCard() {
        string roll = input("\nEnter roll number: ");
        if (!running) return;

        Student* s = findRoll(roll);

        if (!s) {
            cout << "Student not found.\n";
            return;
        }

        if (s->status == "REVOKED") {
            cout << "Card is already revoked.\n";
            return;
        }

        s->status = "REVOKED";
        s->inside = false;

        saveDatabase();
        logAction("Card REVOKED: " + roll);

        cout << "Card permanently revoked.\n";
    }

    void campusEntry() {
        string roll = input("\nEnter roll number: ");
        if (!running) return;

        Student* s = findRoll(roll);

        if (!s) {
            cout << "ENTRY DENIED: Student not found.\n";
            return;
        }

        if (!canAccess(*s)) {
            cout << "ENTRY DENIED: Card is "
                 << s->status << ".\n";

            logAction("Entry denied: " + roll);
            return;
        }

        if (s->inside) {
            cout << "ENTRY DENIED: Student is already INSIDE.\n";
            return;
        }

        s->inside = true;

        saveDatabase();
        logAction("Campus ENTRY: " + roll);

        cout << "ENTRY APPROVED for " << s->name << ".\n";
    }

    void campusExit() {
        string roll = input("\nEnter roll number: ");
        if (!running) return;

        Student* s = findRoll(roll);

        if (!s) {
            cout << "EXIT DENIED: Student not found.\n";
            return;
        }

        if (!canAccess(*s)) {
            cout << "EXIT DENIED: Card is "
                 << s->status << ".\n";

            logAction("Exit denied: " + roll);
            return;
        }

        if (!s->inside) {
            cout << "EXIT DENIED: Student is already OUTSIDE.\n";
            return;
        }

        s->inside = false;

        saveDatabase();
        logAction("Campus EXIT: " + roll);

        cout << "EXIT RECORDED for " << s->name << ".\n";
    }

    void searchStudent() {
        string roll = input("\nEnter roll number: ");
        if (!running) return;

        Student* s = findRoll(roll);

        if (s)
            showStudent(*s);
        else
            cout << "No matching student found.\n";
    }

    void displayAll() {
        cout << "\n--- ALL STUDENT RECORDS ---\n";

        if (students.empty()) {
            cout << "No student records available.\n";
            return;
        }

        for (const auto& s : students) {
            cout << "\nID: " << s.id
                 << " | Name: " << s.name
                 << " | Roll: " << s.roll
                 << " | Status: " << s.status
                 << " | Campus: "
                 << (s.inside ? "INSIDE" : "OUTSIDE")
                 << '\n';
        }
    }

    void dashboard() {
        int active = 0, lost = 0;
        int revoked = 0, expired = 0, inside = 0;

        for (auto& s : students) {
            if (s.status != "LOST" &&
                s.status != "REVOKED" &&
                isExpired(s)) {
                s.status = "EXPIRED";
                s.inside = false;
            }

            if (s.status == "ACTIVE") active++;
            else if (s.status == "LOST") lost++;
            else if (s.status == "REVOKED") revoked++;
            else if (s.status == "EXPIRED") expired++;

            if (s.inside) inside++;
        }

        saveDatabase();

        cout << "\n========== SMARTID DASHBOARD ==========\n";
        cout << "Total Students : " << students.size() << '\n';
        cout << "Active Cards   : " << active << '\n';
        cout << "Lost Cards     : " << lost << '\n';
        cout << "Expired Cards  : " << expired << '\n';
        cout << "Revoked Cards  : " << revoked << '\n';
        cout << "Inside Campus  : " << inside << '\n';
        cout << "Outside Campus : "
             << students.size() - inside << '\n';
        cout << "=======================================\n";
    }

    void exportCSV() {
        ofstream file(csvFile);

        if (!file) {
            cout << "ERROR: Could not create CSV file.\n";
            return;
        }

        file << "Student ID,Name,Roll Number,Department,"
                "Expiry Date,Card Status,Campus Status\n";

        for (const auto& s : students) {
            file << s.id << ','
                 << csvEscape(s.name) << ','
                 << csvEscape(s.roll) << ','
                 << csvEscape(s.department) << ','
                 << csvEscape(s.expiry) << ','
                 << csvEscape(s.status) << ','
                 << csvEscape(s.inside ? "INSIDE" : "OUTSIDE")
                 << '\n';
        }

        cout << "CSV exported to " << csvFile << '\n';
        logAction("Student records exported to CSV");
    }

    void viewAuditLog() {
        ifstream file(auditFile);

        cout << "\n========== AUDIT HISTORY ==========\n";

        if (!file) {
            cout << "No audit history available yet.\n";
            return;
        }

        string line;

        while (getline(file, line))
            cout << line << '\n';

        cout << "===================================\n";
    }

public:
    SmartIDGuardian() {
        loadDatabase();
    }

    void run() {
        while (running) {
            cout << "\n\n";
            cout << "========================================\n";
            cout << "          SMARTID GUARDIAN v3.0\n";
            cout << "    STUDENT ID & CAMPUS SECURITY SYSTEM\n";
            cout << "========================================\n";
            cout << "1. Register Student\n";
            cout << "2. Verify Student ID\n";
            cout << "3. Report Lost Card\n";
            cout << "4. Revoke Student Card\n";
            cout << "5. Campus Entry\n";
            cout << "6. Campus Exit\n";
            cout << "7. Search Student\n";
            cout << "8. Display All Students\n";
            cout << "9. Dashboard\n";
            cout << "10. Export Records to CSV\n";
            cout << "11. View Audit History\n";
            cout << "0. Exit\n";
            cout << "----------------------------------------\n";

            int choice = menuChoice();

            if (!running)
                break;

            switch (choice) {
                case 1: registerStudent(); break;
                case 2: verifyStudent(); break;
                case 3: reportLostCard(); break;
                case 4: revokeCard(); break;
                case 5: campusEntry(); break;
                case 6: campusExit(); break;
                case 7: searchStudent(); break;
                case 8: displayAll(); break;
                case 9: dashboard(); break;
                case 10: exportCSV(); break;
                case 11: viewAuditLog(); break;

                case 0:
                    running = false;
                    break;

                default:
                    cout << "Invalid choice. Enter 0 to 11.\n";
            }
        }

        saveDatabase();
        cout << "\nSmartID Guardian v3.0 closed.\n";
    }
};

int main() {
    SmartIDGuardian app;
    app.run();

    return 0;
}
