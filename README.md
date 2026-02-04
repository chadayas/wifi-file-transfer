# WiFi Transfer

A peer-to-peer file transfer tool built from raw TCP/UDP sockets in C++. No libraries, no frameworks — just POSIX sockets, HTTP built by hand, and DNS packets parsed byte by byte.

Upload files through a browser, pick a device on your network, and the server forwards them over HTTP to the receiver on the other end.

## Demo

<!-- Add your demo video/gif here -->

## How It Works

The program runs three services on three threads:

| Service | Port | Protocol | Role |
|---------|------|----------|------|
| **Web Server** | 8080/TCP | HTTP/1.1 | Serves the upload form, parses multipart uploads, forwards files |
| **Receiver** | 9090/TCP | HTTP/1.1 | Accepts forwarded files, writes them to disk |
| **mDNS** | 5353/UDP | Multicast DNS | Discovers other devices running the program on the LAN |

### The Upload Flow

```
Browser                    Sender (8080)                Receiver (9090)
  |                            |                            |
  |--- GET / ----------------->|                            |
  |<-- HTML form + device list-|                            |
  |                            |                            |
  |--- POST /upload ---------->|                            |
  |    (multipart/form-data)   |                            |
  |                            |--- POST /upload ---------->|
  |                            |    (multipart/form-data)   |
  |                            |                            |-- save to disk
  |                            |<-- 200 OK ----------------|
  |<-- success page -----------|                            |
```

---

## Building the HTTP Request by Hand

There is no HTTP library here. Every request and response is a string assembled from raw parts. This section explains what that actually means at the byte level.

### What an HTTP Response Looks Like on the Wire

When the browser sends `GET /`, the server responds with something like this:

```
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 1432\r\n
\r\n
<!DOCTYPE html>...
```

That entire thing — status line, headers, blank line, body — is one continuous byte stream written to a TCP socket with `send()`. The `\r\n` sequences aren't decorative; they are the protocol. The blank line (`\r\n\r\n`) separates headers from body. The `Content-Length` tells the other side how many bytes follow after that blank line.

### Multipart Form Data: What the Browser Actually Sends

When you upload files through an HTML form with `enctype="multipart/form-data"`, the browser doesn't send the files as-is. It wraps them in a MIME envelope. A two-file upload looks like this on the wire:

```
POST /upload HTTP/1.1\r\n
Content-Type: multipart/form-data; boundary=------WebKitFormBoundaryABC123\r\n
Content-Length: 524288\r\n
\r\n
------WebKitFormBoundaryABC123\r\n
Content-Disposition: form-data; name="device"\r\n
\r\n
arp_192.168.1.42\r\n
------WebKitFormBoundaryABC123\r\n
Content-Disposition: form-data; name="file[]"; filename="photo.jpg"\r\n
Content-Type: image/jpeg\r\n
\r\n
<raw JPEG bytes here>
\r\n
------WebKitFormBoundaryABC123\r\n
Content-Disposition: form-data; name="file[]"; filename="notes.pdf"\r\n
Content-Type: application/pdf\r\n
\r\n
<raw PDF bytes here>
\r\n
------WebKitFormBoundaryABC123--\r\n
```

Key things to notice:
- The **boundary** string separates each part. The browser picks a random one and tells us what it is in the `Content-Type` header.
- Each part has its own mini-headers (`Content-Disposition`, `Content-Type`) followed by `\r\n\r\n`, then the raw data.
- The **final boundary** ends with `--` to signal "no more parts."
- The form field `name="device"` is sent as a part too, not just the files.

### The Parsing State Machine

The server can't just wait for the entire upload to arrive and then process it — a 500MB file would sit in memory as one giant string. Instead, it processes data incrementally as it arrives from `recv()` in chunks.

The parser has three states:

```
MULTIPART_HEADER ──> FILE_DATA ──> DONE
       ^                 |
       |                 |
       └─────────────────┘
         (next file part)
```

**MULTIPART_HEADER**: We're reading the mini-headers of a part (`Content-Disposition`, `Content-Type`). We scan the incoming bytes for `\r\n\r\n` which marks the end of headers. Once found, we extract the filename, create a new buffer for this file, and transition to `FILE_DATA`.

