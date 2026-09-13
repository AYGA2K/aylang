# aylang

Tree-walking interpreter for a dynamically typed language,
written in C++23. Give it a `.ay` file and it runs it. Give it nothing and
you get a REPL: you type a line, it is lexed, parsed and evaluated, and the
value of the last statement is printed back.

Everything lives in one process with no dependencies beyond GNU readline
(for line editing and history) and GoogleTest (for the unit tests).

## Building and running

    make                        # configure with cmake (if needed) and compile
    make run                    # build, then start the REPL
    make run FILE=example.ay    # build, then run a source file
    make test                   # build, then run the unit tests with ctest
    make clean                  # remove build/
    make rebuild                # clean, then build

Release build:

    make CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release"

Type `exit` or press Ctrl-D to leave the REPL.

## Running a file

    build/aylang example.ay        # or: make run FILE=example.ay

Source files use the `.ay` extension. The whole file is lexed, parsed and
evaluated in one go, and unlike the REPL nothing is echoed -- only `print`
writes to stdout. So this file:

    let x = 2 + 3 * 4;
    print("x is", x);

    let fact = fn(n) {
      if (n < 2) {
        return 1;
      }
      return n * fact(n - 1);
    };
    print("5! =", fact(5));

prints:

    x is 14
    5! = 120

Statements can span as many lines as they like; the lexer treats newlines
as plain whitespace. `example.ay` in the repo is a longer tour of the
language.

Parse errors are all reported before anything is evaluated, and a runtime
error stops the run where it happens, the same way it ends a REPL line.
Both go to stderr and exit with status 1, so a failing script is easy to
spot from a shell:

    $ build/aylang broken.ay
    identifier not found: foo
    $ echo $?
    1

## A REPL session

    >> let x = 2 + 3 * 4;
    >> x
    14
    >> let greet = fn(name) { return "hello " + name; };
    >> greet("ayoub")
    hello ayoub
    >> let nums = [1, 2, 3];
    >> push(nums, 4);
    4
    >> nums
    [1,2,3,4]
    >> let ages = {"a": 1, "b": 2};
    >> ages["b"]
    2
    >> let adder = fn(x) { return fn(y) { return x + y; }; };
    >> let add2 = adder(2);
    >> add2(40)
    42
    >> if (x > 10) { print("big") } else { print("small") }
    big
    >> let i = 0;
    >> while (i < 3) { i = i + 1; }
    3
    >> i
    3
    >> foo
    identifier not found: foo

## The language

Values: numbers (doubles), strings, booleans, null, arrays, hash maps,
functions and errors.

Statements:

    let x;                  // declares x as null
    let x = expression;
    return expression;
    if (cond) { ... } else if (cond) { ... } else { ... }
    while (cond) { ... }
    expression;

Expressions:

    numbers      1, 42, 3.14
    strings      "hello"
    booleans     true, false
    null         null
    arrays       [1, "two", [3]]
    hash maps    {"key": value, 1: "one"}
    indexing     arr[0], map["key"], rows[0][1]
    functions    fn(a, b) { return a + b; }
    calls        f(1, 2)
    unary        -x, !flag
    binary       + - * / %   ==  !=  <  <=  >  >=   && ||
    assignment   x = 1, x = y = 1, arr[0] = 1, map["k"] = 1
    grouping     (1 + 2) * 3

Operators bind in this order, loosest first:

    =   ||   &&   == !=   < <= > >=   + -   * / %   -x !x   f(x)   a[i]

Comments run from `//` to the end of the line.

Builtins:

    print(a, b, ...)      prints its arguments separated by spaces
    len(x)                length of a string, array or hash map
    push(arr, value)      appends to an array in place
    set(map, key, value)  binds a key in a hash map, in place
    has(map, key)         true if the key is in the hash map

Semantics worth knowing:

- Functions are closures. A function captures the environment it was
  defined in, so `adder` above works, and recursion works because the
  function's own name is visible in that environment.
- `+` also concatenates two strings and two arrays. Array
  concatenation builds a fresh array; neither operand is mutated.
- Errors are ordinary values. They are produced on bad operands,
  unknown identifiers, wrong argument counts, out-of-range indexing
  and missing hash map keys, and they propagate out of the enclosing
  statement instead of throwing.
- A hash map key that is not there is an error (`key not found: b`),
  not null, so a lookup that might miss is guarded with `has`:

      if (has(ages, "sam")) {
        print(ages["sam"]);
      }
- A key can be a number, a string, a bool or null, but not an array, a
  hash map or a function: those never compare equal to anything, so
  such a key could be stored and never found again. Literals, indexing,
  `set` and `has` all enforce the same rule, and a rejected key reads
  `hash map key must be a number, string, bool or null: Array`.
