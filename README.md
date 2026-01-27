# potamos
A c++ io-stream wrapper on ffmpeg to allow easy custom pipeline processing


# Concepts

## Mux and Demux

Mux and Demux represent multimedia containers.

### Demux

binary stream -> demux -> media streams
```c++
Demux(istream)
    .stream(n)
```

### Mux

media streams -> mux -> binary stream

```c++
Mux(ostream, vector<FrameOStream>)
    .write()
    .seekp()
```

## Frame(I|O)Stream

FrameStream represents encoded media stream - a sequence of encoded frames.

The frames must be processed with Decoder to get raw media.

```c++
FrameOStream(MediaOstream, Codec)
    .header()
    .
```

```c++
FrameIStream(AudioOstream, Codec)
    .header()
    .
```

## MediaStream

### AudioStream

Stream of audio samples.

AudioStreamReader(FrameStream, Codec)

### VideoStream

Stream of video frames.

VideoIStream(FrameIStream, Codec)

### SubtitleStream

SubtitleStreamReader(FrameStream, Codec)

## Coder and Decoder

Coder and Decoder represents coding/decoding of media streams.

MediaOStream -> Coder -> FrameOStream
FrameIStream -> Decoder -> MediaIStream

```c++
AudioIStream& audio_stream = Decoder(FrameIStream)

FrameOStream& coded_audio = Coder(audio_stream, Codec::mp3)
```

