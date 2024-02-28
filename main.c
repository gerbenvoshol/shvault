// gcc main.c sqlite3.c libtct.c libstr.c -DSQLITE_HAS_CODEC /usr/lib/x86_64-linux-gnu/libcrypto.a  -Wall -o shvault

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "libtct.h"
#include "libstr.h"
#include "nanoid.h"
#include "sqlite3.h"

// Function prototypes
static int callback(void *NotUsed, int argc, char **argv, char **azColName);
int open_database(sqlite3 **db, const char* dbPath, const char* password);
int create_table(sqlite3 *db);
int insert_entry(sqlite3 *db, const char *key, const char *value);
char* search_entry(sqlite3 *db, const char *key);
int update_entry(sqlite3 *db, const char *key, const char *newValue);
int delete_entry(sqlite3 *db, const char *key);
int list_entries(sqlite3 *db);

void print_help(char *program);

int main(int argc, char **argv) {
    int opt;
    sqlite3 *db;
    char *dbPath = "default.db"; // Default database path
    char *password = NULL; // Default to no password

    // Flags for operations
    bool appendFlag = false, listFlag = false, eraseFlag = false;
    bool searchFlag = false, execFlag = false, replaceFlag = false;
    bool inputFlag = false, findFlag = false;
    char *key = NULL;
    char *value = NULL;
    int rc;

    char* result = NULL;
    tct_arguments_t* args = NULL;
    char **find_replace = NULL;
    int N = 0;

    // Use getopt to parse command line options
    while ((opt = getopt(argc, argv, "scqf:arev:p:l")) != -1) {
        switch (opt) {
            case 's':
                // Option for showing entries
                searchFlag = true;
                break;
            case 'c':
                // Option for executing commands (located in value)
                execFlag = true;
                break;
            case 'q':
                // Option for executing commands (located in value)
                inputFlag = true;
                break;
            case 'f':
                // Option for find&replace for findFlag
                findFlag = true;
                find_replace = strsplit(optarg, ':', &N);
                if (N != 2) {
                    fprintf(stderr, "Wrong format, use find:replace\n");
                    return 1;
                }
                // Add find replace to templating engine
                tct_add_argument(args, find_replace[0], "%s", find_replace[1]);
                
                // Cleanup
                for (int i = 0; i < N; i++) {
                    free(find_replace[i]);
                }
                free(find_replace);
                find_replace = NULL;
                N = 0;
                break;
            case 'a':
                // Option for appending/insert entries
                appendFlag = true;
                break;
            case 'r':
                // Option for replacing entries
                replaceFlag = true;
                break;
            case 'e':
                // Option for erasing entries
                eraseFlag = true;
                break;
            case 'v':
                // Option for specifying vault (database file)
                dbPath = optarg;
                break;
            case 'p':
                // Option for providing a password
                password = optarg;
                break;
            case 'l':
                // Option for listing all entries
                listFlag = true;
                break;
            default:
                print_help(argv[0]);
                return 1;
        }
    }

    // After parsing options, process the key and value
    int remaining_args = argc - optind;
    if (remaining_args >= 1) {
        key = argv[optind]; // Get the key from the arguments
        if (remaining_args >= 2) {
            if (inputFlag) {
                fprintf(stderr, "Value already provided while requesting prompt\n");
                print_help(argv[0]);
                return 1;
            }
            value = argv[optind + 1]; // Get the value if provided
        } else if (!searchFlag && !execFlag) {
            // If value is not provided, read from stdin
            if (inputFlag) {
                value = read_stdin("Enter value: ");
            } else {
                value = read_stdin(NULL);
            }
        }
    } else if (!listFlag) {
        fprintf(stderr, "Key not provided\n");
        print_help(argv[0]);
        return 1;
    }

    // Ensure both dbPath and password are provided for database operations.
    if (dbPath && password) {
        rc = open_database(&db, dbPath, password);
        memset(password, '\0', strlen(password));
    } else {
        password = getenv("SHVAULT_PASSWORD"); // Assume the environment variable SHVAULT_PASSWORD stores your encryption key
        if (!password) {
            fprintf(stderr, "Database password is not set in SHVAULT_PASSWORD environment variables or given as argument -p.\n");
            exit(1);
        } else {
            rc = open_database(&db, dbPath, password);
            memset(password, '\0', strlen(password));
        }
    }

    if (!(sqlite3_exec(db, "SELECT count(*) FROM sqlite_master;", NULL, NULL, NULL) == SQLITE_OK)) {
        fprintf(stderr, "Database password is not correct.\n");
        exit(1);
    }

    // Create the table if it doesn't exist
    rc = create_table(db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create table\n");
        sqlite3_close(db);
        return 1;
    }

    if (appendFlag && key != NULL) {
        if (insert_entry(db, key, value) != SQLITE_OK) {
            fprintf(stderr, "Failed to create entry %s\n", key);
            return 1;
        }
    }

    if (replaceFlag && key != NULL) {
        if (update_entry(db, key, value) != SQLITE_OK) {
            fprintf(stderr, "Failed to update entry %s\n", key);
            return 1;
        }
    }

    if (listFlag) {
        if (list_entries(db) != SQLITE_OK) {
            fprintf(stderr, "Failed to list entries\n");
            return 1;
        }
    }

    if (eraseFlag && key != NULL) {
        if (delete_entry(db, key) != SQLITE_OK) {
            fprintf(stderr, "Failed to erase entry %s\n", key);
            return 1;
        }
    }

    if (searchFlag && key != NULL) {
        if (!(value = search_entry(db, key))) {
            fprintf(stderr, "Failed to find entry %s\n", key);
            return 1;
        } else {
            //printf("Key: %s, Value: %s\n", key, value);
            printf("%s", value);
        }
    }

    if (execFlag && key != NULL) {
        if (!(value = search_entry(db, key))) {
            fprintf(stderr, "Failed to find entry\n");
            return 1;
        }

        if (findFlag) {
            result = tct_render(value, args);
            system(result);

            if (args) {
                tct_free_argument(args);
            }
            if (result) {
                free(result);
            }
        } else {
            system(value);
        }
    }
    
    if (value) {
        free(value);
    }

    // Close the database
    sqlite3_close(db);
    return 0;
}

