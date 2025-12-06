#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CRED_FILE "credentials.txt"
#define STUD_FILE "students.txt"
#define COURSE_FILE "courses.txt"
#define SUBJECT_FILE "subjects.txt"
#define TIMETABLE_FILE "timetable.txt"
#define RESOURCES_FILE "resources.txt"
#define MAX_LINE 512
#define MAX_FIELD 128

typedef enum { ROLE_ADMIN, ROLE_STAFF, ROLE_GUEST, ROLE_STUDENT, ROLE_UNKNOWN } Role;

typedef struct {
    char username[64];
    Role role;
} User;

void clear_stdin() {
    int c; while ((c = getchar()) != '\n' && c != EOF) {}
}

Role parse_role(const char *s) {
    if (strcmp(s, "Admin") == 0) return ROLE_ADMIN;
    if (strcmp(s, "Staff") == 0) return ROLE_STAFF;
    if (strcmp(s, "Guest") == 0) return ROLE_GUEST;
    if (strcmp(s, "Student") == 0) return ROLE_STUDENT;
    return ROLE_UNKNOWN;
}

const char* role_to_string(Role r) {
    if (r==ROLE_ADMIN) return "Admin";
    if (r==ROLE_STAFF) return "Staff";
    if (r==ROLE_GUEST) return "Guest";
    if (r==ROLE_STUDENT) return "Student";
    return "Unknown";
}

// Simple login: username and password checked against credentials file.
// Students can login using their Student ID (no password required for students)
int login(User *outUser) {
    char uname[64], pwd[64] = "";
    printf("Username/Student ID: "); 
    if (scanf("%63s", uname) != 1) return 0;
    clear_stdin(); // Clear any remaining input
    
    printf("Password (Press Enter for Student login): "); 
    // Read password, but allow empty for students
    fgets(pwd, sizeof(pwd), stdin);
    pwd[strcspn(pwd, "\n")] = 0; // Remove newline

    // First check credentials file (Admin, Staff, Guest)
    FILE *f = fopen(CRED_FILE, "r");
    if (f) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), f)) {
            // file format: username:role:password
            char file_user[64], file_role[64], file_pwd[64];
            if (sscanf(line, "%63[^:]:%63[^:]:%63s", file_user, file_role, file_pwd) == 3) {
                if (strcmp(file_user, uname) == 0 && strcmp(file_pwd, pwd) == 0) {
                    strcpy(outUser->username, file_user);
                    outUser->role = parse_role(file_role);
                    fclose(f);
                    return 1;
                }
            }
        }
        fclose(f);
    }
    
    // If not found in credentials and password is empty, check students file
    if (strlen(pwd) == 0) {
        f = fopen(STUD_FILE, "r");
        if (f) {
            char line[MAX_LINE];
            while (fgets(line, sizeof(line), f)) {
                if (line[0] == '\n' || line[0] == '\0') continue;
                char student_id[32];
                if (sscanf(line, "%31[^|]", student_id) == 1) {
                    if (strcmp(student_id, uname) == 0) {
                        strcpy(outUser->username, student_id);
                        outUser->role = ROLE_STUDENT;
                        fclose(f);
                        return 1;
                    }
                }
            }
            fclose(f);
        }
    }
    
    return 0;
}

// Print all students
void list_students() {
    FILE *f = fopen(STUD_FILE, "r");
    if (!f) { printf("No students found (students.txt missing).\n"); return; }
    char line[MAX_LINE];
    printf("ID\t\tName\t\t\tDept\tCGPA\tAttendance\tClass/Section\n");
    printf("--------------------------------------------------------------------------------------------\n");
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        char id[32], name[128], dept[64], cgpa[16], att[32], class[64] = "";
        int fields = sscanf(line, "%31[^|]|%127[^|]|%63[^|]|%15[^|]|%31[^|]|%63[^\n]", id, name, dept, cgpa, att, class);
        if (fields >= 4) {
            if (fields >= 6 && strlen(class) > 0) {
                printf("%s\t%-20s\t%s\t%s\t%s\t\t%s\n", id, name, dept, cgpa, att[0] ? att : "N/A", class);
            } else {
                printf("%s\t%-20s\t%s\t%s\t%s\t\tN/A\n", id, name, dept, cgpa, att[0] ? att : "N/A");
            }
        }
    }
    fclose(f);
}

// Add a student (Admin & Staff)
void add_student() {
    char id[32], name[128], dept[64], cgpa[16], att[32];
    printf("New Student ID: "); scanf("%31s", id); clear_stdin();
    printf("Name: "); fgets(name, sizeof(name), stdin); name[strcspn(name, "\n")] = 0;
    printf("Department: "); scanf("%63s", dept); clear_stdin();
    printf("CGPA: "); scanf("%15s", cgpa); clear_stdin();
    printf("Attendance status (Present/Absent): "); scanf("%31s", att); clear_stdin();

    FILE *f = fopen(STUD_FILE, "a");
    if (!f) { printf("Failed to open students file for write.\n"); return; }
    fprintf(f, "%s|%s|%s|%s|%s\n", id, name, dept, cgpa, att);
    fclose(f);
    printf("Student added.\n");
}

