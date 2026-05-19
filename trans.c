// Transaction Processing System
// Bank-account program reads a random-access file sequentially,
// updates data already written to the file, creates new data to
// be placed in the file, and deletes data previously in the file.
//
// Version 3.0 Improvements:
//   - THREE-Factor Authentication:
//       Factor 1 : Username + masked Password
//       Factor 2 : 6-digit OTP (simulated send)
//       Factor 3 : Face Recognition (simulated scan with terminal animation)
//   - Auto-initializes credit.dat if it does not exist
//   - Fixed feof() anti-pattern in textFile()
//   - Fixed scanf format specifiers (%u for unsigned int)
//   - Added input validation to prevent infinite loops on bad input
//   - Added new menu option: List all accounts to console
//   - Replaced magic numbers with named constants
//   - Improved user prompts and error messages
//
// NOTE: Face recognition here is simulated via terminal animation.
//       Production-grade face recognition requires OpenCV + a camera SDK
//       integrated as a separate module called from this program.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>      // _getch() – masked password (Windows)
#include <windows.h>    // Sleep()  – animation delays  (Windows)

// ── Constants ────────────────────────────────────────────────────────────────
#define MAX_ACCOUNTS        100
#define DATA_FILE           "credit.dat"
#define TEXT_FILE           "accounts.txt"

// MFA settings
#define MAX_LOGIN_ATTEMPTS  3
#define OTP_ATTEMPTS        3
#define USERNAME            "admin"
#define PASSWORD            "secure123"   // In production: use hashed storage

// Fingerprint simulation settings
#define FP_SCAN_STEPS       20   // progress bar steps for fingerprint scan
#define FP_MATCH_SCORE      92   // simulated confidence score (percent)
#define FP_THRESHOLD        75   // minimum score required
#define MAX_LOGIN_ATTEMPTS  3
#define OTP_ATTEMPTS        3
#define USERNAME            "admin"
#define PASSWORD            "secure123"   // In production: use hashed storage

// Face recognition simulation
#define FACE_SCAN_STEPS     20            // progress-bar width
#define FACE_MATCH_SCORE    87            // simulated confidence score (%)
#define FACE_THRESHOLD      70            // minimum score to grant access

// ── Structure ────────────────────────────────────────────────────────────────
struct clientData
{
    unsigned int acctNum;   // account number  (0 = empty slot)
    char lastName[15];      // account last name
    char firstName[10];     // account first name
    double balance;         // account balance
};

// ── Prototypes ───────────────────────────────────────────────────────────────

// MFA
int          authenticate(void);
int          fingerprintAuth(void);
void         getPassword(char *buf, int maxLen);
int          verifyCredentials(const char *uname, const char *pass);
int          generateOTP(void);
int          verifyOTP(int otp);
int          fingerprintAuth(void);
int          faceRecognition(void);
void         progressBar(int steps, int delayMs, const char *label);
void         printFaceFrame(void);

// Core operations
unsigned int enterChoice(void);
void         initFile(FILE *fPtr);
void         textFile(FILE *readPtr);
void         listAccounts(FILE *fPtr);
void         updateRecord(FILE *fPtr);
void         newRecord(FILE *fPtr);
void         deleteRecord(FILE *fPtr);
int          readUInt(unsigned int *out);
int          readDouble(double *out);

