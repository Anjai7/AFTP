# 📦 AFTP - Advanced File Transfer Protocol

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C](https://img.shields.io/badge/language-C-blue.svg)](https://www.iso.org/standard/74528.html)
[![UDP](https://img.shields.io/badge/protocol-UDP-green.svg)](https://tools.ietf.org/html/rfc768)

A high-performance, UDP-based file transfer protocol designed for speed and reliability without the overhead of TCP connections.

## 🔍 Overview

**AFTP (Advanced File Transfer Protocol)** is a custom file transfer system built over **UDP**, focusing on **reliability, speed, and simplicity**. Unlike traditional FTP or TCP-based transfers, AFTP handles reliability at the application level through custom logic while preserving UDP's low-latency characteristics.

### Key Features

- 🚀 **High Performance**: Continuous data streaming without waiting for acknowledgments
- 🔧 **Custom Reliability**: Application-level packet loss handling and recovery
- 📦 **Chunked Transfer**: Efficient file segmentation and reassembly
- 🔄 **Batch Acknowledgments**: Reduced network overhead with grouped ACKs
- ⚡ **Low Latency**: UDP-based for minimal connection overhead

## ❓ Why AFTP?

### Problems with Traditional FTP

Traditional FTP uses TCP, which provides reliability but introduces performance bottlenecks:

![FTP TCP Behavior](images/active.svg)

*TCP's window sliding mechanism in Wireshark capture*

![FTP Wireshark Analysis](images/ftp_wireshark.png)

As shown in the Wireshark capture, FTP waits for each ACK before sending the next packet batch, creating unnecessary delays.

### AFTP's Solution

AFTP eliminates waiting periods by:

- **Continuous Streaming**: Server sends data packets without waiting for ACKs
- **Batch Processing**: Client acknowledges groups of packets (default: 10 packets)
- **Smart Recovery**: Missing packets trigger specific RESEND requests
- **Dynamic Buffering**: Adaptive buffer management based on network conditions

![AFTP Flow Diagram](images/aftp_flow.svg)

*AFTP's continuous streaming approach with batch acknowledgments*

## 🛠️ Installation

### Prerequisites

- GCC compiler
- Linux/Unix environment (tested on Ubuntu/Debian)
- Network access between client and server machines

### Building

```bash
git clone https://github.com/yourusername/aftp.git
cd aftp
gcc -o server server.c
gcc -o client client.c
```

## 📋 Protocol Specification

### Initial Handshake
```
Client → Server: HELLO <filename>
Server → Client: FILE_INFO <size, total_chunks, chunk_size>
```

### Data Transfer
```
Server → Client: Continuous chunks (chunk_id + data)
Client → Server: ACK <last_chunk_id> (after receiving batch)
```

### Error Recovery
```
Client → Server: RESEND <missing_chunk_id>
Server → Client: Retransmits missing chunk
```

### Configuration Constants

```c
#define CHUNK_SIZE 1024      // Size of each data chunk
#define SERVER_PORT 8888     // Default server port
#define MAX_FILENAME 256     // Maximum filename length
#define BUFFER_SIZE 100      // Server buffer capacity
#define ACK_BATCH 10         // Packets per acknowledgment batch
#define TIMEOUT_MS 1000      // Timeout for retransmissions
```

## 🚀 Usage

### Server
```bash
./server [port]
```

### Client
```bash
./client <server_ip> [port] <filename>
```

### Example Session

**Server:**
```bash
./server 8888
Server listening on port 8888...
```

**Client:**
```bash
./client 192.168.1.100 8888 largefile.txt
Requesting file: largefile.txt
Transfer completed successfully!
```

## 📊 Performance Comparison

| Aspect               | Traditional FTP      | AFTP                            |
|----------------------|----------------------|----------------------------------|
| **Speed**            | Slower (handshakes)  | Faster (continuous streaming)   |
| **Packet Loss**      | TCP automatic retry  | Client-initiated RESEND         |
| **Flow Control**     | TCP windows          | Manual chunk buffering          |
| **Acknowledgments**  | Per-packet (hidden)  | Batched (configurable)          |
| **Buffer Management**| Automatic            | Application-controlled          |

## 🧪 Testing Results

### Test Environment
- **File Size**: 9.6MB text file
- **Network**: Local area network
- **Chunk Size**: 1024 bytes
- **Batch Size**: 10 packets per ACK

### Wireshark Analysis

![AFTP Wireshark Capture](images/aftp_test.png)

*Wireshark capture showing batched ACK behavior (server: port 8888, client: port 45312)*

### File Integrity Verification
```bash
diff original_file.txt received_file.txt
# No output = files are identical
```

## 🔧 Development Journey

This project started as a learning exercise in network programming and protocol design. The development process included:

1. **Basic UDP Communication**: Implementing simple client-server message exchange
2. **File Transfer Logic**: Adding file reading, chunking, and reassembly
3. **Protocol Definition**: Designing message formats and flow control
4. **Performance Optimization**: Tuning chunk sizes and batch parameters
5. **Cross-Network Testing**: Validating behavior across different devices

## 📁 Project Structure

```
AFTP/
├── README.md
├── LICENSE
├── protocol.h          # Protocol constants and structures
├── server.c           # Server implementation
├── client.c           # Client implementation
├── images/            # Documentation images
│   ├── active.svg
│   ├── ftp_wireshark.png
│   ├── aftp_flow.svg
│   └── aftp_test.png
└── examples/          # Example files for testing
```

## 🔮 Future Enhancements

- [ ] **Compression Support**: Integrate data compression algorithms
- [ ] **Encryption**: Add TLS/SSL-like security layer
- [ ] **Multi-threading**: Parallel chunk processing
- [ ] **Adaptive Parameters**: Dynamic chunk and batch size optimization
- [ ] **Cross-platform Support**: Windows and macOS compatibility
- [ ] **Resume Capability**: Support for interrupted transfer recovery
- [ ] **Bandwidth Throttling**: Configurable transfer rate limits

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Inspired by the need for faster file transfer protocols
- Built as a learning project to understand network programming concepts
- Thanks to the networking community for protocol design insights

---

**Note**: This is an educational project demonstrating UDP-based protocol design. For production use, consider additional security and robustness features.