// Update student: simple approach—rewrite file replacing matching id
void update_student() {
    char target[32];
    printf("Enter Student ID to update: "); scanf("%31s", target); clear_stdin();

    FILE *f = fopen(STUD_FILE, "r");
    if (!f) { printf("Student file missing.\n"); return; }
    
    // First, find the student and display current info
    char line[MAX_LINE];
    char found_line[MAX_LINE];
    int found = 0;
    char current_id[32], current_name[128], current_dept[64], current_cgpa[16], current_att[32];
    
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%31[^|]|%127[^|]|%63[^|]|%15[^|]|%31[^\n]", current_id, current_name, current_dept, current_cgpa, current_att) >= 1) {
            if (strcmp(current_id, target) == 0) {
                found = 1;
                strcpy(found_line, line);
                break;
            }
        }
    }
    fclose(f);
    
    if (!found) {
        printf("Student ID not found.\n");
        return;
    }
    
    // Display current information
    printf("\nCurrent Student Information:\n");
    printf("ID: %s\n", current_id);
    printf("Name: %s\n", current_name);
    printf("Department: %s\n", current_dept);
    printf("CGPA: %s\n", current_cgpa);
    printf("Attendance: %s\n", current_att);
    
    // Ask what to update
    printf("\nWhat would you like to update?\n");
    printf("1. Name\n");
    printf("2. Department\n");
    printf("3. CGPA\n");
    printf("4. Attendance\n");
    printf("5. Update All\n");
    printf("0. Cancel\n");
    printf("Enter your choice: ");
    
    int choice;
    scanf("%d", &choice);
    clear_stdin();
    
    char new_name[128], new_dept[64], new_cgpa[16], new_att[32];
    
    // Copy current values
    strcpy(new_name, current_name);
    strcpy(new_dept, current_dept);
    strcpy(new_cgpa, current_cgpa);
    strcpy(new_att, current_att);
    
    // Update based on choice
    if (choice == 1 || choice == 5) {
        printf("Enter new Name: ");
        fgets(new_name, sizeof(new_name), stdin);
        new_name[strcspn(new_name, "\n")] = 0;
    }
    if (choice == 2 || choice == 5) {
        printf("Enter new Department: ");
        scanf("%63s", new_dept);
        clear_stdin();
    }
    if (choice == 3 || choice == 5) {
        printf("Enter new CGPA: ");
        scanf("%15s", new_cgpa);
        clear_stdin();
    }
    if (choice == 4 || choice == 5) {
        printf("Enter new Attendance (Present/Absent): ");
        scanf("%31s", new_att);
        clear_stdin();
    }
    
    if (choice == 0) {
        printf("Update cancelled.\n");
        return;
    }
    
    // Now rewrite the file with updated information
    f = fopen(STUD_FILE, "r");
    if (!f) { printf("Cannot reopen file.\n"); return; }
    FILE *tmp = fopen("tmp_students.txt", "w");
    if (!tmp) { fclose(f); printf("Cannot create temp file.\n"); return; }
    
    while (fgets(line, sizeof(line), f)) {
        char id[32];
        if (sscanf(line, "%31[^|]", id) == 1) {
            if (strcmp(id, target) == 0) {
                // Write updated record
                fprintf(tmp, "%s|%s|%s|%s|%s\n", current_id, new_name, new_dept, new_cgpa, new_att);
            } else {
                fputs(line, tmp);
            }
        } else {
            fputs(line, tmp);
        }
    }
    fclose(f);
    fclose(tmp);
    
    remove(STUD_FILE);
    rename("tmp_students.txt", STUD_FILE);
    printf("\nStudent updated successfully!\n");
}

// Delete student
void delete_student() {
    char target[32];
    printf("Enter Student ID to delete: "); scanf("%31s", target); clear_stdin();

    FILE *f = fopen(STUD_FILE, "r");
    if (!f) { printf("Student file missing.\n"); return; }
    FILE *tmp = fopen("tmp_students.txt", "w");
    if (!tmp) { fclose(f); printf("Cannot create temp file.\n"); return; }

    char line[MAX_LINE];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        char id[32];
        if (sscanf(line, "%31[^|]", id) == 1) {
            if (strcmp(id, target) == 0) {
                found = 1;
                continue; // skip (delete)
            }
        }
        fputs(line, tmp);
    }
    fclose(f); fclose(tmp);
    if (found) {
        remove(STUD_FILE);
        rename("tmp_students.txt", STUD_FILE);
        printf("Student deleted.\n");
    } else {
        remove("tmp_students.txt");
        printf("Student not found.\n");
    }
}

