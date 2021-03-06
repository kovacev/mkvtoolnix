/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the Quicktime & MP4 reader

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_INPUT_R_QTMP4_H
#define MTX_INPUT_R_QTMP4_H

#include "common/common_pch.h"

#include "common/ac3.h"
#include "common/codec.h"
#include "common/dts.h"
#include "common/fourcc.h"
#include "common/mm_io.h"
#include "input/qtmp4_atoms.h"
#include "merge/generic_reader.h"
#include "output/p_pcm.h"
#include "output/p_video_for_windows.h"

#define QTMP4_TFHD_BASE_DATA_OFFSET        0x000001
#define QTMP4_TFHD_SAMPLE_DESCRIPTION_ID   0x000002
#define QTMP4_TFHD_DEFAULT_DURATION        0x000008
#define QTMP4_TFHD_DEFAULT_SIZE            0x000010
#define QTMP4_TFHD_DEFAULT_FLAGS           0x000020
#define QTMP4_TFHD_DURATION_IS_EMPTY       0x010000
#define QTMP4_TFHD_DEFAULT_BASE_IS_MOOF    0x020000

#define QTMP4_TRUN_DATA_OFFSET             0x000001
#define QTMP4_TRUN_FIRST_SAMPLE_FLAGS      0x000004
#define QTMP4_TRUN_SAMPLE_DURATION         0x000100
#define QTMP4_TRUN_SAMPLE_SIZE             0x000200
#define QTMP4_TRUN_SAMPLE_FLAGS            0x000400
#define QTMP4_TRUN_SAMPLE_CTS_OFFSET       0x000800

#define QTMP4_FRAG_SAMPLE_FLAG_IS_NON_SYNC 0x00010000
#define QTMP4_FRAG_SAMPLE_FLAG_DEPENDS_YES 0x01000000

struct qt_durmap_t {
  uint32_t number;
  uint32_t duration;

  qt_durmap_t()
    : number{}
    , duration{}
  {
  }

  qt_durmap_t(uint32_t p_number, uint32_t p_duration)
    : number{p_number}
    , duration{p_duration}
  {
  }
};

struct qt_chunk_t {
  uint32_t samples;
  uint32_t size;
  uint32_t desc;
  uint64_t pos;

  qt_chunk_t()
    : samples{}
    , size{}
    , desc{}
    , pos{}
  {
  }

  qt_chunk_t(uint32_t p_size, uint64_t p_pos)
    : samples{}
    , size{p_size}
    , desc{}
    , pos{p_pos}
  {
  }
};

struct qt_chunkmap_t {
  uint32_t first_chunk;
  uint32_t samples_per_chunk;
  uint32_t sample_description_id;

  qt_chunkmap_t()
    : first_chunk{}
    , samples_per_chunk{}
    , sample_description_id{}
  {
  }
};

struct qt_editlist_t {
  int64_t  segment_duration{}, media_time{};
  uint16_t media_rate_integer{}, media_rate_fraction{};
};

struct qt_sample_t {
  int64_t  pts;
  uint32_t size;
  int64_t  pos;

  qt_sample_t()
    : pts{}
    , size{}
    , pos{}
  {
  }

  qt_sample_t(uint32_t p_size)
    : pts{}
    , size{p_size}
    , pos{}
  {
  }
};

struct qt_frame_offset_t {
  unsigned int count;
  int64_t offset;

  qt_frame_offset_t()
    : count{}
    , offset{}
  {
  }

  qt_frame_offset_t(unsigned int p_count, int64_t p_offset)
    : count{p_count}
    , offset{p_offset}
  {
  }
};

struct qt_index_t {
  int64_t file_pos, size;
  int64_t timecode, duration;
  bool    is_keyframe;

  qt_index_t()
    : file_pos{}
    , size{}
    , timecode{}
    , duration{}
    , is_keyframe{}
  {
  };

  qt_index_t(int64_t p_file_pos, int64_t p_size, int64_t p_timecode, int64_t p_duration, bool p_is_keyframe)
    : file_pos{p_file_pos}
    , size{p_size}
    , timecode{p_timecode}
    , duration{p_duration}
    , is_keyframe{p_is_keyframe}
  {
  }
};

struct qt_track_defaults_t {
  unsigned int sample_description_id, sample_duration, sample_size, sample_flags;

  qt_track_defaults_t()
    : sample_description_id{}
    , sample_duration{}
    , sample_size{}
    , sample_flags{}
  {
  }
};

struct qt_fragment_t {
  unsigned int track_id, sample_description_id, sample_duration, sample_size, sample_flags;
  uint64_t base_data_offset, moof_offset, implicit_offset;

