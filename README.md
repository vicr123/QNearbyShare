# QNearbyShare

QNearbyShare is a [Nearby Share](https://support.google.com/files/answer/10514188?hl=en) client.

Currently, QNearbyShare has been tested with the following Nearby Share implementations:

| Implementation                                  | Send to | Receive from |
|-------------------------------------------------|---------|--------------|
| Android                                         | No*     | Yes          |
| Nearby Share Beta (Windows)                     | No      | Yes          |
| QNearbyShare                                    | Yes     | Yes          |
| [NearDrop](https://github.com/grishka/NearDrop) | Yes     | Yes          |

*Devices with "Quick Share" are no longer compatible with QNearbyShare, as there is no known way to initiate receive mode from the device.

---

## Dependencies

- Qt 6
- Avahi
- Protobuf
- Either OpenSSL or Crypto++ (Crypto++ is preferred)
- CMake (build)

## Build

Run the following commands in your terminal.

```bash
cmake -B build -S .
cmake --build build
```

To build with OpenSSL instead of Crypto++

```bash
cmake -B build -S . -DUSE_OPENSSL
cmake --build build
```

## Install

```bash
cmake --install build
```

## Usage

The Avahi daemon needs to be running before issuing any commands related to QNearbyShare.

### Sending a file

```bash
qnearbyshare-send /path/to/file
```

### Receiving a file

```bash
qnearbyshare-receive
```

---

> © Victor Tran, 2023. This project is licensed under the MIT License.
>
> Check the [LICENSE](LICENSE) file for more information.