// Case-insensitive string search helper
int str_contains_case_insensitive(const char *haystack, const char *needle) {
    int i, j;
    int haystack_len = strlen(haystack);
    int needle_len = strlen(needle);
    
    if (needle_len == 0) return 1;
    if (needle_len > haystack_len) return 0;
    
    for (i = 0; i <= haystack_len - needle_len; i++) {
        int match = 1;
        for (j = 0; j < needle_len; j++) {
            char h = haystack[i + j];
            char n = needle[j];
            // Convert to lowercase for comparison
            if (h >= 'A' && h <= 'Z') h = h - 'A' + 'a';
            if (n >= 'A' && n <= 'Z') n = n - 'A' + 'a';
            if (h != n) {
                match = 0;
                break;
            }
        }
        if (match) return 1;
    }
    return 0;
}

// Search student by ID or name
void search_student() {
    char q[128];
    printf("Enter ID or Name to search: ");
    clear_stdin();
    fgets(q, sizeof(q), stdin);
    q[strcspn(q, "\n")] = 0;
    if (strlen(q) == 0) {
        printf("Search query cannot be empty.\n");
        return;
    }

    FILE *f = fopen(STUD_FILE, "r");
    if (!f) { 
        printf("No students file found.\n"); 
        return; 
    }
    char line[MAX_LINE];
    int found = 0;
    printf("\nSearch Results:\n");
    printf("ID\t\tName\t\t\tDept\tCGPA\tAttendance\n");
    printf("------------------------------------------------------------\n");
    while (fgets(line, sizeof(line), f)) {
        // Skip empty lines
        if (line[0] == '\n' || line[0] == '\0' || line[0] == '\r') continue;
        
        char id[32], name[128], dept[64], cgpa[16], att[32];
        if (sscanf(line, "%31[^|]|%127[^|]|%63[^|]|%15[^|]|%31[^\n]", id, name, dept, cgpa, att) >= 4) {
            // Search in ID or Name fields
            if (str_contains_case_insensitive(id, q) || str_contains_case_insensitive(name, q)) {
                printf("%s\t%-20s\t%s\t%s\t%s\n", id, name, dept, cgpa, att[0] ? att : "N/A");
                found = 1;
            }
        }
    }
    if (!found) {
        printf("No matching student found for '%s'.\n", q);
    }
    fclose(f);
}

// Course/Department Management Functions
void add_course() {
    char course_code[32], course_name[128], dept[64];
    printf("\n=== Add New Course/Branch ===\n");
    printf("Enter Course Code: ");
    scanf("%31s", course_code);
    clear_stdin();
    
    printf("Enter Course Name: ");
    fgets(course_name, sizeof(course_name), stdin);
    course_name[strcspn(course_name, "\n")] = 0;
    
    printf("Enter Department: ");
    scanf("%63s", dept);
    clear_stdin();
    
    FILE *f = fopen(COURSE_FILE, "a");
    if (!f) {
        printf("Cannot open courses file.\n");
        return;
    }
    fprintf(f, "%s|%s|%s\n", course_code, course_name, dept);
    fclose(f);
    printf("Course '%s' added successfully!\n", course_name);
}

void list_courses() {
    FILE *f = fopen(COURSE_FILE, "r");
    if (!f) {
        printf("No courses found.\n");
        return;
    }
    char line[MAX_LINE];
    printf("\nCourse Code\tCourse Name\t\t\tDepartment\n");
    printf("------------------------------------------------------------\n");
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        char code[32], name[128], dept[64];
        if (sscanf(line, "%31[^|]|%127[^|]|%63[^\n]", code, name, dept) == 3) {
            printf("%s\t\t%-30s\t%s\n", code, name, dept);
        }
    }
    fclose(f);
}

void add_subject() {
    char subject_code[32], subject_name[128], dept[64], credits[16];
    printf("\n=== Add New Subject ===\n");
    printf("Enter Subject Code: ");
    scanf("%31s", subject_code);
    clear_stdin();
    
    printf("Enter Subject Name: ");
    fgets(subject_name, sizeof(subject_name), stdin);
    subject_name[strcspn(subject_name, "\n")] = 0;
    
    printf("Enter Department: ");
    scanf("%63s", dept);
    clear_stdin();
    
    printf("Enter Credits: ");
    scanf("%15s", credits);
    clear_stdin();
    
    FILE *f = fopen(SUBJECT_FILE, "a");
    if (!f) {
        printf("Cannot open subjects file.\n");
        return;
    }
    fprintf(f, "%s|%s|%s|%s\n", subject_code, subject_name, dept, credits);
    fclose(f);
    printf("Subject '%s' added successfully!\n", subject_name);
}

void list_subjects() {
    FILE *f = fopen(SUBJECT_FILE, "r");
    if (!f) {
        printf("No subjects found.\n");
        return;
    }
    char line[MAX_LINE];
    printf("\nSubject Code\tSubject Name\t\t\tDepartment\tCredits\n");
    printf("----------------------------------------------------------------------------\n");
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        char code[32], name[128], dept[64], credits[16];
        if (sscanf(line, "%31[^|]|%127[^|]|%63[^|]|%15[^\n]", code, name, dept, credits) == 4) {
            printf("%s\t\t%-30s\t%s\t\t%s\n", code, name, dept, credits);
        }
    }
    fclose(f);
}

