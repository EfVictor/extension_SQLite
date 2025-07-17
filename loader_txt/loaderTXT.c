#include <sqlite3ext.h> // All functions for development for SQLITE
#include <stdio.h>  // Standard library for working with console, files and buffers (input and output I/O)
#include <string.h> // Standard library for working with strings and memory
#include <stdlib.h> // Standard library for working with memory, system operations and program termination (free, malloc, strdup)

SQLITE_EXTENSION_INIT1 // Macro from sqlite3ext.h that includes the necessary pointers and variables for initialization

#define MAX_LINE 1024 //  Analogue of global constant in Javascript

// Structure for virtual table
typedef struct {
    sqlite3_vtab base;
    char *filename;
} TxtTable;

// Structure for cursor
typedef struct {
    sqlite3_vtab_cursor base;
    sqlite_int64 rowid;
    FILE *file;
    char *current_line;
    int eof;
} TxtCursor;

// Create table and parse arguments (called in 1st queue)
static int txtConnect(sqlite3 *db, void *pAux, int argc, const char *const *argv, sqlite3_vtab **ppVtab, char **pzErr) {
    TxtTable *vtab = (TxtTable *)sqlite3_malloc(sizeof(TxtTable));
    if (!vtab) return SQLITE_NOMEM;
    memset(vtab, 0, sizeof(TxtTable));

    if (argc >= 4 && argv[3] && strlen(argv[3]) > 0) {
        const char *arg = argv[3];

        // Remove outer single quotes if any
        if (arg[0] == '\'' && arg[strlen(arg) - 1] == '\'') {
            char *tmp = sqlite3_mprintf("%s", arg + 1);  // Skip the first quote
            if (!tmp) {
                sqlite3_free(vtab);
                return SQLITE_NOMEM;
            }
            tmp[strlen(tmp) - 1] = '\0';  // Remove the last quotation mark
            vtab->filename = tmp;
        } else {
            vtab->filename = sqlite3_mprintf("%s", arg);
            if (!vtab->filename) {
                sqlite3_free(vtab);
                return SQLITE_NOMEM;
            }
        }

        // Replace '\' with '/' for compatibility
        for (char *p = vtab->filename; *p; ++p)
            if (*p == '\\') *p = '/';

    } else {
        *pzErr = sqlite3_mprintf("Expected a filename as an argument");
        sqlite3_free(vtab);
        return SQLITE_ERROR;
    }

    // Create schema
    sqlite3_declare_vtab(db, "CREATE TABLE x(line TEXT)");
    *ppVtab = (sqlite3_vtab *)vtab;
    return SQLITE_OK;
}

// Called before each request is executed (called in the 2nd queue)
static int txtBestIndex(sqlite3_vtab *tab, sqlite3_index_info *pIdxInfo) {
    return SQLITE_OK;
}

// Create a cursor (called in 3rd queue)
static int txtOpen(sqlite3_vtab *p, sqlite3_vtab_cursor **ppCursor) {
     TxtCursor *cur = (TxtCursor *)sqlite3_malloc(sizeof(TxtCursor));
    if (cur == NULL) return SQLITE_NOMEM;
    memset(cur, 0, sizeof(TxtCursor));

    TxtTable *vtab = (TxtTable *)p;
    fprintf(stderr, "Path to open file: %s\n", vtab->filename);

    // Opening file in "r" mode for reading
    cur->file = fopen(vtab->filename, "r");
    if (cur->file == NULL) {
        perror("Error opening file");
        fprintf(stderr, "Cannot open file: %s\n", vtab->filename);
        sqlite3_free(cur);
        return SQLITE_IOERR;
    }

    *ppCursor = (sqlite3_vtab_cursor *)cur;
    return SQLITE_OK;
}

