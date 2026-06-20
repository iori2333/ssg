///
/// Loader - Resource loading and MIDI loop point references
///

#include "config.h"
#include "enemy.h"
#include "game/enum_array.h"
#include "game/format_bmp.h"
#include "game/graphics.h"
#include "game/hash.h"
#include "game/midi.h"
#include "game/snd.h"
#include "gian.h"
#include "lz_uty.h"
#include "music.h"
#include "platform/graphics_backend.h"
#include "platform/path.h"
#include "platform/thread.h"
#include "window_sys.h"
#include <cassert>

#include <utility>

#include "scripts_data.h"

static BYTE_BUFFER_BORROWED LoadEmbeddedScript(int filno) {
  for (size_t i = 0; i < embedded_script_count; i++) {
    if (embedded_scripts[i].index == filno) {
      return BYTE_BUFFER_BORROWED(embedded_scripts[i].data,
                                   embedded_scripts[i].size);
    }
  }
  return {};
}

// Hardcoded loop points for ZUN's original MIDI files
// ---------------------------------------------------

struct MID_LOOP_FOR_HASH {
  HASH hash;
  MID_LOOP loop;
};

// These loop points were detected using https://github.com/nmlgc/mly, with the
// command line
//
// 	<file.mid mly loop-find
//
// unless otherwise specified.
static constinit const auto LOOPS = HashesSorted<MID_LOOP_FOR_HASH, 66>({{
    // Original soundtrack (MUSIC.DAT)
    // -------------------------------
    // clang-format off

	// #01 秋霜玉　～ Clockworks
	{ .hash="04a44d2751f0cc155b9dbcabf7886bf999801f27f31ffee8566a18449e95ba4f"_B3, .loop={  .start=16557,  .end=94317 } },
	// #02 フォルスストロベリー
	{ .hash="e040b0ae4a9a36be23f88d5b0f66c6c5e60f0dfe648404c143f9095b75d1f036"_B3, .loop={  .start=75361, .end=183841 } },
	// #03 プリムローズシバ
	{ .hash="4727240794872e31d2591850b0d662af13fa25a77d9798c925cca61da114df43"_B3, .loop={  .start=31561,  .end=93001 } },
	// #04 幻想帝都
	{ .hash="d01300e4053bb07abc909fba83b0b1addb00104ee1ac3c9ad981e348b9c66622"_B3, .loop={   .start=4800,  .end=89280 } },
	// #05 ディザストラスジェミニ
	{ .hash="694543bea96390d6a6e85771e19561853515f456839f2672516516a6fbd074a4"_B3, .loop={   .start=1184,  .end=77984 } },
	// #06 華の幻想　紅夢の宙
	{ .hash="b58f7178ca77351f8d014efce152c50f6057adabbd969efe93610b65161f2a2e"_B3, .loop={  .start=73666, .end=181186 } },
	// #07 天空アーミー
	{ .hash="ac10269db4ad75f752fe8a52531238871baf8dcd87f6b2e90d130645e9bf0841"_B3, .loop={   .start=8640, .end=104640 } },
	// #08 スプートニク幻夜
	{ .hash="c8402fa2769f9341fb71db27d24550d5e534a652e71a2f21e45417a3a5764f3a"_B3, .loop={  .start=31606, .end=158326 } },
	// #09 機械サーカス　～ Reverie
	{ .hash="bde78f7de7bb640ce3e65589b7ccc79b4b702a32475c902eaf04170b6b1da538"_B3, .loop={    .start=961,  .end=93121 } },
	// #10 カナベラルの夢幻少女
	{ .hash="c804078d44983f3163220d51f22c035dddc19e78e5bbd129564bcb66447096ae"_B3, .loop={ .start=108481, .end=223681 } },
	// #11 魔法少女十字軍
	{ .hash="203b58f72fe30a532575de19ba58de11d0836d20c033eead82c4d4575f359ad1"_B3, .loop={   .start=8641, .end=127681 } },
	// #12 アンティークテラー
	{ .hash="0a0a45aa7bae3a6b7ebb082970960e1679c4ae4ce5c3e011d12e73899b99390f"_B3, .loop={   .start=1198, .end=140398 } },
	// #13 夢機械　～ Innocent Power
	{ .hash="052382df9912024fc1bcb11c60ca4555998d7206be2f7beefddef22e80ad5e3d"_B3, .loop={    .start=961,  .end=62401 } },
	// #14 幻想科学　～ Doll's Phantom
	{ .hash="0b8a97180a2229c42d22556c734668c8c9ccec9d1f32e0c936d7e7bb7a1fddab"_B3, .loop={  .start=75601, .end=185041 } },
	// #15 少女神性　～ Pandora's Box
	{ .hash="109d226ed66538074c2e15d4637631e77506321a66b644e627c77fd91d94ce3e"_B3, .loop={  .start=90510, .end=183630 } },
	// #16 シルクロードアリス
	{ .hash="46c876e99f605c5c8e5957d6adcecc2231c5e795f7284cca7f496a42a3758b2f"_B3, .loop={   .start=6721, .end=141121 } },
	// #17 魔女達の舞踏会　～ Magus
	{ .hash="a0a5ccd7c1b0e78c0365f5290fbf453ddc3d97298a1cf1f6ccc66365785d69c4"_B3, .loop={    .start=961,  .end=77761 } },
	// #18 二色蓮花蝶　～ Ancients
	{ .hash="67b2b4690067193ea1703569eaa0eeada59a6577ff10f4e0fb7f7b0e55559435"_B3, .loop={   .start=2880, .end=144960 } },
	// #19 ハーセルヴス
	{ .hash="f8cfe5c314ad1d8b7ed435cd876dae2b7750c00f03c1fa9a748428344feaa27e"_B3, .loop={  .start=86401, .end=101761 } },
	// #20 タイトルドメイド
	{ .hash="4931e9b4220ecd94d6006fec0805c882f3409840af15dc5a5336d36a65e4e70f"_B3, .loop={    .start=961,  .end=43201 } },
	// -------------------------------

	// Arranged soundtrack (https://www16.big.or.jp/~zun/html/music_old.html)
	// ----------------------------------------------------------------------
	// The first row corresponds to ZUN's original MIDIs, the second and third
	// rows to the edited non-echo and echo versions in the arranged soundtrack
	// BGM packs.

	// #01 秋霜玉　～ Clockworks
	{ .hash="02838ce71bcb2922278d86331af10caebb893ad4dbd7bf66771501dd16640fda"_B3, .loop={  .start=16321,  .end=94081 } },
	{ .hash="cf98509de1158239e06b0e4cba330b47ef10c9d6325aff13fb57f3e1c177308a"_B3, .loop={  .start=16321,  .end=94081 } },
	{ .hash="de54464adade220c4e682d833160b768cbdf7c86a74061064472bb9d2e700799"_B3, .loop={  .start=16321,  .end=94081 } },
	// #02 フォルスストロベリー
	//		<ssg_02.mid mly cut 466: | mly loop-unfold 240: | mly loop-find
	{ .hash="2cada452b1d1d1cbcff2e2f2430217fcbbcf4887e44dcbbd7c48d99ee39771f4"_B3, .loop={  .start=75361, .end=183841 } },
	{ .hash="1e6532a487f574b28d8540d46245884030812c2fba36d86722e6f6ba61feb8d8"_B3, .loop={  .start=75361, .end=183841 } },
	{ .hash="3fd235970a395feba43fac52b8843d225d2828cb9f178ac73833ae26b6e4492c"_B3, .loop={  .start=75361, .end=183841 } },
	// #03 プリムローズシバ
	//   Uses Reverb Macro 0 (Room 1), doesn't need an echo edit.
	{ .hash="63b0ed5d24e83b20ca603052e01a427477a89d9ec1903b188bd184eed09cf034"_B3, .loop={    .start=961,  .end=62401 } },
	{ .hash="ff336846794befbbf4188dcdc7496b3f029896b343b8978553527ecc324677fe"_B3, .loop={    .start=961,  .end=62401 } },
	// #04 幻想帝都
	//		<ssg_04.mid mly smf0 | mly loop-find
	{ .hash="2232a7c30b6bee76709342c62997fafab08a483fcde9d3595ba3b0e5d1819d18"_B3, .loop={   .start=4800,  .end=89280 } },
	{ .hash="8b5f5b50209c725fe7cc6b5506d8f57d5df91cf6fa135e88ca9c6f22dbfda2ea"_B3, .loop={   .start=4800,  .end=89280 } },
	{ .hash="b7c704265773d3f2efa64dbda283c6e0a45ac8d0e28075136d2152e01936a446"_B3, .loop={   .start=4800,  .end=89280 } },
	// #05 ディザストラスジェミニ
	//		<ssg_05.mid mly smf0 | mly cut 386: | mly loop-unfold 226: | mly loop-find
	{ .hash="26b734bec6e53b3ca02ce82d67f5bc2473f892ac7bc3b0f92703f66e55bfef17"_B3, .loop={  .start=62520, .end=139320 } },
	{ .hash="6cb425f3501c6c36dbf563f2f515a99e5cabd9772b198f8a5720a2871a178e5f"_B3, .loop={  .start=62520, .end=139320 } },
	{ .hash="eb2a7526fc9f7d1bbd86aeb99f5055b7d2f75d99240746fdab159645eb0fd1d3"_B3, .loop={  .start=62520, .end=139320 } },
	// #06 華の幻想　紅夢の宙
	//		<ssg_06.mid mly cut 494: | mly loop-unfold 270: | mly loop-find
	{ .hash="6e53bcd7c38a0e54d167d673ecbf0b47404442832c55b858c22edb1b63747939"_B3, .loop={  .start=73681, .end=181201 } },
	{ .hash="f0f4ce32eb747727ca160da4aab6b6432f9365d3b7c76ea411c089d79e87d2ef"_B3, .loop={  .start=73681, .end=181201 } },
	{ .hash="4f24b13ad086126c0825363aafd7f6036b042a828c0ef18cd7d42db7989e8300"_B3, .loop={  .start=73681, .end=181201 } },
	// #07 天空アーミー
	{ .hash="adba179af78f437b209eef82dc5f043087dfc42f8e584bd7688a57f510a3c26a"_B3, .loop={   .start=8640, .end=104640 } },
	{ .hash="9c08d7f4dee7bc344431bf7e1a1edbada0593b41adb9046e19dc890d8c1c4bf2"_B3, .loop={   .start=8640, .end=104640 } },
	{ .hash="f6ff4062e72a2ddf9cd04a1376446b7a87de65b29b30d8cc2370dc328e741dee"_B3, .loop={   .start=8640, .end=104640 } },
	// #08 スプートニク幻夜
	{ .hash="75087ec2ce1237d6dfe62e543d174b558343a8d2b09a0e7e767f0fead08644ea"_B3, .loop={  .start=31606, .end=158326 } },
	{ .hash="07f3114ca2cb648fbcddeb584d9c5c4522ccc05bfca2723267c91de6e87c50c9"_B3, .loop={  .start=31606, .end=158326 } },
	{ .hash="47dc15bc5bf03e6cd09703bd546a387f8bee58ce9c6b1f45946c74c98745adce"_B3, .loop={  .start=31606, .end=158326 } },
	// #09 機械サーカス　～ Reverie
	//		Every supposed loop modulates up by a semitone 16 measures before it
	//		ends and remains in that new key at the start of the next loop, so the
	//		piece technically doesn't loop at all. The original stays in G♯m
	//		throughout.

	// #10 カナベラルの夢幻少女
	//		<ssg_10.mid mly smf0 | mly loop-find
	{ .hash="d959e251a069bb198c8c79dcdd61dacfdbb63041bd163018bf4befd3331d30d1"_B3, .loop={    .start=961, .end=116161 } },
	{ .hash="5bb79c1e0c1fbf94f11a980bed4f43a55107eafa3d9f427f5043101a300b0b3d"_B3, .loop={    .start=961, .end=116161 } },
	{ .hash="58faba5c7f48a29dae657979b139a2fa55335652cff116ddb4a5fe5b25722a7f"_B3, .loop={    .start=961, .end=116161 } },
	// #11 魔法少女十字軍
	{ .hash="b70b6b7ed80b1c605a4b97b26a9d1c564ecab3699e980ba512b40e947260e77c"_B3, .loop={   .start=8641, .end=127681 } },
	{ .hash="1e04bea18f35790e7a8ada516b9f8f98f6ecf64163f604f7c0f8f0699a4952bc"_B3, .loop={   .start=8641, .end=127681 } },
	{ .hash="cdc8c3fbd717048a98cb5f92fabeef78a42892a3fa9ef91ea2c0396049aa659c"_B3, .loop={   .start=8641, .end=127681 } },
	// #12 アンティークテラー
	//		<ssg_12.mid mly cut 602: | mly loop-unfold 312: | mly loop-find
	{ .hash="15eca8c0ea2e60e752a93cbcb09043a7213b4b7993b8493d529a4cd440176c63"_B3, .loop={  .start=16081, .end=155281 } },
	{ .hash="b81e3526f2f6690664106f825d3af8d21a843526e1ceb32191f4e9e82298f1ec"_B3, .loop={  .start=16081, .end=155281 } },
	{ .hash="5b7dfe45bf71fd138f9b439c4fbfeb4f990a9d96c8abd5c41f80317d36875bb7"_B3, .loop={  .start=16081, .end=155281 } },
	// #13 夢機械　～ Innocent Power
	//		Has a unique ending section that starts in Gm and then modulates
	//		through Em and Fm before it fades out on F♯m.

	// #14 幻想科学　～ Doll's Phantom
	//		<ssg_14.mid mly cut 550: | mly loop-unfold 322: | mly loop-find
	{ .hash="3b94f8d5b87cfc2db73dee57744d2de5a4a0daa67cb8b122b0fef26f4fc5cd63"_B3, .loop={  .start=75601, .end=185041 } },
	{ .hash="acdd1bff05f7fdab9ba6569fd7512c58038be69aef26f7b59cf3ed9463f0cdf8"_B3, .loop={  .start=75601, .end=185041 } },
	{ .hash="bbd1bfefdd54506f92e1a3b5d77c61884ff11ae2e41512568d36fa7e1507ac1c"_B3, .loop={  .start=75601, .end=185041 } },
	// #15 少女神性　～ Pandora's Box
	//		<ssg_15.mid mly cut 522: | mly loop-unfold 328: | mly loop-find
	{ .hash="7728681d569155c34a71e28e3c75fc104d7b439835fb33fad2a94b6c46e8fe59"_B3, .loop={  .start=90220, .end=183340 } },
	{ .hash="581ee417e1b3aa578a26c2acc56dd2cffb3bdde722877cfe80d56486122463b4"_B3, .loop={  .start=90220, .end=183340 } },
	{ .hash="0e401c20edc6dbc29c7b9f968e4f9686e09218b22400035261f71aa3ef123551"_B3, .loop={  .start=90220, .end=183340 } },
	// #16 シルクロードアリス
	//		<ssg_16.mid mly cut 624: | mly loop-unfold 344: | mly loop-find
	{ .hash="384f15ef0325b4806f878e5144cccf2a502217705106e2b7e918a4106554e74c"_B3, .loop={  .start=30721, .end=165121 } },
	//	    <16.mid mly cut 614: | mly loop-unfold 334: | mly loop-find
	//   Uses custom reverb settings, doesn't need an echo edit.
	{ .hash="16732bcc91a128f1a9dd5595f99f8fc0b36a3c5781a50e9eeaecc491d08fb89c"_B3, .loop={  .start=25921, .end=160321 } },
	// #17 魔女達の舞踏会　～ Magus
	//		Has a unique 8-bar ending section that first appears in Cm and then
	//		loops in C♯m while fading out. The fade starts during the first loop,
	//		so we can't loop this section in-game (unless Romantique Tp were to
	//		re-record it without the fade).

	// #18 二色蓮花蝶　～ Ancients
	//   Uses Reverb Macro 1 (Room 2), doesn't need an echo edit.
	{ .hash="9a695e4659a1293e2d08fe287eb55f4dc6279beab2317004ae520029e552d602"_B3, .loop={   .start=3841, .end=145921 } },
	{ .hash="3adb2cc55b56bb773704ad564546a07b43df0c7bc4e80928dd03d4aad03a27ce"_B3, .loop={   .start=2881, .end=144961 } },
	// #19 ハーセルヴス
	//		Features a unique and very beautiful ending section. Let's let it
	//		fade out to silence for dramatic effect.
	{ .hash="0f29ba3a086246621cf0624638a044fc6c6622fee49744da66cf4ae8641d3475"_B3, .loop={     .start=-1,     .end=-1 } },
	{ .hash="276e1fa39fe52368986bed512b3a248034e17ab79b23b00122be3cd2ed3a6187"_B3, .loop={     .start=-1,     .end=-1 } },
	{ .hash="5e596cc00a3c99d73e301c858b085f34b13ad33be9417368eec185e7e53b5848"_B3, .loop={     .start=-1,     .end=-1 } },
    // clang-format on
    // ----------------------------------------------------------------------
}});

