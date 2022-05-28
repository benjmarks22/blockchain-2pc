# blockchain-2pc
CS 244B project to use a public blockchain as a two-phase commit coordinator to securely commit an atomic transaction across any two systems of a database.


## Installation

To install the tools necessary for the project, run

`bash setup.sh`

### Docker

To install in Docker, run

`docker build -t image:latest ./`

To run commands in Docker, run

`docker run -i -t image:latest bash`

## Running the server and client

### Server

To run the server, run:

`bazel run //src/server:greeter_server_main`

### Client

To run the client, run:

`bazel run //src/client:greeter_client_main`

## Testing

To run all the tests, run:

`bazel test //src/...`

## Formatting and static analysis

### Bazel files

To format `BUILD` files and `WORKSPACE`, run:

`buildifier <file>`.

To format all Bazel files, run:

`buildifier -mode fix -lint warn -r src/ WORKSPACE`

### C++ and proto files

To format, run:

`clang-format --style=google <file>`

To format all changed C++ and proto files, run:

`git diff -U0 --no-color HEAD^ | clang-format-diff -i --style=google -p1`

To run static analysis, run:

`bazel build //src/...`

`bazel run @hedron_compile_commands//:refresh_all`

`clang-tidy -p compile_commands.json <file>`

To fix static analysis errors for all changed lines, change the third command to

`git diff -U0 HEAD^ | clang-tidy-diff -fix -p1 -path compile_commands.json`

### Solidity files

To format all solidity files:

`prettier --plugin-search-dir=src/blockchain --write 'src/blockchain/**/**/*.sol'`

### JavaScript files

To format javascript files:

`clang-format -i --style=Google src/blockchain/*.js src/blockchain/client/*.js`
