#include <sqlite3ext.h> // All functions for development for SQLITE
#include <string.h>     // Standard library for working with strings and memory

SQLITE_EXTENSION_INIT1  // Macro from sqlite3ext.h that includes the necessary pointers and variables for initialization

// User defined function logic. The function name must be the same as the entry point!
static void length_function(sqlite3_context *context, int argc, sqlite3_value **argv) {
    if (argc != 1) {
        sqlite3_result_error(context, "Incorrect number of arguments", -1);
        return;
    }
    const char *str = (const char *)sqlite3_value_text(argv[0]);
    if (str) {
        int length = strlen(str) ;  // Calculating the length of a string
        sqlite3_result_int(context, length);  // Return the result
    } else sqlite3_result_null(context); 
}

// Export initialization function for Windows
#ifdef _WIN32
__declspec(dllexport)
#endif

// Initializing the extension
int sqlite3_report_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi) {
  SQLITE_EXTENSION_INIT2(pApi); //Macro from sqlite3ext.h indicating the moment of initialization itself
  return sqlite3_create_function(db, "length_function", 1, SQLITE_UTF8, NULL, length_function, NULL, NULL); // Registering a user defined function
}