bool LoadMIDIWithPotentialLoop(BYTE_BUFFER_OWNED buf, const HASH &hash) {
  const auto ret = Mid_Load(std::move(buf));
  if (!ret) {
    return false;
  }
  const auto loop_it = LOOPS.Lookup(hash);
  if (loop_it != LOOPS.end()) {
    Mid_SetLoop(loop_it->loop);
  }
  return ret;
}
// ---------------------------------------------------

// Packfile loading //
bool GrpBMPLoadP(const PACKFILE_READ &in, fil_no_t filno, SURFACE_ID sid) {
  auto maybe_bmp = BMPLoad(in.MemExpand(filno));

  // If this fails, we're going to crash due to the uninitialized surface
  // anyway. Might as well announce it in debug mode.
  assert(maybe_bmp);

  // Still necessary to avoid generation of exception handling code in
  // Release mode.
  if (!maybe_bmp) {
    return false;
  }

  auto &bmp = maybe_bmp.value();
  return GrpSurface_Load(sid, std::move(bmp));
}

bool Snd_SELoadP(const PACKFILE_READ &in, fil_no_t filno, uint8_t id, int max) {
  return Snd_SELoad(in.MemExpand(filno), id, max);
}

bool LoadSound(const PACKFILE_READ &in);

// Packfile cache //
// -------------- //