**FILE_DATA**: We're reading raw file bytes. We scan for the boundary string. Everything before the boundary is file data — we append it to the current file's buffer. If we find the boundary followed by `--`, we're done. Otherwise, we go back to `MULTIPART_HEADER` for the next file.

The tricky part: what if the boundary string is split across two `recv()` calls? The parser handles this by keeping the last N bytes (where N = boundary length) in a stash rather than committing them to the file buffer. On the next `recv()`, the stash gets more data appended and the boundary check runs again.

### Forwarding: Building Our Own HTTP POST

Once the files are parsed, the server opens a new TCP connection to the selected device and constructs a fresh HTTP POST request from scratch. This is where the server acts as an HTTP *client*.

The content length is pre-computed mathematically — every part header is a known size, every file size is known — so we can write the `Content-Length` header without buffering the entire body into one string:

```cpp
size_t content_length = 0;
for (each file) {
    content_length += boundary_line_size;
    content_length += content_disposition_line_size;
    content_length += content_type_line_size;
    content_length += file_buffers[i].size();  // actual file data
    content_length += 2;                       // trailing \r\n
}
content_length += final_boundary_size;
```

Then the data is sent in sequential `send()` calls — the HTTP headers first, then for each file: the part header, the file bytes directly from the buffer (no copy), and the CRLF terminator. Finally the closing boundary. TCP is a stream protocol, so the receiver sees one continuous byte stream regardless of how many `send()` calls produced it.

---

## Device Discovery

### mDNS (Multicast DNS)

The program advertises itself and discovers other instances using mDNS on `224.0.0.251:5353`. This is the same protocol that powers Bonjour / Avahi.

It works by constructing raw DNS packets — there's no resolver library, the bytes are assembled manually:

**DNS packet structure:**
```
+--+--+--+--+--+--+--+
|       Header        |  12 bytes: ID, flags, counts
+--+--+--+--+--+--+--+
|      Questions      |  "What services exist?"
+--+--+--+--+--+--+--+
|       Answers       |  PTR, SRV, A records
+--+--+--+--+--+--+--+
```

The program sends three record types in its announcement:
- **PTR record**: "I provide `_filetransfer._tcp.local`" — maps the service type to this device's name
- **SRV record**: "My receiver is on port 9090" — tells others where to connect
- **A record**: "My IP is 192.168.1.x" — the actual network address

**DNS name encoding** compresses domain names into a length-prefixed format:
```
_filetransfer._tcp.local
→ [13] _filetransfer [4] _tcp [5] local [0]
```

Each label is preceded by its length as a single byte, terminated by a null byte. Compression pointers (two bytes starting with `0xC0`) can reference earlier names in the same packet to save space.

### ARP Fallback

For devices that don't run mDNS (or haven't been discovered yet), the server reads the kernel's ARP cache from `/proc/net/arp`. This gives a list of IP addresses that have recently communicated on the local network. These show up in the device dropdown with an `(ARP)` label.

---

## Project Structure

```
wifi_transfer/
├── src/
│   ├── server.hpp / server.cpp    Web server, multipart parser, HTTP client
│   ├── receiver.hpp / receiver.cpp Receiver service (port 9090)
│   ├── mdns.hpp / mdns.cpp        mDNS discovery (port 5353)
│   └── shared.hpp / shared.cpp    Shared state, device map, mutex
├── frontend/
│   ├── index.html                 Upload form ({{DROPDOWN}} template)
│   ├── success.html               Result page ({{FILE_COUNT}} template)
│   └── styles.css                 Dark mode styling
├── py/
│   └── receiver.py                Standalone Python receiver script
├── tests/                         Test files
└── Makefile
```

## Build & Run

```bash
# Build
make

# Run (starts all three services)
./build/s
```

Then open `http://localhost:8080` in a browser. Select a destination device, pick files, and hit Send.

To receive files on another device, either:
- Run the same binary on that device, or
- Download the Python receiver script from `http://sender-ip:8080/receiver` and run `python3 receiver.py`

Files are saved to `~/Downloads/wifi_transfer/`.

## Requirements

- Linux (uses `/proc/net/arp`, POSIX sockets, `ifaddrs.h`)
- C++17
- g++ with pthread support
