// Transaction Processing System
// Bank-account program reads a random-access file sequentially,
// updates data already written to the file, creates new data to
// be placed in the file, and deletes data previously in the file.
//
// Version 2.0 Improvements:
//   - Multi-Factor Authentication (Username/Password + OTP)
//   - Auto-initializes credit.dat if it does not exist
//   - Fixed feof() anti-pattern in textFile()
//   - Fixed scanf format specifiers (%u for unsigned int)
//   - Added input validation to prevent infinite loops on bad input
//   - Added new menu option: List all accounts to console
//   - Replaced magic numbers with named constants
//   - Improved user prompts and error messages

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>   // _getch() for masked password (Windows)

// ── Constants ────────────────────────────────────────────────────────────────
#define MAX_ACCOUNTS        100
#define DATA_FILE           "credit.dat"
#define TEXT_FILE           "accounts.txt"

// MFA settings
#define MAX_LOGIN_ATTEMPTS  3
#define OTP_DIGITS          6
#define OTP_ATTEMPTS        3
#define USERNAME            "admin"
#define PASSWORD            "secure123"  // In production: use hashed storage

// ── Structure ────────────────────────────────────────────────────────────────
struct clientData
{
    unsigned int acctNum;  // account number  (0 = empty slot)
    char lastName[15];     // account last name
    char firstName[10];    // account first name
    double balance;        // account balance
};

// ── Prototypes ───────────────────────────────────────────────────────────────

// MFA
int          authenticate(void);
void         getPassword(char *buf, int maxLen);
int          verifyCredentials(const char *uname, const char *pass);
int          generateOTP(void);
int          verifyOTP(int otp);

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

// ── main ─────────────────────────────────────────────────────────────────────
int main(void)
{
    // ── Step 1: MFA Login ────────────────────────────────────────────────────
    printf("=========================================\n");
    printf("   Transaction Processing System v2.0\n");
    printf("=========================================\n");
    printf("         Multi-Factor Authentication\n");
    printf("=========================================\n\n");

    if (!authenticate())
    {
        fprintf(stderr, "\nAccess denied. Exiting.\n");
        return EXIT_FAILURE;
    }

    printf("\n[MFA] Authentication successful! Welcome, %s.\n\n", USERNAME);

    // ── Step 2: Open / create data file ─────────────────────────────────────
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

    // Verify file size
    fseek(cfPtr, 0L, SEEK_END);
    long fileSize = ftell(cfPtr);
    if (fileSize < (long)(MAX_ACCOUNTS * sizeof(struct clientData)))
    {
        printf("File too small. Re-initializing...\n");
        rewind(cfPtr);
        initFile(cfPtr);
    }

    // ── Step 3: Main menu loop ───────────────────────────────────────────────
    unsigned int choice;
    while ((choice = enterChoice()) != 6)
    {
        switch (choice)
        {
        case 1:
            textFile(cfPtr);
            break;
        case 2:
            listAccounts(cfPtr);
            break;
        case 3:
            updateRecord(cfPtr);
            break;
        case 4:
            newRecord(cfPtr);
            break;
        case 5:
            deleteRecord(cfPtr);
            break;
        default:
            puts("Invalid choice. Please enter a number from 1 to 6.");
            break;
        }
    }

    fclose(cfPtr);
    puts("\nSession closed. Goodbye!");
    return EXIT_SUCCESS;
}

// ════════════════════════════════════════════════════════════════════════════
//                       MFA FUNCTIONS
// ════════════════════════════════════════════════════════════════════════════

