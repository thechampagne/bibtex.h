#ifndef __BIBTEX_H__
#define __BIBTEX_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum bibtex_error_type_t
{
  BIBTEX_OK                        = 0,
  BIBTEX_ERROR_UNTERMINATED_STRING = 1,
  BIBTEX_ERROR_UNEXPECTED_END      = 1 << 1,
  BIBTEX_ERROR_INVALID_TOKEN       = 1 << 2,

  BIBTEX_ERROR_EXPECT_ID           = 1 << 3, // TODO: rename it
  BIBTEX_ERROR_EXPECT_AT           = 1 << 4,
  BIBTEX_ERROR_EXPECT_LBRACE       = 1 << 5,
  BIBTEX_ERROR_EXPECT_RBRACE       = 1 << 6,
  BIBTEX_ERROR_EXPECT_EQ           = 1 << 7,
  BIBTEX_ERROR_EXPECT_COMMA        = 1 << 8,
  BIBTEX_ERROR_EXPECT_STRING       = 1 << 9,
  BIBTEX_ERROR_EXPECT_NUMBER       = 1 << 10,

  BIBTEX_ERROR_INVALID_ENTRY_TYPE  = 1 << 11,
  BIBTEX_ERROR_INVALID_FIELD_TYPE  = 1 << 12,
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
const char* bibtex_strerror(bibtex_error_type_t type);
const char* bibtex_entry_type_to_string(bibtex_entry_type_t type);
const char* bibtex_field_type_to_string(bibtex_field_type_t type);

#ifdef BIBTEX_IMPLEMENTATION

#define bibtex_if_token_error_break(tok, old_err, new_err) old_err = new_err;  if (tok == BIBTOKEN_TYPE_ERROR) goto clean_up;

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
  biblexer_update_line(lex);
  return lex->input[lex->pos++];
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
static enum bibtex_entry_type_t bibtex_entry_type_check(const char* value)
{
  if (bibtex_compare_values(value, "article")) return BIBTEX_ENTRY_TYPE_ARTICLE;
  if (bibtex_compare_values(value, "book")) return BIBTEX_ENTRY_TYPE_BOOK;
  if (bibtex_compare_values(value, "booklet")) return BIBTEX_ENTRY_TYPE_BOOKLET;
  if (bibtex_compare_values(value, "conference")) return BIBTEX_ENTRY_TYPE_CONFERENCE;
  if (bibtex_compare_values(value, "inbook")) return BIBTEX_ENTRY_TYPE_INBOOK;
  if (bibtex_compare_values(value, "incollection")) return BIBTEX_ENTRY_TYPE_INCOLLECTION;
  if (bibtex_compare_values(value, "inproceedings")) return BIBTEX_ENTRY_TYPE_INPROCEEDINGS;
  if (bibtex_compare_values(value, "manual")) return BIBTEX_ENTRY_TYPE_MANUAL;
  if (bibtex_compare_values(value, "mastersthesis")) return BIBTEX_ENTRY_TYPE_MASTERSTHESIS;
  if (bibtex_compare_values(value, "misc")) return BIBTEX_ENTRY_TYPE_MISC;
  if (bibtex_compare_values(value, "phdthesis")) return BIBTEX_ENTRY_TYPE_PHDTHESIS;
  if (bibtex_compare_values(value, "proceedings")) return BIBTEX_ENTRY_TYPE_PROCEEDINGS;
  if (bibtex_compare_values(value, "techreport")) return BIBTEX_ENTRY_TYPE_TECHREPORT;
  if (bibtex_compare_values(value, "unpublished")) return BIBTEX_ENTRY_TYPE_UNPUBLISHED;
  return -1;
}

static enum bibtex_field_type_t bibtex_field_type_check(const char* value)
{
  if (bibtex_compare_values(value, "address")) return BIBTEX_FIELD_TYPE_ADDRESS;
  if (bibtex_compare_values(value, "annote")) return BIBTEX_FIELD_TYPE_ANNOTE;
  if (bibtex_compare_values(value, "author")) return BIBTEX_FIELD_TYPE_AUTHOR;
  if (bibtex_compare_values(value, "booktitle")) return BIBTEX_FIELD_TYPE_BOOKTITLE;
  if (bibtex_compare_values(value, "chapter")) return BIBTEX_FIELD_TYPE_CHAPTER;
  if (bibtex_compare_values(value, "doi")) return BIBTEX_FIELD_TYPE_DOI;
  if (bibtex_compare_values(value, "edition")) return BIBTEX_FIELD_TYPE_EDITION;
  if (bibtex_compare_values(value, "editor")) return BIBTEX_FIELD_TYPE_EDITOR;
  if (bibtex_compare_values(value, "howpublished")) return BIBTEX_FIELD_TYPE_HOWPUBLISHED;
  if (bibtex_compare_values(value, "institution")) return BIBTEX_FIELD_TYPE_INSTITUTION;
  if (bibtex_compare_values(value, "issn")) return BIBTEX_FIELD_TYPE_ISSN;
  if (bibtex_compare_values(value, "isbn")) return BIBTEX_FIELD_TYPE_ISBN;
  if (bibtex_compare_values(value, "journal")) return BIBTEX_FIELD_TYPE_JOURNAL;
  if (bibtex_compare_values(value, "month")) return BIBTEX_FIELD_TYPE_MONTH;
  if (bibtex_compare_values(value, "note")) return BIBTEX_FIELD_TYPE_NOTE;
  if (bibtex_compare_values(value, "number")) return BIBTEX_FIELD_TYPE_NUMBER;
  if (bibtex_compare_values(value, "organization")) return BIBTEX_FIELD_TYPE_ORGANIZATION;
  if (bibtex_compare_values(value, "pages")) return BIBTEX_FIELD_TYPE_PAGES;
  if (bibtex_compare_values(value, "publisher")) return BIBTEX_FIELD_TYPE_PUBLISHER;
  if (bibtex_compare_values(value, "school")) return BIBTEX_FIELD_TYPE_SCHOOL;
  if (bibtex_compare_values(value, "type")) return BIBTEX_FIELD_TYPE_TYPE;
  if (bibtex_compare_values(value, "series")) return BIBTEX_FIELD_TYPE_SERIES;
  if (bibtex_compare_values(value, "title")) return BIBTEX_FIELD_TYPE_TITLE;
  if (bibtex_compare_values(value, "url")) return BIBTEX_FIELD_TYPE_URL;
  if (bibtex_compare_values(value, "volume")) return BIBTEX_FIELD_TYPE_VOLUME;
  if (bibtex_compare_values(value, "year")) return BIBTEX_FIELD_TYPE_YEAR;
  return -1;
}

static enum bibtex_error_type_t bibtoken_to_error(enum bibtoken_type_t token)
{
  switch (token)
    {
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
	  if (token.type == BIBTOKEN_TYPE_EOF)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_UNEXPECTED_END,token.row, token.col);
	      goto clean_up;
	    }
	  if (token.type != BIBTOKEN_TYPE_ID)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_ID,token.row, token.col);
	      goto clean_up;
	    }
	  break;
	case BIBTOKEN_TYPE_ID:
	  struct bibtoken_t curr_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (token.type == BIBTOKEN_TYPE_EOF)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_UNEXPECTED_END,token.row, token.col);
	      free(curr_token.value);
	      goto clean_up;
	    }
	  if (prev_token.type == BIBTOKEN_TYPE_AT)
	    {
	      enum bibtex_entry_type_t entry_type = bibtex_entry_type_check(curr_token.value);
	      if (entry_type == -1) {
		bibtex_error_init(&error, BIBTEX_ERROR_INVALID_ENTRY_TYPE, curr_token.row, curr_token.col);
		free(curr_token.value);
		goto clean_up;
	      }
	      if (token.type != BIBTOKEN_TYPE_LBRACE)
		{
		  bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_LBRACE,token.row, token.col);
		  free(curr_token.value);
		  goto clean_up;
		}
	      if (head_entry == NULL) {
		head_entry = bibtex_entry_init(entry_type, NULL);
		entry = head_entry;
	      } else {
		entry->next = bibtex_entry_init(entry_type, NULL);
		entry = entry->next;
	      }
	      head_field = NULL;
	      free(curr_token.value);
	    }
	  else if (prev_token.type == BIBTOKEN_TYPE_LBRACE)
	    {
	      entry->key = curr_token.value;
	    }
	  else if (prev_token.type == BIBTOKEN_TYPE_COMMA)
	    {
	      enum bibtex_field_type_t field_type = bibtex_field_type_check(curr_token.value);
	      if (field_type == -1) {
		bibtex_error_init(&error, BIBTEX_ERROR_INVALID_FIELD_TYPE, curr_token.row, curr_token.col);
		free(curr_token.value);
	        goto clean_up;
	      }
	      if (token.type != BIBTOKEN_TYPE_EQ)
		{
		  bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_EQ,token.row, token.col);
		  free(curr_token.value);
		  goto clean_up;
		}
	      if (head_field == NULL) {
		head_field = bibtex_field_init(field_type, NULL);
		field = head_field;
		entry->fields = field;
	      } else {
		field->next = bibtex_field_init(field_type, NULL);
		field = head_field->next;
	      }
	      free(curr_token.value);
	    }
	  else
	    {
       
	      bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_AT, curr_token.row, curr_token.col);
	      free(curr_token.value);
	      goto clean_up;
	    }
	  prev_token = curr_token;
	  break;
	case BIBTOKEN_TYPE_EQ:
	  prev_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (token.type == BIBTOKEN_TYPE_EOF)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_UNEXPECTED_END,token.row, token.col);
	      goto clean_up;
	    }
	  if (token.type != BIBTOKEN_TYPE_STRING && token.type != BIBTOKEN_TYPE_NUMBER)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_ID,token.row, token.col);
	      goto clean_up;
	    }
	  break;
	case BIBTOKEN_TYPE_LBRACE:
	  prev_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (token.type == BIBTOKEN_TYPE_EOF)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_UNEXPECTED_END,token.row, token.col);
	      goto clean_up;
	    }
	  if (token.type != BIBTOKEN_TYPE_ID)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_ID,token.row, token.col);
	      goto clean_up;
	    }
	  break;
	case BIBTOKEN_TYPE_RBRACE:
	  prev_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (token.type != BIBTOKEN_TYPE_EOF && token.type != BIBTOKEN_TYPE_AT)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_AT,token.row, token.col);
	      goto clean_up;
	    }
	  break;
	case BIBTOKEN_TYPE_COMMA:
	  prev_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (token.type == BIBTOKEN_TYPE_EOF)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_UNEXPECTED_END,token.row, token.col);
	      goto clean_up;
	    }
	  if (token.type != BIBTOKEN_TYPE_ID && token.type != BIBTOKEN_TYPE_RBRACE)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_ID | BIBTEX_ERROR_EXPECT_RBRACE,token.row, token.col);
	      goto clean_up;
	    }
	  break;
	case BIBTOKEN_TYPE_STRING:
	  prev_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (token.type == BIBTOKEN_TYPE_EOF)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_UNEXPECTED_END,token.row, token.col);
	      goto clean_up;
	    }
	  if (token.type != BIBTOKEN_TYPE_COMMA && token.type != BIBTOKEN_TYPE_RBRACE)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_COMMA | BIBTEX_ERROR_EXPECT_RBRACE, token.row, token.col);
	      goto clean_up;
	    }
	  field->value = prev_token.value;
	  break;
	case BIBTOKEN_TYPE_NUMBER:
	  prev_token = token;
	  token = biblexer_next_token(&lex);
	  bibtex_if_token_error_break(token.type, error, lex.error);
	  if (token.type == BIBTOKEN_TYPE_EOF)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_UNEXPECTED_END,token.row, token.col);
	      goto clean_up;
	    }
	  if (token.type != BIBTOKEN_TYPE_COMMA && token.type != BIBTOKEN_TYPE_RBRACE)
	    {
	      bibtex_error_init(&error, BIBTEX_ERROR_EXPECT_COMMA | BIBTEX_ERROR_EXPECT_RBRACE, token.row, token.col);
	      goto clean_up;
	    }
	  field->value = prev_token.value;
	  break;
	case BIBTOKEN_TYPE_INVALID:
	  bibtex_error_init(&error, BIBTEX_ERROR_INVALID_TOKEN,token.row, token.col);
	  goto clean_up;
	case BIBTOKEN_TYPE_EOF: break;
	case BIBTOKEN_TYPE_ERROR: break;
	}
    }
  *root = head_entry;
  return error;
 clean_up:
  bibtex_entry_free(entry);
  *root = NULL;
  return error;
}

