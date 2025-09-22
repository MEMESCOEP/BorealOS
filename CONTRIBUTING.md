# Contributing

Contributions are welcome! Please follow these steps to contribute:
1. Fork the repository.
2. Create a new branch for your feature or bug fix.
3. Make your changes and commit them with clear messages.
4. Push your changes to your forked repository.
5. Open a pull request to the main repository.
6. Ensure your code follows the project's coding standards.

These are the coding standards we follow:

### General Guidelines

#### Naming Conventions
- Use descriptive names that convey the purpose of the variable, function, or type.
- Avoid abbreviations unless they are widely understood.

#### Comments
- Use comments to explain the "why" behind complex logic.
- Do not comment obvious code.

### Casing Conventions
- Use `camelCase` for function scoped variables, function parameters.
- Use `PascalCase` for public types, fields, functions and variables.

### Structuring Code

#### Header Files (.h)
- Start with include statements.
- Follow with `#define` statements.
- Then type definitions (typedefs, structs, enums).
- Global variables (defined as `extern`).
- Function declarations (prototypes).

#### Source Files (.c)
- Start with include statements.
- Follow with `#define` statements.
- Then type definitions (typedefs, structs, enums).
- Local (static) variables.
- Function definitions.

Example:
```c
(example.h)
#ifndef EXAMPLE_H
#define EXAMPLE_H

#include <Definitions.h>

#define MAX_NAME_LENGTH 100

typedef struct Person {
    char* FirstName;
    char* LastName;
    int Age;
} Person;

extern Person GlobalPerson;

void DoStuff(Person *person);

#endif // EXAMPLE_H
```

```c
(example.c)
#include "example.h"

GlobalPerson = { "John", "Doe", 30 };

void PrivateFunction() {
    // Implementation
}

void DoStuff(Person *person) {
    int age = person->Age;
}
```

### Commit Messages
- Use the present tense ("Add feature" not "Added feature").
- Limit the first line to 50 characters.
- Use the body to explain what and why, not how.

Example:
```
Commit message: Short (50 chars or less) summary of changes

Commit message body:
Filename:
- Description of changes made
```