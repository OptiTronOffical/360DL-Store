# XboxTLS – TLS 1.2 Client Library for Xbox 360

> A lightweight, production-ready TLS 1.2 client for the Xbox 360 console, built with BearSSL and designed to enable encrypted HTTPS communication from Xbox homebrew projects.

---

## 🚀 Overview

**XboxTLS** is a standalone, static library that brings full TLS 1.2 support to the Xbox 360 via BearSSL. Designed for devkit or modded environments (RGH/JTAG), XboxTLS lets homebrew developers securely connect to modern HTTPS endpoints like GitHub, Discord, consolemods.org, and more — using robust and verifiable X.509 certificate validation.

XboxTLS provides:

- ✅ Full TLS 1.2 handshake + secure data transmission
- 🔐 Certificate validation using trusted RSA or EC root certs
- ⚡ High-performance I/O abstraction using Winsock + XNet
- 🧠 Minimal heap usage with optional buffer tuning
- 🔧 Custom entropy seeding via `XeCryptRandom`
- 📦 Static-only — no dynamic BearSSL linking required

---

## 📷 Screenshots

![TLS Success Screenshot](https://i.gyazo.com/9abc18b940462a971c8b3f3d6c83890f.png)

*Above: Successfully connecting to consolemods.org using P-384 ECC and SHA-384 validation on Xbox 360 homebrew.*

---

## 🔧 Features

| Feature                         | Support |
| ------------------------------ | ------- |
| TLS Protocol Version           | TLS 1.2 |
| BearSSL Core                   | ✅ Minimal TLS & X.509 |
| RSA Trust Anchors              | ✅ PKCS#1 |
| EC Trust Anchors               | ✅ P-256, P-384, etc. |
| SHA Algorithm Support          | SHA-1, SHA-224, SHA-256, SHA-384, SHA-512 |
| SNI Support                    | ✅ Server Name Indication |
| Blocking Socket Mode           | ✅ (non-blocking possible) |
| Custom Random Seed             | ✅ XeCryptRandom |
| Certificate Chain Validation   | ❌ (trust-anchor only) |
| TLS 1.3                        | ❌ Not implemented |
| Server Cert Validation         | ✅ via BearSSL |

---

## 🛠️ Requirements

- Xbox 360 (RGH/JTAG or devkit)
- Xbox XDK + Visual Studio 2010
- XDK Libraries (`winsockx.h`, `xtl.h`, `xbox.h`, etc.)
- BearSSL (statically included in project)
- Working internet connection on console (via XNet)

---

## 🧠 How It Works

### Internals

XboxTLS wraps BearSSL's TLS engine and X.509 minimal validator. It abstracts out platform-specific components such as:

- Socket I/O: via `recv`/`send` + `XNetStartup`
- Randomness: via `XeCryptRandom`
- DNS: via `XNetDnsLookup`

BearSSL’s minimal X.509 engine does not perform full certificate chain validation. Instead, XboxTLS allows **manual injection of trusted public keys (trust anchors)**.

---

## 📦 Installation

### 🔗 Clone the Repository

```bash
git clone https://github.com/JakobRangel/XboxTLS.git

```

## 📄 Example Usage

```c
XboxTLSContext ctx;
XboxTLS_CreateContext(&ctx, "example.com");
ctx.hashAlgo = XboxTLS_Hash_SHA384;

// Add trust anchor (EC or RSA)
XboxTLS_AddTrustAnchor_EC(&ctx, EC_DN, sizeof(EC_DN), EC_Q, sizeof(EC_Q), XboxTLS_Curve_secp384r1);

// Optional: RSA
// XboxTLS_AddTrustAnchor_RSA(&ctx, RSA_DN, sizeof(RSA_DN), RSA_N, sizeof(RSA_N), RSA_E, sizeof(RSA_E));

char ip[64];
ResolveDNS("example.com", ip, sizeof(ip));

if (XboxTLS_Connect(&ctx, ip, "example.com", 443)) {
    const char* getReq = "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
    XboxTLS_Write(&ctx, getReq, strlen(getReq));

    char buf[2048];
    int len;
    while ((len = XboxTLS_Read(&ctx, buf, sizeof(buf) - 1)) > 0) {
        buf[len] = '\0';
        OutputDebugStringA(buf); // print to debugger
    }

    XboxTLS_Free(&ctx);
}
```
*✅ For a fully working integration example, see [ExampleClient.cpp](https://github.com/JakobRangel/XboxTLS/blob/main/ExampleClient.cpp)*


## 🔐 Trust Anchor Details

XboxTLS currently supports loading **manual trust anchors** via raw RSA or EC public keys.

**Supported formats:**
- ASN.1 Distinguished Name (DN)
- RSA public modulus + exponent
- EC uncompressed point `Q` + curve ID

These can be extracted using tools like OpenSSL:

```bash
openssl x509 -in cert.pem -noout -pubkey -subject
```
## 🙌 Credits

- **[BearSSL](https://bearssl.org/)** – Lightweight TLS engine by Thomas Pornin
- **XeCrypt/XNet** – Provided by Xbox Development Kit (XDK)
- **Jakob Rangel** – Author of XboxTLS
- Thanks to the Xbox 360 homebrew and reverse engineering community!