void bibtex_field_free(struct bibtex_field_t* field)
{
  while(field != NULL)
    {
      struct bibtex_field_t* head = field;
      free(head->value);
      free(head);
      field = head->next;
    }
  free(field);
}

void bibtex_entry_free(struct bibtex_entry_t* entry)
{
  while(entry != NULL)
    {
      struct bibtex_entry_t* head = entry;
      free(head->key);
      bibtex_field_free(head->fields);
      free(head);
      entry = head->next;
    }
  free(entry);
}

const char* bibtex_strerror(enum bibtex_error_type_t type)
{
  if(type & (BIBTEX_ERROR_EXPECT_COMMA | BIBTEX_ERROR_EXPECT_RBRACE))
    return "Expect , or }";
  else if(type & (BIBTEX_ERROR_EXPECT_ID | BIBTEX_ERROR_EXPECT_RBRACE))
    return "Expect id or }";
  switch(type)
    {
    case BIBTEX_OK:
      return "No error";
    case BIBTEX_ERROR_UNTERMINATED_STRING:
      return "Unterminated string";
    case BIBTEX_ERROR_UNEXPECTED_END:
      return "Unexpected end";
    case BIBTEX_ERROR_INVALID_TOKEN:
      return "Invalid token";
    case BIBTEX_ERROR_EXPECT_ID:
      return "Expect id";
    case BIBTEX_ERROR_EXPECT_AT:
      return "Expect @";
    case BIBTEX_ERROR_EXPECT_LBRACE:
      return "Expect {";
    case BIBTEX_ERROR_EXPECT_RBRACE:
      return "Expect }";
    case BIBTEX_ERROR_EXPECT_EQ:
      return "Expect =";
    case BIBTEX_ERROR_EXPECT_COMMA:
      return "Expect ,";
    case BIBTEX_ERROR_EXPECT_STRING:
      return "Expect string";
    case BIBTEX_ERROR_EXPECT_NUMBER:
      return "Expect number";
    case BIBTEX_ERROR_INVALID_ENTRY_TYPE:
      return "Invalid entry type";
    case BIBTEX_ERROR_INVALID_FIELD_TYPE:
      return "Invalid field type";
    default:
      return "Unknown error";
    }
}

