#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sqlite3.h>

void error(const char * message) {
  fprintf(stderr, "%s\n", message);
  exit(1);
}

int main() {
  sqlite3 * db;
  if (sqlite3_open("shapow.db", &db) != SQLITE_OK)
    error("Could not open the database.");
  if (sqlite3_exec(db,
      "CREATE TABLE challenges ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nonce TEXT,"
        "challenge BLOB,"
        "timestamp INTEGER,"
        "solved INTEGER,"
        "duration INTEGER,"
        "difficulty INTEGER,"
        "string TEXT GENERATED ALWAYS AS ('SHAPOW:' || timestamp || ':' || challenge || ':' || difficulty || ':' || duration || ':' || nonce || ':') VIRTUAL"
      ");", NULL, NULL, NULL) != SQLITE_OK)
    goto db_error;
  sqlite3_close(db);
  return 0;
db_error:
  sqlite3_close(db);
  error("Database error.");
}