// Class Allocation Functions
void assign_class_to_student() {
    char student_id[32], class_section[64];
    printf("\n=== Assign Class/Section to Student ===\n");
    printf("Enter Student ID: ");
    scanf("%31s", student_id);
    clear_stdin();
    
    printf("Enter Class/Section (e.g., CSE-A, CSE-B): ");
    scanf("%63s", class_section);
    clear_stdin();
    
    FILE *f = fopen(STUD_FILE, "r");
    if (!f) {
        printf("Student file missing.\n");
        return;
    }
    FILE *tmp = fopen("tmp_students.txt", "w");
    if (!tmp) {
        fclose(f);
        printf("Cannot create temp file.\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') {
            fputs(line, tmp);
            continue;
        }
        char id[32], name[128], dept[64], cgpa[16], att[32], class[64] = "";
        int fields = sscanf(line, "%31[^|]|%127[^|]|%63[^|]|%15[^|]|%31[^|]|%63[^\n]", id, name, dept, cgpa, att, class);
        
        if (fields >= 4) {
            if (strcmp(id, student_id) == 0) {
                found = 1;
                // Write with class/section
                if (fields >= 6 && strlen(class) > 0) {
                    fprintf(tmp, "%s|%s|%s|%s|%s|%s\n", id, name, dept, cgpa, att, class_section);
                } else {
                    fprintf(tmp, "%s|%s|%s|%s|%s|%s\n", id, name, dept, cgpa, att, class_section);
                }
            } else {
                // Keep existing class if present
                if (fields >= 6 && strlen(class) > 0) {
                    fprintf(tmp, "%s|%s|%s|%s|%s|%s\n", id, name, dept, cgpa, att, class);
                } else {
                    fprintf(tmp, "%s|%s|%s|%s|%s\n", id, name, dept, cgpa, att);
                }
            }
        } else {
            fputs(line, tmp);
        }
    }
    fclose(f);
    fclose(tmp);
    
    if (found) {
        remove(STUD_FILE);
        rename("tmp_students.txt", STUD_FILE);
        printf("Student '%s' assigned to class '%s' successfully!\n", student_id, class_section);
    } else {
        remove("tmp_students.txt");
        printf("Student ID not found.\n");
    }
}

