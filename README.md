# bibtex.h

A single-file header-only library to parse [BibTeX](https://en.wikipedia.org/wiki/BibTeX).

## How to use

`example.c`:
```c
#define BIBTEX_IMPLEMENTATION
#include "bibtex.h"

int main(void)
{
  const char* input =
    "@book{knuth1986,author=\"Donald E. Knuth\",title=\"The TeX Book\",year=1986,publisher=\"Addison-Wesley\"}"
    "@article{example1990,author=\"Person\",title=\"Introduction to Example\",journal=\"Example Forum\",year=1990,}";
  bibtex_entry_t* entry;
  bibtex_error_t error;
  error = bibtex_parse(&entry, input);
  if (error.type != BIBTEX_OK) {
    fprintf(stderr,"Error at %d:%d: %s\n", error.row, error.col, bibtex_strerror(error.type));
    return 1;
  }
  bibtex_entry_t* e = entry;
  while(e != NULL) {
    printf("@%s{%s,\n", bibtex_entry_type_to_string(e->type), e->key);
    bibtex_field_t* f = e->fields;
    while(f != NULL) {
      printf("  %s = %s,\n", bibtex_field_type_to_string(f->type), f->value);
      f = f->next;
    }
    printf("}\n");
    e = e->next;
  }
  bibtex_entry_free(entry);
  return 0;
}
```

```sh
$ cc example.c -o example
$ ./example
@book{knuth1986,
  author = Donald E. Knuth,
  title = The TeX Book,
  year = 1986,
  publisher = Addison-Wesley,
}
@article{example1990,
  author = Person,
  title = Introduction to Example,
  journal = Example Forum,
  year = 1990,
}
```

## References

- [BibTeX format](https://www.bibtex.com/g/bibtex-format/)
