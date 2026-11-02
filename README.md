# YZ1 / YAMAZAKI-Zip 1.05.2 tools

Reverse-engineered reimplementation of the YZ1 archive format and the
YAMAZAKI-Zip 1.05.2 `yzenc` / `yzdec` tools.

The project contains:

| Path | Contents |
|---|---|
| `src/yzenc.c` | Command-line packer |
| `src/yzdec.c` | Command-line unpacker |
| `src/build.bat` | MSVC 6 build helper for the CLI tools |
| `yz32lib/` | C++ SDK (encoder, decoder, header-only listing) |
| `yz32lib/test/sdktest.cpp` | SDK test harness |
| `yz32lib/build.bat` | MSVC 6 build for the SDK and test harness |
| root `yzenc.exe`, `yzdec.exe` | Original 1.05.2 reference binaries |
| root `yzenc.c`, `yzdec.c`, `*.lst` | Reverse-engineering artifacts used to validate the port |
| `yzcrack/` | Optional password-recovery research tools |

The maintained implementation is in `src/` and `yz32lib/`. The root-level
`.c` files are large decompiler/listings artifacts and are not the buildable
CLI sources.

## Status

The implementation is byte-exact against the original for the differential
corpus used during development:

- single files from 0 bytes through 1 MB, plain and password-protected;
- public-password tokens;
- nested directories, dotfiles, empty files and empty archives;
- multi-file archives, including the original's max-file-size batching;
- encrypted and public-token multi-file archives;
- SDK encode and decode of the same inputs;
- archives produced by either implementation can be decoded by the other.

The test corpus covers small/similar files where naïve per-file compression
differs from the original. The batching rule is described in
[FORMAT.md](FORMAT.md).

Extraction streams one max-file-size batch at a time instead of buffering the
whole uncompressed archive. A 260 MB archive extracted successfully with
about 4.5 MB peak working set. There is no fixed uncompressed-size limit.

### Known differences and limits

- **Compressed input is still read into memory.** The original reads the
  compressed stream incrementally. Peak memory here is compressed-archive
  size plus the largest file, not only the largest file.
- **Malformed archives:** the reimplementation adds bounds checks and rejects
  some corrupt inputs more strictly than the original. Valid archives are
  byte-exact.
- **SDK header table:** a 256 MB guard remains on the uncompressed metadata
  table allocation. This is not an extraction-size limit.
- **Empty directories are not represented** by the format. Encoding an empty
  directory produces a valid empty archive.
- **Windows path handling:** stored names are raw bytes with backslash
  separators. There is no Unicode conversion; path-separator scanning uses the
  Japanese DBCS lead-byte heuristic used by the original.
- **32-bit format:** sizes and timestamps are 32-bit fields.
- The original tools are 16/32-bit Windows-era software; this port is built
  with MSVC 6 and targets the same platform conventions.

## Building

Prerequisites:

- Microsoft Visual C++ 6.0 (the scripts call the traditional
  `VCVARS32.BAT`);
- a **local** working copy. `cl` and `lib` do not reliably handle UNC paths.

Build the CLI tools from the repository root:

```bat
src\build.bat yzenc
src\build.bat yzdec
```

The outputs are `src\yzenc.exe` and `src\yzdec.exe`.

Build the SDK and its test harness:

```bat
yz32lib\build.bat
```

The SDK build produces `yz32lib\yz32lib.lib` and
`yz32lib\test\sdktest.exe`. The SDK requires `_USE_32BIT_TIME_T` and
`YZ1_STATIC`; `build.bat` sets both. Build the SDK with the same compiler and
CRT as the final application, because the public API uses STL strings and
vectors without a DLL boundary.

If the MSVC 6 path differs, edit the `VCVARS32.BAT` line in the build scripts.

## Command-line usage

### Packer

```text
yzenc [-d[outdir]] [-p[password]] [-y] [-j] [-m] <file|dir> ...
```

The option parser uses the rest of the argument as the value. Use
`-dOutDir` and `-pPASSWD` **without a space**; a bare `-d` or `-p` prompts.

| Option | Meaning |
|---|---|
| `-d<dir>` | Put the output archive in `dir`; bare `-d` prompts |
| `-p<password>` | Encrypt; bare `-p` prompts |
| `-y` | Answer yes to all prompts |
| `-j` | Accepted for compatibility; no-op (English-only CLI) |
| `-m` | One archive per input instead of one merged archive |

The default is a deliberate extension: all input paths are merged into one
archive. `-m` restores one-archive-per-input, which matches the original
tool's single-input usage.

The output name is taken from the first input's last path component. The last
filename extension is replaced with `.yz1`; for example `sample.txt` becomes
`sample.yz1`. Inputs whose extension is already `.yz1` are refused.

