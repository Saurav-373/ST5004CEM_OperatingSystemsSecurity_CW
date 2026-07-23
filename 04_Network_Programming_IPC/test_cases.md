# Task 4 Test Cases

| Test ID | Scenario | Expected result | Actual result | Status |
|---|---|---|---|---|
| T4-01 | Valid client login and normal commands | Authentication and protected commands succeed | LOGIN, MSG, STATUS, TIME and QUIT worked correctly | Pass |
| T4-02 | Invalid credentials | Login is rejected and protected commands are denied | Invalid credentials and authentication-required responses were returned | Pass |
| T4-03 | Two concurrent clients | Both clients are handled independently | Both clients authenticated and reported active_clients=2 | Pass |
| T4-04 | Malformed login and unknown command | Clear protocol errors are returned | Invalid LOGIN format and unknown command were rejected safely | Pass |
| T4-05 | Empty and oversized input | Invalid input is rejected without crashing the server | Empty-command and message-too-long errors were returned | Pass |
| T4-06 | Client disconnects without QUIT | Client resources are released and the server continues | Unexpected disconnection was logged and another client connected successfully | Pass |
| T4-07 | Port conflict and server shutdown | Port error is reported and shutdown is handled safely | Address-in-use error was displayed and SIGINT shutdown completed safely | Pass |

## Evidence Files

- `outputs/t4_01_valid_client.txt`
- `outputs/t4_02_invalid_credentials.txt`
- `outputs/t4_03_client1.txt`
- `outputs/t4_03_client2.txt`
- `outputs/t4_04_invalid_commands.txt`
- `outputs/t4_05_input_validation.txt`
- `outputs/t4_06_unexpected_disconnect.txt`
- `outputs/t4_07_port_error.txt`
- `outputs/t4_07_safe_shutdown.txt`

## Compilation Result

The server and client were compiled using `-Wall`, `-Wextra` and `-std=c11`.

The server was additionally linked using `-pthread`.

Both programs compiled without warnings.
