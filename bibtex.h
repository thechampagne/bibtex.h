#ifndef __BIBTEX_H__
#define __BIBTEX_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum bibtex_error_type_t
{
  BIBTEX_OK,
  BIBTEX_ERROR_UNTERMINATED_STRING,
  BIBTEX_ERROR_INVALID_TOKEN,

  BIBTEX_ERROR_EXPECT_EOF,
  BIBTEX_ERROR_EXPECT_ID,
  BIBTEX_ERROR_EXPECT_AT,
  BIBTEX_ERROR_EXPECT_LBRACE,
  BIBTEX_ERROR_EXPECT_RBRACE,
  BIBTEX_ERROR_EXPECT_EQ,
  BIBTEX_ERROR_EXPECT_COMMA,
  BIBTEX_ERROR_EXPECT_STRING,
  BIBTEX_ERROR_EXPECT_NUMBER,

  BIBTEX_ERROR_INVALID_ENTRY_TYPE,
} bibtex_error_type_t;

typedef enum bibtex_entry_type_t
{
  BIBTEX_ENTRY_TYPE_ARTICLE,
  BIBTEX_ENTRY_TYPE_BOOK,
  BIBTEX_ENTRY_TYPE_BOOKLET,
  BIBTEX_ENTRY_TYPE_CONFERENCE,
  BIBTEX_ENTRY_TYPE_INBOOK,
  BIBTEX_ENTRY_TYPE_INCOLLECTION,
  BIBTEX_ENTRY_TYPE_INPROCEEDINGS,
  BIBTEX_ENTRY_TYPE_MANUAL,
  BIBTEX_ENTRY_TYPE_MASTERSTHESIS,
  BIBTEX_ENTRY_TYPE_MISC,
  BIBTEX_ENTRY_TYPE_PHDTHESIS,
  BIBTEX_ENTRY_TYPE_PROCEEDINGS,
  BIBTEX_ENTRY_TYPE_TECHREPORT,
  BIBTEX_ENTRY_TYPE_UNPUBLISHED,
} bibtex_entry_type_t;

typedef enum bibtex_field_type_t
{
  BIBTEX_FIELD_TYPE_ADDRESS,
  BIBTEX_FIELD_TYPE_ANNOTE,
  BIBTEX_FIELD_TYPE_AUTHOR,
  BIBTEX_FIELD_TYPE_BOOKTITLE,
  BIBTEX_FIELD_TYPE_CHAPTER,
  BIBTEX_FIELD_TYPE_DOI,
  BIBTEX_FIELD_TYPE_EDITION,
  BIBTEX_FIELD_TYPE_EDITOR,
  BIBTEX_FIELD_TYPE_HOWPUBLISHED,
  BIBTEX_FIELD_TYPE_INSTITUTION,
  BIBTEX_FIELD_TYPE_ISSN,
  BIBTEX_FIELD_TYPE_ISBN,
  BIBTEX_FIELD_TYPE_JOURNAL,
  BIBTEX_FIELD_TYPE_MONTH,
  BIBTEX_FIELD_TYPE_NOTE,
  BIBTEX_FIELD_TYPE_NUMBER,
  BIBTEX_FIELD_TYPE_ORGANIZATION,
  BIBTEX_FIELD_TYPE_PAGES,
  BIBTEX_FIELD_TYPE_PUBLISHER,
  BIBTEX_FIELD_TYPE_SCHOOL,
  BIBTEX_FIELD_TYPE_TYPE,
  BIBTEX_FIELD_TYPE_SERIES,
  BIBTEX_FIELD_TYPE_TITLE,
  BIBTEX_FIELD_TYPE_URL,
  BIBTEX_FIELD_TYPE_VOLUME,
  BIBTEX_FIELD_TYPE_YEAR,
} bibtex_field_type_t;

typedef struct bibtex_error_t
{
  bibtex_error_type_t type;
  int row;
  int col;
} bibtex_error_t;