Examples:

```bat
yzenc -y -dout samples\hello.txt
yzenc -y -psecret samples\hello.txt
yzenc -y -p samples\hello.txt
yzenc -y -m one.txt two.txt
```

The packer prints `InFile`/`OutFile` records and the final compressed size. If
the output would be larger than the input, it asks before writing unless `-y`
is given.

### Unpacker

```text
yzdec [-d[outdir]] [-p[password]] [-y] [-t] [-j] <archive.yz1> ...
```

| Option | Meaning |
|---|---|
| `-d<dir>` | Extract into `dir`; bare `-d` prompts |
| `-p<password>` | Password or `pub-...` token; bare `-p` prompts |
| `-y` | Answer yes to all prompts |
| `-t` | Test only; decode and validate without writing files |
| `-j` | Accepted for compatibility; no-op (English-only CLI) |
| `-x<file>` | Diagnostic: dump the raw Huffman length tables |

Examples:

```bat
yzdec -y -dout archive.yz1
yzdec -y -psecret -dout archive.yz1
yzdec -y -t archive.yz1
yzdec -y -dout one.yz1 two.yz1
```

The decoder accepts multiple archives. If the executable has a YZ1 archive
appended to it (self-extracting file), it extracts that archive instead of
requiring a separate `.yz1` argument.

## SDK

Include `yz32lib/yz32lib.h` with:

```cpp
#define _USE_32BIT_TIME_T
#define YZ1_STATIC
#include "yz32lib.h"
```

The SDK reports progress and prompts through an application-supplied
`yzFileEv` implementation. `YzMessage::language` selects the SDK message
language (`language_japanese` or `language_english`; the reimplementation
defaults to English). The command-line tools intentionally use English
messages only.

Minimal decode flow:

```cpp
class Ev : public yzFileEv {
    // MessEX, MessINF, MessYN, MessINPUT,
    // gauge_EncodeInit, gauge_DecodeInit,
    // gauge_String, gauge_Value, gauge_WriteInfo
};

Ev ev;
yzFileDecode dec(&ev, "out");
if (dec.YzFile_DecodeHeader("archive.yz1") < 0) return 1;
dec.Print("archive.yz1");          // header-only listing
if (dec.DecodeFile("archive.yz1", 0) < 0) return 1;
```

`YzFile_DecodeHeader` reads and validates the metadata without decoding the
payload. `DecodeFile` decodes and writes the files; pass `in_decode_mode = 1`
to decode without writing (the equivalent of the CLI `-t`).

Encode flow:

```cpp
yzFileEncode enc(&ev, "out", 0, YZ_MAIN_ENCODE_PASSWD_OFF);
if (enc.EncodeFile("input") < 0) return 1;
```

Password modes are `YZ_MAIN_ENCODE_PASSWD_OFF`, `..._ON` and `..._PUB`.
The SDK can also create a self-extracting file by passing an executable path
as the third `yzFileEncode` constructor argument.

The test harness exercises the same operations:

```text
sdktest enc <file|dir> <outdir> [password] [self_exe]
sdktest encpub <file|dir> <outdir>
sdktest hdr <archive.yz1> [password]
sdktest dec <archive.yz1> <outdir> [password]
```

## Password warning

YZ1 "encryption" is a four-byte repeating XOR keystream with no salt, no
KDF and no iteration count. The header also contains an 8-bit check value.
It is obfuscation for compatibility, not modern encryption. Different
passwords can derive the same key and check byte. See
[`yzcrack/YZCRACK-README.md`](yzcrack/YZCRACK-README.md) for the password
research tools and the exact attack cost.

## Format documentation

See [FORMAT.md](FORMAT.md) for the byte layout, header and table fields, the
Huffman bitstream, the LZ token semantics, password derivation, SFX trailer
and the max-file-size batching rule.

## Verification notes

The port was developed by comparing output bytes and decoded file contents
against the original `yzenc.exe` and `yzdec.exe` for plain, password and
public-token archives. The regression corpus includes:

- 0-byte through 1 MB single files;
- `2 × 1 KB` and `3 × 1 KB` identical files;
- 3, 5 and 100 small files;
- a mixed fixture with nested directories, a dotfile, an empty file and
  text/binary data;
- encrypted and public-token versions of the above;
- empty archives and empty directories;
- self-extracting archives;
- cross-decoding: this decoder reads original archives, and the original
  decoder reads archives produced here.

The multi-file batching rule was the last compression difference: the
reference reuses a buffer sized to the largest member and starts a new LZ
stream when the next member would not fit. Reproducing that rule makes the
small/similar-file cases byte-identical.