void print_help(char *program) {
    printf("Usage: %s [options] [key] [value]\n", program);
    printf("Options:\n");
    printf("  -s             Show value for key\n");
    printf("  -c             Execute commands located in value\n");
    printf("  -a             Append/insert entries\n");
    printf("  -r             Replace entries\n");
    printf("  -e             Erase entries\n");
    printf("  -v <vault>     Specify vault (database file)\n");
    printf("  -p <password>  Provide a password for the database\n");
    printf("  -q             Type vault entry using prompt\n");
    printf("  -f             Find and replace for vault templates using the following format find:replace\n");
    printf("  -l             List all entries\n");
    printf("  -h             Show this help message\n");
    printf("\nExamples:\n");
    printf("  export SHVAULT_PASSWORD=secret       Set 'secret' as the password for the database\n");
    printf("  %s -a key \"value to insert\"    Insert 'value to insert' under 'key'\n", program);
    printf("  %s -l                          List all entries in the database\n", program);
    printf("  %s -e key                      Delete the entry with 'key'\n", program);
    printf("  %s -v myvault.db -p secret     Use 'myvault.db' as the database with 'secret' as the password\n", program);
    printf("  \n");
    printf("  You can also append a string or password to a vault from stdin\n");
    printf("  printf \"MyPassword\" | %s -a MySecretPassword\n", program);
    printf("  \n");
    printf("  Run as command : Last but not least you can store short command scripts and execute\n");
    printf("  the vaulted string content as command(s). This is practical if you need to put commands\n");
    printf("  in scripts that have sensitive strings or passwords in plain text. This will hide those\n");
    printf("  strings or commands from the script.\n");
    printf("  echo date | %s -a MySecretCommand\n", program);
    printf("  %s -c MySecretCommand\n", program);
    printf("  \n");
    printf("  Safely store login scripts with hard coded passwords.\n");
    printf("  Content of phrase.sh\n");
    printf("  #!/bin/bash\n");
    printf("  echo \"{{ 1 }} {{ 2 }}!\"\n");
    printf("  \n");
    printf("  %s -a MySecretPhrase < phrase.sh\n", program);
    printf("  %s -c MySecretPhrase -f 1:Hello -f 2:World\n", program);
    exit(1);
}

/**
 * Initializes and opens a SQLite database with encryption and foreign key support.
 * 
 * @param db A pointer to a pointer to the SQLite database, to be initialized by this function.
 * @param password The password used to encrypt the database.
 */
int open_database(sqlite3 **db, const char* dbPath, const char* password) {
    char *errorMessage = NULL;
    int connect;

    // Attempt to open the database
    connect = sqlite3_open(dbPath, db);
    if (connect != SQLITE_OK) {
        fprintf(stderr, "Failed to open database: %s\n", sqlite3_errmsg(*db));
        sqlite3_close(*db); // Close the database if open failed
        return 1; // Exit with a non-zero status to indicate failure
    }

    // Encrypt the database with the given password
    char *pragmaKey = sqlite3_mprintf("PRAGMA key = '%q';", password);
    connect = sqlite3_exec(*db, pragmaKey, NULL, NULL, &errorMessage);
    if (connect != SQLITE_OK) {
        fprintf(stderr, "Failed to encrypt database: %s\n", errorMessage);
        sqlite3_free(errorMessage);
        sqlite3_free(pragmaKey);
        sqlite3_close(*db); // Ensure the database is closed on failure
        return 1;
    }
    sqlite3_free(pragmaKey);

    // Enable foreign key support
    char *pragmaFK = "PRAGMA foreign_keys = ON;";
    connect = sqlite3_exec(*db, pragmaFK, NULL, NULL, &errorMessage);
    if (connect != SQLITE_OK) {
        fprintf(stderr, "Failed to enable foreign key support: %s\n", errorMessage);
        sqlite3_free(errorMessage);
        sqlite3_close(*db); // Ensure the database is closed on failure
        return 1;
    }

    return connect;
}