// ════════════════════════════════════════════════════════════════════════════
//                              main
// ════════════════════════════════════════════════════════════════════════════
int main(void)
{
    printf("=========================================\n");
    printf("   Transaction Processing System v3.0\n");
    printf("=========================================\n");
    printf("   Three-Factor Authentication (3FA)\n");
    printf("=========================================\n\n");

    if (!authenticate())
    {
        fprintf(stderr, "\n[ACCESS DENIED] Authentication failed. Exiting.\n");
        return EXIT_FAILURE;
    }

    printf("\n[3FA] All factors verified! Welcome, %s.\n\n", USERNAME);

    // ── Open / create data file ──────────────────────────────────────────────
    FILE *cfPtr = fopen(DATA_FILE, "rb+");
    if (cfPtr == NULL)
    {
        cfPtr = fopen(DATA_FILE, "wb+");
        if (cfPtr == NULL)
        {
            fprintf(stderr, "Error: Cannot create '%s'.\n", DATA_FILE);
            return EXIT_FAILURE;
        }
        printf("'%s' not found. Initializing with %d empty records...\n",
               DATA_FILE, MAX_ACCOUNTS);
        initFile(cfPtr);
        printf("Initialization complete.\n\n");
    }

    fseek(cfPtr, 0L, SEEK_END);
    if (ftell(cfPtr) < (long)(MAX_ACCOUNTS * sizeof(struct clientData)))
    {
        printf("File too small. Re-initializing...\n");
        rewind(cfPtr);
        initFile(cfPtr);
    }

    // ── Main menu loop ───────────────────────────────────────────────────────
    unsigned int choice;
    while ((choice = enterChoice()) != 6)
    {
        switch (choice)
        {
        case 1:  textFile(cfPtr);    break;
        case 2:  listAccounts(cfPtr);break;
        case 3:  updateRecord(cfPtr);break;
        case 4:  newRecord(cfPtr);   break;
        case 5:  deleteRecord(cfPtr);break;
        default: puts("Invalid choice. Please enter 1 – 6."); break;
        }
    }

    fclose(cfPtr);
    puts("\nSession closed. Goodbye!");
    return EXIT_SUCCESS;
}

// ════════════════════════════════════════════════════════════════════════════
//                          MFA FUNCTIONS
// ════════════════════════════════════════════════════════════════════════════

// ── authenticate ─────────────────────────────────────────────────────────────
// Runs all three factors. Returns 1 on full success, 0 on failure.
int authenticate(void)
{
    // ── Factor 1: Username + Password ────────────────────────────────────────
    printf("[Factor 1 / 3]  Username & Password\n");
    printf("------------------------------------------\n");

    char uname[32], pass[32];
    int  attempts = 0;

    while (attempts < MAX_LOGIN_ATTEMPTS)
    {
        printf("Username: ");
        if (scanf("%31s", uname) != 1)
        {
            int c; while ((c = getchar()) != '\n' && c != EOF);
            attempts++; continue;
        }
        int c; while ((c = getchar()) != '\n' && c != EOF);

        printf("Password: ");
        getPassword(pass, sizeof(pass));
        printf("\n");

        if (verifyCredentials(uname, pass)) break;

        attempts++;
        int rem = MAX_LOGIN_ATTEMPTS - attempts;
        if (rem > 0)
            printf("[!] Invalid credentials. %d attempt(s) left.\n\n", rem);
        else
        {
            printf("[!] Too many failed attempts. Account locked.\n");
            return 0;
        }
    }
    printf("[Factor 1]  PASSED\n\n");

    // ── Factor 2: OTP Verification ───────────────────────────────────────────
    printf("[Factor 2 / 3]  One-Time Password (OTP)\n");
    printf("------------------------------------------\n");

    srand((unsigned int)time(NULL));
    int otp = generateOTP();

    printf("[OTP] Your 6-digit OTP has been dispatched to your registered device.\n");
    printf("[OTP SIMULATOR]  OTP: %06d  <-- (simulated)\n\n", otp);

    if (!verifyOTP(otp)) return 0;
    printf("[Factor 2]  PASSED\n\n");

    // ── Factor 3: Face Recognition ───────────────────────────────────────────
    printf("[Factor 3 / 3]  Face Recognition\n");
    printf("------------------------------------------\n");

    if (!faceRecognition()) return 0;
    printf("[Factor 3]  PASSED\n");

    return 1;
}

// ── getPassword ──────────────────────────────────────────────────────────────
void getPassword(char *buf, int maxLen)
{
    int  i = 0;
    char ch;
    while (i < maxLen - 1)
    {
        ch = (char)_getch();
        if (ch == '\r' || ch == '\n') break;
        if (ch == '\b' || ch == 127)
        {
            if (i > 0) { i--; printf("\b \b"); }
            continue;
        }
        buf[i++] = ch;
        printf("*");
    }
    buf[i] = '\0';
}

// ── verifyCredentials ────────────────────────────────────────────────────────
int verifyCredentials(const char *uname, const char *pass)
{
    return (strcmp(uname, USERNAME) == 0 && strcmp(pass, PASSWORD) == 0);
}