const char* bibtex_entry_type_to_string(enum bibtex_entry_type_t type)
{
  switch(type)
    {
    case BIBTEX_ENTRY_TYPE_ARTICLE:
      return "article";
    case BIBTEX_ENTRY_TYPE_BOOK:
      return "book";
    case BIBTEX_ENTRY_TYPE_BOOKLET:
      return "booklet";
    case BIBTEX_ENTRY_TYPE_CONFERENCE:
      return "conference";
    case BIBTEX_ENTRY_TYPE_INBOOK:
      return "inbook";
    case BIBTEX_ENTRY_TYPE_INCOLLECTION:
      return "incollection";
    case BIBTEX_ENTRY_TYPE_INPROCEEDINGS:
      return "inproceedings";
    case BIBTEX_ENTRY_TYPE_MANUAL:
      return "manual";
    case BIBTEX_ENTRY_TYPE_MASTERSTHESIS:
      return "mastersthesis";
    case BIBTEX_ENTRY_TYPE_MISC:
      return "misc";
    case BIBTEX_ENTRY_TYPE_PHDTHESIS:
      return "phdthesis";
    case BIBTEX_ENTRY_TYPE_PROCEEDINGS:
      return "proceedings";
    case BIBTEX_ENTRY_TYPE_TECHREPORT:
      return "techreport";
    case BIBTEX_ENTRY_TYPE_UNPUBLISHED:
      return "unpublished";
    default:
      return "Unknown entry";
    }
}

