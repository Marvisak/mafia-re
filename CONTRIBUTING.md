# Contributing

Thank you for even considering contributing

If you have no experience with reverse engineering or don't know what tools to use, I kindly ask you to look at different resources first.

The goal of the project isn't instruction perfect decompilation, this was attempted but proved to be incredibly difficult.

But what should be kept is the ability to compile the whole binary using Visual Studio 6.0 (the compiler used to compile most of the binaries), so most of C++ features everybody is used to and comfortable with are off the table (like range-based for loops).

## Debug Symbols

The USA PS2 version was compiled with debug symbols enabled, but sadly only very basic debug symbols so only the function names and global variable names are known. This is still an incredible help so I very highly recommend using them. The game wasn't compiled with RTTI so this makes finding methods much easier.

Most of the function names are written in English and those are fairly descriptive, but a big chunk of the names are written in Czech language - while those are absolutely hilarious if you understand the language, they are also absolutely unrelated to what the function does, but you should still keep the original names where possible.

## Coding Style

Ideally the goal is to mirror the coding style of the original source code, which is a bit of a problem because the game doesn't respect any. You can find basically every single naming convention in here, but it seems like most of the game sticks to some weird mixture of snake case and hungarian notion, but you can find plenty of exceptions.

But there are some rules we can pin point that seem to be sometimes followed (or I came up with them), but most of the time you can just rely on the names in debug symbols.

```cpp
class C_very_important_class {
public:
    void ImportantMethod();
private:
    int important_field;
}

struct S_cool_struct {
    int cool_field;
}

void IncredibleFunction(int incredible_argument) {
    int incredible_variable = 100;
}
```