- Keys compare the way `==` does, so `true` and `1` are the same key:
  `{1: "one", true: "yes"}` is a one-entry map, like Python's
  `{1: "a", True: "b"}`.
- Hash map keys are unique. `set` replaces the value of a key that is
  already there rather than adding a second pair, and a key repeated in
  a literal keeps its last value.
- `let` with no initializer binds null.
- `x = 1` rebinds a variable where it lives, walking outward through
  the enclosing environments, so a function can update a variable of an
  outer scope instead of shadowing it. Assigning to a name that was
  never declared is an error; use `let` for that.
- `arr[0] = 1` and `map["k"] = 1` write into the array or hash map
  itself, so every name holding it sees the change. An array index must
  already exist -- assigning past the end is the same error as reading
  past it, and arrays only grow through `push`. A hash map key is
  created if it is missing, which makes `map[k] = v` and `set(map, k, v)`
  two spellings of the same thing.
- Blocks do not open a scope. Only a call does, so a `let` inside an
  `if` or a `while` body writes to the enclosing environment and is
  still there afterwards.
- `&&` and `||` short circuit, and judge any value the way an `if`
  condition does: only null and false are falsy, so `0` is true.
- `%` is a floating point remainder and takes the sign of the left
  side, so `-7 % 2` is `-1`.
- The REPL keeps one parser and one evaluator across lines, so bindings
  persist. Declarations and statements evaluating to null echo
  nothing, so a `let` whose initializer fails looks silent: the error
  is bound to the variable and only shows up when it is used. A line
  that fails to parse is rolled back whole, so the next line starts
  from a clean state.

## The lexer

`Lexer::tokenize` walks the input once and produces a flat
`std::vector<Token>`, where a token is a type plus its literal text.
Single-character tokens are a plain switch; the two-character ones
(`==`, `!=`, `<=`, `>=`, `&&`, `||`, `//`) peek at the next character
before deciding. Identifiers and numbers are scanned greedily, and
keywords are recognized by looking the finished identifier up in a
static keyword table. Whitespace and comments are skipped. Unterminated
strings and stray characters come back as `Unknown` tokens rather than
stopping the scan.

## The parser

The parser is a **Pratt parser** (top-down operator precedence). This is
the algorithm from Vaughan Pratt's 1973 paper: instead of one grammar
rule per precedence level, every token type gets up to two functions,

- a _prefix_ function, for when the token starts an expression
  (a number, an identifier, `-`, `!`, `(`, `[`, `{`, `fn`), and
- an _infix_ function, for when the token appears after a
  subexpression (`+`, `*`, `==`, `<`, and also `(` for a call and `[`
  for an index),

and each token type gets a binding power. The functions are registered
in two hash tables in the `Parser` constructor.

The core is `parseExpression(precedence)`:

    1. Look up the prefix function for the current token and call it.
       That yields the left-hand side.
    2. While the next token is not `;` and its binding power is
       *greater* than the precedence we were called with, advance to
       that token, look up its infix function, and call it with the
       left-hand side. Its result becomes the new left-hand side.
    3. Return the left-hand side.

That loop is the whole precedence mechanism. `parseBinary` recurses with
its own operator's precedence, so `2 + 3 * 4` parses as `2 + (3 * 4)`:
when the recursive call for the right side of `+` sees `*`, the higher
binding power of `*` lets the loop keep going, but when the call for the
right side of `*` sees `+`, the lower binding power stops it and control
returns to the `+` frame. The levels, lowest to highest, are

    LOWEST < ASSIGN < LOGIC_OR < LOGIC_AND < EQUALS < LESSGREATER
          < SUM < PRODUCT < UNARY < CALL < INDEX

`=` is an infix operator like any other, except it parses its right
side from `LOWEST` rather than from its own level, which is what makes
`a = b = 1` group to the right.

Statements are handled by an ordinary recursive-descent layer on top:
`parse` loops over tokens calling `parseStatement`, which dispatches on
the leading keyword (`let`, `if`, `while`, `return`) and otherwise falls
back to an expression statement. `if`, `while` and function bodies use
`parseBlockStatement`, which recurses back into `parseStatement`, so
blocks nest.

**The AST is not a tree of pointers.** It is two flat vectors on
`ParserResult` -- `statements` and `expressions` -- and nodes refer to
their children by `int` index into those vectors, with `-1` meaning
"none". `programStatementsIndexes` lists the top-level statements to
evaluate, in order, so nested block statements are stored but not run
twice. Nodes are `push_back`ed _after_ their children are parsed, so a
node's children always sit at lower indices than the node itself. The
payoff is that the AST is contiguous, needs no allocation per node, no
ownership rules, and no destructor walk, and the REPL can simply append
each new line's nodes to the same vectors.