const char* bibtex_field_type_to_string(enum bibtex_field_type_t type)
{
  switch(type)
    {
    case BIBTEX_FIELD_TYPE_ADDRESS:
      return "address";
    case BIBTEX_FIELD_TYPE_ANNOTE:
      return "annote";
    case BIBTEX_FIELD_TYPE_AUTHOR:
      return "author";
    case BIBTEX_FIELD_TYPE_BOOKTITLE:
      return "booktitle";
    case BIBTEX_FIELD_TYPE_CHAPTER:
      return "chapter";
    case BIBTEX_FIELD_TYPE_DOI:
      return "doi";
    case BIBTEX_FIELD_TYPE_EDITION:
      return "edition";
    case BIBTEX_FIELD_TYPE_EDITOR:
      return "editor";
    case BIBTEX_FIELD_TYPE_HOWPUBLISHED:
      return "howpublished";
    case BIBTEX_FIELD_TYPE_INSTITUTION:
      return "institution";
    case BIBTEX_FIELD_TYPE_ISSN:
      return "issn";
    case BIBTEX_FIELD_TYPE_ISBN:
      return "isbn";
    case BIBTEX_FIELD_TYPE_JOURNAL:
      return "journal";
    case BIBTEX_FIELD_TYPE_MONTH:
      return "month";
    case BIBTEX_FIELD_TYPE_NOTE:
      return "note";
    case BIBTEX_FIELD_TYPE_NUMBER:
      return "number";
    case BIBTEX_FIELD_TYPE_ORGANIZATION:
      return "organization";
    case BIBTEX_FIELD_TYPE_PAGES:
      return "pages";
    case BIBTEX_FIELD_TYPE_PUBLISHER:
      return "publisher";
    case BIBTEX_FIELD_TYPE_SCHOOL:
      return "school";
    case BIBTEX_FIELD_TYPE_TYPE:
      return "type";
    case BIBTEX_FIELD_TYPE_SERIES:
      return "series";
    case BIBTEX_FIELD_TYPE_TITLE:
      return "title";
    case BIBTEX_FIELD_TYPE_URL:
      return "url";
    case BIBTEX_FIELD_TYPE_VOLUME:
      return "volume";
    case BIBTEX_FIELD_TYPE_YEAR:
      return "year";
    default:
      return "Unknown field";
    }
}

#endif // BIBTEX_IMPLEMENTATION

#endif // __BIBTEX_H__
