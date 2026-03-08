/*
 * Smart Fleet Management Algorithm Engine
 * Compile: gcc fleet_algo.c -o fleet_algo -lm
 * Usage: ./fleet_algo <employees_json> <num_cabs> <seats_per_cab>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_EMPLOYEES 100
#define MAX_CABS 20

typedef struct {
    int id;
    int x, y;
    int assigned_cab;
} Employee;

typedef struct {
    int id;
    int employees[MAX_EMPLOYEES];
    int count;
    double total_distance;
} Cab;

// Manhattan distance
double manhattan(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

// Parse simple JSON array of employees
int parse_employees(char *json, Employee *emps) {
    int count = 0;
    char *ptr = json;
    while (*ptr && count < MAX_EMPLOYEES) {
        if (*ptr == '{') {
            int id, x, y;
            if (sscanf(ptr, "{\"id\":%d,\"x\":%d,\"y\":%d}", &id, &x, &y) == 3) {
                emps[count].id = id;
                emps[count].x = x;
                emps[count].y = y;
                emps[count].assigned_cab = -1;
                count++;
            }
        }
        ptr++;
    }
    return count;
}

// Greedy algorithm: start EACH CAB from office, then pick nearest unassigned
double greedy_assign(Employee *emps, int n, Cab *cabs, int num_cabs, int seats, int office_x, int office_y) {
    int assigned[MAX_EMPLOYEES] = {0};
    double total = 0;

    for (int c = 0; c < num_cabs; c++) {
        cabs[c].id = c;
        cabs[c].count = 0;
        cabs[c].total_distance = 0;

        // current_location = office (as per pseudocode)
        int cur_x = office_x, cur_y = office_y;

        // Fill seats: always pick nearest unassigned employee
        for (int s = 0; s < seats; s++) {
            double min_d = 1e18;
            int best = -1;
            for (int i = 0; i < n; i++) {
                if (!assigned[i]) {
                    double d = manhattan(cur_x, cur_y, emps[i].x, emps[i].y);
                    if (d < min_d) { min_d = d; best = i; }
                }
            }
            if (best == -1) break;   // no more employees for this cab

            // Add to route, mark visited, update current_location
            assigned[best] = 1;
            emps[best].assigned_cab = c;
            cabs[c].employees[cabs[c].count++] = best;
            cabs[c].total_distance += min_d;
            cur_x = emps[best].x;
            cur_y = emps[best].y;
        }
        // Return to office
        cabs[c].total_distance += manhattan(cur_x, cur_y, office_x, office_y);
        total += cabs[c].total_distance;
    }
    return total;
}

// Random assignment
double random_assign(Employee *emps, int n, Cab *cabs, int num_cabs, int seats, int office_x, int office_y) {
    // Shuffle employees
    int order[MAX_EMPLOYEES];
    for (int i = 0; i < n; i++) order[i] = i;
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
    }

    double total = 0;
    int idx = 0;
    for (int c = 0; c < num_cabs && idx < n; c++) {
        cabs[c].id = c;
        cabs[c].count = 0;
        cabs[c].total_distance = 0;
        int cur_x = office_x, cur_y = office_y;

        for (int s = 0; s < seats && idx < n; s++, idx++) {
            int e = order[idx];
            cabs[c].total_distance += manhattan(cur_x, cur_y, emps[e].x, emps[e].y);
            cur_x = emps[e].x; cur_y = emps[e].y;
            cabs[c].employees[cabs[c].count++] = e;
        }
        cabs[c].total_distance += manhattan(cur_x, cur_y, office_x, office_y);
        total += cabs[c].total_distance;
    }
    return total;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("{\"error\":\"Usage: fleet_algo <employees_json> <num_cabs> <seats_per_cab>\"}\n");
        return 1;
    }

    srand(time(NULL));

    Employee emps[MAX_EMPLOYEES];
    Cab greedy_cabs[MAX_CABS], random_cabs[MAX_CABS];

    int n = parse_employees(argv[1], emps);
    int num_cabs = atoi(argv[2]);
    int seats = atoi(argv[3]);
    int office_x = 0, office_y = 0;

    if (n == 0) {
        printf("{\"error\":\"No employees parsed\"}\n");
        return 1;
    }
    if (num_cabs > MAX_CABS) num_cabs = MAX_CABS;

    double greedy_dist = greedy_assign(emps, n, greedy_cabs, num_cabs, seats, office_x, office_y);
    double random_dist = random_assign(emps, n, random_cabs, num_cabs, seats, office_x, office_y);

    double improvement = random_dist > 0 ? ((random_dist - greedy_dist) / random_dist) * 100.0 : 0;
    int seats_used = n < num_cabs * seats ? n : num_cabs * seats;
    double utilization = (double)seats_used / (num_cabs * seats) * 100.0;

    // Count active cabs
    int active = 0;
    for (int c = 0; c < num_cabs; c++) if (greedy_cabs[c].count > 0) active++;
    double avg_dist = active > 0 ? greedy_dist / active : 0;

    // Output JSON
    printf("{");
    printf("\"greedy_distance\":%.2f,", greedy_dist);
    printf("\"random_distance\":%.2f,", random_dist);
    printf("\"improvement\":%.2f,", improvement);
    printf("\"seats_used\":%d,", seats_used);
    printf("\"total_seats\":%d,", num_cabs * seats);
    printf("\"utilization\":%.2f,", utilization);
    printf("\"avg_distance\":%.2f,", avg_dist);
    printf("\"active_cabs\":%d,", active);
    printf("\"assignments\":[");
    for (int c = 0; c < num_cabs; c++) {
        if (c > 0) printf(",");
        printf("{\"cab\":%d,\"employees\":[", c);
        for (int i = 0; i < greedy_cabs[c].count; i++) {
            if (i > 0) printf(",");
            printf("%d", greedy_cabs[c].employees[i]);
        }
        printf("],\"distance\":%.2f}", greedy_cabs[c].total_distance);
    }
    printf("]}\n");

    return 0;
}