namespace DAT {
enum class PACK_ID : uint8_t {
  MAP,
  IMAGES,
  MUSIC,
  SOUND,
  COUNT,
};

constexpr ENUMARRAY<std::string_view, PACK_ID> BASENAMES = {{
    "MAP.PAK",
    "IMAGES.PAK",
    "MUSIC.PAK",
    "SOUND.DAT",
}};

constexpr std::string_view NOT_FOUND = "\xe2\x98\x90 ";
constexpr std::string_view FOUND = "\xe2\x98\x91 ";

// Packfiles can be in these four states:
//
// 1) `THREAD` blank, `PACKFILE_READ` default-constructed:
//    Uninitialized, at the start of the process.
//
// 2) `THREAD` joinable, `PACKFILE_READ` default-constructed:
//    In the process of loading the packfile and running any post-loading
//    code.
//
// 3) `THREAD` blank, `PACKFILE_READ` empty:
//    Invalid packfile, after FilStartR() returned an error. Currently not
//    distinguished from 1), but we might in the future.
//
// 4) `THREAD` blank, `PACKFILE_READ` valid:
//    Checksums are validated, and the packfile is ready for extraction.
//
// A single `std::variant` per pack might look like the better way to
// represent this, but actually comes with two drawbacks:
// • Since `THREAD` rejoins on destruction, an attempt to change the
//   variant type from within the thread would deadlock the program.
// • Suddenly, we also have to handle the `valueless_by_exception` case in
//   the Packfile() function, which can then no longer return the neat
//   `const PACKFILE_READ&`.
class PACK {
private:
  PACKFILE_READ pack;
  THREAD load_thread;
  std::string filename_with_found_prefix;

public:
  bool Load(std::string_view path_data, PACK_ID id);
  [[nodiscard]] const std::string &FilenameWithFoundPrefix() const {
    return filename_with_found_prefix;
  }

  const PACKFILE_READ &BlockUntilLoaded() {
    if (load_thread.Joinable()) {
      load_thread.Join();
    }
    return pack;
  }

  void AbortLoading() {
    if (load_thread.Joinable()) {
      load_thread.Abort();
    }
  }
};
ENUMARRAY<PACK, PACK_ID> Packs;

// For MUSIC.PAK, we want to start asynchronously calculating all hashes
// once the process starts.
std::vector<HASH> MusicHashes;

struct MusicMeta {
  std::string title;   // UTF-8
  std::string comment; // UTF-8, \n-separated
};
std::vector<MusicMeta> MusicMetas;

const PACKFILE_READ &Packfile(PACK_ID id) {
  return Packs[id].BlockUntilLoaded();
}

void LoadMusicHashes(const PACKFILE_READ &in, const THREAD_STOP &st) {
  MusicNum = in.info.size();
  MusicHashes.reserve(MusicNum);
  MusicMetas.resize(MusicNum);

  for (auto i = 0; std::cmp_less(i, MusicNum); i++) {
    if (st) {
      break;
    }
    if (const auto file = in.MemExpand(i)) {
      if (st) {
        break;
      }
      auto cursor = file.cursor();

      std::string_view title, comment;
      if (const auto title_len_val =
              cursor.next<ENDIAN_LITTLE<uint32_t>>()) {
        const auto title_len = title_len_val.value()[0];
        if (cursor.cursor + title_len <= cursor.size()) {
          title = {reinterpret_cast<const char *>(&cursor[cursor.cursor]),
                   title_len};
          cursor.next<uint8_t>(title_len);
        }
      }
      if (const auto comment_len_val =
              cursor.next<ENDIAN_LITTLE<uint32_t>>()) {
        const auto comment_len = comment_len_val.value()[0];
        if (cursor.cursor + comment_len <= cursor.size()) {
          comment = {reinterpret_cast<const char *>(&cursor[cursor.cursor]),
                     comment_len};
          cursor.next<uint8_t>(comment_len);
        }
      }

      MusicMetas[i].title = title;
      MusicMetas[i].comment = comment;

      // Hash only the MIDI portion for loop point matching
      const auto *midi_data = file.get() + cursor.cursor;
      const auto midi_size = file.size() - cursor.cursor;
      MusicHashes.emplace_back(Hash({midi_data, midi_size}));
    } else {
      assert(!"Failure extracting BGM file?");
    }
  }
};

bool PACK::Load(std::string_view path_data, PACK_ID id) {
  if (pack) {
    return true;
  }
  if (filename_with_found_prefix.empty()) {
    static_assert(NOT_FOUND.size() == FOUND.size());
    const auto basename = BASENAMES[id];
    const auto cap = (NOT_FOUND.size() + path_data.size() + basename.size());
    filename_with_found_prefix.resize_and_overwrite(
        cap, [&](char *buf, size_t) {
          std::ranges::in_out_result p = {.in = path_data.begin(), .out = buf};
          p = std::ranges::copy(NOT_FOUND, p.out);
          p = std::ranges::copy(path_data, p.out);
          p = std::ranges::copy(basename, p.out);
          return (p.out - buf);
        });
  }
  auto *stream = SDL_IOFromFile(
      (filename_with_found_prefix.c_str() + NOT_FOUND.size()), "rb");
  if (stream == nullptr) {
    return false;
  }
  std::ranges::copy(FOUND, filename_with_found_prefix.begin());
  load_thread =
      ThreadStart([this, stream = stream, id](const THREAD_STOP &st) mutable {
        auto in = FilStartR(stream);
        if (id == PACK_ID::MUSIC) {
          LoadMusicHashes(in, st);
        } else if (id == PACK_ID::SOUND) {
          LoadSound(in);
        }
        pack = std::move(in);
      });
  return true;
}

bool Check() {
  const auto path_data = PathForData();
  bool ret = true;
  for (const auto i : std::views::iota(0U, BASENAMES.size())) {
    const auto id = Cast::down_enum<DAT::PACK_ID>(i);
    ret &= Packs[id].Load(path_data, id);
  }
  return ret;
}

// Load the n-th song //
bool LoadMusic(fil_no_t filno) {
  const auto &music = Packs[PACK_ID::MUSIC].BlockUntilLoaded();
  if (filno >= MusicHashes.size()) {
    return false;
  }

  auto raw = music.MemExpand(filno);
  if (!raw) {
    return false;
  }

  auto cursor = raw.cursor();
  if (const auto title_len_val = cursor.next<ENDIAN_LITTLE<uint32_t>>()) {
    const auto title_len = title_len_val.value()[0];
    cursor.next<uint8_t>(title_len);
  }
  if (const auto comment_len_val = cursor.next<ENDIAN_LITTLE<uint32_t>>()) {
    const auto comment_len = comment_len_val.value()[0];
    cursor.next<uint8_t>(comment_len);
  }

  const auto midi_size = raw.size() - cursor.cursor;
  BYTE_BUFFER_OWNED midi_buf(midi_size);
  if (midi_buf) {
    std::memcpy(midi_buf.get(), raw.get() + cursor.cursor, midi_size);
  }

  return LoadMIDIWithPotentialLoop(std::move(midi_buf), MusicHashes[filno]);
}

bool LoadMusicByHash(const HASH &hash) {
  Packs[PACK_ID::MUSIC].BlockUntilLoaded();
  const auto ret = std::ranges::find(MusicHashes, hash);
  if (ret == MusicHashes.cend()) {
    return false;
  }
  return LoadMusic(ret - MusicHashes.begin());
}
} // namespace DAT
// -------------- //