// ── generateOTP ──────────────────────────────────────────────────────────────
int generateOTP(void)
{
    return (rand() % 900000) + 100000;
}

// ── verifyOTP ────────────────────────────────────────────────────────────────
int verifyOTP(int otp)
{
    int entered, attempts = 0;
    while (attempts < OTP_ATTEMPTS)
    {
        printf("Enter OTP: ");
        int r = scanf("%d", &entered);
        int c; while ((c = getchar()) != '\n' && c != EOF);

        if (r != 1)
        {
            printf("[!] Invalid input.\n");
            attempts++; continue;
        }
        if (entered == otp)
        {
            printf("[OTP] Verified.\n");
            return 1;
        }
        attempts++;
        int rem = OTP_ATTEMPTS - attempts;
        if (rem > 0)
            printf("[!] Incorrect OTP. %d attempt(s) left.\n\n", rem);
        else
            printf("[!] OTP failed. Access denied.\n");
    }
    return 0;
}

// ── faceRecognition ──────────────────────────────────────────────────────────
// Simulates a face-recognition scan with terminal animations.
// In a production system this would call an OpenCV / face-API library.
// Returns 1 if face matches, 0 otherwise.
int faceRecognition(void)
{
    // Existing face recognition simulation (unchanged)
    printf("Initializing camera module...\n");
    Sleep(800);
    printFaceFrame();
    Sleep(500);
    printf("Detecting facial landmarks");
    for (int i = 0; i < 5; i++) { printf("."); fflush(stdout); Sleep(300); }
    printf("  done\n");
    printf("Extracting facial features ");
    for (int i = 0; i < 5; i++) { printf("."); fflush(stdout); Sleep(300); }
    printf("  done\n");
    printf("Matching against stored profile:\n");
    progressBar(FACE_SCAN_STEPS, 80, "Scanning");
    int score = FACE_MATCH_SCORE;
    printf("\nConfidence Score : %d%%\n", score);
    printf("Threshold        : %d%%\n", FACE_THRESHOLD);
    if (score >= FACE_THRESHOLD) {
        printf("[FACE] Identity VERIFIED. Access granted.\n");
        return 1;
    } else {
        printf("[FACE] Face NOT recognized. Access denied.\n");
        return 0;
    }
}

// ── fingerprintAuth ───────────────────────────────────────────────────────────────
// Simulated fingerprint authentication with terminal animation.
int fingerprintAuth(void)
{
    printf("Initializing fingerprint sensor...\n");
    Sleep(600);
    // Simple ASCII representation of fingerprint scanner
    printf("+-------------------+\n");
    printf("|   _____________   |\n");
    printf("|  /             \  |\n");
    printf("| |   ~~~~~~~~   | |\n");
    printf("| |   ~~~~~~~~   | |\n");
    printf("|  \_____________/  |\n");
    printf("+-------------------+\n");
    Sleep(500);
    // Simulate scanning progress
    progressBar(FP_SCAN_STEPS, 70, "Scanning Fingerprint");
    // Simulated confidence score
    int score = FP_MATCH_SCORE;
    printf("\nFingerprint confidence: %d%% (threshold: %d%%)\n", score, FP_THRESHOLD);
    if (score >= FP_THRESHOLD) {
        printf("[FINGERPRINT] Match successful.\n");
        return 1;
    } else {
        printf("[FINGERPRINT] Match failed.\n");
        return 0;
    }
}
    printf("Initializing camera module...\n");
    Sleep(800);

    // Draw a simple ASCII face-scan frame
    printFaceFrame();
    Sleep(500);

    // Landmark detection phase
    printf("Detecting facial landmarks");
    for (int i = 0; i < 5; i++) { printf("."); fflush(stdout); Sleep(300); }
    printf("  done\n");

    // Feature extraction phase
    printf("Extracting facial features ");
    for (int i = 0; i < 5; i++) { printf("."); fflush(stdout); Sleep(300); }
    printf("  done\n");

    // Matching progress bar
    printf("Matching against stored profile:\n");
    progressBar(FACE_SCAN_STEPS, 80, "Scanning");

    // Simulate confidence score
    int score = FACE_MATCH_SCORE;
    printf("\nConfidence Score : %d%%\n", score);
    printf("Threshold        : %d%%\n", FACE_THRESHOLD);

    if (score >= FACE_THRESHOLD)
    {
        printf("[FACE] Identity VERIFIED. Access granted.\n");
        return 1;
    }
    else
    {
        printf("[FACE] Face NOT recognized. Access denied.\n");
        return 0;
    }
}