void view_students_by_class() {
    char class_section[64];
    printf("\n=== View Students by Class/Section ===\n");
    printf("Enter Class/Section: ");
    scanf("%63s", class_section);
    clear_stdin();
    
    FILE *f = fopen(STUD_FILE, "r");
    if (!f) {
        printf("Student file missing.\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    printf("\nStudents in Class '%s':\n", class_section);
    printf("ID\t\tName\t\t\tDepartment\tCGPA\tAttendance\n");
    printf("----------------------------------------------------------------------------\n");
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        char id[32], name[128], dept[64], cgpa[16], att[32], class[64] = "";
        int fields = sscanf(line, "%31[^|]|%127[^|]|%63[^|]|%15[^|]|%31[^|]|%63[^\n]", id, name, dept, cgpa, att, class);
        
        if (fields >= 4) {
            if (fields >= 6 && strcmp(class, class_section) == 0) {
                printf("%s\t%-20s\t%s\t\t%s\t%s\n", id, name, dept, cgpa, att);
                found = 1;
            }
        }
    }
    fclose(f);
    
    if (!found) {
        printf("No students found in class '%s'.\n", class_section);
    }
}

// Timetable Generator Functions
void create_timetable() {
    char day[32], time[32], subject[128], class_section[64], room[32];
    printf("\n=== Create Timetable Entry ===\n");
    printf("Enter Day (Mon/Tue/Wed/Thu/Fri/Sat): ");
    scanf("%31s", day);
    clear_stdin();
    
    printf("Enter Time (e.g., 09:00-10:00): ");
    scanf("%31s", time);
    clear_stdin();
    
    printf("Enter Subject: ");
    fgets(subject, sizeof(subject), stdin);
    subject[strcspn(subject, "\n")] = 0;
    
    printf("Enter Class/Section: ");
    scanf("%63s", class_section);
    clear_stdin();
    
    printf("Enter Room Number: ");
    scanf("%31s", room);
    clear_stdin();
    
    FILE *f = fopen(TIMETABLE_FILE, "a");
    if (!f) {
        printf("Cannot open timetable file.\n");
        return;
    }
    fprintf(f, "%s|%s|%s|%s|%s\n", day, time, subject, class_section, room);
    fclose(f);
    printf("Timetable entry added successfully!\n");
}

void view_timetable() {
    char filter_class[64] = "";
    int filter_choice;
    
    printf("\n=== View Timetable ===\n");
    printf("1. View All\n");
    printf("2. View by Class/Section\n");
    printf("Choice: ");
    scanf("%d", &filter_choice);
    clear_stdin();
    
    if (filter_choice == 2) {
        printf("Enter Class/Section: ");
        scanf("%63s", filter_class);
        clear_stdin();
    }
    
    FILE *f = fopen(TIMETABLE_FILE, "r");
    if (!f) {
        printf("No timetable found.\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    printf("\nDay\t\tTime\t\t\tSubject\t\t\t\tClass\t\tRoom\n");
    printf("--------------------------------------------------------------------------------------------\n");
    
    while (fgets(line, sizeof(line), f)) {
        // Skip empty lines
        if (line[0] == '\n' || line[0] == '\0' || line[0] == '\r') continue;
        
        // Remove trailing newline and carriage return
        int len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[len-1] = '\0';
            len--;
        }
        
        if (len == 0) continue;
        
        char day[32] = "", time[32] = "", subject[128] = "", class_section[64] = "", room[32] = "";
        
        // Try to parse 5 fields first
        int fields = sscanf(line, "%31[^|]|%31[^|]|%127[^|]|%63[^|]|%31s", day, time, subject, class_section, room);
        
        // If only 4 fields found, parse again without room
        if (fields == 4) {
            strcpy(room, "N/A");
        }
        
        // Only process if we got at least 4 fields
        if (fields >= 4 && strlen(day) > 0 && strlen(time) > 0 && strlen(subject) > 0 && strlen(class_section) > 0) {
            if (filter_choice == 1 || strcmp(class_section, filter_class) == 0) {
                printf("%s\t\t%-20s\t%-30s\t%s\t\t%s\n", day, time, subject, class_section, room[0] ? room : "N/A");
                found = 1;
            }
        }
    }
    fclose(f);
    
    if (!found) {
        printf("No timetable entries found.\n");
    }
}

void delete_timetable_entry() {
    char day[32], time[32];
    printf("\n=== Delete Timetable Entry ===\n");
    printf("Enter Day: ");
    scanf("%31s", day);
    clear_stdin();
    
    printf("Enter Time: ");
    scanf("%31s", time);
    clear_stdin();
    
    FILE *f = fopen(TIMETABLE_FILE, "r");
    if (!f) {
        printf("Timetable file missing.\n");
        return;
    }
    FILE *tmp = fopen("tmp_timetable.txt", "w");
    if (!tmp) {
        fclose(f);
        printf("Cannot create temp file.\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        char file_day[32], file_time[32];
        if (sscanf(line, "%31[^|]|%31[^|]", file_day, file_time) == 2) {
            if (strcmp(file_day, day) == 0 && strcmp(file_time, time) == 0) {
                found = 1;
                continue; // Skip (delete)
            }
        }
        fputs(line, tmp);
    }
    fclose(f);
    fclose(tmp);
    
    if (found) {
        remove(TIMETABLE_FILE);
        rename("tmp_timetable.txt", TIMETABLE_FILE);
        printf("Timetable entry deleted successfully!\n");
    } else {
        remove("tmp_timetable.txt");
        printf("Timetable entry not found.\n");
    }
}

// Student Menu Functions
void view_personal_record(const char *student_id) {
    FILE *f = fopen(STUD_FILE, "r");
    if (!f) {
        printf("Student file not found.\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    printf("\n=== Your Personal Record ===\n");
    printf("----------------------------------------\n");
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        char id[32], name[128], dept[64], cgpa[16], att[32], class[64] = "";
        int fields = sscanf(line, "%31[^|]|%127[^|]|%63[^|]|%15[^|]|%31[^|]|%63[^\n]", id, name, dept, cgpa, att, class);
        
        if (fields >= 4 && strcmp(id, student_id) == 0) {
            found = 1;
            printf("Student ID: %s\n", id);
            printf("Name: %s\n", name);
            printf("Department: %s\n", dept);
            printf("CGPA: %s\n", cgpa);
            printf("Attendance: %s\n", att[0] ? att : "N/A");
            if (fields >= 6 && strlen(class) > 0) {
                printf("Class/Section: %s\n", class);
            }
            break;
        }
    }
    fclose(f);
    
    if (!found) {
        printf("Your record not found.\n");
    }
    printf("----------------------------------------\n");
}

void search_friends() {
    char search_term[128];
    printf("\n=== Search Friends ===\n");
    printf("Enter name or department to search: ");
    clear_stdin();
    fgets(search_term, sizeof(search_term), stdin);
    search_term[strcspn(search_term, "\n")] = 0;
    
    if (strlen(search_term) == 0) {
        printf("Search term cannot be empty.\n");
        return;
    }
    
    FILE *f = fopen(STUD_FILE, "r");
    if (!f) {
        printf("Student file not found.\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    printf("\nSearch Results:\n");
    printf("ID\t\tName\t\t\tDepartment\tCGPA\tClass/Section\n");
    printf("--------------------------------------------------------------------------------------------\n");
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        char id[32], name[128], dept[64], cgpa[16], att[32], class[64] = "";
        int fields = sscanf(line, "%31[^|]|%127[^|]|%63[^|]|%15[^|]|%31[^|]|%63[^\n]", id, name, dept, cgpa, att, class);
        
        if (fields >= 4) {
            // Search in name or department
            if (str_contains_case_insensitive(name, search_term) || str_contains_case_insensitive(dept, search_term)) {
                printf("%s\t%-20s\t%s\t\t%s\t%s\n", id, name, dept, cgpa, (fields >= 6 && strlen(class) > 0) ? class : "N/A");
                found = 1;
            }
        }
    }
    fclose(f);
    
    if (!found) {
        printf("No friends found matching '%s'.\n", search_term);
    }
}

void view_resources() {
    FILE *f = fopen(RESOURCES_FILE, "r");
    if (!f) {
        printf("Resources file not found. Creating default resources...\n");
        // Create default resources file
        f = fopen(RESOURCES_FILE, "w");
        if (f) {
            fprintf(f, "Notes|Study Materials|https://example.com/notes\n");
            fprintf(f, "eBooks|Digital Library|https://example.com/ebooks\n");
            fprintf(f, "Motivation|Inspirational Videos|https://example.com/motivation\n");
            fclose(f);
            printf("Default resources created. Please add your resources.\n");
        }
        return;
    }
    
    char line[MAX_LINE];    
    printf("\n=== Resource Center ===\n");
    printf("========================================\n");
    
    printf("\n📚 STUDY NOTES & MATERIALS:\n");
    printf("----------------------------------------\n");
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        
        char type[64], title[128], link[256];
        if (sscanf(line, "%63[^|]|%127[^|]|%255[^\n]", type, title, link) == 3) {
            if (strcmp(type, "Notes") == 0) {
                printf("  • %s\n    Link: %s\n", title, link);
            }
        }
    }
    rewind(f);
    
    printf("\n📖 EBOOKS & DIGITAL LIBRARY:\n");
    printf("----------------------------------------\n");
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        char type[64], title[128], link[256];
        if (sscanf(line, "%63[^|]|%127[^|]|%255[^\n]", type, title, link) == 3) {
            if (strcmp(type, "eBooks") == 0) {
                printf("  • %s\n    Link: %s\n", title, link);
            }
        }
    }
    rewind(f);
    
    printf("\n💪 MOTIVATION & INSPIRATION:\n");
    printf("----------------------------------------\n");
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        char type[64], title[128], link[256];
        if (sscanf(line, "%63[^|]|%127[^|]|%255[^\n]", type, title, link) == 3) {
            if (strcmp(type, "Motivation") == 0) {
                printf("  • %s\n    Link: %s\n", title, link);
            }
        }
    }
    
    fclose(f);
    printf("\n========================================\n");
}

void add_resource() {
    int type_choice;
    char type[64], title[128], link[256];
    
    printf("\n=== Add Resource ===\n");
    printf("Select resource type:\n");
    printf("1. Notes/Study Materials\n");
    printf("2. eBooks\n");
    printf("3. Motivation/Inspiration\n");
    printf("Choice: ");
    scanf("%d", &type_choice);
    clear_stdin();
    
    if (type_choice == 1) strcpy(type, "Notes");
    else if (type_choice == 2) strcpy(type, "eBooks");
    else if (type_choice == 3) strcpy(type, "Motivation");
    else {
        printf("Invalid choice.\n");
        return;
    }
    
    printf("Enter title/description: ");
    fgets(title, sizeof(title), stdin);
    title[strcspn(title, "\n")] = 0;
    
    printf("Enter link/URL: ");
    scanf("%255s", link);
    clear_stdin();
    
    FILE *f = fopen(RESOURCES_FILE, "a");
    if (!f) {
        printf("Cannot open resources file.\n");
        return;
    }
    fprintf(f, "%s|%s|%s\n", type, title, link);
    fclose(f);
    printf("Resource added successfully!\n");
}

// Student menu
void student_menu(const char *student_id) {
    int ch;
    do {
        printf("\n=== Student Menu ===\n");
        printf("1. View My Grades & Attendance\n");
        printf("2. Search Friends\n");
        printf("3. Resource Center\n");
        printf("4. Add Resource (Share with others)\n");
        printf("0. Logout\n");
        printf("Choice: ");
        scanf("%d", &ch);
        clear_stdin();
        if (ch == 1) view_personal_record(student_id);
        else if (ch == 2) search_friends();
        else if (ch == 3) view_resources();
        else if (ch == 4) add_resource();
    } while (ch != 0);
}

// Guest view: only list & search
void guest_menu() {
    int ch;
    do {
        printf("\nGuest Menu:\n1. List students\n2. Search student\n0. Logout\nChoice: ");
        scanf("%d", &ch); clear_stdin();
        if (ch==1) list_students();
        else if (ch==2) search_student();
    } while (ch != 0);
}

// Staff menu: can add, view, search, update (not delete)
void staff_menu() {
    int ch;
    do {
        printf("\n=== Staff Menu ===\n");
        printf("Student Management:\n");
        printf("  1. List students\n");
        printf("  2. Search student\n");
        printf("  3. Add student\n");
        printf("  4. Update student\n");
        printf("\nCourse/Department Management:\n");
        printf("  5. Add Course/Branch\n");
        printf("  6. List Courses\n");
        printf("  7. Add Subject\n");
        printf("  8. List Subjects\n");
        printf("\nClass Management:\n");
        printf("  9. Assign Class to Student\n");
        printf("  10. View Students by Class\n");
        printf("\nTimetable:\n");
        printf("  11. Create Timetable Entry\n");
        printf("  12. View Timetable\n");
        printf("\n  0. Logout\n");
        printf("Choice: ");
        scanf("%d", &ch); clear_stdin();
        if (ch==1) list_students();
        else if (ch==2) search_student();
        else if (ch==3) add_student();
        else if (ch==4) update_student();
        else if (ch==5) add_course();
        else if (ch==6) list_courses();
        else if (ch==7) add_subject();
        else if (ch==8) list_subjects();
        else if (ch==9) assign_class_to_student();
        else if (ch==10) view_students_by_class();
        else if (ch==11) create_timetable();
        else if (ch==12) view_timetable();
    } while (ch != 0);
}

// Add new staff/guest account (Admin only)
void add_staff_account() {
    char username[64], password[64], role_str[64];
    int role_choice;
    
    printf("\n=== Add New Account ===\n");
    printf("Enter username: ");
    scanf("%63s", username);
    clear_stdin();
    
    // Check if username already exists
    FILE *f = fopen(CRED_FILE, "r");
    if (f) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), f)) {
            char file_user[64];
            if (sscanf(line, "%63[^:]", file_user) == 1) {
                if (strcmp(file_user, username) == 0) {
                    printf("Username already exists!\n");
                    fclose(f);
                    return;
                }
            }
        }
        fclose(f);
    }
    
    printf("Select role:\n1. Staff\n2. Guest\nChoice: ");
    scanf("%d", &role_choice);
    clear_stdin();
    
    if (role_choice == 1) {
        strcpy(role_str, "Staff");
    } else if (role_choice == 2) {
        strcpy(role_str, "Guest");
    } else {
        printf("Invalid choice.\n");
        return;
    }
    
    printf("Enter password: ");
    scanf("%63s", password);
    clear_stdin();
    
    // Append to credentials file
    f = fopen(CRED_FILE, "a");
    if (!f) {
        printf("Cannot open credentials file.\n");
        return;
    }
    fprintf(f, "%s:%s:%s\n", username, role_str, password);
    fclose(f);
    printf("Account '%s' (%s) added successfully!\n", username, role_str);
}

