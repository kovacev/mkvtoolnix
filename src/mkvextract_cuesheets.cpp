/*
  mkvextract -- extract tracks from Matroska files into other files

  mkvextract_cuesheets.cpp

  Written by Moritz Bunkus <moritz@bunkus.org>

  Distributed under the GPL
  see the file COPYING for details
  or visit http://www.gnu.org/copyleft/gpl.html
*/

/*!
    \file
    \version $Id$
    \brief extracts chapters and tags as CUE sheets from Matroska files
    \author Moritz Bunkus <moritz@bunkus.org>
*/

#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#if defined(COMP_MSC)
#include <assert.h>
#else
#include <unistd.h>
#endif

#include <iostream>
#include <string>
#include <vector>

extern "C" {
#include <avilib.h>
}

#include <ebml/EbmlHead.h>
#include <ebml/EbmlSubHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVoid.h>
#include <matroska/FileKax.h>

#include <matroska/KaxChapters.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxClusterData.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTag.h>
#include <matroska/KaxTags.h>

#include "chapters.h"
#include "common.h"
#include "commonebml.h"
#include "matroska.h"
#include "mkvextract.h"
#include "mm_io.h"

using namespace libmatroska;
using namespace std;

#define FINDFIRST(p, c) (static_cast<c *> \
  (((EbmlMaster *)p)->FindFirstElt(c::ClassInfos, false)))

static string
get_simple_tag(const char *name,
               EbmlMaster &m) {
  int i;
  UTFstring uname;
  string rvalue;

  uname = cstrutf8_to_UTFstring(name);

  for (i = 0; i < m.ListSize(); i++)
    if (EbmlId(*m[i]) == KaxTagSimple::ClassInfos.GlobalId) {
      KaxTagName *tname;
      UTFstring utname;

      tname = FINDFIRST(m[i], KaxTagName);
      if (tname == NULL)
        continue;
      utname = UTFstring(*static_cast<EbmlUnicodeString *>(tname));
      if (utname == uname) {
        KaxTagString *tvalue;
        tvalue = FINDFIRST(m[i], KaxTagString);
        if (tvalue != NULL) {
          char *value;

          value =
            UTFstring_to_cstrutf8(UTFstring(*static_cast<EbmlUnicodeString *>
                                            (tvalue)));
          rvalue = value;
          safefree(value);
          return rvalue;
        } else {
          return "";
        }
      }
    }

  for (i = 0; i < m.ListSize(); i++)
    if ((EbmlId(*m[i]) == KaxTag::ClassInfos.GlobalId) ||
        (EbmlId(*m[i]) == KaxTagSimple::ClassInfos.GlobalId)) {
      rvalue = get_simple_tag(name, *static_cast<EbmlMaster *>(m[i]));
      if (rvalue != "")
        return rvalue;
    }

  return "";
}

static KaxTag *
find_tag_for_track(int idx,
                   EbmlMaster &m) {
  string sidx;
  int i;

  sidx = mxsprintf("%d", idx);

  for (i = 0; i < m.ListSize(); i++)
    if ((EbmlId(*m[i]) == KaxTag::ClassInfos.GlobalId) &&
        (get_simple_tag("TRACKNUMBER", *static_cast<EbmlMaster *>(m[i])) ==
         sidx))
      return static_cast<KaxTag *>(m[i]);

  return NULL;
}

static string
get_global_from_tags(int num_tracks,
                     const char *name,
                     KaxTags &tags) {
  KaxTag *tag;
  string global_value, value;
  int i;

  tag = find_tag_for_track(1, tags);
  if (tag != NULL)
    global_value = get_simple_tag(name, *tag);
  if (global_value == "")
    return "";
  for (i = 2; i <= num_tracks; i++) {
    tag = find_tag_for_track(i, tags);
    if (tag != NULL) {
      value = get_simple_tag(name, *tag);
      if ((value != "") && (value != global_value))
        return "";
    }
  }

  return global_value;
}

