#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <sqlite3.h>

void parse_query_string(char * query, int * id, char ** padding) {
  char *token = strtok(query, "&");
  while (token != NULL) {
    if (strncmp(token, "id=", 3) == 0) {
      *id = atoi(token + 3);
    }
    if (strncmp(token, "padding=", 8) == 0) {
      *padding = token + 8;
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
  int id = -1; char * padding = NULL;
  parse_query_string(query_string, &id, &padding);
  if (id == -1) error("400 Bad Request", "No id provided.");
  if (padding == NULL) error("400 Bad Request", "No padding provided.");
  if (strlen(padding) > 128) error("400 Bad Request", "Invalid padding.");
  sqlite3 * db;
  if (sqlite3_open("../../shapow.db", &db) != SQLITE_OK)
    error("500 Internal Server Error", "Could not open the database.");
  time_t timestamp = time(NULL);
  char * sql = "SELECT string, difficulty FROM challenges WHERE id = ? AND timestamp + duration > ? AND solved = 0";
  sqlite3_stmt * stmt;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) goto db_error;
  if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK) goto db_error;
  if (sqlite3_bind_int64(stmt, 2, timestamp) != SQLITE_OK) goto db_error;
  int res = sqlite3_step(stmt);
  if (res != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    error("403 Forbidden", "Challenge not found.");
  }
  const char * challenge = (const char *)sqlite3_column_text(stmt, 0);
  int difficulty = sqlite3_column_int(stmt, 1);
  EVP_MD_CTX * ctx = EVP_MD_CTX_new();
  EVP_DigestInit(ctx, EVP_sha256());
  EVP_DigestUpdate(ctx, challenge, strlen(challenge));
  EVP_DigestUpdate(ctx, padding, strlen(padding));
  unsigned char output[32];
  unsigned int length;
  EVP_DigestFinal(ctx, output, &length);
  EVP_MD_CTX_free(ctx);
  for (int i = 0; i < difficulty; i++) {
    if (output[i] != 0xFF) {
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      error("403 Forbidden", "Challenge not solved.");
    }
  }
  sqlite3_finalize(stmt);
  sqlite3_close(db);
  printf("Status: 200 OK\n");
  printf("Content-Type: text/plain\n\n");
  printf("Challenge solved!\n");
  return 0;
db_error:
  sqlite3_close(db);
  error("500 Internal Server Error", "Database error.");
}
