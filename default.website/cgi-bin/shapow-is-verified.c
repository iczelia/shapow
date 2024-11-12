#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <sqlite3.h>

void parse_query_string(char * query, int * id) {
  char *token = strtok(query, "&");
  while (token != NULL) {
    if (strncmp(token, "id=", 3) == 0) {
      *id = atoi(token + 3);
    }
    token = strtok(NULL, "&");
  }
}

void error(char * error_code, char * message) {
  printf("Status: %s\n", error_code);
  printf("Content-Type: text/plain\n\n");
  printf("%s\n", message);
  exit(0);
}

int main() {
  char * query_string = getenv("QUERY_STRING");
  if (query_string == NULL) error("400 Bad Request", "No query string provided.");
  int id = -1;
  parse_query_string(query_string, &id);
  if (id == -1) error("400 Bad Request", "No id provided.");
  sqlite3 * db;
  if (sqlite3_open("../../shapow.db", &db) != SQLITE_OK)
    error("500 Internal Server Error", "Could not open the database.");
  time_t timestamp = time(NULL);
  char * sql = "SELECT * FROM challenges WHERE id = ? AND timestamp + duration > ? AND solved = 0";
  sqlite3_stmt * stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    { sqlite3_finalize(stmt); goto db_error; }
  if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK)
    { sqlite3_finalize(stmt); goto db_error; }
  if (sqlite3_bind_int64(stmt, 2, timestamp) != SQLITE_OK)
    { sqlite3_finalize(stmt); goto db_error; }
  int res = sqlite3_step(stmt);
  printf("Content-Type: text/plain\n\n");
  printf("%d\n", res == SQLITE_ROW);
  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return 0;
db_error:
  sqlite3_close(db);
  error("500 Internal Server Error", "Database error.");
}