// ── progressBar ──────────────────────────────────────────────────────────────
// Prints an animated progress bar of `steps` blocks with `delayMs` per block.
void progressBar(int steps, int delayMs, const char *label)
{
    printf("%s [", label);
    for (int i = 0; i < steps; i++)
    {
        printf("#");
        fflush(stdout);
        Sleep(delayMs);
    }
    printf("] 100%%\n");
}

// ── printFaceFrame ────────────────────────────────────────────────────────────
// Prints an ASCII face-scan viewport to the terminal.
void printFaceFrame(void)
{
    printf("\n");
    printf("  +---------------------------+\n");
    printf("  |                           |\n");
    printf("  |    [  Face Scan Area  ]   |\n");
    printf("  |                           |\n");
    printf("  |    .--.                   |\n");
    printf("  |   |o  o|   <-- subject    |\n");
    printf("  |    \\__/                   |\n");
    printf("  |                           |\n");
    printf("  |  Scanning...              |\n");
    printf("  +---------------------------+\n");
    printf("\n");
}

// ════════════════════════════════════════════════════════════════════════════
//                      CORE BANK FUNCTIONS
// ════════════════════════════════════════════════════════════════════════════

// ── initFile ─────────────────────────────────────────────────────────────────
void initFile(FILE *fPtr)
{
    struct clientData blank = {0, "", "", 0.0};
    rewind(fPtr);
    for (int i = 0; i < MAX_ACCOUNTS; i++)
        fwrite(&blank, sizeof(struct clientData), 1, fPtr);
}

// ── textFile ─────────────────────────────────────────────────────────────────
void textFile(FILE *readPtr)
{
    FILE *writePtr = fopen(TEXT_FILE, "w");
    if (writePtr == NULL)
    {
        fprintf(stderr, "Error: Cannot open '%s' for writing.\n", TEXT_FILE);
        return;
    }

    struct clientData client;
    int count = 0;

    fprintf(writePtr, "%-6s  %-15s  %-10s  %10s\n",
            "Acct", "Last Name", "First Name", "Balance");
    fprintf(writePtr, "%-6s  %-15s  %-10s  %10s\n",
            "------", "---------------", "----------", "----------");

    rewind(readPtr);
    while (fread(&client, sizeof(struct clientData), 1, readPtr) == 1)
        if (client.acctNum != 0)
        {
            fprintf(writePtr, "%-6u  %-15s  %-10s  %10.2f\n",
                    client.acctNum, client.lastName,
                    client.firstName, client.balance);
            count++;
        }

    fprintf(writePtr, "\nTotal active accounts: %d\n", count);
    fclose(writePtr);
    printf("'%s' written (%d account(s)).\n", TEXT_FILE, count);
}

// ── listAccounts ─────────────────────────────────────────────────────────────
void listAccounts(FILE *fPtr)
{
    struct clientData client;
    int count = 0;

    printf("\n%-6s  %-15s  %-10s  %10s\n",
           "Acct", "Last Name", "First Name", "Balance");
    printf("%-6s  %-15s  %-10s  %10s\n",
           "------", "---------------", "----------", "----------");

    rewind(fPtr);
    while (fread(&client, sizeof(struct clientData), 1, fPtr) == 1)
        if (client.acctNum != 0)
        {
            printf("%-6u  %-15s  %-10s  %10.2f\n",
                   client.acctNum, client.lastName,
                   client.firstName, client.balance);
            count++;
        }

    if (count == 0) puts("No active accounts found.");
    else printf("\nTotal active accounts: %d\n", count);
}

