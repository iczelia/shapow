#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <sqlite3.h>

// Function to encode bytes to base64
char * base64_encode(const unsigned char * input, int length) {
  EVP_ENCODE_CTX * ctx = EVP_ENCODE_CTX_new();
  int output_len = 4 * ((length + 2) / 3) + 1;
  unsigned char * output = malloc(output_len);
  EVP_EncodeInit(ctx);
  int len;
  EVP_EncodeUpdate(ctx, output, &len, input, length);
  int final_len;
  EVP_EncodeFinal(ctx, output + len, &final_len);
  output[len + final_len] = '\0';
  EVP_ENCODE_CTX_free(ctx);
  return output;
}

void parse_query_string(char * query, char ** nonce, int * duration, int * difficulty) {
  char *token = strtok(query, "&");
  while (token != NULL) {
    if (strncmp(token, "nonce=", 6) == 0) {
      *nonce = token + 6;
    } else if (strncmp(token, "difficulty=", 11) == 0) {
      *difficulty = atoi(token + 11);
    } else if (strncmp(token, "duration=", 9) == 0) {
      *duration = atoi(token + 9);
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
  time_t timestamp = time(NULL);
  srand(timestamp);
  unsigned char random_bytes[32];
  if (RAND_bytes(random_bytes, sizeof(random_bytes)) != 1)
    error("500 Internal Server Error", "Could not generate a challenge.");
  char * query_string = getenv("QUERY_STRING");
  if (query_string == NULL) error("400 Bad Request", "No query string provided.");
  char * nonce = NULL;
  int difficulty = 0, duration = 0;
  parse_query_string(query_string, &nonce, &duration, &difficulty);
  if (difficulty <= 0) error("400 Bad Request", "Invalid difficulty.");
  if (duration <= 0 || duration > 7200) error("400 Bad Request", "Invalid duration.");
  if (nonce == NULL) error("400 Bad Request", "No nonce provided.");
  if (strlen(nonce) > 64) error("400 Bad Request", "Nonce too long.");
  char * encoded_random_bytes = base64_encode(random_bytes, sizeof(random_bytes));
  encoded_random_bytes[strlen(encoded_random_bytes) - 1] = '\0';
  sqlite3 * db;
  if (sqlite3_open("../../shapow.db", &db) != SQLITE_OK)
    error("500 Internal Server Error", "Could not open the database.");
  sqlite3_stmt * statement;
  if (sqlite3_prepare_v2(db,
      "INSERT INTO challenges (nonce, challenge, timestamp, solved, duration, difficulty) VALUES (?, ?, ?, 0, ?, ?);",
      -1, &statement, NULL) != SQLITE_OK)
    { sqlite3_finalize(statement); goto db_error; }
  if (sqlite3_bind_text(statement, 1, nonce, strlen(nonce), SQLITE_STATIC) != SQLITE_OK) { sqlite3_finalize(statement); goto db_error; }
  if (sqlite3_bind_text(statement, 2, encoded_random_bytes, strlen(encoded_random_bytes), SQLITE_STATIC) != SQLITE_OK) { sqlite3_finalize(statement); goto db_error; }
  if (sqlite3_bind_int64(statement, 3, timestamp) != SQLITE_OK) { sqlite3_finalize(statement); goto db_error; }
  if (sqlite3_bind_int(statement, 4, duration) != SQLITE_OK) { sqlite3_finalize(statement); goto db_error; }
  if (sqlite3_bind_int(statement, 5, difficulty) != SQLITE_OK) { sqlite3_finalize(statement); goto db_error; }
  int result = sqlite3_step(statement);
  if (result != SQLITE_DONE) { sqlite3_finalize(statement); goto db_error; }
  printf("Content-Type: text/plain\n\n");
  printf("%d SHAPOW:%ld:%s:%d:%d:%s:\n", sqlite3_last_insert_rowid(db), timestamp, encoded_random_bytes, difficulty, duration, nonce);
  sqlite3_finalize(statement);
  sqlite3_close(db);
  return 0;
db_error:
  sqlite3_close(db);
  error("500 Internal Server Error", "Database error.");
}
