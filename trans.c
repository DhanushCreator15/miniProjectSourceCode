// Transaction Processing System
// Bank-account program reads a random-access file sequentially,
// updates data already written to the file, creates new data to
// be placed in the file, and deletes data previously in the file.
//
// Improvements over original:
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

// ── Constants ────────────────────────────────────────────────────────────────
#define MAX_ACCOUNTS   100
#define DATA_FILE      "credit.dat"
#define TEXT_FILE      "accounts.txt"

// ── Structure ────────────────────────────────────────────────────────────────
struct clientData
{
    unsigned int acctNum;  // account number  (0 = empty slot)
    char lastName[15];     // account last name
    char firstName[10];    // account first name
    double balance;        // account balance
};

// ── Prototypes ───────────────────────────────────────────────────────────────
unsigned int enterChoice(void);
void         initFile(FILE *fPtr);
void         textFile(FILE *readPtr);
void         listAccounts(FILE *fPtr);
void         updateRecord(FILE *fPtr);
void         newRecord(FILE *fPtr);
void         deleteRecord(FILE *fPtr);
int          readUInt(unsigned int *out);   // safe unsigned-int input
int          readDouble(double *out);       // safe double input

// ── main ─────────────────────────────────────────────────────────────────────
int main(void)
{
    FILE        *cfPtr;
    unsigned int choice;

    // Open for read+write (binary). Create if not present.
    cfPtr = fopen(DATA_FILE, "rb+");
    if (cfPtr == NULL)
    {
        // File doesn't exist – create it and fill with blank records
        cfPtr = fopen(DATA_FILE, "wb+");
        if (cfPtr == NULL)
        {
            fprintf(stderr, "Error: Cannot create %s.\n", DATA_FILE);
            return EXIT_FAILURE;
        }
        printf("'%s' not found. Initializing with %d empty records...\n",
               DATA_FILE, MAX_ACCOUNTS);
        initFile(cfPtr);
        printf("Initialization complete.\n");
    }

    // Verify file has at least MAX_ACCOUNTS records
    fseek(cfPtr, 0L, SEEK_END);
    long fileSize = ftell(cfPtr);
    if (fileSize < (long)(MAX_ACCOUNTS * sizeof(struct clientData)))
    {
        printf("File is smaller than expected. Re-initializing...\n");
        rewind(cfPtr);
        initFile(cfPtr);
    }

    // Main menu loop
    while ((choice = enterChoice()) != 6)
    {
        switch (choice)
        {
        case 1:   // print text file of accounts
            textFile(cfPtr);
            break;
        case 2:   // list all accounts to console
            listAccounts(cfPtr);
            break;
        case 3:   // update an existing record
            updateRecord(cfPtr);
            break;
        case 4:   // add a new record
            newRecord(cfPtr);
            break;
        case 5:   // delete an existing record
            deleteRecord(cfPtr);
            break;
        default:
            puts("Invalid choice. Please enter a number from 1 to 6.");
            break;
        }
    }

    fclose(cfPtr);
    puts("Goodbye!");
    return EXIT_SUCCESS;
}

// ── initFile ─────────────────────────────────────────────────────────────────
// Writes MAX_ACCOUNTS blank (zero) clientData records into the file.
void initFile(FILE *fPtr)
{
    struct clientData blank = {0, "", "", 0.0};
    rewind(fPtr);
    for (int i = 0; i < MAX_ACCOUNTS; i++)
        fwrite(&blank, sizeof(struct clientData), 1, fPtr);
}

// ── textFile ─────────────────────────────────────────────────────────────────
// Saves all active accounts to accounts.txt for printing.
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
    // Use fread return value – avoids the feof() anti-pattern
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
// Displays all active accounts directly on the console.
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
// Updates the balance of an existing record.
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

    // Seek back and overwrite
    fseek(fPtr, (long)(account - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&client, sizeof(struct clientData), 1, fPtr);
}

// ── deleteRecord ─────────────────────────────────────────────────────────────
// Deletes a record by zeroing it out.
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

    printf("Deleting account: %-6u  %-15s  %-10s  %10.2f\n",
           client.acctNum, client.lastName, client.firstName, client.balance);

    // Confirm deletion
    printf("Are you sure? (y/n): ");
    char confirm = getchar();
    // Flush remaining input
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);

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
// Creates and inserts a new record.
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

    printf("Enter last name (max 14 chars): ");
    if (scanf("%14s", client.lastName) != 1)
    {
        puts("Error reading last name.");
        return;
    }

    printf("Enter first name (max 9 chars): ");
    if (scanf("%9s", client.firstName) != 1)
    {
        puts("Error reading first name.");
        return;
    }

    printf("Enter opening balance: ");
    if (!readDouble(&client.balance))
    {
        puts("Invalid balance entered.");
        return;
    }

    client.acctNum = accountNum;
    fseek(fPtr, (long)(accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&client, sizeof(struct clientData), 1, fPtr);
    printf("Account #%u created successfully.\n", accountNum);
}

// ── enterChoice ──────────────────────────────────────────────────────────────
unsigned int enterChoice(void)
{
    unsigned int menuChoice = 0;

    printf("\n========================================\n");
    printf("  Transaction Processing System\n");
    printf("========================================\n");
    printf("  1 - Save accounts to '%s'\n", TEXT_FILE);
    printf("  2 - List all accounts (console)\n");
    printf("  3 - Update an account\n");
    printf("  4 - Add a new account\n");
    printf("  5 - Delete an account\n");
    printf("  6 - Exit\n");
    printf("========================================\n");
    printf("Your choice: ");

    if (!readUInt(&menuChoice))
        menuChoice = 0; // will hit default in switch

    return menuChoice;
}

// ── readUInt ─────────────────────────────────────────────────────────────────
// Safely reads an unsigned int. Returns 1 on success, 0 on failure.
// Clears the input buffer on failure to prevent infinite loops.
int readUInt(unsigned int *out)
{
    int result = scanf("%u", out);
    int ch;
    // Flush rest of line
    while ((ch = getchar()) != '\n' && ch != EOF);
    return (result == 1);
}

// ── readDouble ───────────────────────────────────────────────────────────────
// Safely reads a double. Returns 1 on success, 0 on failure.
int readDouble(double *out)
{
    int result = scanf("%lf", out);
    int ch;
    // Flush rest of line
    while ((ch = getchar()) != '\n' && ch != EOF);
    return (result == 1);
}