// Remove staff/guest account (Admin only)
void remove_staff_account() {
    char target[64];
    printf("\n=== Remove Account ===\n");
    printf("Enter username to remove: ");
    scanf("%63s", target);
    clear_stdin();
    
    // Prevent removing admin account
    if (strcmp(target, "admin") == 0) {
        printf("Cannot remove admin account!\n");
        return;
    }
    
    FILE *f = fopen(CRED_FILE, "r");
    if (!f) {
        printf("Credentials file missing.\n");
        return;
    }
    FILE *tmp = fopen("tmp_credentials.txt", "w");
    if (!tmp) {
        fclose(f);
        printf("Cannot create temp file.\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        char username[64];
        if (sscanf(line, "%63[^:]", username) == 1) {
            if (strcmp(username, target) == 0) {
                found = 1;
                continue; // Skip (delete)
            }
        }
        fputs(line, tmp);
    }
    fclose(f);
    fclose(tmp);
    
    if (found) {
        remove(CRED_FILE);
        rename("tmp_credentials.txt", CRED_FILE);
        printf("Account '%s' removed successfully!\n", target);
    } else {
        remove("tmp_credentials.txt");
        printf("Account not found.\n");
    }
}

// Reset password (Admin only)
void reset_password() {
    char target[64], new_password[64];
    printf("\n=== Reset Password ===\n");
    printf("Enter username to reset password: ");
    scanf("%63s", target);
    clear_stdin();
    
    FILE *f = fopen(CRED_FILE, "r");
    if (!f) {
        printf("Credentials file missing.\n");
        return;
    }
    FILE *tmp = fopen("tmp_credentials.txt", "w");
    if (!tmp) {
        fclose(f);
        printf("Cannot create temp file.\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        char username[64], role[64], password[64];
        if (sscanf(line, "%63[^:]:%63[^:]:%63s", username, role, password) == 3) {
            if (strcmp(username, target) == 0) {
                found = 1;
                printf("Enter new password: ");
                scanf("%63s", new_password);
                clear_stdin();
                fprintf(tmp, "%s:%s:%s\n", username, role, new_password);
            } else {
                fputs(line, tmp);
            }
        } else {
            fputs(line, tmp);
        }
    }
    fclose(f);
    fclose(tmp);
    
    if (found) {
        remove(CRED_FILE);
        rename("tmp_credentials.txt", CRED_FILE);
        printf("Password for '%s' reset successfully!\n", target);
    } else {
        remove("tmp_credentials.txt");
        printf("Account not found.\n");
    }
}

// Change user role (Admin only)
void change_role() {
    char target[64];
    int new_role_choice;
    char new_role[64];
    
    printf("\n=== Change User Role ===\n");
    printf("Enter username to change role: ");
    scanf("%63s", target);
    clear_stdin();
    
    // Prevent changing admin role
    if (strcmp(target, "admin") == 0) {
        printf("Cannot change admin role!\n");
        return;
    }
    
    printf("Select new role:\n1. Admin\n2. Staff\n3. Guest\nChoice: ");
    scanf("%d", &new_role_choice);
    clear_stdin();
    
    if (new_role_choice == 1) {
        strcpy(new_role, "Admin");
    } else if (new_role_choice == 2) {
        strcpy(new_role, "Staff");
    } else if (new_role_choice == 3) {
        strcpy(new_role, "Guest");
    } else {
        printf("Invalid choice.\n");
        return;
    }
    
    FILE *f = fopen(CRED_FILE, "r");
    if (!f) {
        printf("Credentials file missing.\n");
        return;
    }
    FILE *tmp = fopen("tmp_credentials.txt", "w");
    if (!tmp) {
        fclose(f);
        printf("Cannot create temp file.\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        char username[64], role[64], password[64];
        if (sscanf(line, "%63[^:]:%63[^:]:%63s", username, role, password) == 3) {
            if (strcmp(username, target) == 0) {
                found = 1;
                fprintf(tmp, "%s:%s:%s\n", username, new_role, password);
            } else {
                fputs(line, tmp);
            }
        } else {
            fputs(line, tmp);
        }
    }
    fclose(f);
    fclose(tmp);
    
    if (found) {
        remove(CRED_FILE);
        rename("tmp_credentials.txt", CRED_FILE);
        printf("Role for '%s' changed to %s successfully!\n", target, new_role);
    } else {
        remove("tmp_credentials.txt");
        printf("Account not found.\n");
    }
}

// Admin menu: full access
void admin_menu() {
    int ch;
    do {
        printf("\n=== Admin Menu ===\n");
        printf("Student Management:\n");
        printf("  1. List students\n");
        printf("  2. Search student\n");
        printf("  3. Add student\n");
        printf("  4. Update student\n");
        printf("  5. Delete student\n");
        printf("\nCourse/Department Management:\n");
        printf("  6. Add Course/Branch\n");
        printf("  7. List Courses\n");
        printf("  8. Add Subject\n");
        printf("  9. List Subjects\n");
        printf("\nClass Management:\n");
        printf("  10. Assign Class to Student\n");
        printf("  11. View Students by Class\n");
        printf("\nTimetable Management:\n");
        printf("  12. Create Timetable Entry\n");
        printf("  13. View Timetable\n");
        printf("  14. Delete Timetable Entry\n");
        printf("\nAccount Management:\n");
        printf("  15. Add Staff/Guest Account\n");
        printf("  16. Remove Staff/Guest Account\n");
        printf("  17. Reset Password\n");
        printf("  18. Change User Role\n");
        printf("\n  0. Logout\n");
        printf("Choice: ");
        scanf("%d", &ch); clear_stdin();
        if (ch==1) list_students();
        else if (ch==2) search_student();
        else if (ch==3) add_student();
        else if (ch==4) update_student();
        else if (ch==5) delete_student();
        else if (ch==6) add_course();
        else if (ch==7) list_courses();
        else if (ch==8) add_subject();
        else if (ch==9) list_subjects();
        else if (ch==10) assign_class_to_student();
        else if (ch==11) view_students_by_class();
        else if (ch==12) create_timetable();
        else if (ch==13) view_timetable();
        else if (ch==14) delete_timetable_entry();
        else if (ch==15) add_staff_account();
        else if (ch==16) remove_staff_account();
        else if (ch==17) reset_password();
        else if (ch==18) change_role();
    } while (ch != 0);
}

int main() {
    printf("=== Student Management System ===\n");
    User me;
    if (!login(&me)) {
        printf("Login failed. Exiting.\n");
        printf("\nPress Enter to exit...");
        clear_stdin();
        getchar();
        return 0;
    }
    printf("Welcome %s! Role: %s\n", me.username, role_to_string(me.role));
    if (me.role == ROLE_GUEST) guest_menu();
    else if (me.role == ROLE_STAFF) staff_menu();
    else if (me.role == ROLE_ADMIN) admin_menu();
    else if (me.role == ROLE_STUDENT) student_menu(me.username);
    else printf("Role not recognized. Exiting.\n");
    printf("Goodbye!\n");
    printf("\nPress Enter to exit...");
    clear_stdin();
    getchar();
    return 0;
}