// ── authenticate ─────────────────────────────────────────────────────────────
// Runs both factors. Returns 1 on full success, 0 on failure.
int authenticate(void)
{
    char uname[32];
    char pass[32];
    int  attempts = 0;

    // ── Factor 1: Username + Password ────────────────────────────────────────
    printf("[Factor 1] Username & Password\n");
    printf("----------------------------------------\n");

    while (attempts < MAX_LOGIN_ATTEMPTS)
    {
        printf("Username: ");
        if (scanf("%31s", uname) != 1)
        {
            int ch; while ((ch = getchar()) != '\n' && ch != EOF);
            attempts++;
            continue;
        }
        int ch; while ((ch = getchar()) != '\n' && ch != EOF); // flush

        printf("Password: ");
        getPassword(pass, sizeof(pass));
        printf("\n");

        if (verifyCredentials(uname, pass))
            break;

        attempts++;
        int remaining = MAX_LOGIN_ATTEMPTS - attempts;
        if (remaining > 0)
            printf("[!] Invalid credentials. %d attempt(s) remaining.\n\n",
                   remaining);
        else
        {
            printf("[!] Too many failed attempts. Account locked.\n");
            return 0;
        }
    }

    // ── Factor 2: OTP Verification ───────────────────────────────────────────
    printf("\n[Factor 2] One-Time Password (OTP)\n");
    printf("----------------------------------------\n");

    srand((unsigned int)time(NULL));
    int otp = generateOTP();

    // Simulate sending OTP (in production: email/SMS via external service)
    printf("[OTP] Your 6-digit OTP has been sent to your registered device.\n");
    printf("[OTP SIMULATOR] OTP: %06d  <-- (simulated; normally sent to device)\n\n",
           otp);

    if (!verifyOTP(otp))
        return 0;

    return 1;
}

// ── getPassword ──────────────────────────────────────────────────────────────
// Reads a password character by character, masking with '*'.
void getPassword(char *buf, int maxLen)
{
    int  i = 0;
    char ch;

    while (i < maxLen - 1)
    {
        ch = (char)_getch();

        if (ch == '\r' || ch == '\n') // Enter
            break;

        if (ch == '\b' || ch == 127)  // Backspace
        {
            if (i > 0)
            {
                i--;
                printf("\b \b"); // erase '*'
            }
            continue;
        }

        buf[i++] = ch;
        printf("*"); // mask the character
    }
    buf[i] = '\0';
}

// ── verifyCredentials ────────────────────────────────────────────────────────
// Checks username and password against stored credentials.
// Returns 1 if valid, 0 otherwise.
int verifyCredentials(const char *uname, const char *pass)
{
    return (strcmp(uname, USERNAME) == 0 && strcmp(pass, PASSWORD) == 0);
}

// ── generateOTP ──────────────────────────────────────────────────────────────
// Generates a random 6-digit OTP (100000 – 999999).
int generateOTP(void)
{
    return (rand() % 900000) + 100000;
}

// ── verifyOTP ────────────────────────────────────────────────────────────────
// Prompts the user to enter the OTP. Allows OTP_ATTEMPTS tries.
// Returns 1 on success, 0 on failure.
int verifyOTP(int otp)
{
    int entered;
    int attempts = 0;

    while (attempts < OTP_ATTEMPTS)
    {
        printf("Enter OTP: ");
        int result = scanf("%d", &entered);
        int ch; while ((ch = getchar()) != '\n' && ch != EOF); // flush

        if (result != 1)
        {
            printf("[!] Invalid input. Please enter the 6-digit OTP.\n");
            attempts++;
            continue;
        }

        if (entered == otp)
        {
            printf("[OTP] Verified successfully.\n");
            return 1;
        }

        attempts++;
        int remaining = OTP_ATTEMPTS - attempts;
        if (remaining > 0)
            printf("[!] Incorrect OTP. %d attempt(s) remaining.\n\n", remaining);
        else
            printf("[!] OTP verification failed. Access denied.\n");
    }

    return 0;
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
    FILE              *writePtr;
    struct clientData  client;
    int                count = 0;

    writePtr = fopen(TEXT_FILE, "w");
    if (writePtr == NULL)
    {
        fprintf(stderr, "Error: Cannot open '%s' for writing.\n", TEXT_FILE);
        return;
    }

    fprintf(writePtr, "%-6s  %-15s  %-10s  %10s\n",
            "Acct", "Last Name", "First Name", "Balance");
    fprintf(writePtr, "%-6s  %-15s  %-10s  %10s\n",
            "------", "---------------", "----------", "----------");

    rewind(readPtr);
    while (fread(&client, sizeof(struct clientData), 1, readPtr) == 1)
    {
        if (client.acctNum != 0)
        {
            fprintf(writePtr, "%-6u  %-15s  %-10s  %10.2f\n",
                    client.acctNum, client.lastName,
                    client.firstName, client.balance);
            count++;
        }
    }

    fprintf(writePtr, "\nTotal active accounts: %d\n", count);
    fclose(writePtr);
    printf("'%s' written successfully (%d account(s)).\n", TEXT_FILE, count);
}