static int64_t
get_chapter_start(KaxChapterAtom &atom) {
  KaxChapterTimeStart *start;

  start = FINDFIRST(&atom, KaxChapterTimeStart);
  if (start == NULL)
    return -1;
  return uint64(*static_cast<EbmlUInteger *>(start));
}

static string
get_chapter_name(KaxChapterAtom &atom) {
  KaxChapterDisplay *display;
  KaxChapterString *name;

  display = FINDFIRST(&atom, KaxChapterDisplay);
  if (display == NULL)
    return "";
  name = FINDFIRST(display, KaxChapterString);
  if (name == NULL)
    return "";
  return UTFstring_to_cstrutf8(UTFstring(*name));
}

static int64_t
get_chapter_index(int idx,
                  KaxChapterAtom &atom) {
  int i;
  string sidx;

  sidx = mxsprintf("INDEX 0%d", idx);
  for (i = 0; i < atom.ListSize(); i++)
    if ((EbmlId(*atom[i]) == KaxChapterAtom::ClassInfos.GlobalId) &&
        (get_chapter_name(*static_cast<KaxChapterAtom *>(atom[i])) == sidx))
      return get_chapter_start(*static_cast<KaxChapterAtom *>(atom[i]));

  return -1;
}

#define print_if_global(name, format) \
  _print_if_global(name, format, chapters.ListSize(), tags)
static void
_print_if_global(const char *name,
                 const char *format,
                 int num_entries,
                 KaxTags &tags) {
  string global;

  global = get_global_from_tags(num_entries, name, tags);
  if (global != "")
    mxinfo(format, global.c_str());
}

#define print_if_available(name, format) \
  _print_if_available(name, format, *tag)
static void
_print_if_available(const char *name,
                    const char *format,
                    KaxTag &tag) {
  string value;

  value = get_simple_tag(name, tag);
  if (value != "")
    mxinfo(format, value.c_str());
}

static void
print_cuesheet(const char *file_name,
               KaxChapters &chapters,
               KaxTags &tags) {
  KaxTag *tag;
  string s;
  int i;
  int64_t index_00, index_01;

  if (chapters.ListSize() == 0)
    return;

  print_if_global("DATE", "REM DATE %s\n");
  print_if_global("GENRE", "REM GENRE %s\n");
  print_if_global("DISCID", "REM DISCID %s\n");
  print_if_global("ARTIST", "PERFORMER \"%s\"\n");
  print_if_global("ALBUM", "TITLE \"%s\"\n");

  mxinfo("FILE \"%s.RENAMEME\" WAVE\n", file_name);

  for (i = 0; i < chapters.ListSize(); i++) {
    mxinfo(" TRACK %02d AUDIO\n", i + 1);
    tag = find_tag_for_track(i + 1, tags);
    if (tag != NULL) {
      KaxChapterAtom &atom = 
        *static_cast<KaxChapterAtom *>(chapters[i]);
      print_if_available("TITLE", "   TITLE \"%s\"\n");
      print_if_available("ARTIST", "   PERFORMER \"%s\"\n");
      print_if_available("ISRC", "   ISRC %s\n");
      index_00 = get_chapter_index(0, atom);
      index_01 = get_chapter_index(1, atom);
      if (index_01 == -1) {
        index_01 = get_chapter_start(atom);
        if (index_01 == -1)
          index_01 = 0;
      }
      if (index_00 != -1)
        mxinfo("   INDEX 00 %02lld:%02lld:%02lld\n", 
               index_00 / 1000000 / 1000 / 60,
               (index_00 / 1000000 / 1000) % 60,
               irnd((double)(index_00 % 1000000000ll) * 75.0 / 1000000000.0));
      mxinfo("   INDEX 01 %02lld:%02lld:%02lld\n", 
             index_01 / 1000000 / 1000 / 60,
             (index_01 / 1000000 / 1000) % 60,
             irnd((double)(index_01 % 1000000000ll) * 75.0 / 1000000000.0));
    }
  }
}