  qt_fragment_t()
    : track_id{}
    , sample_description_id{}
    , sample_duration{}
    , sample_size{}
    , sample_flags{}
    , base_data_offset{}
    , moof_offset{}
    , implicit_offset{}
  {}
};

struct qt_random_access_point_t {
  bool num_leading_samples_known{};
  unsigned int num_leading_samples{};

  qt_random_access_point_t(bool p_num_leading_samples_known, unsigned int p_num_leading_samples)
    : num_leading_samples_known{p_num_leading_samples_known}
    , num_leading_samples{p_num_leading_samples}
  {
  }
};

struct qt_sample_to_group_t {
  uint32_t sample_count{}, group_description_index{};

  qt_sample_to_group_t(uint32_t p_sample_count, uint32_t p_group_description_index)
    : sample_count{p_sample_count}
    , group_description_index{p_group_description_index}
  {
  }
};

class qtmp4_reader_c;

struct qtmp4_demuxer_c {
  qtmp4_reader_c &m_reader;

  bool ok{}, m_tables_updated{}, m_timecodes_calculated{};

  char type;
  uint32_t id, container_id;
  fourcc_c fourcc;
  uint32_t pos;

  codec_c codec;
  pcm_packetizer_c::pcm_format_e m_pcm_format;

  int64_t time_scale, duration, global_duration, num_frames_from_trun;
  uint32_t sample_size;

  std::vector<qt_sample_t> sample_table;
  std::vector<qt_chunk_t> chunk_table;
  std::vector<qt_chunkmap_t> chunkmap_table;
  std::vector<qt_durmap_t> durmap_table;
  std::vector<uint32_t> keyframe_table;
  std::vector<qt_editlist_t> editlist_table;
  std::vector<qt_frame_offset_t> raw_frame_offset_table;
  std::vector<int32_t> frame_offset_table;
  std::vector<qt_random_access_point_t> random_access_point_table;
  std::unordered_map<uint32_t, std::vector<qt_sample_to_group_t> > sample_to_group_tables;

  std::vector<int64_t> timecodes, durations, frame_indices;

  std::vector<qt_index_t> m_index;
  std::vector<qt_fragment_t> m_fragments;

  int64_rational_c frame_rate;
  boost::optional<int64_t> m_use_frame_rate_for_duration;

  esds_t esds;
  bool esds_parsed;

  memory_cptr stsd;
  unsigned int stsd_non_priv_struct_size;
  uint32_t v_width, v_height, v_bitdepth, v_display_width_flt{}, v_display_height_flt{};
  uint16_t v_colour_primaries, v_colour_transfer_characteristics, v_colour_matrix_coefficients;
  std::deque<int64_t> references;
  uint32_t a_channels, a_bitdepth;
  float a_samplerate;
  int a_aac_profile, a_aac_output_sample_rate;
  bool a_aac_is_sbr, a_aac_config_parsed;
  ac3::frame_c m_ac3_header;
  mtx::dts::header_t m_dts_header;

  memory_cptr priv;

  bool warning_printed;

  int ptzr;

  std::string language;

  debugging_option_c m_debug_tables, m_debug_tables_full, m_debug_frame_rate, m_debug_headers, m_debug_editlists, m_debug_indexes, m_debug_indexes_full;

  qtmp4_demuxer_c(qtmp4_reader_c &reader)
    : m_reader(reader)
    , type{'?'}
    , id{0}
    , container_id{0}
    , pos{0}
    , m_pcm_format{pcm_packetizer_c::little_endian_integer}
    , time_scale{1}
    , duration{0}
    , global_duration{0}
    , num_frames_from_trun{}
    , sample_size{0}
    , esds_parsed{false}
    , stsd_non_priv_struct_size{}
    , v_width{0}
    , v_height{0}
    , v_bitdepth{0}
    , v_colour_primaries{2}
    , v_colour_transfer_characteristics{2}
    , v_colour_matrix_coefficients{2}
    , a_channels{0}
    , a_bitdepth{0}
    , a_samplerate{0.0}
    , a_aac_profile{0}
    , a_aac_output_sample_rate{0}
    , a_aac_is_sbr{false}
    , a_aac_config_parsed{false}
    , warning_printed{false}
    , ptzr{-1}
    , m_debug_tables{            "qtmp4_full|qtmp4_tables|qtmp4_tables_full"}
    , m_debug_tables_full{                               "qtmp4_tables_full"}
    , m_debug_frame_rate{"qtmp4|qtmp4_full|qtmp4_frame_rate"}
    , m_debug_headers{   "qtmp4|qtmp4_full|qtmp4_headers"}
    , m_debug_editlists{ "qtmp4|qtmp4_full|qtmp4_editlists"}
    , m_debug_indexes{         "qtmp4_full|qtmp4_indexes|qtmp4_indexes_full"}
    , m_debug_indexes_full{                             "qtmp4_indexes_full"}
  {
    memset(&esds, 0, sizeof(esds_t));
  }