// ── listAccounts ─────────────────────────────────────────────────────────────
void listAccounts(FILE *fPtr)
{
    struct clientData client;
    int               count = 0;

    printf("\n%-6s  %-15s  %-10s  %10s\n",
           "Acct", "Last Name", "First Name", "Balance");
    printf("%-6s  %-15s  %-10s  %10s\n",
           "------", "---------------", "----------", "----------");

    rewind(fPtr);
    while (fread(&client, sizeof(struct clientData), 1, fPtr) == 1)
    {
        if (client.acctNum != 0)
        {
            printf("%-6u  %-15s  %-10s  %10.2f\n",
                   client.acctNum, client.lastName,
                   client.firstName, client.balance);
            count++;
        }
    }

    if (count == 0)
        puts("No active accounts found.");
    else
        printf("\nTotal active accounts: %d\n", count);
}

// ── updateRecord ─────────────────────────────────────────────────────────────
void updateRecord(FILE *fPtr)
{
    struct clientData client = {0, "", "", 0.0};
    unsigned int      account;
    double            transaction;

    printf("Enter account to update (1 - %d): ", MAX_ACCOUNTS);
    if (!readUInt(&account) || account < 1 || account > MAX_ACCOUNTS)
    {
        puts("Invalid account number.");
        return;
    }

    fseek(fPtr, (long)(account - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum == 0)
    {
        printf("Account #%u has no information.\n", account);
        return;
    }

    printf("\n%-6u  %-15s  %-10s  %10.2f\n\n",
           client.acctNum, client.lastName, client.firstName, client.balance);

    printf("Enter charge (+) or payment (-): ");
    if (!readDouble(&transaction))
    {
        puts("Invalid amount entered.");
        return;
    }

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
    unsigned int      accountNum;

    printf("Enter account number to delete (1 - %d): ", MAX_ACCOUNTS);
    if (!readUInt(&accountNum) || accountNum < 1 || accountNum > MAX_ACCOUNTS)
    {
        puts("Invalid account number.");
        return;
    }

    fseek(fPtr, (long)(accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum == 0)
    {
        printf("Account #%u does not exist.\n", accountNum);
        return;
    }

    printf("Deleting: %-6u  %-15s  %-10s  %10.2f\n",
           client.acctNum, client.lastName, client.firstName, client.balance);
    printf("Are you sure? (y/n): ");

    char confirm = (char)_getch();
    printf("%c\n", confirm);

    if (confirm != 'y' && confirm != 'Y')
    {
        puts("Deletion cancelled.");
        return;
    }

    fseek(fPtr, (long)(accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&blank, sizeof(struct clientData), 1, fPtr);
    printf("Account #%u deleted.\n", accountNum);
}

// ── newRecord ────────────────────────────────────────────────────────────────
void newRecord(FILE *fPtr)
{
    struct clientData client = {0, "", "", 0.0};
    unsigned int      accountNum;

    printf("Enter new account number (1 - %d): ", MAX_ACCOUNTS);
    if (!readUInt(&accountNum) || accountNum < 1 || accountNum > MAX_ACCOUNTS)
    {
        puts("Invalid account number.");
        return;
    }

    fseek(fPtr, (long)(accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum != 0)
    {
        printf("Account #%u already contains information.\n", client.acctNum);
        return;
    }

    printf("Enter last name  (max 14 chars): ");
    if (scanf("%14s", client.lastName) != 1) { puts("Error."); return; }

    printf("Enter first name (max  9 chars): ");
    if (scanf("%9s", client.firstName) != 1) { puts("Error."); return; }
    int ch; while ((ch = getchar()) != '\n' && ch != EOF);

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
    printf("   Transaction Processing System v2.0\n");
    printf("=========================================\n");
    printf("  1 - Save accounts to '%s'\n", TEXT_FILE);
    printf("  2 - List all accounts (console)\n");
    printf("  3 - Update an account\n");
    printf("  4 - Add a new account\n");
    printf("  5 - Delete an account\n");
    printf("  6 - Exit\n");
    printf("=========================================\n");
    printf("Your choice: ");

    if (!readUInt(&menuChoice))
        menuChoice = 0;

    return menuChoice;
}

// ── readUInt ─────────────────────────────────────────────────────────────────
int readUInt(unsigned int *out)
{
    int result = scanf("%u", out);
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
    return (result == 1);
}

// ── readDouble ───────────────────────────────────────────────────────────────
int readDouble(double *out)
{
    int result = scanf("%lf", out);
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
    return (result == 1);
}