Errors are accumulated in `Parser::errors` as strings rather than
thrown, so one bad line can report more than one problem.

## The evaluator

`Evaluator` walks the flat AST directly -- there is no bytecode and no
compilation step. `evalStatement` and `evalExpression` switch on the
node kind, look up children by index and recurse, returning a `Value`.

Environments are a chain: an `ObjEnv` holds a
`unordered_map<string, Value>` plus a pointer to its enclosing
environment. `envGet` walks outward until the name is found, and returns
an error value if it never is. A call creates a fresh environment
enclosed by _the function's_ environment, not the caller's -- that one
choice is what makes closures work -- and binds the parameters in it.

`let` binds through `envSet`, which always writes to the environment it
is handed. Assignment uses `envAssign`, which walks outward like
`envGet` and rebinds where the name already lives; that difference is
what lets a closure keep a counter instead of shadowing it on every
call.

`return` sets a `returning` flag that unwinds the nested
`evalBlockStatement` loops, and the `while` loop, up to the call that
owns it, where it is cleared. Error values short-circuit the same
loops.

## The garbage collector

A `Value` is 16 bytes: a `Tag` plus a union. Null, numbers and booleans
are stored inline in the `Value` and are never heap-allocated, so the
collector never has to think about them. Strings, errors, arrays, hash
maps, functions and environments are heap objects; every one starts with
the same `Obj` header (`kind`, `marked`, `next`) and a `Value` with tag
`Obj` points to it.

The algorithm is **mark and sweep**, tricolor, non-moving,
stop-the-world.

_The object list._ `allocateObj<T>` is the single door onto the heap. It
links every new object into a singly linked list through `Obj::next`,
whose head is `gc.objects`, and adds `sizeof(T)` to
`gc.bytesAllocated`. That list is the collector's complete inventory of
live objects -- the sweep needs no separate bookkeeping.

_When it runs._ `allocateObj` calls `maybeCollect(sizeof(T))` first, and
a collection happens when `bytesAllocated + size` would pass the
threshold `gc.nextGC` (1 MiB initially). Collecting _before_ the new
object exists is deliberate: a half-built object that nothing points to
yet would otherwise be swept away immediately. After each collection
`nextGC` is set to `bytesAllocated * 2`, so the heap grows geometrically
and collection cost stays proportional to live data.

_The tricolor states._ Two pieces of information -- the `marked` bit and
membership in `gc.grayStack` -- encode three states:

    marked   in grayStack   color   meaning
    false    no             white   not reached; presumed garbage
    true     yes            gray    reached, references not scanned yet
    true     no             black   reached, references marked too

_Mark._ `markRoots` colors the roots gray: the global environment, plus
the temporary roots described below. Then the collector pops from
`grayStack` until it is empty, calling `scanObject` on each object to
mark whatever it references -- array items, hash map entries, a
function's environment, an environment's bindings and its `outer`
pointer. Marking an already-marked object is a no-op, which is what
terminates the trace on cycles (a recursive function pointing at the
environment that binds it, for instance).

_Sweep._ `sweep` walks the object list with a pointer-to-pointer cursor
so it can unlink in place. Marked objects are unmarked, ready for the
next cycle, and kept; unmarked objects are unlinked, have their size
subtracted from `bytesAllocated`, and are deleted through their concrete
type so the contained `std::string`/`std::vector` destructors run.

_Roots on the C++ stack._ The evaluator's intermediate `Value`s live in
C++ local variables, which the collector cannot see, and a collection
can be triggered by any allocation in the middle of evaluating an
expression. There is no stack scanning; instead `root.h` provides three
RAII guards that register the address of a local with the GC on
construction and pop it on destruction:

    Rooted        one Value
    RootedVector  a std::vector<Value> (an argument list under construction)
    RootedObj     a bare Obj* (a call's environment)

So `evalCallExpression` roots the callee before evaluating the
arguments, `evalExpressions` roots the result vector so earlier
arguments survive later ones, `applyFunction` roots the call
environment for the duration of the call, and `evalArray` allocates the
empty array and roots it _before_ filling it in. Registering an address
rather than a copy is what lets the guard cover a value that is still
being mutated.

## Tests

    make test

`tests/lexer_test.cpp` checks the token stream, `tests/parser_test.cpp`
checks the shape of the flat AST and the parse errors, and
`tests/evaluator_test.cpp` checks evaluation results, including closures,
recursion and error messages. GoogleTest is fetched by cmake at
configure time.