typedef struct bibtex_field_t
{
  bibtex_field_type_t type;
  char* value;
  struct bibtex_field_t* next;
} bibtex_field_t;

typedef struct bibtex_entry_t
{
  bibtex_entry_type_t type;
  char* key;
  bibtex_field_t* fields;
  struct bibtex_entry_t* next;
} bibtex_entry_t;

bibtex_error_t bibtex_parse(bibtex_entry_t** root, const char* input);
void bibtex_field_free(bibtex_field_t* field);
void bibtex_entry_free(bibtex_entry_t* entry);

#ifdef BIBTEX_IMPLEMENTATION

#define bibtex_if_token_error_break(tok, old_err, new_err) old_err = new_err;  if (tok == BIBTOKEN_TYPE_ERROR) break;

static void bibtex_error_init(struct bibtex_error_t* error, enum bibtex_error_type_t type, int row, int col)
{
  error->type = type;
  error->row = row;
  error->col = col;
}

struct biblexer_t
{
  const char* input;
  struct bibtex_error_t error;
  int pos;
  int row;
  int col;
};

enum bibtoken_type_t
{
  BIBTOKEN_TYPE_EOF,
  BIBTOKEN_TYPE_ID,
  BIBTOKEN_TYPE_AT,
  BIBTOKEN_TYPE_LBRACE,
  BIBTOKEN_TYPE_RBRACE,
  BIBTOKEN_TYPE_EQ,
  BIBTOKEN_TYPE_COMMA,
  BIBTOKEN_TYPE_STRING,
  BIBTOKEN_TYPE_NUMBER,
  BIBTOKEN_TYPE_INVALID,
  BIBTOKEN_TYPE_ERROR,
};

struct bibtoken_t
{
  char* value;
  enum bibtoken_type_t type;
  int row;
  int col;
};

static struct bibtex_entry_t* bibtex_entry_init(enum bibtex_entry_type_t type, char* key)
{
  struct bibtex_entry_t* entry = malloc(sizeof(struct bibtex_entry_t));
  entry->type = type;
  entry->key = key;
  entry->fields = NULL;
  entry->next = NULL;
  return entry;
}

static struct bibtex_field_t* bibtex_field_init(enum bibtex_field_type_t type, char* value)
{
  struct bibtex_field_t* field = malloc(sizeof(struct bibtex_field_t));
  field->type = type;
  field->value = value;
  field->next = NULL;
  return field;
}

static struct bibtoken_t bibtoken_init_value(enum bibtoken_type_t type, const char* start, size_t len, int row, int col)
{
  struct bibtoken_t token;
  token.type = type;
  token.row = row;
  token.col = col;
  token.value = malloc(len + 1);
  memcpy(token.value, start, len);
  token.value[len] = '\0';
  return token;
}

static struct bibtoken_t bibtoken_init(enum bibtoken_type_t type, int row, int col)
{
  struct bibtoken_t token;
  token.type = type;
  token.row = row;
  token.col = col;
  token.value = NULL;
  return token;
}

static struct biblexer_t biblexer_init(const char* input)
{
  struct biblexer_t lex;
  lex.input = input;
  lex.pos = 0;
  lex.row = 1;
  lex.col = 1;
  lex.error.type = BIBTEX_OK;
  lex.error.row = 0;
  lex.error.col = 0;
  return lex;
}

static char biblexer_peek(struct biblexer_t* lex)
{
  return lex->input[lex->pos];
}

static void biblexer_update_line(struct biblexer_t* lex)
{
  if (biblexer_peek(lex) == '\n') {
    lex->row++;
    lex->col = 1;
  } else lex->col++;
}

static char biblexer_advance(struct biblexer_t* lex)
{
  char c = lex->input[lex->pos++];
  biblexer_update_line(lex);
  return c;
}

