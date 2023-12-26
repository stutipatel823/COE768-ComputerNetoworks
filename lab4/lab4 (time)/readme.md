### Understanding UDP's Lack of Connection Establishment with `recvfrom` and `sendto` Functions

#### Connectionless Protocol

- **UDP (User Datagram Protocol)** is a connectionless protocol, designed for speed and simplicity in scenarios where low latency is crucial.
- It differs from TCP (Transmission Control Protocol) in that it doesn't establish a persistent connection before sending data.

#### Key Characteristics of UDP

1. **No Handshaking:** UDP skips the handshake process, enabling stateless communication where each datagram is independent.
2. **Destination in Datagram:** Each UDP packet includes the destination IP address and port number in its header.
3. **Direct Read/Write:** Applications use direct read from and write to UDP sockets, specifying the destination for each packet.
4. **Optional `connect` Function:** This can set a default destination for a UDP socket, without establishing a TCP-like connection.

#### `recvfrom` and `sendto` Functions in Detail

- **`recvfrom` Function:**

  ```c
  ssize_t recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
  ```

  - `socket`: Socket descriptor to receive data on.
  - `buffer`: Buffer to store received data.
  - `length`: Maximum length of the buffer.
  - `flags`: Flags modifying function behavior.
  - `src_addr`: Address structure for sender's address information.
  - `addrlen`: Size of the `src_addr` structure.

- **`sendto` Function:**
  ```c
  ssize_t sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
  ```
  - `socket`: Socket descriptor to send data on.
  - `message`: Data to be sent.
  - `length`: Length of the data.
  - `flags`: Flags modifying function behavior.
  - `dest_addr`: Address structure for destination address.
  - `addrlen`: Size of the `dest_addr` structure.

#### Summary

- `recvfrom` and `sendto` are integral to UDP socket programming, facilitating data transmission without established connections.
- These functions are commonly used in connectionless protocols like UDP, where each packet is individually addressed.

---