// Missing packfile screen
// -----------------------

namespace DAT_MISSING {
constexpr std::string_view TITLE = "Missing game data files";

bool FoundAll = false;

bool FnRecheck(MenuController & /*ctrl*/, INPUT_BITS key) {
  if ((key == KEY_BOMB) || (key == KEY_ESC)) {
    return false;
  }
  if ((Input_OptionKeyDelta(key) != 0) && DAT::Check()) {
    FoundAll = true;
    return false;
  }
  return true;
}

constexpr auto CENTER = MenuFlags::CENTER;
MenuLabel Title = {TITLE.data(), CENTER};
std::array<MenuItem, (DAT::BASENAMES.size() + 6)> Info = {{
    {},
    {},
    {},
    {},
    {},
    {},
    {"Must be provided from an original game copy.", "", CENTER},
    {},
    {"Recheck", "", FnRecheck, CENTER},
    {"Quit", "", CWinExitFn, CENTER},
}};
MenuDef Menu = {std::span(Info), [](MenuController &, bool) {}, &Title};
MenuController Window(Menu);

void Proc(bool &quit) {
  Window.Tick(Key_Data);
  if (!Window.Active()) {
    if (FoundAll) {
      SProjectInit();
    } else {
      quit = true;
    }
  }
  if (GameFlow.IsDraw()) {
    GrpBackend_Clear();
    Window.Draw();
    Grp_Flip();
  }
}

void Init() {
  for (const auto i : std::views::iota(0U, DAT::BASENAMES.size())) {
    const auto id = Cast::down_enum<DAT::PACK_ID>(i);
    const auto &title = DAT::Packs[id].FilenameWithFoundPrefix();
    Info[1 + i].Title = title.c_str();
  }
  const auto w = (std::max)(CWinTextExtent(TITLE).w,
                            std::ranges::max(std::views::transform(
                                Info, [](const auto &info) {
                                  return (CWinItemExtent(info.Title).w + 8);
                                })));

  Window.Init(w);
  GameFlow.game_main = Proc;
  GameFlow.current_state = GameState::External;
}
} // namespace DAT_MISSING
// -----------------------

void LoaderInit() {
  if (!DAT::Check()) {
    DAT_MISSING::Init();
  } else {
    // Call a different *Init() function here to quickly test a different
    // game state.
    SProjectInit();
  }
}

void LoaderCleanup() {
  // Cleanly shut down any loading threads that might not have joined yet
  for (auto &pack : DAT::Packs) {
    pack.AbortLoading();
  }
}

// Global variables //
uint32_t MusicNum = 0;                // Number of songs
FaceData face_data[FACE_MAX];         // For face graphics
EndingGrp ending_pic[ENDING_PIC_MAX]; // For endings

// Unused
// static BOOL			bIsBombPalette = FALSE;
// static PALETTEENTRY	tempPalette[256];

static PALETTE EnemyPalette;
PALETTE SProjectPalette;

// Secret function //
static void SetAnimeRect2(ANIME_DATA *anm, int x1, int y1, int x2, int y2);

static int LoadedStage = 0;

// Load graphics for a given stage //
bool LoadGraph(int stage) {
  //	bIsBombPalette = FALSE;
  LoadedStage = stage;
  const auto &graph = DAT::Packfile(DAT::PACK_ID::IMAGES);

  // For music room //
  if (stage == GRAPH_ID_MUSICROOM) {
    return (GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM) &&
            GrpBMPLoadP(graph, (19 + 4), SURFACE_ID::MUSIC));
  }
  // For title screen //
  if (stage == GRAPH_ID_TITLE) {
    return (GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM) &&
            GrpBMPLoadP(graph, (20 + 4), SURFACE_ID::TITLE));
    // LoadPaletteFrom(SURFACE_ID::ENEMY);
  }
  // For name registration screen //
  if (stage == GRAPH_ID_NAMEREGIST) {
    return (GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM) &&
            GrpBMPLoadP(graph, (21 + 4), SURFACE_ID::NAMEREG));
  }
  // For Seihou Project display //
  if (stage == GRAPH_ID_SPROJECT) {
    if (!GrpBMPLoadP(graph, 31, SURFACE_ID::SPROJECT)) {
      return false;
    }
    GrpBackend_PaletteGet(SProjectPalette);

    // if(!GrpBMPLoadP(graph, (21 + 4), SURFACE_ID::NAMEREG)) {
    // 	return false;
    // }
    return true;
  }
  // Load all ending images (including palette) //
  if (stage == GRAPH_ID_ENDING) {
    const auto &in = DAT::Packfile(DAT::PACK_ID::IMAGES);

    if (!GrpBMPLoadP(in, 32, SURFACE_ID::ENDING_CREDITS)) {
      return false;
    }
    for (auto i = 0; i < ENDING_PIC_MAX; i++) {
      if (!GrpBMPLoadP(in, (33 + i), (SURFACE_ID::ENDING_PIC + i))) {
        return false;
      }
      GrpBackend_PaletteGet(ending_pic[i].pal);
    }
    return true;
  }

  // For extra stage system //
  if (stage == GRAPH_ID_EXSTAGE) {
    if (!GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM)) {
      return false;
    }
    if (!GrpBMPLoadP(graph, (27 + 1), SURFACE_ID::ENEMY)) {
      return false;
    }
    GrpBackend_PaletteGet(EnemyPalette);

    if (!GrpBMPLoadP(graph, 27, SURFACE_ID::MAPCHIP)) {
      return false;
    }

    // For various reasons, it's here //
    if (!GrpBMPLoadP(graph, 26, SURFACE_ID::BOMBER)) {
      return false;
    }

    return true;
  }

  // For extra stage boss (1) //
  if (stage == GRAPH_ID_EXBOSS1) {
    if (!GrpBMPLoadP(graph, 29, SURFACE_ID::ENEMY)) {
      return false;
    }
    GrpBackend_PaletteGet(EnemyPalette);
    return true;
  }

  // For extra stage boss (2) //
  if (stage == GRAPH_ID_EXBOSS2) {
    if (!GrpBMPLoadP(graph, 30, SURFACE_ID::ENEMY)) {
      return false;
    }
    GrpBackend_PaletteGet(EnemyPalette);
    return true;
  }

  if ((stage < 0) || (stage > STAGE_MAX)) {
    return false;
  }

  // Map chips will be converted after loading //
  if (!GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM)) {
    return false;
  }
  if (!GrpBMPLoadP(graph, (stage + 0), SURFACE_ID::ENEMY)) {
    return false;
  }
  GrpBackend_PaletteGet(EnemyPalette);

  // Should really be STAGE_MAX
  // const fil_no_t MapChipID[STAGE_MAX] = { 7, 7, 8, 9, 10, 11 };
  const fil_no_t MapChipID[STAGE_MAX] = {7, 8, 9, 10, 11, 12};
  if (!GrpBMPLoadP(graph, MapChipID[stage - 1], SURFACE_ID::MAPCHIP)) {
    return false;
  }

  // For various reasons, it's here //
  return GrpBMPLoadP(graph, 26, SURFACE_ID::BOMBER);
}

void ReloadGraph() {
  assert(LoadedStage != 0);
  LoadGraph(LoadedStage);
}

bool LoadFace(uint8_t FaceID, uint8_t FileNo) {
  if (FaceID >= FACE_MAX) {
    return false;
  }
  const auto &graph = DAT::Packfile(DAT::PACK_ID::IMAGES);
  if (!GrpBMPLoadP(graph, (13 + FileNo), (SURFACE_ID::FACE + FaceID))) {
    return false;
  }

  // Save palette //
  GrpBackend_PaletteGet(face_data[FaceID].pal);

  return true;
}

// Set map palette
void LoadPaletteFromMAP() {
}

