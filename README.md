# POP3 Server Implementation

This project implements a simple POP3 server designed to work with a `mailDir` structure. The server supports basic commands and simulates the behavior of a mail server.

---

## 🛠️ Setup Instructions

### Prerequisites
- GCC or any compatible C compiler
- `make` utility
- Bash for script execution

### Steps to Compile and Run

1. **Clean and Build the Project**
   ```bash
   make clean all
2. **Generate maildir example**
   ```bash
   chmod +x ./generateMailDir.sh
   ./generateMailDir.sh
3. **Run server**
   ```bash
   ./bin/server -d ./mailDir
4. **Add necessary information**
   ```text
   In case there's no users registered, run -u username:password
   Run ./bin/server -h for more information 
