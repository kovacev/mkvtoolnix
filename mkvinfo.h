/*
  mkvmerge -- utility for splicing together matroska files
      from component media subtypes

  mkvinfo.h

  Written by Moritz Bunkus <moritz@bunkus.org>

  Distributed under the GPL
  see the file COPYING for details
  or visit http://www.gnu.org/copyleft/gpl.html
*/

/*!
    \file
    \version \$Id: mkvinfo.h,v 1.2 2003/05/28 11:31:55 mosu Exp $
    \brief definition of global variables and functions
    \author Moritz Bunkus <moritz@bunkus.org>
*/


#ifndef __MKVINFO_H
#define __MKVINFO_H

#include "config.h"

#ifdef HAVE_WXWINDOWS
#include <wx/wx.h>
#include <wx/treectrl.h>
#endif

#define NAME "MKVInfo"

void parse_args(int argc, char **argv, char *&file_name, bool &use_gui);
int console_main(int argc, char **argv);
bool process_file(const char *file_name);

extern bool use_gui;

#ifdef HAVE_WXWINDOWS

class mi_app: public wxApp {
public:
  virtual bool OnInit();
};

class mi_frame: public wxFrame {
private:
  wxMenu *menu_options, *menu_file;
  bool show_all_elements, show_all_elements_expanded;
  bool file_open;
  int last_percent;
  wxString current_file;

  wxTreeCtrl *tree;
  wxTreeItemId item_ids[10];

public:
  mi_frame(const wxString &title, const wxPoint &pos, const wxSize &size,
           long style = wxDEFAULT_FRAME_STYLE);

  void open_file(const char *file_name);
  void show_progress(int percent);
  void add_item(int level, const char *text);

protected:
  void on_file_open(wxCommandEvent &event);
  void on_file_savetext(wxCommandEvent &event);
  void on_file_quit(wxCommandEvent &event);
  void on_options_showall(wxCommandEvent &event);
  void on_options_expandall(wxCommandEvent &event);
  void on_help_about(wxCommandEvent &event);

  void expand_items();
  void expand_all_items(wxTreeItemId &root, bool expand = true);

private:
  DECLARE_EVENT_TABLE()
};

extern mi_frame *frame;

DECLARE_APP(mi_app);

#endif // HAVE_WXWINDOWS

#endif // __MKVINFO_H