static void biblexer_skip_whitespace(struct biblexer_t* lex)
{
  while(isspace(biblexer_peek(lex))) biblexer_advance(lex);
}

static struct bibtoken_t biblexer_lex_id(struct biblexer_t* lex)
{
  size_t start = lex->pos;
  int row = lex->row;
  int col = lex->col;
  while(isalnum(biblexer_peek(lex))) biblexer_advance(lex);
  return bibtoken_init_value(BIBTOKEN_TYPE_ID, lex->input + start, lex->pos - start, row, col);
}

// TODO: handle inner strings
static struct bibtoken_t biblexer_lex_string(struct biblexer_t* lex)
{
  int row = lex->row;
  int col = lex->col;
  biblexer_advance(lex);
  size_t start = lex->pos;
  while(biblexer_peek(lex) != '\0' && biblexer_peek(lex) != '"') biblexer_advance(lex);
  if (biblexer_peek(lex) == '\0' && biblexer_peek(lex) != '"')
    {
      bibtex_error_init(&lex->error, BIBTEX_ERROR_UNTERMINATED_STRING, row, col);
      return bibtoken_init(BIBTOKEN_TYPE_ERROR, row, col);
    }
  size_t len = lex->pos - start;
  return bibtoken_init_value(BIBTOKEN_TYPE_STRING, lex->input + start, len, row, col);
}

static struct bibtoken_t biblexer_lex_number(struct biblexer_t* lex)
{
  size_t start = lex->pos;
  int row = lex->row;
  int col = lex->col;
  while(isdigit(biblexer_peek(lex))) biblexer_advance(lex);
  return bibtoken_init_value(BIBTOKEN_TYPE_NUMBER, lex->input + start, lex->pos - start, row, col);
}

static struct bibtoken_t biblexer_next_token(struct biblexer_t* lex)
{
  biblexer_skip_whitespace(lex);
  char c = biblexer_peek(lex);
  if (c == '\0') return bibtoken_init(BIBTOKEN_TYPE_EOF, lex->row, lex->col);

  struct bibtoken_t token;
  
  
  if (isalpha(c)) {
    token = biblexer_lex_id(lex);
    return token;
  }
  if (isdigit(c)) {
    token = biblexer_lex_number(lex);
    return token;
  }

  
  switch(c)
    {
    case '@': token = bibtoken_init(BIBTOKEN_TYPE_AT, lex->row, lex->col);
      biblexer_advance(lex);
      return token;
    case '{': token = bibtoken_init(BIBTOKEN_TYPE_LBRACE, lex->row, lex->col);
      biblexer_advance(lex);
      return token;
    case '}': token = bibtoken_init(BIBTOKEN_TYPE_RBRACE, lex->row, lex->col);
      biblexer_advance(lex);
      return token;
    case '=': token = bibtoken_init(BIBTOKEN_TYPE_EQ, lex->row, lex->col);
      biblexer_advance(lex);
      return token;
    case ',': token = bibtoken_init(BIBTOKEN_TYPE_COMMA, lex->row, lex->col);
      biblexer_advance(lex);
      return token;
    case '"': token = biblexer_lex_string(lex);
      biblexer_advance(lex);
      return token;
    }
  token = bibtoken_init(BIBTOKEN_TYPE_INVALID, lex->row, lex->col);
  biblexer_advance(lex);
  return token;
}


static int bibtex_compare_values(const char* v0, const char* v1)
{
  int v0_len = strlen(v0);
  int v1_len = strlen(v1);
  if (v0_len != v1_len) return 0;
  int same = 1;
  while(*v0)
    {
      if (tolower(*v0) != tolower(*v1)) {
	same = 0;
	break;
      }
      v0++;
      v1++;
    }
  return same;
}