/**
 * Searches for an entry in the database by key and returns the encrypted value.
 * 
 * @param db A pointer to the SQLite database.
 * @param key The key to search for in the database.
 * @return A dynamically allocated string containing the encrypted value, or NULL if the key is not found or an error occurs.
 */
char* search_entry(sqlite3 *db, const char *key) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT value FROM vault WHERE key = ?";
    char *result = NULL;

    // Prepare the SQL statement
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    
    // Bind the key to the SQL query
    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
    
    // Execute the query and check if the key exists
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char *encryptedValue = sqlite3_column_text(stmt, 0);
        result = strdup((const char *)encryptedValue);
    } else if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }
    
    // Clean up
    sqlite3_finalize(stmt);
    
    return result; // Return the dynamically allocated string or NULL
}

/**
 * Attempts to create a table in the provided SQLite database.
 * 
 * @param db A pointer to the SQLite database.
 * @return SQLITE_OK on success, or an SQLite error code on failure.
 */
int create_table(sqlite3 *db) {
    char *errMsg = NULL;
    const char *sql = 
        "CREATE TABLE IF NOT EXISTS vault("
        "key TEXT PRIMARY KEY NOT NULL, "
        "value TEXT NOT NULL);";

    int rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }

    return rc;
}

/**
 * Inserts a new entry into the database.
 * 
 * @param db A pointer to the SQLite database connection.
 * @param key The key of the entry to insert.
 * @param value The value associated with the key.
 * @return SQLITE_OK on success, or an error code on failure.
 */
int insert_entry(sqlite3 *db, const char *key, const char *value) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO vault (key, value) VALUES (?, ?);";
    int rc;

    // Prepare the SQL statement
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    // Bind the key to the first parameter (?) in the SQL statement
    rc = sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to bind key: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt); // Finalize the statement to avoid resource leaks
        return rc;
    }

    // Bind the value to the second parameter (?) in the SQL statement
    rc = sqlite3_bind_text(stmt, 2, value, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to bind value: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt); // Finalize the statement to avoid resource leaks
        return rc;
    }

    // Execute the SQL statement
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert entry: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt); // Finalize the statement to avoid resource leaks
        return rc;
    }

    // Finalize the statement to release resources
    sqlite3_finalize(stmt);

    return SQLITE_OK; // Success
}

/**
 * Deletes an entry from the database based on the provided key.
 * 
 * @param db A pointer to the SQLite database connection.
 * @param key The key of the entry to delete.
 * @return SQLITE_OK on success, or an error code on failure.
 */
int delete_entry(sqlite3 *db, const char *key) {
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM vault WHERE key = ?;";
    int rc;

    // Prepare the SQL statement for execution
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    // Bind the key to the parameter (?) in the SQL statement
    rc = sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to bind key: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt); // Always finalize the statement to avoid resource leaks
        return rc;
    }

    // Execute the SQL statement
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to delete entry: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt); // Always finalize the statement to avoid resource leaks
        return rc;
    }

    // Finalize the statement to release resources
    sqlite3_finalize(stmt);

    return SQLITE_OK; // Success
}

/**
 * Updates the value of an entry in the database based on the provided key.
 * 
 * @param db A pointer to the SQLite database connection.
 * @param key The key of the entry to update.
 * @param newValue The new value to assign to the entry.
 * @return SQLITE_OK on success, or an error code on failure.
 */
int update_entry(sqlite3 *db, const char *key, const char *newValue) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE vault SET value = ? WHERE key = ?;";
    int rc;

    // Prepare the SQL statement for execution
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    // Bind the new value to the first parameter (?) in the SQL statement
    rc = sqlite3_bind_text(stmt, 1, newValue, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to bind new value: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt); // Always finalize the statement to avoid resource leaks
        return rc;
    }

    // Bind the key to the second parameter (?) in the SQL statement
    rc = sqlite3_bind_text(stmt, 2, key, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to bind key: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt); // Always finalize the statement to avoid resource leaks
        return rc;
    }

    // Execute the SQL statement
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to update entry: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt); // Always finalize the statement to avoid resource leaks
        return rc;
    }

    // Finalize the statement to release resources
    sqlite3_finalize(stmt);

    return SQLITE_OK; // Success
}

/**
 * Lists all entries in the database.
 * 
 * @param db A pointer to the SQLite database connection.
 * @return SQLITE_OK on success, or an error code on failure.
 */
int list_entries(sqlite3 *db) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT key, value FROM vault;";
    int rc;

    // Prepare the SQL statement for execution
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    // Execute the SQL statement and iterate over the results
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char *key = sqlite3_column_text(stmt, 0);
        const unsigned char *value = sqlite3_column_text(stmt, 1);
        printf("Key: %s, Value: %s\n", key, value);
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to list entries: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt); // Always finalize the statement to avoid resource leaks
        return rc;
    }

    // Finalize the statement to release resources
    sqlite3_finalize(stmt);

    return SQLITE_OK; // Success
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for(i=0; i < argc; i++){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}