  ~qtmp4_demuxer_c() {
  }

  void calculate_frame_rate();
  int64_t to_nsecs(int64_t value, boost::optional<int64_t> time_scale_to_use = boost::none);
  void calculate_timecodes();
  void adjust_timecodes(int64_t delta);

  bool update_tables();
  void apply_edit_list();

  void build_index();

  memory_cptr read_first_bytes(int num_bytes);

  bool is_audio() const;
  bool is_video() const;
  bool is_subtitles() const;
  bool is_chapters() const;
  bool is_unknown() const;

  void handle_stsd_atom(uint64_t atom_size, int level);
  void handle_audio_stsd_atom(uint64_t atom_size, int level);
  void handle_video_stsd_atom(uint64_t atom_size, int level);
  void handle_subtitles_stsd_atom(uint64_t atom_size, int level);
  void handle_colr_atom(memory_cptr const &atom_content, int level);

  void parse_audio_header_priv_atoms(uint64_t atom_size, int level);
  void parse_video_header_priv_atoms(uint64_t atom_size, int level);
  void parse_subtitles_header_priv_atoms(uint64_t atom_size, int level);

  void parse_aac_esds_decoder_config();

  bool verify_audio_parameters();
  bool verify_alac_audio_parameters();
  bool verify_dts_audio_parameters();
  bool verify_mp4a_audio_parameters();

  bool verify_video_parameters();
  bool verify_avc_video_parameters();
  bool verify_hevc_video_parameters();
  bool verify_mp4v_video_parameters();

  bool verify_subtitles_parameters();
  bool verify_vobsub_subtitles_parameters();

  void derive_track_params_from_ac3_audio_bitstream();
  void derive_track_params_from_dts_audio_bitstream();
  void derive_track_params_from_mp3_audio_bitstream();

  void set_packetizer_display_dimensions();
  void set_packetizer_colour_properties();

  boost::optional<int64_t> min_timecode() const;

  void determine_codec();

private:
  void build_index_chunk_mode();
  void build_index_constant_sample_size_mode();
  void dump_index_entries(std::string const &message) const;
  void mark_key_frames_from_key_frame_table();
  void mark_open_gop_random_access_points_as_key_frames();

  void calculate_timecodes_constant_sample_size();
  void calculate_timecodes_variable_sample_size();

  bool parse_esds_atom(mm_mem_io_c &memio, int level);
  uint32_t read_esds_descr_len(mm_mem_io_c &memio);
};
using qtmp4_demuxer_cptr = std::shared_ptr<qtmp4_demuxer_c>;

struct qt_atom_t {
  fourcc_c fourcc;
  uint64_t size;
  uint64_t pos;
  uint32_t hsize;

  qt_atom_t()
    : fourcc{}
    , size{}
    , pos{}
    , hsize{}
  {
  }

  qt_atom_t to_parent() {
    qt_atom_t parent;

    parent.fourcc = fourcc;
    parent.size   = size - hsize;
    parent.pos    = pos  + hsize;
    return parent;
  }
};

inline std::ostream &
operator <<(std::ostream &out,
            qt_atom_t const &atom) {
  out << (boost::format("<%1% at %2% size %3% hsize %4%>") % atom.fourcc % atom.pos % atom.size % atom.hsize);
  return out;
}

struct qtmp4_chapter_entry_t {
  std::string m_name;
  int64_t m_timecode;

  qtmp4_chapter_entry_t()
    : m_timecode{}
  {
  }

  qtmp4_chapter_entry_t(const std::string &name,
                        int64_t timecode)
    : m_name{name}
    , m_timecode{timecode}
  {
  }

  bool operator <(const qtmp4_chapter_entry_t &cmp) const {
    return m_timecode < cmp.m_timecode;
  }
};

class qtmp4_reader_c: public generic_reader_c {
private:
  std::vector<qtmp4_demuxer_cptr> m_demuxers;
  std::unordered_map<unsigned int, bool> m_chapter_track_ids;
  std::unordered_map<unsigned int, qt_track_defaults_t> m_track_defaults;

  int64_t m_time_scale;
  fourcc_c m_compression_algorithm;
  int m_main_dmx;

  unsigned int m_audio_encoder_delay_samples;

