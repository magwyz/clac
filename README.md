# 💩 CLAC: Crappy Lossless Audio Codec 💩

## What is this shit?

CLAC is a simple and limited lossless audio codec. It curently only supports as input and output **16-bit PCM mono WAV** files.
It is also significantly slower than FLAC reference encoder. However, it can achieve slightly better compression performances.

CLAC was written to be the support of a tutorial presentation about lossless audio compression. The presentation slides are also on this respository.

## Compilation

You should have cmake installed.

```
mkdir build
cd build
cmake ../
make
```

## Usage

```
clac -e/-d INPUT OUTPUT
```

### Encoding

```
clac -e input.wav output.clac
```

### Decoding

```
clac -d input.clac output.wav
```