// TODO: use hash for checking
static int bibtex_entry_type_check(const char* value)
{
  if (bibtex_compare_values(value, "article")) return 1;
  if (bibtex_compare_values(value, "book")) return 1;
  if (bibtex_compare_values(value, "booklet")) return 1;
  if (bibtex_compare_values(value, "conference")) return 1;
  if (bibtex_compare_values(value, "inbook")) return 1;
  if (bibtex_compare_values(value, "incollection")) return 1;
  if (bibtex_compare_values(value, "inproceedings")) return 1;
  if (bibtex_compare_values(value, "manual")) return 1;
  if (bibtex_compare_values(value, "mastersthesis")) return 1;
  if (bibtex_compare_values(value, "misc")) return 1;
  if (bibtex_compare_values(value, "phdthesis")) return 1;
  if (bibtex_compare_values(value, "proceedings")) return 1;
  if (bibtex_compare_values(value, "techreport")) return 1;
  if (bibtex_compare_values(value, "unpublished")) return 1;
  return 0;
}


static enum bibtex_error_type_t bibtoken_to_error(enum bibtoken_type_t token)
{
  switch (token)
    {
    case BIBTOKEN_TYPE_EOF: return BIBTEX_ERROR_EXPECT_EOF;
    case BIBTOKEN_TYPE_ID: return BIBTEX_ERROR_EXPECT_ID;
    case BIBTOKEN_TYPE_AT: return BIBTEX_ERROR_EXPECT_AT;
    case BIBTOKEN_TYPE_LBRACE: return BIBTEX_ERROR_EXPECT_LBRACE;
    case BIBTOKEN_TYPE_RBRACE: return BIBTEX_ERROR_EXPECT_RBRACE;
    case BIBTOKEN_TYPE_EQ: return BIBTEX_ERROR_EXPECT_EQ;
    case BIBTOKEN_TYPE_COMMA: return BIBTEX_ERROR_EXPECT_COMMA;
    case BIBTOKEN_TYPE_STRING: return BIBTEX_ERROR_EXPECT_STRING;
    case BIBTOKEN_TYPE_NUMBER: return BIBTEX_ERROR_EXPECT_NUMBER;
    case BIBTOKEN_TYPE_INVALID: return BIBTEX_ERROR_INVALID_TOKEN;
    case BIBTOKEN_TYPE_ERROR: return BIBTEX_ERROR_INVALID_TOKEN; // NOTE: must not reache it!
    default:
      return 0;
    }
}