// Initialization of the cursor. Iterating over rows through selection - launching a SQL query
static int txtFilter(sqlite3_vtab_cursor *cur, int idxNum, const char *idxStr,int argc, sqlite3_value **argv) {
    TxtCursor *c = (TxtCursor *)cur;
    TxtTable *vtab = (TxtTable *)(cur->pVtab);
    fseek(c->file, 0, SEEK_SET);
    c->eof = 0;
    c->rowid = 0;
    if (c->current_line) {
        free(c->current_line);
        c->current_line = NULL;
    }
    return txtNext(cur);
}

// Get next row
static int txtNext(sqlite3_vtab_cursor *cur) {
    TxtCursor *c = (TxtCursor *)cur;
    if (c->current_line) {
        free(c->current_line);
        c->current_line = NULL;
    }

    char buffer[MAX_LINE];
    if (fgets(buffer, MAX_LINE, c->file)) {
        c->current_line = strdup(buffer);
        size_t len = strlen(c->current_line);
        if (len > 0 && c->current_line[len - 1] == '\n') c->current_line[len - 1] = '\0';
        c->rowid++;
    } else c->eof = 1;
    return SQLITE_OK;
}

// Cursor handling when reading the last data. Called after each row (Next)
static int txtEof(sqlite3_vtab_cursor *cur) {
    TxtCursor *c = (TxtCursor *)cur;
    return c->eof;
}

// Getting the value of a column
static int txtColumn(sqlite3_vtab_cursor *cur, sqlite3_context *ctx, int i) {
    TxtCursor *c = (TxtCursor *)cur;
    if (c->current_line) sqlite3_result_text(ctx, c->current_line, -1, SQLITE_TRANSIENT);
    else sqlite3_result_null(ctx);
    return SQLITE_OK;
}

// Getting a unique identifier for a row
static int txtRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid) {
    TxtCursor *c = (TxtCursor *)cur;
    *pRowid = c->rowid;
    return SQLITE_OK;
}

// Close cursor
static int txtClose(sqlite3_vtab_cursor *cur) {
    TxtCursor *c = (TxtCursor *)cur;
    if (c->file) fclose(c->file);
    if (c->current_line) free(c->current_line);
    sqlite3_free(c);
    return SQLITE_OK;
}

// Close a database or drop a virtual table (called last)
static int txtDisconnect(sqlite3_vtab *pVtab) {
    TxtTable *vtab = (TxtTable *)pVtab;
    sqlite3_free(vtab->filename);
    sqlite3_free(vtab);
    return SQLITE_OK;
}

// Registering a virtual table
static sqlite3_module TxtModule = {
    0,                 // iVersion
    txtConnect,        // xCreate
    txtConnect,        // xConnect
    txtBestIndex,      // xBestIndex
    txtDisconnect,     // xDisconnect
    txtDisconnect,     // xDestroy
    txtOpen,           // xOpen
    txtClose,          // xClose
    txtFilter,         // xFilter
    txtNext,           // xNext
    txtEof,            // xEof
    txtColumn,         // xColumn
    txtRowid,          // xRowid
    NULL,              // xUpdate          used to define operations INSERT/DELETE/UPDATE on a virtual table
    NULL,              // xBegin           used to define operations INSERT/DELETE/UPDATE on a virtual table 
    NULL,              // xSync            used to define operations INSERT/DELETE/UPDATE on a virtual table 
    NULL,              // xCommit          used to define operations INSERT/DELETE/UPDATE on a virtual table 
    NULL,              // xRollback        used to define operations INSERT/DELETE/UPDATE on a virtual table 
    NULL,              // xFindFunction
    NULL,              // xRename
    NULL,              // xSavepoint
    NULL,              // xRelease
    NULL,              // xRollbackTo
    NULL,              // xShadowName
};

// Export initialization function for Windows
#ifdef _WIN32
__declspec(dllexport)
#endif

// Initializing the extension
int sqlite3_loaderTXT_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi) {
    SQLITE_EXTENSION_INIT2(pApi) //Macro from sqlite3ext.h indicating the moment of initialization itself
    return sqlite3_create_module(db, "loaderTXT", &TxtModule, 0);
}