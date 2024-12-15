# TCP Multi-Client Server with Socket Programming

This project is a TCP multi-client server implemented in C. It allows multiple clients to connect simultaneously, enabling functionalities such as user registration and authentication. Communication between the server and clients occurs via TCP sockets.

## Features
- User registration (`signup` command)
- User login (`login` command)
- Shared memory for user management
- Persistent storage for user data
- Concurrent client handling using `fork`

---

## Requirements

### Libraries and Tools
- **GCC**: For compiling the C programs.
- **netcat**: (Optional) Useful for testing TCP connections.
- **POSIX-compliant system**: Required for system calls such as `mmap` and `fork`.

### Installation
To set up the environment:
1. Ensure GCC is installed:
   ```bash
   sudo apt update
   sudo apt install gcc
   ```
2. (Optional) Install `netcat`:
   ```bash
   sudo apt install netcat
   ```

---

## Getting Started

### Compile the Code
Compile the `server` and `client` programs using GCC:
```bash
gcc -o server server.c
```
```bash
gcc -o client client.c
```

### Run the Server
Start the server on port `8080`:
```bash
./server
```
The server will create or load a `users.txt` file to manage user data persistently.

### Run a Client
Start a client and connect to the server:
```bash
./client
```
Once connected, the client can register or log in using commands:
- Register: `signup <username> <password>`
- Login: `login <username> <password>`

### Testing with Netcat (Optional)
You can use `netcat` to test the server's response:
```bash
nc localhost 8080
```

---

## Usage Instructions

1. **Start the server**: Run the server program on a system.
2. **Connect clients**: Each client can run the `client` program to connect to the server.
3. **Interact with the server**: Clients can use the following commands:
   - `signup <username> <password>`: Register a new account.
   - `login <username> <password>`: Log in to an existing account.
   - Any other messages will be treated as general communication.
4. **Exit the client**: Type `exit` to disconnect.

---

## File Details

### Server (`server.c`)
The server handles:
- Listening for incoming connections.
- Handling multiple clients using `fork`.
- Managing user data in shared memory.
- Persisting user data in `users.txt`.

### Client (`client.c`)
The client allows users to:
- Connect to the server.
- Send commands and messages to the server.
- Receive server responses.

---

## Notes
- Ensure the `users.txt` file is writable by the server process.
- Concurrent client handling uses `fork`, which creates separate processes for each client.
- Use `Ctrl+C` to terminate the server gracefully.

---

## Troubleshooting

### Common Issues
- **Address already in use**:
  Restart the server after ensuring no process is using port 8080:
  ```bash
  sudo netstat -tuln | grep 8080
  sudo kill -9 <PID>
  ```
- **Permission Denied**:
  Ensure sufficient permissions for `users.txt`:
  ```bash
  chmod 600 users.txt
  ```

---

Feel free to contribute or report issues by opening a pull request or an issue in the repository.

