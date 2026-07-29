///
/// FLAC streaming support (adapted from thcrap's bgmmod module)
///

#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_STDIO

#include <cstdint>
#include <istream>
#include <limits>
#include <memory>

// GCC 15 throws `error: conflicting declaration 'typedef struct max_align_t
// max_align_t'` if this appears after a module import.
#include <dr_flac.h>

#include "audio/bgm_track.h"

namespace BGM {

// Callbacks
// ---------

// dr_flac's metadata-supporting opening function takes a single [user_data]
// pointer, just like the regular one without metadata support. This assumes
// that both the file stream and the target location for the metadata are
// reachable from the same pointer and share the same lifetime, which doesn't
// match our architecture of centralized metadata parsing. This forces us to
// load the intro file twice (once with metadata, and once without), and employ
// a bit of trickery to avoid duplicating the read and seek callbacks for the
// two cases...
template <class T>
concept CB_DATA = requires(T t) {
  { t.stream() } -> std::same_as<std::istream &>;
};

struct CB_DATA_STREAM {
  std::istream &input;

  std::istream &stream() { return input; }
};

struct CB_DATA_STREAM_AND_METADATA {
  std::istream &_stream;
  METADATA_CALLBACK on_metadata;

  std::istream &stream() { return _stream; }
};

template <CB_DATA CB>
static size_t CB_FLAC_Read(void *user_data, void *buf, size_t size) {
  if (static_cast<uintmax_t>(size) >
      static_cast<uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
    return 0;
  }
  auto &stream = static_cast<CB *>(user_data)->stream();
  stream.read(static_cast<char *>(buf), static_cast<std::streamsize>(size));
  return static_cast<size_t>(stream.gcount());
}

template <CB_DATA CB>
static drflac_bool32 CB_FLAC_Seek(void *user_data, int offset,
                                  drflac_seek_origin origin) {
  auto &stream = static_cast<CB *>(user_data)->stream();
  std::ios_base::seekdir whence;
  switch (origin) {
  case DRFLAC_SEEK_SET:
    whence = std::ios::beg;
    break;
  case DRFLAC_SEEK_CUR:
    whence = std::ios::cur;
    break;
  case DRFLAC_SEEK_END:
    whence = std::ios::end;
    break;
  }
  stream.clear();
  stream.seekg(offset, whence);
  return stream ? DRFLAC_TRUE : DRFLAC_FALSE;
}

template <CB_DATA CB>
static drflac_bool32 CB_FLAC_Tell(void *user_data, drflac_int64 *cursor) {
  auto &stream = static_cast<CB *>(user_data)->stream();
  const std::streamoff offset = stream.tellg();
  if (!cursor || offset < 0) {
    return DRFLAC_FALSE;
  }
  *cursor = offset;
  return DRFLAC_TRUE;
}

#pragma warning(suppress : 26461) // con.3
static void CB_FLAC_Meta(void *user_data, drflac_metadata *metadata) {
  if (metadata->type != DRFLAC_METADATA_BLOCK_TYPE_VORBIS_COMMENT) {
    return;
  }
  const auto *cb_data = static_cast<CB_DATA_STREAM_AND_METADATA *>(user_data);

  drflac_vorbis_comment_iterator it;
  drflac_init_vorbis_comment_iterator(
      &it, metadata->data.vorbis_comment.commentCount,
      metadata->data.vorbis_comment.pComments);
  const char *cmt_str = nullptr;
  drflac_uint32 cmt_len = 0;
  while ((cmt_str = drflac_next_vorbis_comment(&it, &cmt_len)) != nullptr) {
    OnVorbisComment(cb_data->on_metadata, {cmt_str, cmt_len});
  }
}
// ---------

typedef drflac_uint64 drflac_read_func_t(drflac *, drflac_uint64, void *);

struct PCM_PART_FLAC : public PCM_PART {
  std::unique_ptr<CB_DATA_STREAM> callback_data;
  drflac *ff;
  drflac_read_func_t &read_func;

  size_t PartDecodeSingle(std::span<std::byte> buf) override;
  void PartSeekToSample(size_t sample) override;

  PCM_PART_FLAC(std::unique_ptr<CB_DATA_STREAM> callback_data, drflac *ff,
                const PCM_FORMAT &pcmf, drflac_read_func_t &read_func)
      : PCM_PART(pcmf), callback_data(std::move(callback_data)), ff(ff),
        read_func(read_func) {}
  virtual ~PCM_PART_FLAC();
};

size_t PCM_PART_FLAC::PartDecodeSingle(std::span<std::byte> buf) {
  const auto sample_size = pcmf.SampleSize();
  const auto samples = (buf.size_bytes() / sample_size);
  return (read_func(ff, samples, buf.data()) * sample_size);
}

void PCM_PART_FLAC::PartSeekToSample(size_t sample) {
  drflac_seek_to_pcm_frame(ff, sample);
}

PCM_PART_FLAC::~PCM_PART_FLAC() { drflac_close(ff); }

std::unique_ptr<PCM_PART>
FLAC_Open(std::istream &stream, std::optional<METADATA_CALLBACK> on_metadata) {
  if (const auto &metadata_cb = on_metadata) {
    const std::streamoff initial_offset = stream.tellg();
    if (initial_offset < 0) {
      return nullptr;
    }
    CB_DATA_STREAM_AND_METADATA data = {stream, *metadata_cb};
    auto *ff =
        drflac_open_with_metadata(CB_FLAC_Read<CB_DATA_STREAM_AND_METADATA>,
                                  CB_FLAC_Seek<CB_DATA_STREAM_AND_METADATA>,
                                  CB_FLAC_Tell<CB_DATA_STREAM_AND_METADATA>,
                                  CB_FLAC_Meta, &data, nullptr);
    if (ff) {
      drflac_close(ff);
    }
    stream.clear();
    stream.seekg(initial_offset);
    if (!stream) {
      return nullptr;
    }
  }
  auto callback_data = std::make_unique<CB_DATA_STREAM>(stream);
  auto *ff =
      drflac_open(CB_FLAC_Read<CB_DATA_STREAM>, CB_FLAC_Seek<CB_DATA_STREAM>,
                  CB_FLAC_Tell<CB_DATA_STREAM>, callback_data.get(), nullptr);
  if (!ff) {
    return nullptr;
  }
  const auto output_format =
      ((ff->bitsPerSample <= 16) ? PCM_SAMPLE_FORMAT::S16
                                 : PCM_SAMPLE_FORMAT::S32);
  const auto read_func =
      ((ff->bitsPerSample <= 16)
           ? reinterpret_cast<drflac_read_func_t &>(drflac_read_pcm_frames_s16)
           : reinterpret_cast<drflac_read_func_t &>(
                 drflac_read_pcm_frames_s32));
  PCM_FORMAT pcmf = {ff->sampleRate, ff->channels, output_format};
  return std::make_unique<PCM_PART_FLAC>(std::move(callback_data), ff, pcmf,
                                         *read_func);
}

} // namespace BGM