// Load ECL & SCL data into memory //
bool LoadStageData(uint8_t stage) {
  int i = 0;

  // Free memory! //
  Enemies.scl_now = nullptr;
  Enemies.ecl_head = {};
  Enemies.scl_head = {};
  Scroller.scroll.DataHead = nullptr;

  const auto &map_pack = DAT::Packfile(DAT::PACK_ID::MAP);

  // For extra stage system //
  if (stage == GRAPH_ID_EXSTAGE) {
    // ECL Load
    if ((Enemies.ecl_head = LoadEmbeddedScript(24)).data() == nullptr) {
      return false;
    }

    // SCL Load
    if ((Enemies.scl_head = LoadEmbeddedScript(25)).data() == nullptr) {
      return false;
    }

    // MapData Load
    if ((Scroller.scroll.DataHead = map_pack.MemExpand(12)) == nullptr) {
      return false;
    }
  } else if (stage == GRAPH_ID_ENDING) {
    // SCL Load
    if ((Enemies.scl_head = LoadEmbeddedScript(47)).data() == nullptr) {
      return false;
    }
    Enemies.scl_now = Enemies.scl_head.data();
    GameState.game_count = 0;
    return true;
  } else {
    // Load each data //
    if ((stage < 1) || (stage > STAGE_MAX)) {
      return false;
    }

    // ECL Load
    if ((Enemies.ecl_head =
             LoadEmbeddedScript(stage + 0 - 1)).data() == nullptr) {
      return false;
    }

    // SCL Load
    if ((Enemies.scl_head =
             LoadEmbeddedScript(stage + 6 - 1)).data() == nullptr) {
      return false;
    }

    // MapData Load
    if ((Scroller.scroll.DataHead = map_pack.MemExpand(stage - 1)) ==
        nullptr) {
      return false;
    }
  }

  // Initialize scroll variables //
  if (!Scroller.Init()) {
    return false;
  }

  // Initialize variables //
  Enemies.scl_now = Enemies.scl_head.data();
  GameState.game_count = 0;

  // Prepare animations //
  switch (stage) {
  case GRAPH_ID_EXSTAGE: // Extra stage graphics rectangles
    // Extra Boss I //
    // 00 : ■A  0-3   :  Normal (no wings) (10fps)
    Enemies.anime[0].SetSheet<4, 80>({.x = 0, .y = 0}, ANM_NORM);

    // 01 : ■B  4-7   :  Normal (with wings) (10fps)
    Enemies.anime[1].SetSheet<4, 80>({.x = 320, .y = 0}, ANM_NORM);

    // 02 : ■C  8-13  :  Attach wings (no wings -> with wings) (6fps)
    Enemies.anime[2].SetSheet<6, 80>({.x = 0, .y = 80}, ANM_STOP);

    // 03 : ■D  14-15 :  Attack with wings (stationary) (6fps)
    Enemies.anime[3].SetSheet<2, 80>({.x = 480, .y = 80}, ANM_NORM);

    // 04 : ■E  16-17 :  Move with wings (or moving attack) left (6fps)
    Enemies.anime[4].SetSheet<2, 80>({.x = 0, .y = 160}, ANM_NORM);

    // 05 : ■F  18-19 :  Move with wings (or moving attack) right (6fps)
    Enemies.anime[5].SetSheet<2, 80>({.x = 160, .y = 160}, ANM_NORM);

    // 06 : ■G  24-30 :  Gradual change (with wings -> no wings) (6fps)
    Enemies.anime[6].SetSheet<6, 80>({.x = 0, .y = 240}, ANM_STOP);

    // 07 : ■20 : Damage mask for normal state (with/without wings)
    Enemies.anime[7].SetSheet<1, 80>({.x = 320, .y = 160}, ANM_NORM);

    // 08 : ■21 : Damage mask for stationary attack
    Enemies.anime[8].SetSheet<1, 80>({.x = 400, .y = 160}, ANM_NORM);

    // 09 : ■22 : Damage mask for moving (left)
    Enemies.anime[9].SetSheet<1, 80>({.x = 480, .y = 160}, ANM_NORM);

    // 10 : ■23 : Damage mask for moving (right)
    Enemies.anime[10].SetSheet<1, 80>({.x = 560, .y = 160}, ANM_NORM);

    Enemies.anime[11].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 0))},
                                      ANM_NORM);
    Enemies.anime[12].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 1))},
                                      ANM_NORM);
    Enemies.anime[13].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 2))},
                                      ANM_NORM);
    Enemies.anime[14].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 3))},
                                      ANM_NORM);
    Enemies.anime[15].SetSheet<4, 32>({.x = 0, .y = (320 + (32 * 4))},
                                      ANM_NORM);
    Enemies.anime[16].SetSheet<4, 32>({.x = (32 * 4), .y = 320}, ANM_NORM);
    Enemies.anime[17].SetSheet<1, 32>({.x = (32 * 4), .y = (320 + (32 * 1))},
                                      ANM_NORM);

    // Extra Boss II //
    // 18 : ■A : Stopped animation (10-12fps)
    Enemies.anime[18].SetSheet<4, 80>({.x = 0, .y = 0}, ANM_NORM);

    // 19 : ■B : Normal phase attack 1 (?fps)
    Enemies.anime[19].SetSheet<4, 80>({.x = 320, .y = 0}, ANM_STOP);

    // 20 : ■C :  Normal phase attack 2 and high-speed move charge pose (6fps)
    Enemies.anime[20].SetSheet<2, 80>({.x = 0, .y = 80}, ANM_NORM);

    // 21 : ■D : Soul state (invincible, not hit by shots) (1-2 fps)
    // (160,80), (200,80), (240,80), (280,80)
    Enemies.anime[21].SetSheet<4, 40>({.x = 160, .y = 80}, ANM_NORM);

    // 22 : ■E : Damage mask (A)
    Enemies.anime[22].SetSheet<1, 80>({.x = 320, .y = 80}, ANM_NORM);

    // 23 : ■E : Damage mask (B)
    Enemies.anime[23].SetSheet<1, 80>({.x = 400, .y = 80}, ANM_NORM);

    // 24 : ■E : Damage mask (G)
    Enemies.anime[24].SetSheet<1, 80>({.x = 480, .y = 80}, ANM_NORM);

    // 25 : ■E : Damage mask (C)
    Enemies.anime[25].SetSheet<1, 80>({.x = 560, .y = 80}, ANM_NORM);

    // 26 : ■F : High-speed movement animation
    Enemies.anime[26].size = {.w = 80, .h = 80};
    Enemies.anime[26].n = 16;
    Enemies.anime[26].mode = ANM_DEG; // Glad it fits in 16 patterns...
    for (i = 0; i < 16; i++) {
      Enemies.anime[26].ptn[i] = PIXEL_LTWH{((i * 80) % 640), 160, 80, 80};
    }

    // 27 : ■G : Normal phase attack 2 charge pose and before/after warp
    Enemies.anime[27].SetSheet<1, 80>({.x = 560, .y = 320}, ANM_NORM);

    // 28-32 : Yin-Yang Orbs x5
    Enemies.anime[28].SetSheet<8, 32>({.x = 0, .y = 384}, ANM_NORM);
    Enemies.anime[29].SetSheet<8, 32>({.x = 0, .y = (384 + 32)}, ANM_NORM);
    Enemies.anime[30].SetSheet<8, 32>({.x = 0, .y = (384 + 64)}, ANM_NORM);
    Enemies.anime[31].SetSheet<8, 32>({.x = 256, .y = (384 + 32)}, ANM_NORM);
    Enemies.anime[32].SetSheet<8, 32>({.x = 256, .y = (384 + 64)}, ANM_NORM);

    Enemies.anime[33].SetSheetDeg<32>({.x = 0, .y = 0});
    Enemies.anime[34].SetSheetDeg<32>({.x = 0, .y = 32});
    Enemies.anime[35].SetSheetDeg<32>({.x = 0, .y = 64});
    Enemies.anime[36].SetSheetDeg<32>({.x = 0, .y = 96});
    Enemies.anime[37].SetSheetDeg<32>({.x = 0, .y = 128});

    // Laser projectile //
    Enemies.anime[38].size = {.w = 40, .h = 56};
    Enemies.anime[38].n = 1;
    Enemies.anime[38].mode = ANM_NORM;
    Enemies.anime[38].ptn[0] = PIXEL_LTWH{512, 0, 40, 56};

    // Mid-boss //
    Enemies.anime[39].size = {.w = 72, .h = 56};
    Enemies.anime[39].n = 2;
    Enemies.anime[39].mode = ANM_NORM;
    Enemies.anime[39].ptn[0] = {0, 424, 72, 480};
    Enemies.anime[39].ptn[1] = {72, 424, (72 * 2), 480};

    // Mid-boss hit //
    Enemies.anime[40].size = {.w = 72, .h = 56};
    Enemies.anime[40].n = 1;
    Enemies.anime[40].mode = ANM_NORM;
    Enemies.anime[40].ptn[0] = {(72 * 2), 424, (72 * 3), 480};

    // Laser projectile hit //
    Enemies.anime[41].size = {.w = 40, .h = 64};
    Enemies.anime[41].n = 1;
    Enemies.anime[41].mode = ANM_NORM;
    Enemies.anime[41].ptn[0] = PIXEL_LTWH{512, 56, 40, 56};

    // Mysterious light bullet //
    Enemies.anime[42].size = {.w = 24, .h = 24};
    Enemies.anime[42].n = 4;
    Enemies.anime[42].mode = ANM_NORM;
    Enemies.anime[42].ptn[0] = PIXEL_LTWH{552, 0, 24, 24};
    Enemies.anime[42].ptn[1] = PIXEL_LTWH{552, 24, 24, 24};
    Enemies.anime[42].ptn[2] = PIXEL_LTWH{552, 0, 24, 24};
    Enemies.anime[42].ptn[3] = PIXEL_LTWH{552, 48, 24, 24};
    break;

  case 1: // Stage 1 graphics rectangles
    // Mid-boss //
    Enemies.anime[0].size = {.w = 72, .h = 56};
    Enemies.anime[0].n = 2;
    Enemies.anime[0].mode = ANM_NORM;
    Enemies.anime[0].ptn[0] = {0, 0, 72, 56};
    Enemies.anime[0].ptn[1] = {72, 0, (72 * 2), 56};

    Enemies.anime[1].SetSheetDeg<32>({.x = 0, .y = (56 + 0)});
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = (56 + 32)});
    Enemies.anime[3].SetSheetDeg<32>({.x = 0, .y = (56 + 64)});
    Enemies.anime[4].SetSheetDeg<32>({.x = 0, .y = (56 + 96)});

    // Boss //
    Enemies.anime[5].size = {.w = 72, .h = 64};
    Enemies.anime[5].n = 1;
    Enemies.anime[5].mode = ANM_NORM;
    Enemies.anime[5].ptn[0] = {0, 184, 72, 248};

    // Mid-boss flash //
    Enemies.anime[6].size = {.w = 72, .h = 56};
    Enemies.anime[6].n = 2;
    Enemies.anime[6].mode = ANM_NORM;
    Enemies.anime[6].ptn[0] = {(72 * 2), 0, (72 * 3), 56};
    Enemies.anime[6].ptn[1] = {(72 * 3), 0, (72 * 4), 56};

    // Boss flash //
    Enemies.anime[7].size = {.w = 72, .h = 64};
    Enemies.anime[7].n = 1;
    Enemies.anime[7].mode = ANM_NORM;
    Enemies.anime[7].ptn[0] = {72, 184, (72 * 2), 248};
    break;

  case 2: // Stage 2 graphics rectangles
    Enemies.anime[0].SetSheetDeg<32>({.x = 0, .y = 0});
    Enemies.anime[1].SetSheetDeg<32>({.x = 0, .y = 32});
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = 64});
    Enemies.anime[3].SetSheetDeg<32>({.x = 0, .y = 96});
    Enemies.anime[4].SetSheetDeg<32>({.x = 0, .y = 128});

    Enemies.anime[5].size = {.w = 112, .h = 48};
    Enemies.anime[5].n = 1;
    Enemies.anime[5].mode = ANM_NORM;
    Enemies.anime[5].ptn[0] = {0, 160, 112, 208};

    Enemies.anime[6].size = {.w = 64, .h = 48};
    Enemies.anime[6].n = 1;
    Enemies.anime[6].mode = ANM_NORM;
    Enemies.anime[6].ptn[0] = {112, 160, 176, 208};

    // Mid-boss //
    Enemies.anime[7].size = {.w = 64, .h = 64};
    Enemies.anime[7].n = 1;
    Enemies.anime[7].mode = ANM_NORM;
    Enemies.anime[7].ptn[0] = {0, 208, 64, 272};

    // Boss wings //
    Enemies.anime[8].size = {.w = 112, .h = 48};
    Enemies.anime[8].n = 1;
    Enemies.anime[8].mode = ANM_NORM;
    Enemies.anime[8].ptn[0] = {176, 160, 288, 208};

    // Boss orb //
    Enemies.anime[9].size = {.w = 64, .h = 48};
    Enemies.anime[9].n = 1;
    Enemies.anime[9].mode = ANM_NORM;
    Enemies.anime[9].ptn[0] = {288, 160, 352, 208};

    // Boss flash 1 //
    Enemies.anime[10].size = {.w = 112, .h = 48};
    Enemies.anime[10].n = 1;
    Enemies.anime[10].mode = ANM_NORM;
    Enemies.anime[10].ptn[0] = {176, (160 + 48), 288, (208 + 48)};

    // Boss flash 2 //
    Enemies.anime[11].size = {.w = 64, .h = 48};
    Enemies.anime[11].n = 1;
    Enemies.anime[11].mode = ANM_NORM;
    Enemies.anime[11].ptn[0] = {288, (160 + 48), 352, (208 + 48)};

    // Mid-boss flash //
    Enemies.anime[12].size = {.w = 64, .h = 64};
    Enemies.anime[12].n = 1;
    Enemies.anime[12].mode = ANM_NORM;
    Enemies.anime[12].ptn[0] = {(0 + 64), 208, (64 + 64), 272};

    //                            // Winged Left-I //
    //                            Enemies.anime[10].size = { 104, 72 };
    //                            Enemies.anime[10].n      = 1;
    //                            Enemies.anime[10].mode   = ANM_NORM;
    //                            Enemies.anime[10].ptn[0] = { 184, 208, 288, 280 };
    //
    //                            // Winged Right-I //
    //                            Enemies.anime[11].size = { 104, 72 };
    //                            Enemies.anime[11].n      = 1;
    //                            Enemies.anime[11].mode   = ANM_NORM;
    //                            Enemies.anime[11].ptn[0] = { 288, 208, 392, 280 };
    //
    //                            // Winged Left-0 //
    //                            Enemies.anime[12].size = { 88, 80 };
    //                            Enemies.anime[12].n      = 1;
    //                            Enemies.anime[12].mode   = ANM_NORM;
    //                            Enemies.anime[12].ptn[0] = { 200, 280, 288, 360 };
    //
    //                            // Winged Right-0 //
    //                            Enemies.anime[13].size = { 88, 80 };
    //                            Enemies.anime[13].n      = 1;
    //                            Enemies.anime[13].mode   = ANM_NORM;
    //                            Enemies.anime[13].ptn[0] = { 288, 280, 376, 360 };
    SetAnimeRect2(Enemies.anime + 14, 0, 288, 159, 479);   // Clouds
    SetAnimeRect2(Enemies.anime + 15, 160, 384, 271, 479); //
    SetAnimeRect2(Enemies.anime + 16, 272, 368, 390, 478); //
    SetAnimeRect2(Enemies.anime + 17, 400, 368, 496, 431); //
    SetAnimeRect2(Enemies.anime + 18, 400, 160, 558, 359); //
    SetAnimeRect2(Enemies.anime + 19, 528, 48, 639, 160);  //
    SetAnimeRect2(Enemies.anime + 20, 560, 160, 639, 270); //
    SetAnimeRect2(Enemies.anime + 21, 576, 320, 639, 399); //
    break;

  case 3: // Lord Gates' stage
    Enemies.anime[0].size = {.w = 56, .h = 56};
    Enemies.anime[0].n = 16;
    Enemies.anime[0].mode = ANM_DEG;
    for (i = 0; i < 8; i++) {
      Enemies.anime[0].ptn[i] = PIXEL_LTWH{i * 56, 0, 56, 56};
    }
    for (i = 0; i < 8; i++) {
      Enemies.anime[0].ptn[i + 8] = PIXEL_LTWH{i * 56, 56, 56, 56};
    }

    Enemies.anime[1].SetSheetDeg<32>({.x = 0, .y = 112});
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = 144});
    Enemies.anime[3].SetSheetDeg<32>({.x = 0, .y = 176});

    Enemies.anime[4].size = {.w = 48, .h = 16};
    Enemies.anime[4].n = 2;
    Enemies.anime[4].mode = ANM_NORM;
    Enemies.anime[4].ptn[0] = PIXEL_LTWH{592, 0, 48, 16};
    Enemies.anime[4].ptn[1] = PIXEL_LTWH{592, 16, 48, 16};

    Enemies.anime[5].size = {.w = 48, .h = 16};
    Enemies.anime[5].n = 2;
    Enemies.anime[5].mode = ANM_NORM;
    Enemies.anime[5].ptn[0] = PIXEL_LTWH{592, 32, 48, 16};
    Enemies.anime[5].ptn[1] = PIXEL_LTWH{592, 48, 48, 16};

    // Boss (464,384)
    Enemies.anime[6].size = {.w = 11 * 16, .h = (5 * 16) + 8};
    Enemies.anime[6].n = 1;
    Enemies.anime[6].mode = ANM_NORM;
    Enemies.anime[6].ptn[0] = PIXEL_LTWH{464, 392, (11 * 16), ((5 * 16) + 8)};

    Enemies.anime[7].SetSheetDeg<32>({.x = 0, .y = 208});
    Enemies.anime[8].SetSheetDeg<40>({.x = 0, .y = 240});

    // Boss shadow //
    Enemies.anime[10].size = {.w = 196, .h = 100};
    Enemies.anime[10].n = 1;
    Enemies.anime[10].mode = ANM_NORM;
    Enemies.anime[10].ptn[0] = {444, 292, 640, 392};

    // Boss flash
    Enemies.anime[9].size = {.w = 128, .h = 76};
    Enemies.anime[9].n = 1;
    Enemies.anime[9].mode = ANM_NORM;
    Enemies.anime[9].ptn[0] = {512, 164, 640, 240};
    //	Enemies.anime[9].size = { (11 * 16), ((5 *16) + 8) };
    //            Enemies.anime[9].n      = 1;
    //            Enemies.anime[9].mode   = ANM_NORM;
    //            Enemies.anime[9].ptn[0] = PIXEL_LTWH{ 464, (392 - 88), (11 * 16),
    //       ((5 * 16)
    //       + 8) };
    break;

  case 4: // Marie's stage
    Enemies.anime[0].SetSheetDeg<32>({.x = 0, .y = 0});
    Enemies.anime[1].SetSheetDeg<32>({.x = 0, .y = 32});
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = 64});
    Enemies.anime[3].SetSheet<2, 32>({.x = 0, .y = 96}, ANM_NORM);
    Enemies.anime[4].SetSheetDeg<24>({.x = 64, .y = 96});
    Enemies.anime[5].SetSheetDeg<32>({.x = 0, .y = 128});

    //(304,296)-(640,480)
    Enemies.anime[6].size = {.w = (640 - 304), .h = (480 - 296)};
    Enemies.anime[6].n = 1;
    Enemies.anime[6].mode = ANM_NORM;
    Enemies.anime[6].ptn[0] = {304, 296, 640, 480};

    // Boss flash //
    Enemies.anime[7].size = {.w = (640 - 304 - 32),
                             .h = (480 - 296)}; // Note this
    Enemies.anime[7].n = 1;
    Enemies.anime[7].mode = ANM_NORM;
    Enemies.anime[7].ptn[0] = {0, 296, 304, 480};
    break;

  case 5:                                                // Master's stage
    Enemies.anime[0].SetSheetDeg<32>({.x = 0, .y = 0});  // Red one
    Enemies.anime[1].SetSheetDeg<32>({.x = 0, .y = 32}); // Red one appearance effect
    Enemies.anime[2].SetSheetDeg<32>({.x = 0, .y = 64}); // Blue one
    Enemies.anime[3].SetSheetDeg<32>({.x = 0, .y = 96}); // Green one
    Enemies.anime[4].SetSheetDeg<32>({.x = 0, .y = 128}); // Orange one
    Enemies.anime[5].SetSheet<4, 32>({.x = 512, .y = 0},
                                     ANM_NORM); // Reactor-equipped bit
    Enemies.anime[6].SetSheet<4, 32>({.x = 512, .y = 64},
                                     ANM_NORM); // Orange one appearance effect

    // Mid-boss options //
    Enemies.anime[7].size = {.w = 24, .h = 24};
    Enemies.anime[7].n = 4;
    Enemies.anime[7].mode = ANM_NORM;
    Enemies.anime[7].ptn[0] = PIXEL_LTWH{592, (96 + 0), 24, 24};
    Enemies.anime[7].ptn[1] = PIXEL_LTWH{592, (96 + 24), 24, 24};
    Enemies.anime[7].ptn[2] = PIXEL_LTWH{592, (96 + 0), 24, 24};
    Enemies.anime[7].ptn[3] = PIXEL_LTWH{592, (96 + 48), 24, 24};

    Enemies.anime[8].SetSheet<1>({.x = 512, .y = 96}, {.w = 80, .h = 72},
                                 ANM_NORM); // Sturdy mid-boss

    // Metallic master //
    Enemies.anime[9].SetSheet<1>({.x = 304, .y = 256}, {.w = 336, .h = 224},
                                 ANM_NORM);
    break;

  case 6:
    // Final boss (sitting -> standing) //
    Enemies.anime[0].size = {.w = 56, .h = 72};
    Enemies.anime[0].n = 6;
    Enemies.anime[0].mode = ANM_STOP;
    Enemies.anime[0].ptn[0] = PIXEL_LTWH{(56 * 0), 72, 56, 72};
    Enemies.anime[0].ptn[1] = PIXEL_LTWH{(56 * 1), 72, 56, 72};
    Enemies.anime[0].ptn[2] = PIXEL_LTWH{(56 * 2), 72, 56, 72};
    Enemies.anime[0].ptn[3] = PIXEL_LTWH{(56 * 3), 72, 56, 72};
    Enemies.anime[0].ptn[4] = PIXEL_LTWH{(56 * 4), 72, 56, 72};
    Enemies.anime[0].ptn[5] = PIXEL_LTWH{(56 * 5), 72, 56, 72};

    // Final boss (standing -> sitting) //
    Enemies.anime[1].size = {.w = 56, .h = 72};
    Enemies.anime[1].n = 6;
    Enemies.anime[1].mode = ANM_STOP;
    Enemies.anime[1].ptn[0] = PIXEL_LTWH{(56 * 5), 72, 56, 72};
    Enemies.anime[1].ptn[1] = PIXEL_LTWH{(56 * 4), 72, 56, 72};
    Enemies.anime[1].ptn[2] = PIXEL_LTWH{(56 * 3), 72, 56, 72};
    Enemies.anime[1].ptn[3] = PIXEL_LTWH{(56 * 2), 72, 56, 72};
    Enemies.anime[1].ptn[4] = PIXEL_LTWH{(56 * 1), 72, 56, 72};
    Enemies.anime[1].ptn[5] = PIXEL_LTWH{(56 * 0), 72, 56, 72};

    // Final boss (guard) //
    Enemies.anime[2].size = {.w = 56, .h = 72};
    Enemies.anime[2].n = 4;
    Enemies.anime[2].mode = ANM_NORM;
    Enemies.anime[2].ptn[0] = PIXEL_LTWH{(56 * 6), 72, 56, 72};
    Enemies.anime[2].ptn[1] = PIXEL_LTWH{(56 * 7), 72, 56, 72};
    Enemies.anime[2].ptn[2] = PIXEL_LTWH{(56 * 6), 72, 56, 72};
    Enemies.anime[2].ptn[3] = PIXEL_LTWH{(56 * 8), 72, 56, 72};

    // Final boss (attack 1) //
    Enemies.anime[3].size = {.w = 56, .h = 72};
    Enemies.anime[3].n = 9 + 1;
    Enemies.anime[3].mode = ANM_STOP;
    Enemies.anime[3].ptn[0] = PIXEL_LTWH{(56 * 0), 0, 56, 72};
    Enemies.anime[3].ptn[1] = PIXEL_LTWH{(56 * 1), 0, 56, 72};
    Enemies.anime[3].ptn[2] = PIXEL_LTWH{(56 * 2), 0, 56, 72};
    Enemies.anime[3].ptn[3] = PIXEL_LTWH{(56 * 3), 0, 56, 72};
    Enemies.anime[3].ptn[4] = PIXEL_LTWH{(56 * 4), 0, 56, 72};
    Enemies.anime[3].ptn[5] = PIXEL_LTWH{(56 * 5), 0, 56, 72};
    Enemies.anime[3].ptn[6] = PIXEL_LTWH{(56 * 6), 0, 56, 72};
    Enemies.anime[3].ptn[7] = PIXEL_LTWH{(56 * 7), 0, 56, 72};
    Enemies.anime[3].ptn[8] = PIXEL_LTWH{(56 * 8), 0, 56, 72};
    Enemies.anime[3].ptn[9] = PIXEL_LTWH{(56 * 5), 72, 56, 72}; // Added extra

    // Larva stage //
    SetAnimeRect2(Enemies.anime + 4, 432, 272, 632, 464);

    // Final boss (hope it looks like a jump) //
    Enemies.anime[5].size = {.w = 56, .h = 72};
    Enemies.anime[5].n = 11;
    Enemies.anime[5].mode = ANM_STOP;
    Enemies.anime[5].ptn[0] = PIXEL_LTWH{(56 * 0), 72, 56, 72};
    Enemies.anime[5].ptn[1] = PIXEL_LTWH{(56 * 1), 72, 56, 72};
    Enemies.anime[5].ptn[2] = PIXEL_LTWH{(56 * 2), 72, 56, 72};
    Enemies.anime[5].ptn[3] = PIXEL_LTWH{(56 * 3), 72, 56, 72};
    Enemies.anime[5].ptn[4] = PIXEL_LTWH{(56 * 4), 72, 56, 72};
    Enemies.anime[5].ptn[5] = PIXEL_LTWH{(56 * 5), 72, 56, 72};
    Enemies.anime[5].ptn[6] = PIXEL_LTWH{(56 * 4), 72, 56, 72};
    Enemies.anime[5].ptn[7] = PIXEL_LTWH{(56 * 3), 72, 56, 72};
    Enemies.anime[5].ptn[8] = PIXEL_LTWH{(56 * 2), 72, 56, 72};
    Enemies.anime[5].ptn[9] = PIXEL_LTWH{(56 * 1), 72, 56, 72};
    Enemies.anime[5].ptn[10] = PIXEL_LTWH{(56 * 0), 72, 56, 72};

    // Bits released in butterfly state? (Open) //
    Enemies.anime[6].size = {.w = 33, .h = 32};
    Enemies.anime[6].n = 10;
    Enemies.anime[6].mode = ANM_STOP;
    Enemies.anime[6].ptn[0] = PIXEL_LTWH{(32 * 0), 416, 32, 32};
    Enemies.anime[6].ptn[1] = PIXEL_LTWH{(32 * 1), 416, 32, 32};
    Enemies.anime[6].ptn[2] = PIXEL_LTWH{(32 * 2), 416, 32, 32};
    Enemies.anime[6].ptn[3] = PIXEL_LTWH{(32 * 3), 416, 32, 32};
    Enemies.anime[6].ptn[4] = PIXEL_LTWH{(32 * 4), 416, 32, 32};
    Enemies.anime[6].ptn[5] = PIXEL_LTWH{(32 * 5), 416, 32, 32};
    Enemies.anime[6].ptn[6] = PIXEL_LTWH{(32 * 0), 448, 32, 32};
    Enemies.anime[6].ptn[7] = PIXEL_LTWH{(32 * 1), 448, 32, 32};
    Enemies.anime[6].ptn[8] = PIXEL_LTWH{(32 * 2), 448, 32, 32};
    Enemies.anime[6].ptn[9] = PIXEL_LTWH{(32 * 3), 448, 32, 32};

    // Bits released in butterfly state? (Close) //
    Enemies.anime[7].size = {.w = 33, .h = 32};
    Enemies.anime[7].n = 10;
    Enemies.anime[7].mode = ANM_STOP;
    Enemies.anime[7].ptn[0] = PIXEL_LTWH{(32 * 3), 448, 32, 32};
    Enemies.anime[7].ptn[1] = PIXEL_LTWH{(32 * 2), 448, 32, 32};
    Enemies.anime[7].ptn[2] = PIXEL_LTWH{(32 * 1), 448, 32, 32};
    Enemies.anime[7].ptn[3] = PIXEL_LTWH{(32 * 0), 448, 32, 32};
    Enemies.anime[7].ptn[4] = PIXEL_LTWH{(32 * 5), 416, 32, 32};
    Enemies.anime[7].ptn[5] = PIXEL_LTWH{(32 * 4), 416, 32, 32};
    Enemies.anime[7].ptn[6] = PIXEL_LTWH{(32 * 3), 416, 32, 32};
    Enemies.anime[7].ptn[7] = PIXEL_LTWH{(32 * 2), 416, 32, 32};
    Enemies.anime[7].ptn[8] = PIXEL_LTWH{(32 * 1), 416, 32, 32};
    Enemies.anime[7].ptn[9] = PIXEL_LTWH{(32 * 0), 416, 32, 32};

    Enemies.anime[8].SetSheet<1>({.x = 0, .y = 368}, {.w = 48, .h = 48},
                                 ANM_NORM); // Sturdy mid-boss
    break;
  }

  return true;
}