void
extract_cuesheets(const char *file_name) {
  int upper_lvl_el;
  // Elements for different levels
  EbmlElement *l0 = NULL, *l1 = NULL, *l2 = NULL;
  EbmlStream *es;
  mm_io_c *in;
  KaxChapters *all_chapters;
  KaxTags *all_tags;

  // open input file
  try {
    in = new mm_io_c(file_name, MODE_READ);
  } catch (std::exception &ex) {
    show_error(_("The file '%s' could not be opened for reading (%s)."),
               file_name, strerror(errno));
    return;
  }

  all_chapters = new KaxChapters;
  all_tags = new KaxTags;

  try {
    es = new EbmlStream(*in);

    // Find the EbmlHead element. Must be the first one.
    l0 = es->FindNextID(EbmlHead::ClassInfos, 0xFFFFFFFFL);
    if (l0 == NULL) {
      show_error(_("Error: No EBML head found."));
      delete es;

      return;
    }
      
    // Don't verify its data for now.
    l0->SkipData(*es, l0->Generic().Context);
    delete l0;

    while (1) {
      // Next element must be a segment
      l0 = es->FindNextID(KaxSegment::ClassInfos, 0xFFFFFFFFFFFFFFFFLL);
      if (l0 == NULL) {
        show_error(_("No segment/level 0 element found."));
        return;
      }
      if (EbmlId(*l0) == KaxSegment::ClassInfos.GlobalId) {
        show_element(l0, 0, _("Segment"));
        break;
      }

      show_element(l0, 0, _("Next level 0 element is not a segment but %s"),
                   l0->Generic().DebugName);

      l0->SkipData(*es, l0->Generic().Context);
      delete l0;
    }

    upper_lvl_el = 0;
    // We've got our segment, so let's find the chapters
    l1 = es->FindNextElement(l0->Generic().Context, upper_lvl_el, 0xFFFFFFFFL,
                             true, 1);
    while ((l1 != NULL) && (upper_lvl_el <= 0)) {

      if (EbmlId(*l1) == KaxChapters::ClassInfos.GlobalId) {
        KaxChapters &chapters = *static_cast<KaxChapters *>(l1);
        chapters.Read(*es, KaxChapters::ClassInfos.Context, upper_lvl_el, l2,
                      true);
        if (verbose > 0)
          debug_dump_elements(&chapters, 0);

        while (chapters.ListSize() > 0) {
          if (EbmlId(*chapters[0]) == KaxEditionEntry::ClassInfos.GlobalId) {
            KaxEditionEntry &entry =
              *static_cast<KaxEditionEntry *>(chapters[0]);
            while (entry.ListSize() > 0) {
              if (EbmlId(*entry[0]) == KaxChapterAtom::ClassInfos.GlobalId)
                all_chapters->PushElement(*entry[0]);
              entry.Remove(0);
            }
          }
          chapters.Remove(0);
        }

      } else if (EbmlId(*l1) == KaxTags::ClassInfos.GlobalId) {
        KaxTags &tags = *static_cast<KaxTags *>(l1);
        tags.Read(*es, KaxTags::ClassInfos.Context, upper_lvl_el, l2, true);
        if (verbose > 0)
          debug_dump_elements(&tags, 0);

        while (tags.ListSize() > 0) {
          all_tags->PushElement(*tags[0]);
          tags.Remove(0);
        }

      } else
        l1->SkipData(*es, l1->Generic().Context);

      if (!in_parent(l0)) {
        delete l1;
        break;
      }

      if (upper_lvl_el > 0) {
        upper_lvl_el--;
        if (upper_lvl_el > 0)
          break;
        delete l1;
        l1 = l2;
        continue;

      } else if (upper_lvl_el < 0) {
        upper_lvl_el++;
        if (upper_lvl_el < 0)
          break;

      }

      l1->SkipData(*es, l1->Generic().Context);
      delete l1;
      l1 = es->FindNextElement(l0->Generic().Context, upper_lvl_el,
                               0xFFFFFFFFL, true);

    } // while (l1 != NULL)

    print_cuesheet(file_name, *all_chapters, *all_tags);

    delete all_chapters;
    delete all_tags;

    delete l0;
    delete es;
    delete in;

  } catch (exception &ex) {
    show_error(_("Caught exception: %s"), ex.what());
    delete in;

    return;
  }
}