// ── updateRecord ─────────────────────────────────────────────────────────────
void updateRecord(FILE *fPtr)
{
    struct clientData client = {0, "", "", 0.0};
    unsigned int account;
    double transaction;

    printf("Enter account to update (1 - %d): ", MAX_ACCOUNTS);
    if (!readUInt(&account) || account < 1 || account > MAX_ACCOUNTS)
    { puts("Invalid account number."); return; }

    fseek(fPtr, (long)(account - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum == 0)
    { printf("Account #%u has no information.\n", account); return; }

    printf("\n%-6u  %-15s  %-10s  %10.2f\n\n",
           client.acctNum, client.lastName, client.firstName, client.balance);

    printf("Enter charge (+) or payment (-): ");
    if (!readDouble(&transaction)) { puts("Invalid amount."); return; }

    client.balance += transaction;
    printf("Updated: %-6u  %-15s  %-10s  %10.2f\n",
           client.acctNum, client.lastName, client.firstName, client.balance);

    fseek(fPtr, (long)(account - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&client, sizeof(struct clientData), 1, fPtr);
}

// ── deleteRecord ─────────────────────────────────────────────────────────────
void deleteRecord(FILE *fPtr)
{
    struct clientData client;
    struct clientData blank = {0, "", "", 0.0};
    unsigned int accountNum;

    printf("Enter account number to delete (1 - %d): ", MAX_ACCOUNTS);
    if (!readUInt(&accountNum) || accountNum < 1 || accountNum > MAX_ACCOUNTS)
    { puts("Invalid account number."); return; }

    fseek(fPtr, (long)(accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum == 0)
    { printf("Account #%u does not exist.\n", accountNum); return; }

    printf("Deleting: %-6u  %-15s  %-10s  %10.2f\n",
           client.acctNum, client.lastName, client.firstName, client.balance);
    printf("Are you sure? (y/n): ");

    char confirm = (char)_getch();
    printf("%c\n", confirm);

    if (confirm != 'y' && confirm != 'Y')
    { puts("Deletion cancelled."); return; }

    fseek(fPtr, (long)(accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&blank, sizeof(struct clientData), 1, fPtr);
    printf("Account #%u deleted.\n", accountNum);
}

// ── newRecord ────────────────────────────────────────────────────────────────
void newRecord(FILE *fPtr)
{
    struct clientData client = {0, "", "", 0.0};
    unsigned int accountNum;

    printf("Enter new account number (1 - %d): ", MAX_ACCOUNTS);
    if (!readUInt(&accountNum) || accountNum < 1 || accountNum > MAX_ACCOUNTS)
    { puts("Invalid account number."); return; }

    fseek(fPtr, (long)(accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum != 0)
    { printf("Account #%u already contains information.\n", client.acctNum); return; }

    printf("Enter last name  (max 14 chars): ");
    if (scanf("%14s", client.lastName) != 1)  { puts("Error."); return; }

    printf("Enter first name (max  9 chars): ");
    if (scanf("%9s", client.firstName) != 1)  { puts("Error."); return; }
    int c; while ((c = getchar()) != '\n' && c != EOF);

    printf("Enter opening balance: ");
    if (!readDouble(&client.balance)) { puts("Invalid balance."); return; }

    client.acctNum = accountNum;
    fseek(fPtr, (long)(accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&client, sizeof(struct clientData), 1, fPtr);
    printf("Account #%u created successfully.\n", accountNum);
}

// ── enterChoice ──────────────────────────────────────────────────────────────
unsigned int enterChoice(void)
{
    unsigned int menuChoice = 0;
    printf("\n=========================================\n");
    printf("   Transaction Processing System v3.0\n");
    printf("=========================================\n");
    printf("  1 - Save accounts to '%s'\n", TEXT_FILE);
    printf("  2 - List all accounts (console)\n");
    printf("  3 - Update an account\n");
    printf("  4 - Add a new account\n");
    printf("  5 - Delete an account\n");
    printf("  6 - Exit\n");
    printf("=========================================\n");
    printf("Your choice: ");
    if (!readUInt(&menuChoice)) menuChoice = 0;
    return menuChoice;
}

// ── readUInt ─────────────────────────────────────────────────────────────────
int readUInt(unsigned int *out)
{
    int r = scanf("%u", out);
    int c; while ((c = getchar()) != '\n' && c != EOF);
    return (r == 1);
}

// ── readDouble ───────────────────────────────────────────────────────────────
int readDouble(double *out)
{
    int r = scanf("%lf", out);
    int c; while ((c = getchar()) != '\n' && c != EOF);
    return (r == 1);
}