// TODO: support { } values
struct bibtex_error_t bibtex_parse(struct bibtex_entry_t** root, const char* input)
{
  struct biblexer_t lex = biblexer_init(input);
  struct bibtex_error_t error = lex.error;
  struct bibtex_entry_t* head_entry = NULL;
  struct bibtex_entry_t* entry = NULL;
  struct bibtex_field_t* head_field = NULL;
  struct bibtex_field_t* field = NULL;
  struct bibtoken_t token = biblexer_next_token(&lex);
  struct bibtoken_t prev_token = token;
  while(token.type != BIBTOKEN_TYPE_EOF && token.type != BIBTOKEN_TYPE_ERROR && error.type == BIBTEX_OK)
    {
      
      switch(token.type)
	{
	case BIBTOKEN_TYPE_AT:
	  prev_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (token.type != BIBTOKEN_TYPE_ID)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_ID,token.row, token.col);
	      break;
	    }
	  break;
	case BIBTOKEN_TYPE_ID:
	  struct bibtoken_t curr_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (prev_token.type == BIBTOKEN_TYPE_AT)
	    {
	      if (token.type != BIBTOKEN_TYPE_LBRACE)
		{
		  bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_LBRACE,token.row, token.col);
		  break;
		}
	      if (!bibtex_entry_type_check(curr_token.value)) {
		bibtex_error_init(&error, BIBTEX_ERROR_INVALID_ENTRY_TYPE, curr_token.row, curr_token.col);
		break;
	      }
	      if (head_entry == NULL) {
		head_entry = bibtex_entry_init(BIBTEX_ENTRY_TYPE_ARTICLE, NULL);
		entry = head_entry;
	      } else {
		entry->next = bibtex_entry_init(BIBTEX_ENTRY_TYPE_ARTICLE, NULL);
		entry = entry->next;
	      }
	      head_field = NULL;
	    }
	  else if (prev_token.type == BIBTOKEN_TYPE_LBRACE)
	    {
	      entry->key = curr_token.value;
	    }
	  else if (prev_token.type == BIBTOKEN_TYPE_COMMA)
	    {
	      if (token.type != BIBTOKEN_TYPE_EQ)
		{
		  bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_EQ,token.row, token.col);
		  break;
		}
	      if (head_field == NULL) {
		head_field = bibtex_field_init(BIBTEX_FIELD_TYPE_ANNOTE, NULL);
		field = head_field;
		entry->fields = field;
	      } else {
		field->next = bibtex_field_init(BIBTEX_FIELD_TYPE_ANNOTE, NULL);
		field = head_field->next;
	      }
	    }
	  prev_token = curr_token;
	  break;
	case BIBTOKEN_TYPE_EQ:
	  prev_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (token.type != BIBTOKEN_TYPE_STRING && token.type != BIBTOKEN_TYPE_NUMBER)
	    {
	     
	      bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_ID,token.row, token.col);
	      break;
	    }
	  break;
	case BIBTOKEN_TYPE_LBRACE:
	  prev_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (token.type != BIBTOKEN_TYPE_ID)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_ID,token.row, token.col);
	      break;
	    }
	  break;
	case BIBTOKEN_TYPE_RBRACE:
	  prev_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (token.type != BIBTOKEN_TYPE_EOF && token.type != BIBTOKEN_TYPE_AT)
	    {
	      
	      bibtex_error_init(&error, bibtoken_to_error(token.type),token.row, token.col);
	      break;
	    }
	  break;
	case BIBTOKEN_TYPE_COMMA:
	  prev_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (token.type != BIBTOKEN_TYPE_ID && token.type != BIBTOKEN_TYPE_RBRACE)
	    {
	      
	      bibtex_error_init(&error, bibtoken_to_error(token.type),token.row, token.col);
	      break;
	    }
	  break;
	case BIBTOKEN_TYPE_STRING:
	  prev_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (token.type != BIBTOKEN_TYPE_COMMA && token.type != BIBTOKEN_TYPE_RBRACE)
	    {
	      
	      bibtex_error_init(&error, bibtoken_to_error(token.type),token.row, token.col);
	      break;
	    }
	  field->value = prev_token.value;
	  break;
	case BIBTOKEN_TYPE_NUMBER:
	  prev_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (token.type != BIBTOKEN_TYPE_COMMA && token.type != BIBTOKEN_TYPE_RBRACE)
	    {
	      bibtex_error_init(&error, bibtoken_to_error(token.type),token.row, token.col);
	      break;
	    }
	  field->value = prev_token.value;
	  break;;
	case BIBTOKEN_TYPE_INVALID:
	  bibtex_error_init(&error, BIBTEX_ERROR_INVALID_TOKEN,token.row, token.col);
	  break;
	case BIBTOKEN_TYPE_EOF: break;
	case BIBTOKEN_TYPE_ERROR: break;
	}
    }
  *root = head_entry;
  return error;
}

void bibtex_field_free(bibtex_field_t* field)
{
  while(field != NULL)
    {
      bibtex_field_t* head = field;
      free(head->value);
      free(head);
      field = head->next;
    }
  free(field);
}

void bibtex_entry_free(bibtex_entry_t* entry)
{
  while(entry != NULL)
    {
      bibtex_entry_t* head = entry;
      free(head->key);
      bibtex_field_free(head->fields);
      free(head);
      entry = head->next;
    }
  free(entry);
}


#endif // BIBTEX_IMPLEMENTATION

#endif // __BIBTEX_H__