  uint64_t m_moof_offset, m_fragment_implicit_offset;
  qt_fragment_t *m_fragment;
  qtmp4_demuxer_c *m_track_for_fragment;

  bool m_timecodes_calculated;

  debugging_option_c m_debug_chapters, m_debug_headers, m_debug_tables, m_debug_tables_full, m_debug_interleaving, m_debug_resync;

  friend class qtmp4_demuxer_c;

public:
  qtmp4_reader_c(const track_info_c &ti, const mm_io_cptr &in);
  virtual ~qtmp4_reader_c();

  virtual file_type_e get_format_type() const {
    return FILE_TYPE_QTMP4;
  }

  virtual void read_headers();
  virtual file_status_e read(generic_packetizer_c *ptzr, bool force = false);
  virtual int get_progress();
  virtual void identify();
  virtual void create_packetizers();
  virtual void create_packetizer(int64_t tid);
  virtual void add_available_track_ids();

  static int probe_file(mm_io_c *in, uint64_t size);

protected:
  virtual void parse_headers();
  virtual void verify_track_parameters_and_update_indexes();
  virtual void calculate_timecodes();
  virtual boost::optional<int64_t> calculate_global_min_timecode() const;
  virtual qt_atom_t read_atom(mm_io_c *read_from = nullptr, bool exit_on_error = true);
  virtual bool resync_to_top_level_atom(uint64_t start_pos);
  virtual void parse_itunsmpb(std::string data);

  virtual void handle_cmov_atom(qt_atom_t parent, int level);
  virtual void handle_cmvd_atom(qt_atom_t parent, int level);
  virtual void handle_ctts_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_sgpd_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_sbgp_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_dcom_atom(qt_atom_t parent, int level);
  virtual void handle_hdlr_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_mdhd_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_mdia_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_minf_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_moov_atom(qt_atom_t parent, int level);
  virtual void handle_mvhd_atom(qt_atom_t parent, int level);
  virtual void handle_udta_atom(qt_atom_t parent, int level);
  virtual void handle_chpl_atom(qt_atom_t parent, int level);
  virtual void handle_meta_atom(qt_atom_t parent, int level);
  virtual void handle_ilst_atom(qt_atom_t parent, int level);
  virtual void handle_4dashes_atom(qt_atom_t parent, int level);
  virtual void handle_mvex_atom(qt_atom_t parent, int level);
  virtual void handle_trex_atom(qt_atom_t parent, int level);
  virtual void handle_moof_atom(qt_atom_t parent, int level, qt_atom_t const &moof_atom);
  virtual void handle_traf_atom(qt_atom_t parent, int level);
  virtual void handle_tfhd_atom(qt_atom_t parent, int level);
  virtual void handle_trun_atom(qt_atom_t parent, int level);
  virtual void handle_stbl_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_stco_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_co64_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_stsc_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_stsd_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_stss_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_stsz_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_sttd_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_stts_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_tkhd_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_trak_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_edts_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_elst_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_tref_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);

  virtual memory_cptr create_bitmap_info_header(qtmp4_demuxer_c &dmx, const char *fourcc, size_t extra_size = 0, const void *extra_data = nullptr);

  virtual void create_audio_packetizer_aac(qtmp4_demuxer_c &dmx);
  virtual bool create_audio_packetizer_ac3(qtmp4_demuxer_c &dmx);
  virtual bool create_audio_packetizer_alac(qtmp4_demuxer_c &dmx);
  virtual bool create_audio_packetizer_dts(qtmp4_demuxer_c &dmx);
  virtual void create_audio_packetizer_mp3(qtmp4_demuxer_c &dmx);
  virtual void create_audio_packetizer_passthrough(qtmp4_demuxer_c &dmx);
  virtual void create_audio_packetizer_pcm(qtmp4_demuxer_c &dmx);

  virtual void create_video_packetizer_mpeg1_2(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_mpeg4_p10(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_mpeg4_p2(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_mpegh_p2(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_standard(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_svq1(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_prores(qtmp4_demuxer_c &dmx);

  virtual void create_subtitles_packetizer_vobsub(qtmp4_demuxer_c &dmx);

  virtual void handle_audio_encoder_delay(qtmp4_demuxer_c &dmx);

  virtual std::string decode_and_verify_language(uint16_t coded_language);
  virtual void read_chapter_track();
  virtual void recode_chapter_entries(std::vector<qtmp4_chapter_entry_t> &entries);
  virtual void process_chapter_entries(int level, std::vector<qtmp4_chapter_entry_t> &entries);

  virtual void detect_interleaving();

  virtual std::string read_string_atom(qt_atom_t atom, size_t num_skipped);
};

#endif  // MTX_INPUT_R_QTMP4_H