// Define a single pattern graphic as an animation //
static void SetAnimeRect2(ANIME_DATA *anm, int x1, int y1, int x2, int y2) {
  anm->size = {.w = (x2 - x1), .h = (y2 - y1)};
  anm->n = 1;
  anm->mode = ANM_NORM;

  anm->ptn[0] = {x1, y1, x2, y2};
}

// Load the n-th song //
bool LoadMusic(unsigned int no) { return DAT::LoadMusic(no); }

bool LoadMusicByHash(const HASH &hash) { return DAT::LoadMusicByHash(hash); }

bool LoadMIDIBuffer(BYTE_BUFFER_OWNED buf) {
  const auto hash = Hash(buf.cursor());
  return LoadMIDIWithPotentialLoop(std::move(buf), hash);
}

// Load all Sound data //
bool LoadSound(const PACKFILE_READ &in) {
  // Initialize sound //
  // Disable if unavailable for any reason //
  if ((!ConfigDat.se_enabled) || !Snd_SEInit()) {
    ConfigDat.se_enabled = false;
    return false;
  }

  while (1) {
    if (!Snd_SELoadP(in, SOUND_ID_KEBARI, SOUND_ID_KEBARI, SNDMAX_KEBARI)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_TAME, SOUND_ID_TAME, SNDMAX_TAME)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_LASER, SOUND_ID_LASER, SNDMAX_LASER)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_LASER2, SOUND_ID_LASER2, SNDMAX_LASER2)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_BOMB, SOUND_ID_BOMB, SNDMAX_BOMB)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_SELECT, SOUND_ID_SELECT, SNDMAX_SELECT)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_HIT, SOUND_ID_HIT, SNDMAX_HIT)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_CANCEL, SOUND_ID_CANCEL, SNDMAX_CANCEL)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_WARNING, SOUND_ID_WARNING, SNDMAX_WARNING)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_SBLASER, SOUND_ID_SBLASER, SNDMAX_SBLASER)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_BUZZ, SOUND_ID_BUZZ, SNDMAX_BUZZ)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_MISSILE, SOUND_ID_MISSILE, SNDMAX_MISSILE)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_JOINT, SOUND_ID_JOINT, SNDMAX_JOINT)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_DEAD, SOUND_ID_DEAD, SNDMAX_DEAD)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_SBBOMB, SOUND_ID_SBBOMB, SNDMAX_SBBOMB)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_BOSSBOMB, SOUND_ID_BOSSBOMB,
                     SNDMAX_BOSSBOMB)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_ENEMYSHOT, SOUND_ID_ENEMYSHOT,
                     SNDMAX_ENEMYSHOT)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_HLASER, SOUND_ID_HLASER, SNDMAX_HLASER)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_TAMEFAST, SOUND_ID_TAMEFAST,
                     SNDMAX_TAMEFAST)) {
      break;
    }
    if (!Snd_SELoadP(in, SOUND_ID_WARP, SOUND_ID_WARP, SNDMAX_WARP)) {
      break;
    }

    // DirectSound can only apply volume onto loaded buffers.
    Snd_UpdateVolumes();
    return true;
  }

  ConfigDat.se_enabled = false;
  Snd_SECleanup();
  return false;
}

bool LoadSound() { return LoadSound(DAT::Packfile(DAT::PACK_ID::SOUND)); }

BYTE_BUFFER_OWNED LoadMusicRoomComment(int no) {
  if ((no < 0) || (no >= static_cast<int>(DAT::MusicMetas.size()))) {
    return nullptr;
  }
  const auto &comment = DAT::MusicMetas[no].comment;
  BYTE_BUFFER_OWNED buf(comment.size());
  if (buf) {
    std::memcpy(buf.get(), comment.data(), comment.size());
  }
  return buf;
}

std::string_view MusicTitle(unsigned int index) {
  if (index < DAT::MusicMetas.size()) {
    return DAT::MusicMetas[index].title;
  }
  return {};
}

std::string_view MusicComment(unsigned int index) {
  if (index < DAT::MusicMetas.size()) {
    return DAT::MusicMetas[index].comment;
  }
  return {};
}

BYTE_BUFFER_OWNED LoadDemo(int stage) {
  return DAT::Packfile(DAT::PACK_ID::MAP).MemExpand(stage - 1 + 6);
}
